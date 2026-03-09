#include "Editor.hpp"
#include "../utils/FileManager.hpp"

#include <QKeyEvent>
#include <QWheelEvent>
#include <QHelpEvent>
#include <QPainter>
#include <QScrollBar>
#include <QFileDialog>
#include <QToolTip>
#include <QCompleter>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QTextBlock>
#include <QApplication>
#include <QFontDatabase>
#include <QShortcut>
#include <algorithm>

namespace minicode {

// ============================================================
// LineNumberArea
// ============================================================
LineNumberArea::LineNumberArea(Editor* editor)
    : QWidget(editor), m_editor(editor) {}

QSize LineNumberArea::sizeHint() const
{
    return {m_editor->lineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent* event)
{
    m_editor->lineNumberAreaPaintEvent(event);
}

// ============================================================
// Editor
// ============================================================
Editor::Editor(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_parser(std::make_unique<CodeParser>("text"))
{
    setupFont();
    setupEditor();
    setupConnections();

    // Diagnostic debounce: 800 ms after last keystroke
    m_diagTimer.setSingleShot(true);
    m_diagTimer.setInterval(800);
    connect(&m_diagTimer, &QTimer::timeout, this, &Editor::runDiagnostics);

    setDarkTheme();
    updateLineNumberAreaWidth(0);
}

// ============================================================
void Editor::setupFont()
{
    // Prefer monospace fonts in order
    static const QStringList fonts = {
        "JetBrains Mono", "Fira Code", "Cascadia Code",
        "Source Code Pro", "Hack", "DejaVu Sans Mono", "Monospace"
    };

    QFont font;
    font.setPointSize(12);
    for (const auto& name : fonts) {
        font.setFamily(name);
        if (QFontDatabase().hasFamily(name)) break;
    }
    font.setFixedPitch(true);
    setFont(font);
}

// ============================================================
void Editor::setupEditor()
{
    setLineWrapMode(QPlainTextEdit::NoWrap);
    // Tab stop will be updated once widget is shown and font metrics are valid
    setTabStopDistance(40); // safe default: 4 * 10px

    // Viewport margins set via updateLineNumberAreaWidth once widget is realized
}

// ============================================================
void Editor::setupConnections()
{
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &Editor::updateLineNumberAreaWidth);

    connect(this, &QPlainTextEdit::updateRequest,
            this, &Editor::updateLineNumberArea);

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &Editor::highlightCurrentLine);

    connect(this, &QPlainTextEdit::textChanged,
            this, &Editor::onTextChanged);
}

// ============================================================
// File operations
// ============================================================
bool Editor::openFile(const QString& path)
{
    auto content = FileManager::readFile(path);
    if (!content) return false;

    m_filePath = path;
    m_language = FileManager::detectLanguage(path);
    m_parser->setLanguage(m_language);

    // Replace / create highlighter
    m_highlighter = std::make_unique<SyntaxHighlighter>(document(), m_language);

    setPlainText(*content);
    document()->setModified(false);
    m_docVersion = 0;

    // Notify LSP
    if (m_lspClient && m_lspClient->isRunning()) {
        m_lspClient->didOpen(LSPClient::pathToUri(path), m_language,
                              m_docVersion, *content);
    }

    emit fileModified(path);
    return true;
}

bool Editor::saveFile()
{
    if (m_filePath.isEmpty()) return false;
    return saveFileAs(m_filePath);
}

bool Editor::saveFileAs(const QString& path)
{
    if (!FileManager::writeFile(path, toPlainText())) return false;
    m_filePath = path;
    document()->setModified(false);
    emit fileSaved(path);
    return true;
}

// ============================================================
void Editor::setLSPClient(LSPClient* client)
{
    if (m_lspClient) {
        disconnect(m_lspClient, &LSPClient::diagnosticsReceived,
                   this, &Editor::onDiagnosticsReceived);
        disconnect(m_lspClient, &LSPClient::completionReceived,
                   this, &Editor::showCompletionPopup);
    }

    m_lspClient = client;

    if (m_lspClient) {
        connect(m_lspClient, &LSPClient::diagnosticsReceived,
                this, &Editor::onDiagnosticsReceived);
        connect(m_lspClient, &LSPClient::completionReceived,
                this, &Editor::showCompletionPopup);
    }
}

// ============================================================
// Line number area
// ============================================================
int Editor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max    = qMax(1, blockCount());
    while (max >= 10) { max /= 10; ++digits; }
    return 6 + fontMetrics().horizontalAdvance('9') * (digits + 1);
}

