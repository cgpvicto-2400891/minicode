#pragma once

#include "../editor/Editor.hpp"
#include "../lsp/LSPClient.hpp"
#include "../utils/FileManager.hpp"

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QDockWidget>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QProcess>
#include <QToolBar>
#include <memory>
#include <vector>

namespace minicode {

/**
 * @brief MainWindow — Top-level application window.
 *
 * Layout (VS Code–inspired):
 *
 *   ┌─────────────────────────────────────────────┐
 *   │  Menu Bar                                   │
 *   │  Tool Bar                                   │
 *   ├───────────┬─────────────────────────────────┤
 *   │           │                                 │
 *   │  Sidebar  │   Editor Tabs                   │
 *   │  (Files)  │                                 │
 *   │           ├─────────────────────────────────┤
 *   │           │   Integrated Terminal            │
 *   └───────────┴─────────────────────────────────┘
 *   │  Status Bar (line:col, language, LSP)       │
 *   └─────────────────────────────────────────────┘
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    // ---- File actions ----
    void newFile();
    void openFile();
    void openFile(const QString& path);
    void saveFile();
    void saveFileAs();
    void closeTab(int index);

    // ---- Edit actions ----
    void findInFile();
    void findInFiles(); // global search
    void toggleComment();

    // ---- View actions ----
    void toggleSidebar();
    void toggleTerminal();
    void zoomIn();
    void zoomOut();

    // ---- Build & run ----
    void buildProject();
    void runProject();
    void runInTerminal(const QString& cmd);

    // ---- LSP ----
    void startLSP();
    void onLSPStarted();
    void onLSPStopped();
    void onLSPError(const QString& msg);

    // ---- Status updates ----
    void updateStatusBar(int line, int col);
    void onTabChanged(int index);
    void onFileModified(const QString& path);
    void onFileSaved(const QString& path);

    // ---- Sidebar ----
    void onSidebarItemActivated(const QModelIndex& index);
    void openFolder();
    void openFolder(const QString& dir);  // overload for CLI / programmatic use

    // ---- Terminal ----
    void onTerminalCommand();
    void onTerminalOutput();
    void onTerminalFinished();

    // ---- Search bar ----
    void onSearchBarReturn();

private:
    // ---- Setup helpers ----
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupSidebar();
    void setupEditorArea();
    void setupTerminal();
    void setupStatusBar();
    // ---- Themes ----
    enum class Theme { Dark, Light, Solarized };
    void applyTheme(Theme theme);
    void applyDarkTheme();
    void applyThemeToEditor(Editor* ed);
    void applyLightTheme();
    void applySolarizedTheme();

    Theme m_currentTheme {Theme::Dark};

    // ---- Tab helpers ----
    Editor* currentEditor() const;
    Editor* editorAt(int index) const;
    int     findTabForPath(const QString& path) const;
    void    addEditorTab(Editor* editor, const QString& title);

    // ---- Widgets ----
    QSplitter*          m_mainSplitter    {nullptr};
    QSplitter*          m_editorSplitter  {nullptr};

    // Sidebar
    QDockWidget*        m_sidebarDock     {nullptr};
    QWidget*            m_sidebarWidget   {nullptr};
    QTreeView*          m_fileTree        {nullptr};
    QFileSystemModel*   m_fsModel         {nullptr};

    // Editor
    QTabWidget*         m_tabWidget       {nullptr};

    // Find bar
    QWidget*            m_findBar         {nullptr};
    QLineEdit*          m_findInput       {nullptr};

    // Terminal
    QDockWidget*        m_terminalDock    {nullptr};
    QPlainTextEdit*     m_terminalOutput  {nullptr};
    QLineEdit*          m_terminalInput   {nullptr};
    QProcess*           m_terminalProcess {nullptr};

    // Status bar
    QLabel* m_statusPos      {nullptr};
    QLabel* m_statusLang     {nullptr};
    QLabel* m_statusLSP      {nullptr};
    QLabel* m_statusModified {nullptr};

    // LSP
    std::unique_ptr<LSPClient> m_lspClient;

    // Tab counter (for "Untitled N" naming)
    int m_untitledCount {0};
};

} // namespace minicode
