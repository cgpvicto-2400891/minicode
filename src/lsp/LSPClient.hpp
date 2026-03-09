#pragma once

#include "../parser/CodeParser.hpp"

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <memory>
#include <functional>
#include <unordered_map>

namespace minicode {

/**
 * @brief LSP completion item.
 */
struct CompletionItem {
    QString label;
    QString detail;
    QString documentation;
    int     kind {1}; // 1=Text, 2=Method, 3=Function, 6=Variable, 7=Class
};

/**
 * @brief LSPClient — Language Server Protocol client (JSON-RPC over stdio).
 *
 * Connects to clangd (or any LSP server) via QProcess stdio pipes.
 * Implements a subset of LSP 3.17:
 *   - initialize / initialized
 *   - textDocument/didOpen, didChange, didClose
 *   - textDocument/publishDiagnostics  (notification → signal)
 *   - textDocument/completion
 *   - textDocument/definition
 *   - textDocument/hover
 */
class LSPClient : public QObject {
    Q_OBJECT

public:
    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient() override;

    // ---- Lifecycle ----

    /**
     * @brief Start the LSP server process.
     * @param serverCmd  e.g. "clangd" or "/usr/bin/clangd"
     * @param args       Extra arguments passed to the server
     * @param rootPath   Workspace root directory
     */
    bool start(const QString& serverCmd,
               const QStringList& args,
               const QString& rootPath);

    /** Stop the server gracefully. */
    void stop();

    [[nodiscard]] bool isRunning() const;

    // ---- Document synchronisation ----
    void didOpen  (const QString& uri, const QString& language,
                   int version, const QString& text);
    void didChange(const QString& uri, int version, const QString& text);
    void didClose (const QString& uri);

    // ---- Requests (results delivered via signals) ----
    void requestCompletion(const QString& uri, int line, int character);
    void requestDefinition(const QString& uri, int line, int character);
    void requestHover     (const QString& uri, int line, int character);

    // ---- Utility ----
    /** Convert a file path to a LSP URI (file:///…). */
    [[nodiscard]] static QString pathToUri(const QString& path);
    [[nodiscard]] static QString uriToPath(const QString& uri);

signals:
    void diagnosticsReceived(const QString& uri,
                             const QVector<Diagnostic>& diagnostics);
    void completionReceived (const QVector<CompletionItem>& items);
    void definitionReceived (const QString& uri, int line, int character);
    void hoverReceived       (const QString& message);
    void serverError         (const QString& message);
    void serverStarted();
    void serverStopped();

private slots:
    void onReadyRead();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    // ---- JSON-RPC helpers ----
    void sendRequest     (const QString& method, const QJsonObject& params);
    void sendNotification(const QString& method, const QJsonObject& params);
    void sendMessage     (const QJsonObject& message);

    void processMessage  (const QJsonObject& msg);
    void handleResponse  (int id, const QJsonObject& result);
    void handleNotification(const QString& method, const QJsonObject& params);

    QVector<Diagnostic> parseDiagnostics(const QJsonArray& arr);

    // ---- State ----
    std::unique_ptr<QProcess> m_process;
    QByteArray  m_readBuffer;
    int         m_nextId {1};

    // Pending request id → method name
    std::unordered_map<int, QString> m_pendingRequests;

    bool m_initialised {false};
};

} // namespace minicode
