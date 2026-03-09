#pragma once

#include "../parser/CodeParser.hpp"

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextCharFormat>
#include <memory>
#include <unordered_map>

namespace minicode {

/**
 * @brief SyntaxHighlighter — Qt-based incremental syntax highlighter.
 *
 * Integrates with CodeParser for token extraction.
 * Supports dark VS Code–style colour theme.
 *
 * Usage:
 *   auto hl = std::make_unique<SyntaxHighlighter>(doc, "cpp");
 *   // Automatically rehighlights on document changes.
 */
class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent, const QString& language);
    ~SyntaxHighlighter() override = default;

    /** Change language and re-highlight the whole document. */
    void setLanguage(const QString& language);

    /** Apply a set of external diagnostics (from LSP / parser). */
    void setDiagnostics(const QVector<Diagnostic>& diags);

    /** Clear all diagnostics. */
    void clearDiagnostics();

protected:
    /** Called by Qt for each text block (line) that needs highlighting. */
    void highlightBlock(const QString& text) override;

private:
    // ---- Colour palette (VS Code Dark+ inspired) ----
    struct Theme {
        QColor keyword     {0x56, 0x9c, 0xd6};  // blue
        QColor type_       {0x4e, 0xc9, 0xb0};  // teal
        QColor string_     {0xce, 0x91, 0x78};  // orange-brown
        QColor number      {0xb5, 0xce, 0xa8};  // light green
        QColor comment     {0x6a, 0x99, 0x55};  // olive green
        QColor preproc     {0x9b, 0x9b, 0x9b};  // grey
        QColor decorator   {0xdd, 0xdd, 0xaa};  // yellow-green
        QColor error_under {0xff, 0x00, 0x00};  // red
        QColor warn_under  {0xff, 0xcc, 0x00};  // yellow
    };

    // ---- Highlight rules ----
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat    format;
    };

    void buildRules();
    void setupFormats();

    void applyDiagnosticHighlights(const QString& text, int blockLine);

    QString                    m_language;
    QVector<HighlightRule>     m_rules;
    QVector<Diagnostic>        m_diagnostics;
    Theme                      m_theme;

    // Formats
    QTextCharFormat m_keywordFmt;
    QTextCharFormat m_typeFmt;
    QTextCharFormat m_stringFmt;
    QTextCharFormat m_numberFmt;
    QTextCharFormat m_commentFmt;
    QTextCharFormat m_preprocFmt;
    QTextCharFormat m_decoratorFmt;
    QTextCharFormat m_errorFmt;
    QTextCharFormat m_warningFmt;

    // Multi-line comment state
    enum BlockState { Normal = 0, InMultilineComment = 1 };

    std::unique_ptr<CodeParser> m_parser;
};

} // namespace minicode
