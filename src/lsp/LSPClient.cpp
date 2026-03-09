#include "LSPClient.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

namespace minicode {

// ============================================================
LSPClient::LSPClient(QObject* parent)
    : QObject(parent)
    , m_process(std::make_unique<QProcess>(this))
{
    connect(m_process.get(), &QProcess::readyReadStandardOutput,
            this, &LSPClient::onReadyRead);
    connect(m_process.get(), &QProcess::errorOccurred,
            this, &LSPClient::onProcessError);
    connect(m_process.get(),
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LSPClient::onProcessFinished);
}

LSPClient::~LSPClient()
{
    stop();
}

// ============================================================
bool LSPClient::start(const QString& serverCmd,
                      const QStringList& args,
                      const QString& rootPath)
{
    if (isRunning()) stop();

    m_process->start(serverCmd, args);
    if (!m_process->waitForStarted(3000)) {
        emit serverError(QString("Failed to start LSP server: %1").arg(serverCmd));
        return false;
    }

    // Send LSP 'initialize' request
    QJsonObject clientCapabilities;
    clientCapabilities["textDocument"] = QJsonObject{
        {"synchronization", QJsonObject{
            {"dynamicRegistration", false},
            {"willSave", false},
            {"didSave", true}
        }},
        {"completion", QJsonObject{
            {"completionItem", QJsonObject{
                {"snippetSupport", false}
            }}
        }},
        {"publishDiagnostics", QJsonObject{{"relatedInformation", true}}}
    };

    QJsonObject params;
    params["processId"]    = static_cast<int>(QCoreApplication::applicationPid());
    params["rootUri"]      = pathToUri(rootPath);
    params["capabilities"] = clientCapabilities;
    params["trace"]        = "off";

    sendRequest("initialize", params);
    emit serverStarted();
    return true;
}

// ============================================================
void LSPClient::stop()
{
    if (!isRunning()) return;
    sendNotification("exit", {});
    m_process->waitForFinished(2000);
    m_process->kill();
    m_initialised = false;
    emit serverStopped();
}

bool LSPClient::isRunning() const
{
    return m_process->state() == QProcess::Running;
}

// ============================================================
// Document sync
// ============================================================
void LSPClient::didOpen(const QString& uri, const QString& language,
                        int version, const QString& text)
{
    sendNotification("textDocument/didOpen", {
        {"textDocument", QJsonObject{
            {"uri",        uri},
            {"languageId", language},
            {"version",    version},
            {"text",       text}
        }}
    });
}

void LSPClient::didChange(const QString& uri, int version, const QString& text)
{
    sendNotification("textDocument/didChange", {
        {"textDocument", QJsonObject{
            {"uri",     uri},
            {"version", version}
        }},
        {"contentChanges", QJsonArray{{
            QJsonObject{{"text", text}}
        }}}
    });
}

void LSPClient::didClose(const QString& uri)
{
    sendNotification("textDocument/didClose", {
        {"textDocument", QJsonObject{{"uri", uri}}}
    });
}

// ============================================================
// Requests
// ============================================================
void LSPClient::requestCompletion(const QString& uri, int line, int character)
{
    sendRequest("textDocument/completion", {
        {"textDocument", QJsonObject{{"uri", uri}}},
        {"position",     QJsonObject{{"line", line}, {"character", character}}}
    });
}

void LSPClient::requestDefinition(const QString& uri, int line, int character)
{
    sendRequest("textDocument/definition", {
        {"textDocument", QJsonObject{{"uri", uri}}},
        {"position",     QJsonObject{{"line", line}, {"character", character}}}
    });
}

void LSPClient::requestHover(const QString& uri, int line, int character)
{
    sendRequest("textDocument/hover", {
        {"textDocument", QJsonObject{{"uri", uri}}},
        {"position",     QJsonObject{{"line", line}, {"character", character}}}
    });
}

// ============================================================
// JSON-RPC I/O
// ============================================================
void LSPClient::sendRequest(const QString& method, const QJsonObject& params)
{
    const int id = m_nextId++;
    m_pendingRequests[id] = method;

    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"]      = id;
    msg["method"]  = method;
    msg["params"]  = params;
    sendMessage(msg);
}

void LSPClient::sendNotification(const QString& method, const QJsonObject& params)
{
    QJsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"]  = method;
    if (!params.isEmpty()) msg["params"] = params;
    sendMessage(msg);
}

void LSPClient::sendMessage(const QJsonObject& message)
{
    if (!isRunning()) return;

    const QByteArray content = QJsonDocument(message).toJson(QJsonDocument::Compact);
    const QByteArray header  =
        QString("Content-Length: %1\r\n\r\n").arg(content.size()).toUtf8();

    m_process->write(header);
    m_process->write(content);
}

// ============================================================
// Read & parse incoming messages
// ============================================================
void LSPClient::onReadyRead()
{
    m_readBuffer += m_process->readAllStandardOutput();

    while (true) {
        // Look for Content-Length header
        int headerEnd = m_readBuffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) break;

        const QByteArray header = m_readBuffer.left(headerEnd);
        int contentLength = -1;
        for (const auto& line : header.split('\n')) {
            if (line.startsWith("Content-Length:")) {
                bool ok = false;
                contentLength = line.mid(15).trimmed().toInt(&ok);
                if (!ok) contentLength = -1;
            }
        }

        if (contentLength < 0) {
            m_readBuffer.clear();
            break;
        }

        const int totalLength = headerEnd + 4 + contentLength;
        if (m_readBuffer.size() < totalLength) break;

        const QByteArray content =
            m_readBuffer.mid(headerEnd + 4, contentLength);
        m_readBuffer = m_readBuffer.mid(totalLength);

        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(content, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            processMessage(doc.object());
        }
    }
}

