#include "CodeParser.hpp"

#include <QStringList>
#include <QRegularExpression>
#include <QStack>

namespace minicode {

// ============================================================
// Constructor
// ============================================================
CodeParser::CodeParser(const QString& language)
    : m_language(language)
{}

void CodeParser::setLanguage(const QString& language)
{
    m_language = language;
}

// ============================================================
// Public API
// ============================================================
QVector<Diagnostic> CodeParser::parse(const QString& source)
{
    if (m_language == "cpp") return parseCpp(source);
    if (m_language == "python") return parsePython(source);
    if (m_language == "javascript" || m_language == "typescript")
        return parseJavaScript(source);
    return {};
}

QVector<Token> CodeParser::tokenize(const QString& source)
{
    if (m_language == "cpp")   return tokenizeCpp(source);
    if (m_language == "python") return tokenizePython(source);
    return tokenizeGeneric(source);
}

// ============================================================
// C++ Parser
// ============================================================
QVector<Diagnostic> CodeParser::parseCpp(const QString& source)
{
    QVector<Diagnostic> diags;
    const QStringList lines = source.split('\n');

    // Keywords that typically end a statement (not control flow openers)
    static const QRegularExpression reStmtEnd(
        R"(^\s*(return|break|continue|throw)\b.*[^;{}\s]\s*$)");

    // Function-like call or assignment not followed by ; or {
    // Heuristic: line ends with ) or identifier, not ; or { or ,
    static const QRegularExpression reMissingSemicolon(
        R"(^\s*(?!\/\/)(?!#)(?!if\b)(?!else\b)(?!for\b)(?!while\b)(?!switch\b)(?!do\b)(?!class\b)(?!struct\b)(?!namespace\b)(?!public|private|protected).+\)\s*$)");

    // Unmatched braces / parentheses
    QStack<QPair<QChar, int>> braceStack; // char, line

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];

