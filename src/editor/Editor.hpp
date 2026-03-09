#pragma once

#include "../syntax/SyntaxHighlighter.hpp"
#include "../parser/CodeParser.hpp"
#include "../lsp/LSPClient.hpp"

#include <QPlainTextEdit>
#include <QWidget>
#include <QString>
#include <QTimer>
#include <memory>

namespace minicode {

class LineNumberArea;   // forward declaration

/**
 * @brief Editor — The core code editing widget.
 *
 * Features:
 *   - Syntax highlighting (via SyntaxHighlighter)
 *   - Line numbers (via LineNumberArea)
 *   - Diagnostic underlines + tooltips
 *   - Auto-indent on Enter
 *   - Bracket auto-close
 *   - Find (Ctrl+F)
 *   - Minimap (rendered in paintEvent)
 *   - LSP completion popup
 *   - Keyboard shortcuts: Ctrl+S, Ctrl+O, Ctrl+F, Ctrl+N
 */
class Editor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit Editor(QWidget* parent = nullptr);
    ~Editor() override = default;

    // ---- File management ----
    bool openFile(const QString& path);
    bool saveFile();
    bool saveFileAs(const QString& path);

    [[nodiscard]] const QString& filePath() const { return m_filePath; }
    [[nodiscard]] bool isModified()          const { return document()->isModified(); }
    [[nodiscard]] const QString& language()  const { return m_language; }

    // ---- LSP integration ----
    void setLSPClient(LSPClient* client);

    // ---- Line number area ----
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent* event);

    // ---- Search ----
    void findText(const QString& text, bool caseSensitive = false);
    void findNext();
    void findPrev();

    // ---- Appearance ----
    void setDarkTheme();

signals:
    void fileModified(const QString& path);
    void fileSaved   (const QString& path);
    void cursorPositionInfo(int line, int col);

protected:
    void keyPressEvent      (QKeyEvent*  event) override;
    void resizeEvent        (QResizeEvent* event) override;
    void paintEvent         (QPaintEvent* event) override;
    bool event              (QEvent* e)          override;  // tooltips
    void wheelEvent         (QWheelEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    void onTextChanged();
    void runDiagnostics();
    void onDiagnosticsReceived(const QString& uri,
                                const QVector<Diagnostic>& diags);
    void showCompletionPopup(const QVector<CompletionItem>& items);

private:
    void setupEditor();
    void setupConnections();
    void setupFont();

    // Auto-indent helpers
    QString currentLineIndent() const;
    void    handleEnterKey();
    void    handleBracketClose(QChar closingBracket);

    // Minimap
    void drawMinimap(QPainter& painter);

    // Diagnostic tooltip
    QString diagnosticAtCursor() const;

    // ---- State ----
    QString     m_filePath;
    QString     m_language {"text"};
    QVector<Diagnostic> m_diagnostics;

    // Search state
    QString m_searchText;
    bool    m_searchCaseSensitive {false};

    // Widgets
    LineNumberArea* m_lineNumberArea {nullptr};

    // Owned objects
    std::unique_ptr<SyntaxHighlighter> m_highlighter;
    std::unique_ptr<CodeParser>        m_parser;

    // Non-owned (lives in MainWindow)
    LSPClient* m_lspClient {nullptr};

    // Debounce timer for diagnostics
    QTimer m_diagTimer;

    // Version counter for LSP sync
    int m_docVersion {0};
};

// ============================================================
/**
 * @brief LineNumberArea — Left margin widget showing line numbers.
 *
 * Thin companion class; delegates painting to Editor.
 */
class LineNumberArea : public QWidget {
    Q_OBJECT
public:
    explicit LineNumberArea(Editor* editor);
    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Editor* m_editor;
};

} // namespace minicode
