#include "FileManager.hpp"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace minicode {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
std::optional<QString> FileManager::readFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

// ---------------------------------------------------------------------------
bool FileManager::writeFile(const QString& path, const QString& content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    return true;
}

// ---------------------------------------------------------------------------
QString FileManager::detectLanguage(const QString& path)
{
    const QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "cpp" || ext == "cxx" || ext == "cc" || ext == "c" ||
        ext == "hpp" || ext == "hxx" || ext == "h")
        return "cpp";

    if (ext == "js" || ext == "mjs" || ext == "cjs")
        return "javascript";

    if (ext == "ts" || ext == "tsx")
        return "typescript";

    if (ext == "py" || ext == "pyw")
        return "python";

    if (ext == "rs")
        return "rust";

    if (ext == "go")
        return "go";

    if (ext == "java")
        return "java";

    if (ext == "html" || ext == "htm")
        return "html";

    if (ext == "css" || ext == "scss" || ext == "sass")
        return "css";

    if (ext == "json")
        return "json";

    if (ext == "md" || ext == "markdown")
        return "markdown";

    if (ext == "sh" || ext == "bash")
        return "bash";

    return "text";
}

// ---------------------------------------------------------------------------
QString FileManager::fileName(const QString& path)
{
    return QFileInfo(path).fileName();
}

// ---------------------------------------------------------------------------
bool FileManager::exists(const QString& path)
{
    return QFileInfo::exists(path);
}

// ---------------------------------------------------------------------------
QStringList FileManager::listDirectory(const QString& dirPath, int maxDepth)
{
    fs::path dir(dirPath.toStdString());
    if (!fs::is_directory(dir)) return {};
    return listDirectoryImpl(dir, 0, maxDepth);
}

// ---------------------------------------------------------------------------
QStringList FileManager::listDirectoryImpl(const fs::path& dir,
                                            int currentDepth, int maxDepth)
{
    QStringList result;
    if (currentDepth > maxDepth) return result;

    try {
        for (const auto& entry : fs::directory_iterator(dir)) {
            // Skip hidden files/dirs
            if (entry.path().filename().string().starts_with('.')) continue;

            result << QString::fromStdString(entry.path().string());

            if (entry.is_directory()) {
                result += listDirectoryImpl(entry.path(), currentDepth + 1, maxDepth);
            }
        }
    } catch (const fs::filesystem_error&) {
        // Permission denied, etc. — silently skip
    }

    return result;
}

} // namespace minicode
