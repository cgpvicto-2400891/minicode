#include "SyntaxHighlighter.hpp"

#include <QTextBlock>
#include <QRegularExpressionMatchIterator>

namespace minicode {

// ============================================================
SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent,
                                     const QString& language)
    : QSyntaxHighlighter(parent)
    , m_language(language)
    , m_parser(std::make_unique<CodeParser>(language))
{
    setupFormats();
    buildRules();
}

// ============================================================
void SyntaxHighlighter::setLanguage(const QString& language)
{
    if (m_language == language) return;
    m_language = language;
    m_parser->setLanguage(language);
    buildRules();
    rehighlight();
}

// ============================================================
void SyntaxHighlighter::setDiagnostics(const QVector<Diagnostic>& diags)
{
    m_diagnostics = diags;
    rehighlight();
}

void SyntaxHighlighter::clearDiagnostics()
{
    m_diagnostics.clear();
    rehighlight();
}

// ============================================================
// Setup formats (colour theme)
// ============================================================
void SyntaxHighlighter::setupFormats()
{
    auto mkFmt = [](const QColor& col, bool bold = false,
                    bool italic = false) {
        QTextCharFormat f;
        f.setForeground(col);
        if (bold)   f.setFontWeight(QFont::Bold);
        if (italic) f.setFontItalic(true);
        return f;
    };

    m_keywordFmt   = mkFmt(m_theme.keyword,   true);
    m_typeFmt      = mkFmt(m_theme.type_);
    m_stringFmt    = mkFmt(m_theme.string_);
    m_numberFmt    = mkFmt(m_theme.number);
    m_commentFmt   = mkFmt(m_theme.comment, false, true);
    m_preprocFmt   = mkFmt(m_theme.preproc);
    m_decoratorFmt = mkFmt(m_theme.decorator);

    // Error underline (wavy red)
    m_errorFmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_errorFmt.setUnderlineColor(m_theme.error_under);

    // Warning underline (wavy yellow)
    m_warningFmt.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_warningFmt.setUnderlineColor(m_theme.warn_under);
}