        // Skip preprocessor / blank / pure comment lines
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#') ||
            trimmed.startsWith("//")) continue;

        // Strip single-line string literals to avoid false positives
        QString stripped = trimmed;
        stripped.remove(QRegularExpression(R"("(?:[^"\\]|\\.)*")"));
        stripped.remove(QRegularExpression(R"('(?:[^'\\]|\\.)*')"));

        // --- Missing semicolon heuristic ---
        // Line ends with ) but not followed by { or ; (common: printf("…"))
        if (stripped.endsWith(')') &&
            !stripped.startsWith("if") &&
            !stripped.startsWith("while") &&
            !stripped.startsWith("for") &&
            !stripped.startsWith("switch") &&
            !stripped.startsWith("//") &&
            !stripped.contains("//")) {

            diags.push_back({
                i, 0, static_cast<int>(line.length()),
                QString("Possible missing semicolon at end of statement"),
                Diagnostic::Severity::Warning
            });
        }

        // --- return/break/continue without semicolon ---
        if (reStmtEnd.match(stripped).hasMatch()) {
            diags.push_back({
                i, 0, static_cast<int>(line.length()),
                QString("Missing semicolon after '%1'").arg(stripped.split(' ').first()),
                Diagnostic::Severity::Error
            });
        }

        // --- Brace matching ---
        for (int col = 0; col < stripped.size(); ++col) {
            QChar c = stripped[col];
            if (c == '{' || c == '(' || c == '[') {
                braceStack.push({c, i});
            } else if (c == '}' || c == ')' || c == ']') {
                if (braceStack.isEmpty()) {
                    diags.push_back({
                        i, col, col + 1,
                        QString("Unmatched closing '%1'").arg(c),
                        Diagnostic::Severity::Error
                    });
                } else {
                    auto [opener, openLine] = braceStack.pop();
                    // Check matching pair
                    bool ok = (opener == '{' && c == '}') ||
                              (opener == '(' && c == ')') ||
                              (opener == '[' && c == ']');
                    if (!ok) {
                        diags.push_back({
                            i, col, col + 1,
                            QString("Mismatched bracket: opened '%1' on line %2, closed '%3' here")
                                .arg(opener).arg(openLine + 1).arg(c),
                            Diagnostic::Severity::Error
                        });
                    }
                }
            }
        }
    }

    // Unclosed braces
    while (!braceStack.isEmpty()) {
        auto [opener, openLine] = braceStack.pop();
        diags.push_back({
            openLine, 0, 1,
            QString("Unclosed '%1' — no matching closing bracket").arg(opener),
            Diagnostic::Severity::Error
        });
    }

    return diags;
}

// ============================================================
// Python Parser
// ============================================================
QVector<Diagnostic> CodeParser::parsePython(const QString& source)
{
    QVector<Diagnostic> diags;
    const QStringList lines = source.split('\n');

    // Detect mixed indentation (tabs + spaces) and basic colon checks
    static const QRegularExpression reColonRequired(
        R"(^\s*(if|elif|else|for|while|def|class|with|try|except|finally)\b.+[^:]$)");

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;

        // Leading whitespace — check for mixed tabs/spaces
        int j = 0;
        bool hasTab = false, hasSpace = false;
        while (j < line.size() && (line[j] == ' ' || line[j] == '\t')) {
            if (line[j] == '\t') hasTab = true;
            if (line[j] == ' ')  hasSpace = true;
            ++j;
        }
        if (hasTab && hasSpace) {
            diags.push_back({
                i, 0, j,
                "Mixed tabs and spaces in indentation (TabError)",
                Diagnostic::Severity::Error
            });
        }

        // Missing colon at end of compound statements
        // Strip inline comments first
        QString noComment = trimmed;
        int commentPos = noComment.indexOf('#');
        if (commentPos > 0) noComment = noComment.left(commentPos).trimmed();

        if (reColonRequired.match(noComment).hasMatch()) {
            diags.push_back({
                i, 0, static_cast<int>(line.length()),
                QString("Expected ':' at end of '%1' statement")
                    .arg(trimmed.split(' ').first()),
                Diagnostic::Severity::Error
            });
        }
    }

    return diags;
}

// ============================================================
// JavaScript Parser
// ============================================================
QVector<Diagnostic> CodeParser::parseJavaScript(const QString& source)
{
    QVector<Diagnostic> diags;
    const QStringList lines = source.split('\n');

    // Very similar to C++ — look for missing semicolons on expression statements
    static const QRegularExpression reExprStmt(
        R"(^\s*(?!\/\/)(?!if\b)(?!else\b)(?!for\b)(?!while\b)(?!function\b)(?!class\b)(?!=>\s*{).+[^;{},]\s*$)");

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("//") ||
            trimmed.startsWith("/*")) continue;

        // Unclosed template literal (backtick) detection
        int backticks = trimmed.count('`');
        if (backticks % 2 != 0) {
            diags.push_back({
                i, 0, static_cast<int>(line.length()),
                "Unclosed template literal (backtick)",
                Diagnostic::Severity::Warning
            });
        }
    }

    return diags;
}

