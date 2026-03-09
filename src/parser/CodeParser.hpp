#pragma once

#include <QString>
#include <QVector>
#include <memory>

namespace minicode {

/**
 * @brief A single diagnostic error/warning found by the parser.
 */
struct Diagnostic {
    int  line;        ///< 0-based line number
    int  column;      ///< 0-based column
    int  endColumn;   ///< end column (for underlining)
    QString message;  ///< Human-readable error message
    enum class Severity { Error, Warning, Info } severity;
};

/**
 * @brief A syntax token produced by the parser.
 */
struct Token {
    int     startOffset; ///< byte offset in document
    int     endOffset;
    QString type;        ///< "keyword", "string", "comment", "number", …
};

/**
 * @brief CodeParser — lightweight rule-based code parser.
 *
 * Performs:
 *   1. Basic syntax error detection (missing semicolons, unmatched braces, etc.)
 *   2. Token extraction for syntax highlighting fallback
 *
 * Tree-sitter integration can replace this when the native library is available.
 * The interface is kept identical so the swap is transparent.
 */
class CodeParser {
public:
    explicit CodeParser(const QString& language);
    ~CodeParser() = default;

    CodeParser(const CodeParser&) = delete;
    CodeParser& operator=(const CodeParser&) = delete;

    /**
     * @brief Parse source text and return diagnostics.
     */
    [[nodiscard]] QVector<Diagnostic> parse(const QString& source);

    /**
     * @brief Tokenise source text for syntax highlighting.
     */
    [[nodiscard]] QVector<Token> tokenize(const QString& source);

    /**
     * @brief Change the active language (re-uses same object).
     */
    void setLanguage(const QString& language);

    [[nodiscard]] const QString& language() const { return m_language; }

private:
    QString m_language;

    // Language-specific parsers
    QVector<Diagnostic> parseCpp(const QString& source);
    QVector<Diagnostic> parsePython(const QString& source);
    QVector<Diagnostic> parseJavaScript(const QString& source);

    QVector<Token> tokenizeCpp(const QString& source);
    QVector<Token> tokenizePython(const QString& source);
    QVector<Token> tokenizeGeneric(const QString& source);
};

} // namespace minicode
