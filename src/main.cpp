/**
 * @file main.cpp
 * @brief MiniCode — Mini VS Code clone for Linux
 *
 * Entry point. Initialises the Qt application and launches MainWindow.
 *
 * Build:
 *   mkdir build && cd build && cmake .. && make -j$(nproc)
 *
 * Run:
 *   ./MiniCode [file1] [file2] …
 */

#include "ui/MainWindow.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QDir>
#include <QDebug>

int main(int argc, char* argv[])
{
    // ---- Qt high-DPI support ----
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("MiniCode");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MiniCode");
    app.setOrganizationDomain("minicode.dev");

    // ---- Command line ----
    QCommandLineParser parser;
    parser.setApplicationDescription("MiniCode — A lightweight code editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("files", "Files to open on startup", "[file...]");
    parser.process(app);

    const QStringList filesToOpen = parser.positionalArguments();

    // ---- Main window ----
    minicode::MainWindow window;
    window.show();

    // Open files/folders passed as arguments
    // Use QDir::current() to resolve relative paths correctly
    const QString cwd = QDir::currentPath();

    for (const QString& arg : filesToOpen) {
        // Resolve relative to the CWD where minicode was invoked
        const QString abs = QFileInfo(QDir(cwd), arg).absoluteFilePath();
        const QFileInfo info(abs);

        if (info.isDir()) {
            // Open as folder in sidebar
            QMetaObject::invokeMethod(&window, "openFolder",
                                       Qt::QueuedConnection,
                                       Q_ARG(QString, abs));
        } else if (info.isFile()) {
            QMetaObject::invokeMethod(&window, "openFile",
                                       Qt::QueuedConnection,
                                       Q_ARG(QString, abs));
        } else {
            qWarning() << "Path not found:" << abs;
        }
    }

    return app.exec();
}