void Editor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 120, 0);
    // Also sync tab stop distance now that font metrics are valid
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
}

void Editor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(),
                                  m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void Editor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0x1e, 0x1e, 0x1e));

    QTextBlock block = firstVisibleBlock();
    if (!block.isValid()) return;
    int blockNumber        = block.blockNumber();
    int top                = qRound(blockBoundingGeometry(block)
                                        .translated(contentOffset()).top());
    int bottom             = top + qRound(blockBoundingRect(block).height());
    const int currentLine  = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            // Highlight current line number
            if (blockNumber == currentLine)
                painter.setPen(QColor(0xc8, 0xc8, 0xc8));
            else
                painter.setPen(QColor(0x60, 0x60, 0x60));

            painter.drawText(0, top,
                             m_lineNumberArea->width() - 4,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }

        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ============================================================
// Current line highlight
// ============================================================
void Editor::highlightCurrentLine()
{
    if (!isVisible()) return;  // don't run before widget is shown
    QList<QTextEdit::ExtraSelection> extras;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(0x26, 0x26, 0x2e));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extras.append(sel);
    }

    setExtraSelections(extras);

    // Emit cursor info
    const auto cur = textCursor();
    emit cursorPositionInfo(cur.blockNumber() + 1,
                             cur.columnNumber() + 1);
}

// ============================================================
// Text changed → schedule diagnostics
// ============================================================
void Editor::onTextChanged()
{
    document()->setModified(true);
    m_diagTimer.start(); // restart debounce

    // Incremental LSP sync
    if (m_lspClient && m_lspClient->isRunning() && !m_filePath.isEmpty()) {
        m_lspClient->didChange(LSPClient::pathToUri(m_filePath),
                                ++m_docVersion, toPlainText());
    }
}

// ============================================================
// Run local parser diagnostics
// ============================================================
void Editor::runDiagnostics()
{
    if (!m_parser) return;
    m_diagnostics = m_parser->parse(toPlainText());

    if (m_highlighter) {
        m_highlighter->setDiagnostics(m_diagnostics);
    }
}

// ============================================================
void Editor::onDiagnosticsReceived(const QString& uri,
                                    const QVector<Diagnostic>& diags)
{
    if (LSPClient::uriToPath(uri) != m_filePath) return;
    m_diagnostics = diags;
    if (m_highlighter) m_highlighter->setDiagnostics(diags);
}

// ============================================================
// Completion popup
// ============================================================
void Editor::showCompletionPopup(const QVector<CompletionItem>& items)
{
    QStringList words;
    for (const auto& item : items)
        words << item.label;

    if (words.isEmpty()) return;

    auto* completer = new QCompleter(words, this);
    completer->setWidget(this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);

    connect(completer,
            QOverload<const QString&>::of(&QCompleter::activated),
            [this, completer](const QString& completion) {
                auto cur = textCursor();
                cur.select(QTextCursor::WordUnderCursor);
                cur.insertText(completion);
                setTextCursor(cur);
                completer->deleteLater();
            });

    const QRect cr = cursorRect();
    completer->complete(QRect(cr.left(), cr.bottom(), 200, 200));
}

