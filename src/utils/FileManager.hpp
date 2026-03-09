#pragma once

#include <QString>
#include <QStringList>
#include <optional>
#include <filesystem>

namespace minicode {

/**
 * @brief FileManager — RAII wrapper for file I/O operations.
 *
 * Handles reading, writing, and tracking recent files.
 * Uses std::filesystem (C++17) internally.
 */
class FileManager {
public:
    FileManager() = default;
    ~FileManager() = default;

    // Non-copyable, movable
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    FileManager(FileManager&&) = default;
    FileManager& operator=(FileManager&&) = default;

    /**
     * @brief Read entire file content into a QString.
     * @return File content, or std::nullopt on error.
     */
    [[nodiscard]] static std::optional<QString> readFile(const QString& path);

    /**
     * @brief Write content to a file (creates/overwrites).
     * @return true on success.
     */
    [[nodiscard]] static bool writeFile(const QString& path, const QString& content);

    /**
     * @brief Detect language from file extension.
     * @return "cpp", "javascript", "python", "text", etc.
     */
    [[nodiscard]] static QString detectLanguage(const QString& path);

    /**
     * @brief Return the file name (without directory).
     */
    [[nodiscard]] static QString fileName(const QString& path);

    /**
     * @brief Return true if path points to an existing regular file.
     */
    [[nodiscard]] static bool exists(const QString& path);

    /**
     * @brief List files recursively in a directory.
     * @param maxDepth  Maximum recursion depth (default: 5).
     */
    [[nodiscard]] static QStringList listDirectory(const QString& dirPath, int maxDepth = 5);

private:
    static QStringList listDirectoryImpl(const std::filesystem::path& dir,
                                          int currentDepth, int maxDepth);
};

} // namespace minicode