// ============================================================
// Build regex rules per language
// ============================================================
void SyntaxHighlighter::buildRules()
{
    m_rules.clear();

    if (m_language == "cpp") {
        static const QStringList kwCpp = {
            "alignas","alignof","auto","bool","break","case","catch","char",
            "char8_t","char16_t","char32_t","class","concept","const",
            "consteval","constexpr","constinit","const_cast","continue",
            "co_await","co_return","co_yield","decltype","default","delete",
            "do","double","dynamic_cast","else","enum","explicit","export",
            "extern","false","float","for","friend","goto","if","inline",
            "int","long","mutable","namespace","new","noexcept","nullptr",
            "operator","private","protected","public","register",
            "reinterpret_cast","requires","return","short","signed","sizeof",
            "static","static_assert","static_cast","struct","switch",
            "template","this","thread_local","throw","true","try","typedef",
            "typeid","typename","union","unsigned","using","virtual","void",
            "volatile","wchar_t","while","override","final","import","module"
        };

        // Preprocessor directives
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(^\s*#\s*\w+)"); _r.format = m_preprocFmt; m_rules.push_back(std::move(_r)); }

        // C++ keyword
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:)" + kwCpp.join('|') + R"()\b)"); _r.format = m_keywordFmt; m_rules.push_back(std::move(_r)); }

        // std:: types / common types
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:std::\w+|size_t|uint\d+_t|int\d+_t|ptrdiff_t)\b)"); _r.format = m_typeFmt; m_rules.push_back(std::move(_r)); }

        // Numbers (hex, binary, float, int)
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:0x[0-9A-Fa-f]+|0b[01]+|\d+\.?\d*(?:[eE][+-]?\d+)?[fFlLuU]*)\b)"); _r.format = m_numberFmt; m_rules.push_back(std::move(_r)); }

        // String / char literals
        { HighlightRule _r; _r.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"); _r.format = m_stringFmt; m_rules.push_back(std::move(_r)); }

        // Single-line comment (handled in highlightBlock for multi-line too)
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(//[^\n]*)"); _r.format = m_commentFmt; m_rules.push_back(std::move(_r)); }

    } else if (m_language == "python") {
        static const QStringList kwPy = {
            "False","None","True","and","as","assert","async","await",
            "break","class","continue","def","del","elif","else","except",
            "finally","for","from","global","if","import","in","is",
            "lambda","nonlocal","not","or","pass","raise","return",
            "try","while","with","yield"
        };

        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:)" + kwPy.join('|') + R"()\b)"); _r.format = m_keywordFmt; m_rules.push_back(std::move(_r)); }

        // Built-in types / functions
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:int|float|str|list|dict|set|tuple|bool|bytes|type|object|range|len|print|input|open|super|self)\b)"); _r.format = m_typeFmt; m_rules.push_back(std::move(_r)); }

        // Decorators
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(@\w+)"); _r.format = m_decoratorFmt; m_rules.push_back(std::move(_r)); }

        // Numbers
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b\d+\.?\d*(?:[eE][+-]?\d+)?\b)"); _r.format = m_numberFmt; m_rules.push_back(std::move(_r)); }

        // Strings (single + double, with triple-quote handled via block state)
        { HighlightRule _r; _r.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"); _r.format = m_stringFmt; m_rules.push_back(std::move(_r)); }

        // Comments
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(#[^\n]*)"); _r.format = m_commentFmt; m_rules.push_back(std::move(_r)); }

    } else if (m_language == "javascript" || m_language == "typescript") {
        static const QStringList kwJs = {
            "async","await","break","case","catch","class","const","continue",
            "debugger","default","delete","do","else","enum","export",
            "extends","false","finally","for","function","if","implements",
            "import","in","instanceof","interface","let","new","null",
            "package","private","protected","public","return","static",
            "super","switch","this","throw","true","try","typeof","var",
            "void","while","with","yield","of","from","as","type","declare"
        };

        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:)" + kwJs.join('|') + R"()\b)"); _r.format = m_keywordFmt; m_rules.push_back(std::move(_r)); }

        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b(?:console|window|document|process|module|require|exports|Promise|Array|Object|String|Number|Boolean|Math|Date|RegExp|Error|Map|Set|JSON|Symbol)\b)"); _r.format = m_typeFmt; m_rules.push_back(std::move(_r)); }

        // Template literals
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(`(?:[^`\\]|\\.)*`)"); _r.format = m_stringFmt; m_rules.push_back(std::move(_r)); }

        { HighlightRule _r; _r.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"); _r.format = m_stringFmt; m_rules.push_back(std::move(_r)); }

        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b\d+\.?\d*\b)"); _r.format = m_numberFmt; m_rules.push_back(std::move(_r)); }

        { HighlightRule _r; _r.pattern = QRegularExpression(R"(//[^\n]*)"); _r.format = m_commentFmt; m_rules.push_back(std::move(_r)); }

    } else {
        // Generic: strings + numbers
        { HighlightRule _r; _r.pattern = QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')"); _r.format = m_stringFmt; m_rules.push_back(std::move(_r)); }
        { HighlightRule _r; _r.pattern = QRegularExpression(R"(\b\d+\.?\d*\b)"); _r.format = m_numberFmt; m_rules.push_back(std::move(_r)); }
    }
}

// ============================================================
// Main highlight entry point (called per block by Qt)
// ============================================================
void SyntaxHighlighter::highlightBlock(const QString& text)
{
    // ---- Multi-line comment handling (C-style /* … */) ----
    if (m_language == "cpp" || m_language == "javascript" ||
        m_language == "typescript")
    {
        static const QRegularExpression commentStart(R"(/\*)");
        static const QRegularExpression commentEnd(R"(\*/)");

        int startIndex = 0;
        if (previousBlockState() != InMultilineComment) {
            startIndex = text.indexOf(commentStart);
        }

        while (startIndex >= 0) {
            auto endMatch = commentEnd.match(text, startIndex);
            int endIndex  = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
            int length;

            if (endIndex < 0) {
                // Comment continues to next block
                setCurrentBlockState(InMultilineComment);
                length = text.length() - startIndex;
            } else {
                setCurrentBlockState(Normal);
                length = endIndex - startIndex + endMatch.capturedLength();
            }

            setFormat(startIndex, length, m_commentFmt);
            startIndex = text.indexOf(commentStart, startIndex + length);
        }

        if (previousBlockState() == InMultilineComment &&
            currentBlockState() != Normal)
        {
            // Whole line is inside comment
            setFormat(0, text.length(), m_commentFmt);
        }
    }

    // ---- Apply regex rules ----
    for (const auto& rule : m_rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(static_cast<int>(match.capturedStart()),
                      static_cast<int>(match.capturedLength()),
                      rule.format);
        }
    }

    // ---- Diagnostic underlines ----
    int blockLine = currentBlock().blockNumber();
    applyDiagnosticHighlights(text, blockLine);
}

// ============================================================
void SyntaxHighlighter::applyDiagnosticHighlights(const QString& text,
                                                    int blockLine)
{
    for (const auto& diag : m_diagnostics) {
        if (diag.line != blockLine) continue;

        int start = diag.column;
        int end   = (diag.endColumn > diag.column)
                        ? diag.endColumn
                        : static_cast<int>(text.length());
        int len   = qMax(1, end - start);

        const QTextCharFormat& fmt =
            (diag.severity == Diagnostic::Severity::Error)
                ? m_errorFmt : m_warningFmt;

        setFormat(start, len, fmt);
    }
}

} // namespace minicode