// ============================================================
// Tokenizers
// ============================================================
QVector<Token> CodeParser::tokenizeCpp(const QString& source)
{
    QVector<Token> tokens;

    // C++ keywords
    static const QStringList keywords = {
        "alignas","alignof","and","and_eq","asm","auto","bitand","bitor",
        "bool","break","case","catch","char","char8_t","char16_t","char32_t",
        "class","compl","concept","const","consteval","constexpr","constinit",
        "const_cast","continue","co_await","co_return","co_yield","decltype",
        "default","delete","do","double","dynamic_cast","else","enum",
        "explicit","export","extern","false","float","for","friend","goto",
        "if","inline","int","long","mutable","namespace","new","noexcept",
        "not","not_eq","nullptr","operator","or","or_eq","private","protected",
        "public","register","reinterpret_cast","requires","return","short",
        "signed","sizeof","static","static_assert","static_cast","struct",
        "switch","template","this","thread_local","throw","true","try",
        "typedef","typeid","typename","union","unsigned","using","virtual",
        "void","volatile","wchar_t","while","xor","xor_eq","override","final"
    };

    // Use regex to find all token classes
    struct Rule {
        QString type;
        QRegularExpression re;
    };

    static const QVector<Rule> rules = {
        {"comment",  QRegularExpression(R"(//[^\n]*|/\*[\s\S]*?\*/)")},
        {"string",   QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|R"[^(]*\([\s\S]*?\)[^)]*")")},
        {"preproc",  QRegularExpression(R"(^\s*#[^\n]*)", QRegularExpression::MultilineOption)},
        {"number",   QRegularExpression(R"(\b(?:0x[0-9a-fA-F]+|0b[01]+|\d+\.?\d*(?:[eE][+-]?\d+)?[fFuUlL]*)\b)")},
        {"keyword",  QRegularExpression(R"(\b(?:)" + keywords.join('|') + R"()\b)")},
        {"type",     QRegularExpression(R"(\b(?:std::\w+|size_t|ptrdiff_t|uint\d+_t|int\d+_t)\b)")},
    };

    for (const auto& rule : rules) {
        auto it = rule.re.globalMatch(source);
        while (it.hasNext()) {
            auto m = it.next();
            tokens.push_back({
                static_cast<int>(m.capturedStart()),
                static_cast<int>(m.capturedEnd()),
                rule.type
            });
        }
    }

    return tokens;
}

QVector<Token> CodeParser::tokenizePython(const QString& source)
{
    QVector<Token> tokens;

    static const QStringList keywords = {
        "False","None","True","and","as","assert","async","await",
        "break","class","continue","def","del","elif","else","except",
        "finally","for","from","global","if","import","in","is",
        "lambda","nonlocal","not","or","pass","raise","return",
        "try","while","with","yield"
    };

    struct Rule { QString type; QRegularExpression re; };
    static const QVector<Rule> rules = {
        {"comment", QRegularExpression(R"(#[^\n]*)")},
        {"string",  QRegularExpression(R"(\"\"\"[\s\S]*?\"\"\"|\'\'\'[\s\S]*?\'\'\'|\"(?:[^\"\\]|\\.)*\"|\'(?:[^\'\\]|\\.)*\')")},
        {"number",  QRegularExpression(R"(\b\d+\.?\d*(?:[eE][+-]?\d+)?\b)")},
        {"keyword", QRegularExpression(R"(\b(?:)" + keywords.join('|') + R"()\b)")},
        {"decorator",QRegularExpression(R"(@\w+)")},
    };

    for (const auto& rule : rules) {
        auto it = rule.re.globalMatch(source);
        while (it.hasNext()) {
            auto m = it.next();
            tokens.push_back({
                static_cast<int>(m.capturedStart()),
                static_cast<int>(m.capturedEnd()),
                rule.type
            });
        }
    }
    return tokens;
}

QVector<Token> CodeParser::tokenizeGeneric(const QString& source)
{
    QVector<Token> tokens;
    struct Rule { QString type; QRegularExpression re; };
    static const QVector<Rule> rules = {
        {"string",  QRegularExpression(R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*')")},
        {"number",  QRegularExpression(R"(\b\d+\.?\d*\b)")},
        {"comment", QRegularExpression(R"(//[^\n]*|#[^\n]*)")},
    };
    for (const auto& rule : rules) {
        auto it = rule.re.globalMatch(source);
        while (it.hasNext()) {
            auto m = it.next();
            tokens.push_back({
                static_cast<int>(m.capturedStart()),
                static_cast<int>(m.capturedEnd()),
                rule.type
            });
        }
    }
    return tokens;
}

} // namespace minicode