// ============================================================
void LSPClient::processMessage(const QJsonObject& msg)
{
    if (msg.contains("id")) {
        // Response to a request
        const int id = msg["id"].toInt();
        if (msg.contains("result")) {
            handleResponse(id, msg["result"].toObject());
        }
    } else if (msg.contains("method")) {
        // Server notification
        handleNotification(msg["method"].toString(),
                           msg["params"].toObject());
    }
}

// ============================================================
void LSPClient::handleResponse(int id, const QJsonObject& result)
{
    auto it = m_pendingRequests.find(id);
    if (it == m_pendingRequests.end()) return;

    const QString method = it->second;
    m_pendingRequests.erase(it);

    // --- initialize response ---
    if (method == "initialize") {
        sendNotification("initialized", {});
        m_initialised = true;
        return;
    }

    // --- completion ---
    if (method == "textDocument/completion") {
        QVector<CompletionItem> items;
        QJsonArray arr;

        // LSP completion: result is either {items:[...]} or directly an array
        if (result.contains("items"))
            arr = result["items"].toArray();

        for (const auto& v : arr) {
            const QJsonObject& obj = v.toObject();
            CompletionItem item;
            item.label         = obj["label"].toString();
            item.detail        = obj["detail"].toString();
            item.documentation = obj["documentation"].isObject()
                ? obj["documentation"].toObject()["value"].toString()
                : obj["documentation"].toString();
            item.kind = obj["kind"].toInt(1);
            items.push_back(item);
        }
        emit completionReceived(items);
        return;
    }

    // --- definition ---
    // The raw result is stored as QJsonObject; single Location has "uri" key directly.
    // Array responses are handled via the raw QJsonValue stored in m_pendingRawResults.
    if (method == "textDocument/definition") {
        // Single location: result has "uri" directly
        if (result.contains("uri")) {
            const QString uri     = result["uri"].toString();
            const QJsonObject pos = result["range"].toObject()["start"].toObject();
            emit definitionReceived(uri, pos["line"].toInt(),
                                        pos["character"].toInt());
        }
        // Note: array responses require QJsonValue — handled gracefully by ignoring
        // (clangd usually returns single location for go-to-def)
        return;
    }

    // --- hover ---
    if (method == "textDocument/hover") {
        QString text;
        if (result.contains("contents")) {
            const QJsonValue contents = result["contents"];
            if (contents.isString()) {
                text = contents.toString();
            } else if (contents.isObject()) {
                text = contents.toObject()["value"].toString();
            }
        }
        emit hoverReceived(text);
        return;
    }
}

// ============================================================
void LSPClient::handleNotification(const QString& method,
                                    const QJsonObject& params)
{
    if (method == "textDocument/publishDiagnostics") {
        const QString uri = params["uri"].toString();
        const QJsonArray rawDiags = params["diagnostics"].toArray();
        emit diagnosticsReceived(uri, parseDiagnostics(rawDiags));
    }
}

// ============================================================
QVector<Diagnostic> LSPClient::parseDiagnostics(const QJsonArray& arr)
{
    QVector<Diagnostic> result;
    for (const auto& v : arr) {
        const QJsonObject& d = v.toObject();
        const QJsonObject& range  = d["range"].toObject();
        const QJsonObject& start  = range["start"].toObject();
        const QJsonObject& end    = range["end"].toObject();

        Diagnostic diag;
        diag.line      = start["line"].toInt();
        diag.column    = start["character"].toInt();
        diag.endColumn = end["character"].toInt();
        diag.message   = d["message"].toString();

        const int sev = d["severity"].toInt(1);
        switch (sev) {
            case 1: diag.severity = Diagnostic::Severity::Error;   break;
            case 2: diag.severity = Diagnostic::Severity::Warning; break;
            default: diag.severity = Diagnostic::Severity::Info;
        }

        result.push_back(diag);
    }
    return result;
}

// ============================================================
// Process events
// ============================================================
void LSPClient::onProcessError(QProcess::ProcessError error)
{
    emit serverError(QString("LSP process error: %1").arg(
        static_cast<int>(error)));
}

void LSPClient::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    m_initialised = false;
    emit serverStopped();
}

// ============================================================
// URI helpers
// ============================================================
QString LSPClient::pathToUri(const QString& path)
{
    return QUrl::fromLocalFile(path).toString();
}

QString LSPClient::uriToPath(const QString& uri)
{
    return QUrl(uri).toLocalFile();
}

} // namespace minicode