// ============================================================
// Keyboard handling
// ============================================================
void Editor::keyPressEvent(QKeyEvent* event)
{
    const Qt::KeyboardModifiers mods = event->modifiers();
    const int key = event->key();

    // ---- Shortcuts ----
    if (mods & Qt::ControlModifier) {
        if (key == Qt::Key_S) {
            if (mods & Qt::ShiftModifier) {
                // Ctrl+Shift+S → Save As
                QString path = QFileDialog::getSaveFileName(this,
                    "Save File As", m_filePath);
                if (!path.isEmpty()) saveFileAs(path);
            } else {
                saveFile();
            }
            return;
        }

        if (key == Qt::Key_Space) {
            // Ctrl+Space → trigger completion
            if (m_lspClient && m_lspClient->isRunning() &&
                !m_filePath.isEmpty())
            {
                const auto cur = textCursor();
                m_lspClient->requestCompletion(
                    LSPClient::pathToUri(m_filePath),
                    cur.blockNumber(), cur.columnNumber());
            }
            return;
        }

        if (key == Qt::Key_Slash) {
            // Ctrl+/ → toggle comment
            auto cur = textCursor();
            cur.beginEditBlock();
            QTextBlock block = document()->findBlock(cur.selectionStart());
            const QTextBlock endBlock =
                document()->findBlock(cur.selectionEnd());

            while (true) {
                QString text = block.text().trimmed();
                QTextCursor bc(block);
                if (text.startsWith("//")) {
                    bc.movePosition(QTextCursor::StartOfBlock);
                    bc.movePosition(QTextCursor::Right,
                                    QTextCursor::KeepAnchor, 2);
                    bc.removeSelectedText();
                } else {
                    bc.movePosition(QTextCursor::StartOfBlock);
                    bc.insertText("//");
                }
                if (block == endBlock) break;
                block = block.next();
            }
            cur.endEditBlock();
            return;
        }
    }

    // ---- Auto-indent on Enter ----
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        handleEnterKey();
        return;
    }

    // ---- Auto-close brackets ----
    if (key == Qt::Key_BraceLeft)  { QPlainTextEdit::keyPressEvent(event);
                                     insertPlainText("}"); moveCursor(QTextCursor::Left); return; }
    if (key == Qt::Key_BracketLeft){ QPlainTextEdit::keyPressEvent(event);
                                     insertPlainText("]"); moveCursor(QTextCursor::Left); return; }
    if (key == Qt::Key_ParenLeft)  { QPlainTextEdit::keyPressEvent(event);
                                     insertPlainText(")"); moveCursor(QTextCursor::Left); return; }
    if (key == Qt::Key_QuoteDbl)   { QPlainTextEdit::keyPressEvent(event);
                                     insertPlainText("\""); moveCursor(QTextCursor::Left); return; }

    // ---- Tab → 4 spaces ----
    if (key == Qt::Key_Tab) {
        insertPlainText("    ");
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

// ============================================================
void Editor::handleEnterKey()
{
    const auto cur   = textCursor();
    const QString indent = currentLineIndent();

    // Check if previous line ends with {, [, ( → extra indent
    const QString lineText = cur.block().text().trimmed();
    const bool opensBrace  = lineText.endsWith('{') ||
                              lineText.endsWith('(') ||
                              lineText.endsWith('[') ||
                              lineText.endsWith(':');

    insertPlainText("\n" + indent + (opensBrace ? "    " : ""));
}

QString Editor::currentLineIndent() const
{
    const QString text = textCursor().block().text();
    int i = 0;
    while (i < text.size() && (text[i] == ' ' || text[i] == '\t'))
        ++i;
    return text.left(i);
}

// ============================================================
// Search
// ============================================================
void Editor::findText(const QString& text, bool caseSensitive)
{
    m_searchText          = text;
    m_searchCaseSensitive = caseSensitive;
    findNext();
}

void Editor::findNext()
{
    if (m_searchText.isEmpty()) return;
    QTextDocument::FindFlags flags;
    if (m_searchCaseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (!find(m_searchText, flags)) {
        // Wrap around
        moveCursor(QTextCursor::Start);
        find(m_searchText, flags);
    }
}

void Editor::findPrev()
{
    if (m_searchText.isEmpty()) return;
    QTextDocument::FindFlags flags = QTextDocument::FindBackward;
    if (m_searchCaseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (!find(m_searchText, flags)) {
        moveCursor(QTextCursor::End);
        find(m_searchText, flags);
    }
}

// ============================================================
// Resize — keep line number area in sync
// ============================================================
void Editor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

// ============================================================
// Paint — draw minimap overlay
// ============================================================
void Editor::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    drawMinimap(painter);
}

// ============================================================
void Editor::drawMinimap(QPainter& painter)
{
    const int mapWidth  = 110;
    const int mapHeight = viewport()->height();
    const int mapX      = viewport()->width() - mapWidth;
    if (mapWidth <= 0 || mapHeight <= 0 || mapX < 0) return;

    // Background
    painter.fillRect(mapX, 0, mapWidth, mapHeight,
                     QColor(0x1a, 0x1a, 0x1a, 220));

    const QString text    = toPlainText();
    const QStringList lines = text.split('\n');
    const int totalLines  = lines.size();
    if (totalLines == 0) return;

    const float lineH  = qMax(1.0f, static_cast<float>(mapHeight) / totalLines);
    const int curLine  = textCursor().blockNumber();
    const QTextBlock firstVB = firstVisibleBlock();
    if (!firstVB.isValid()) return;
    const int firstVis = firstVB.blockNumber();
    const qreal blockH = blockBoundingRect(firstVB).height();
    if (blockH <= 0) return;
    const int visLines = qRound(viewport()->height() / blockH);

    // Viewport indicator
    const int vpY      = qRound(firstVis * lineH);
    const int vpHeight = qRound(visLines * lineH);
    painter.fillRect(mapX, vpY, mapWidth, vpHeight,
                     QColor(0x52, 0x52, 0x64, 80));

    // Render lines as thin strokes
    painter.setPen(QPen(QColor(0x88, 0x88, 0x99, 160), 1));
    for (int i = 0; i < totalLines; ++i) {
        const int y   = qRound(i * lineH);
        const int len = qMin(mapWidth - 4,
                             static_cast<int>(lines[i].trimmed().size() * 0.6));
        if (len > 0)
            painter.drawLine(mapX + 2, y, mapX + 2 + len, y);
    }

    // Current line
    painter.setPen(QPen(QColor(0x00, 0x7a, 0xcc, 200), 2));
    const int cy = qRound(curLine * lineH);
    painter.drawLine(mapX, cy, mapX + mapWidth, cy);

    // Diagnostic marks on minimap
    for (const auto& diag : m_diagnostics) {
        const int dy = qRound(diag.line * lineH);
        const QColor col = (diag.severity == Diagnostic::Severity::Error)
            ? QColor(0xff, 0x33, 0x33, 200)
            : QColor(0xff, 0xcc, 0x00, 200);
        painter.setPen(QPen(col, 2));
        painter.drawLine(mapX, dy, mapX + mapWidth, dy);
    }
}

// ============================================================
// Tooltip for diagnostics
// ============================================================
bool Editor::event(QEvent* e)
{
    if (e->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(e);
        const QTextCursor cur = cursorForPosition(helpEvent->pos());
        const int line = cur.blockNumber();

        for (const auto& diag : m_diagnostics) {
            if (diag.line == line) {
                QToolTip::showText(helpEvent->globalPos(),
                                   QString("[%1] %2")
                                       .arg(diag.severity ==
                                            Diagnostic::Severity::Error
                                                ? "Error" : "Warning")
                                       .arg(diag.message));
                return true;
            }
        }
        QToolTip::hideText();
    }
    return QPlainTextEdit::event(e);
}

// ============================================================
// Ctrl+scroll → font zoom
// ============================================================
void Editor::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        QFont f = font();
        const int delta = event->angleDelta().y() > 0 ? 1 : -1;
        f.setPointSize(qBound(8, f.pointSize() + delta, 28));
        setFont(f);
        setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

// ============================================================
// Dark theme palette
// ============================================================
void Editor::setDarkTheme()
{
    QPalette p = palette();
    p.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::Text,            QColor(0xd4, 0xd4, 0xd4));
    p.setColor(QPalette::Highlight,       QColor(0x26, 0x4f, 0x78));
    p.setColor(QPalette::HighlightedText, QColor(0xd4, 0xd4, 0xd4));
    setPalette(p);
    setToolTip("");  // enable tooltip events
    setAttribute(Qt::WA_AlwaysShowToolTips);
}

} // namespace minicode
