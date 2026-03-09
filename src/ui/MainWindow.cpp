#include "MainWindow.hpp"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QInputDialog>
#include <QKeySequence>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QHeaderView>
#include <QDir>
#include <QFontDatabase>
#include <QTimer>
#include <QRegularExpression>
#include <QActionGroup>

namespace minicode {

// ============================================================
// Constructor
// ============================================================
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_lspClient(std::make_unique<LSPClient>(this))
{
    setWindowTitle("MiniCode");
    setMinimumSize(1024, 700);
    resize(1400, 900);

    setAcceptDrops(true);

    setupUI();
    applyDarkTheme();

    // Connect LSP signals
    connect(m_lspClient.get(), &LSPClient::serverStarted,
            this, &MainWindow::onLSPStarted);
    connect(m_lspClient.get(), &LSPClient::serverStopped,
            this, &MainWindow::onLSPStopped);
    connect(m_lspClient.get(), &LSPClient::serverError,
            this, &MainWindow::onLSPError);

    // Auto-start clangd if available
    QTimer::singleShot(500, this, &MainWindow::startLSP);
}

// ============================================================
// UI Setup
// ============================================================
void MainWindow::setupUI()
{
    setupMenuBar();
    setupToolBar();
    setupSidebar();
    setupEditorArea();
    setupTerminal();
    setupStatusBar();

    // Find bar (hidden by default)
    m_findBar   = new QWidget(this);
    m_findInput = new QLineEdit(m_findBar);
    m_findInput->setPlaceholderText("Find…");
    m_findInput->setMaximumWidth(300);

    auto* findLayout  = new QHBoxLayout(m_findBar);
    auto* closeFind   = new QPushButton("✕", m_findBar);
    closeFind->setFixedWidth(24);
    auto* prevBtn     = new QPushButton("▲", m_findBar);
    auto* nextBtn     = new QPushButton("▼", m_findBar);
    prevBtn->setFixedWidth(24); nextBtn->setFixedWidth(24);

    findLayout->addStretch();
    findLayout->addWidget(new QLabel("Find:", m_findBar));
    findLayout->addWidget(m_findInput);
    findLayout->addWidget(prevBtn);
    findLayout->addWidget(nextBtn);
    findLayout->addWidget(closeFind);
    findLayout->setContentsMargins(4, 2, 4, 2);
    m_findBar->setMaximumHeight(32);
    m_findBar->hide();

    connect(m_findInput, &QLineEdit::returnPressed,
            this, &MainWindow::onSearchBarReturn);
    connect(nextBtn, &QPushButton::clicked, [this]() {
        if (auto* ed = currentEditor()) ed->findNext();
    });
    connect(prevBtn, &QPushButton::clicked, [this]() {
        if (auto* ed = currentEditor()) ed->findPrev();
    });
    connect(closeFind, &QPushButton::clicked, [this]() {
        m_findBar->hide();
    });

    // Central widget: vertical splitter (editor + find bar)
    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_tabWidget);
    centralLayout->addWidget(m_findBar);
    setCentralWidget(centralWidget);

    // Open welcome tab AFTER window is fully constructed
    newFile();
}

// ============================================================
void MainWindow::setupMenuBar()
{
    auto* mb = menuBar();

    // Helper lambda — creates a QAction with shortcut and connects it
    // Avoids deprecated Qt6 addAction(text, obj, slot, shortcut) overloads
    auto act = [](QMenu* menu, const QString& text,
                  const QKeySequence& key = {}) -> QAction* {
        auto* a = menu->addAction(text);
        if (!key.isEmpty()) a->setShortcut(key);
        return a;
    };
    auto actIcon = [](QMenu* menu, const QIcon& icon, const QString& text,
                      const QKeySequence& key = {}) -> QAction* {
        auto* a = menu->addAction(icon, text);
        if (!key.isEmpty()) a->setShortcut(key);
        return a;
    };

    // ---- File ----
    auto* fileMenu = mb->addMenu("&File");
    connect(actIcon(fileMenu, QIcon::fromTheme("document-new"),  "&New File",   QKeySequence::New),
            &QAction::triggered, this, &MainWindow::newFile);
    connect(actIcon(fileMenu, QIcon::fromTheme("document-open"), "&Open File…", QKeySequence::Open),
            &QAction::triggered, this, QOverload<>::of(&MainWindow::openFile));
    connect(act(fileMenu, "Open &Folder…"),
            &QAction::triggered, this, QOverload<>::of(&MainWindow::openFolder));
    fileMenu->addSeparator();
    connect(actIcon(fileMenu, QIcon::fromTheme("document-save"), "&Save", QKeySequence::Save),
            &QAction::triggered, this, &MainWindow::saveFile);
    connect(act(fileMenu, "Save &As…", QKeySequence::SaveAs),
            &QAction::triggered, this, &MainWindow::saveFileAs);
    fileMenu->addSeparator();
    connect(act(fileMenu, "E&xit", QKeySequence::Quit),
            &QAction::triggered, qApp, &QApplication::quit);

    // ---- Edit ----
    auto* editMenu = mb->addMenu("&Edit");
    connect(actIcon(editMenu, QIcon::fromTheme("edit-undo"), "&Undo", QKeySequence::Undo),
            &QAction::triggered, this, [this]() { if (auto* e = currentEditor()) e->undo(); });
    connect(actIcon(editMenu, QIcon::fromTheme("edit-redo"), "&Redo", QKeySequence::Redo),
            &QAction::triggered, this, [this]() { if (auto* e = currentEditor()) e->redo(); });
    editMenu->addSeparator();
    connect(actIcon(editMenu, QIcon::fromTheme("edit-cut"),   "Cu&t",   QKeySequence::Cut),
            &QAction::triggered, this, [this]() { if (auto* e = currentEditor()) e->cut(); });
    connect(actIcon(editMenu, QIcon::fromTheme("edit-copy"),  "&Copy",  QKeySequence::Copy),
            &QAction::triggered, this, [this]() { if (auto* e = currentEditor()) e->copy(); });
    connect(actIcon(editMenu, QIcon::fromTheme("edit-paste"), "&Paste", QKeySequence::Paste),
            &QAction::triggered, this, [this]() { if (auto* e = currentEditor()) e->paste(); });
    editMenu->addSeparator();
    connect(act(editMenu, "&Find…",           QKeySequence::Find),
            &QAction::triggered, this, &MainWindow::findInFile);
    connect(act(editMenu, "Find in &Files…"),
            &QAction::triggered, this, &MainWindow::findInFiles);
    editMenu->addSeparator();
    connect(act(editMenu, "Toggle Line &Comment", QKeySequence("Ctrl+/")),
            &QAction::triggered, this, &MainWindow::toggleComment);

    // ---- View ----
    auto* viewMenu = mb->addMenu("&View");
    connect(act(viewMenu, "Toggle &Sidebar",  QKeySequence("Ctrl+B")),
            &QAction::triggered, this, &MainWindow::toggleSidebar);
    connect(act(viewMenu, "Toggle &Terminal", QKeySequence("Ctrl+`")),
            &QAction::triggered, this, &MainWindow::toggleTerminal);
    viewMenu->addSeparator();
    connect(act(viewMenu, "Zoom &In",  QKeySequence::ZoomIn),
            &QAction::triggered, this, &MainWindow::zoomIn);
    connect(act(viewMenu, "Zoom &Out", QKeySequence::ZoomOut),
            &QAction::triggered, this, &MainWindow::zoomOut);

    // ---- View / Theme submenu ----
    auto* themeMenu = viewMenu->addMenu("&Theme");
    auto* darkAct      = act(themeMenu, "🌑  Dark");
    auto* lightAct     = act(themeMenu, "☀️   Light");
    auto* solarizedAct = act(themeMenu, "🌿  Solarized (Eye Care)");
    darkAct->setCheckable(true);      darkAct->setChecked(true);
    lightAct->setCheckable(true);
    solarizedAct->setCheckable(true);
    auto* themeGroup = new QActionGroup(this);
    themeGroup->addAction(darkAct);
    themeGroup->addAction(lightAct);
    themeGroup->addAction(solarizedAct);
    themeGroup->setExclusive(true);
    connect(darkAct,      &QAction::triggered, this, [this]{ applyTheme(Theme::Dark); });
    connect(lightAct,     &QAction::triggered, this, [this]{ applyTheme(Theme::Light); });
    connect(solarizedAct, &QAction::triggered, this, [this]{ applyTheme(Theme::Solarized); });

    // ---- Run ----
    auto* runMenu = mb->addMenu("&Run");
    connect(act(runMenu, "&Build (make)", QKeySequence("Ctrl+Shift+B")),
            &QAction::triggered, this, &MainWindow::buildProject);
    connect(act(runMenu, "&Run", QKeySequence("F5")),
            &QAction::triggered, this, &MainWindow::runProject);
    runMenu->addSeparator();
    connect(act(runMenu, "Run in &Terminal…"),
            &QAction::triggered, this, [this]() {
        bool ok;
        QString cmd = QInputDialog::getText(this, "Run Command",
            "Command:", QLineEdit::Normal, "", &ok);
        if (ok && !cmd.isEmpty()) runInTerminal(cmd);
    });
}

// ============================================================
void MainWindow::setupToolBar()
{
    auto* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setIconSize(QSize(18, 18));

    tb->addAction(QIcon::fromTheme("document-new"),   "New",  this, &MainWindow::newFile);
    tb->addAction(QIcon::fromTheme("document-open"),  "Open", this, QOverload<>::of(&MainWindow::openFile));
    tb->addAction(QIcon::fromTheme("document-save"),  "Save", this, &MainWindow::saveFile);
    tb->addSeparator();
    tb->addAction(QIcon::fromTheme("edit-undo"),      "Undo", [this]() { if (auto* e = currentEditor()) e->undo(); });
    tb->addAction(QIcon::fromTheme("edit-redo"),      "Redo", [this]() { if (auto* e = currentEditor()) e->redo(); });
    tb->addSeparator();
    tb->addAction(QIcon::fromTheme("system-run"),     "Run",  this, &MainWindow::runProject);
}

// ============================================================
void MainWindow::setupSidebar()
{
    m_sidebarDock   = new QDockWidget("Explorer", this);
    m_sidebarDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_sidebarDock->setFeatures(QDockWidget::DockWidgetClosable |
                                 QDockWidget::DockWidgetMovable);
    m_sidebarDock->setMinimumWidth(180);

    m_sidebarWidget = new QWidget;
    auto* layout    = new QVBoxLayout(m_sidebarWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Header
    auto* header = new QLabel("  EXPLORER", m_sidebarWidget);
    header->setFixedHeight(26);
    header->setStyleSheet(
        "background:#252526; color:#bbb; font-size:10px; font-weight:bold;");
    layout->addWidget(header);

    // File tree
    m_fsModel  = new QFileSystemModel(this);
    m_fsModel->setRootPath(QDir::homePath());
    m_fsModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    m_fileTree = new QTreeView(m_sidebarWidget);
    m_fileTree->setModel(m_fsModel);
    m_fileTree->setRootIndex(m_fsModel->index(QDir::homePath()));
    m_fileTree->setHeaderHidden(true);
    m_fileTree->hideColumn(1); m_fileTree->hideColumn(2); m_fileTree->hideColumn(3);
    m_fileTree->setAnimated(true);
    m_fileTree->setIndentation(12);
    m_fileTree->setFrameShape(QFrame::NoFrame);
    m_fileTree->setStyleSheet(
        "QTreeView { background:#252526; color:#ccc; border:none; }"
        "QTreeView::item:hover    { background:#2a2d2e; }"
        "QTreeView::item:selected { background:#094771; color:#fff; }");

    connect(m_fileTree, &QTreeView::activated,
            this, &MainWindow::onSidebarItemActivated);

    layout->addWidget(m_fileTree);
    m_sidebarDock->setWidget(m_sidebarWidget);

    addDockWidget(Qt::LeftDockWidgetArea, m_sidebarDock);
}

// ============================================================
void MainWindow::setupEditorArea()
{
    m_tabWidget = new QTabWidget;
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setUsesScrollButtons(true);

    m_tabWidget->setStyleSheet(
        "QTabWidget::pane   { border:0; background:#1e1e1e; }"
        "QTabBar::tab       { background:#2d2d2d; color:#aaa; padding:5px 14px;"
        "                     border-right:1px solid #333; }"
        "QTabBar::tab:selected { background:#1e1e1e; color:#fff;"
        "                        border-top:2px solid #007acc; }"
        "QTabBar::tab:hover { background:#252526; }");

    connect(m_tabWidget, &QTabWidget::tabCloseRequested,
            this, &MainWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::onTabChanged);

    // Welcome tab opened at end of setupUI() after window is fully constructed
}

// ============================================================
void MainWindow::setupTerminal()
{
    m_terminalDock = new QDockWidget("Terminal", this);
    m_terminalDock->setAllowedAreas(Qt::BottomDockWidgetArea |
                                      Qt::TopDockWidgetArea);
    m_terminalDock->setFeatures(QDockWidget::DockWidgetClosable |
                                  QDockWidget::DockWidgetMovable);

    auto* w      = new QWidget;
    auto* layout = new QVBoxLayout(w);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Output area
    m_terminalOutput = new QPlainTextEdit(w);
    m_terminalOutput->setReadOnly(true);
    m_terminalOutput->setMaximumBlockCount(5000);
    m_terminalOutput->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_terminalOutput->setStyleSheet(
        "QPlainTextEdit { background:#0c0c0c; color:#cccccc;"
        "                  border:none; font-size:12px; }");

    // Input row
    auto* inputRow = new QWidget(w);
    auto* inputLay = new QHBoxLayout(inputRow);
    inputLay->setContentsMargins(0, 0, 0, 0);
    inputLay->setSpacing(4);

    auto* prompt = new QLabel("$", inputRow);
    prompt->setStyleSheet("color:#0c0; font-weight:bold;");

    m_terminalInput = new QLineEdit(inputRow);
    m_terminalInput->setStyleSheet(
        "QLineEdit { background:#1e1e1e; color:#ddd; border:none; font-size:12px; }");
    m_terminalInput->setPlaceholderText("Enter command…");

    inputLay->addWidget(prompt);
    inputLay->addWidget(m_terminalInput);
    inputRow->setFixedHeight(28);

    layout->addWidget(m_terminalOutput);
    layout->addWidget(inputRow);

    m_terminalDock->setWidget(w);
    m_terminalDock->setMinimumHeight(150);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalDock);

    // Terminal process
    m_terminalProcess = new QProcess(this);
    m_terminalProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_terminalInput, &QLineEdit::returnPressed,
            this, &MainWindow::onTerminalCommand);
    connect(m_terminalProcess, &QProcess::readyRead,
            this, &MainWindow::onTerminalOutput);
    connect(m_terminalProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onTerminalFinished);
}

// ============================================================
void MainWindow::setupStatusBar()
{
    m_statusPos      = new QLabel("Ln 1, Col 1");
    m_statusLang     = new QLabel("Plain Text");
    m_statusLSP      = new QLabel("LSP: Off");
    m_statusModified = new QLabel("");

    statusBar()->addWidget(m_statusPos);
    statusBar()->addWidget(new QLabel("|"));
    statusBar()->addWidget(m_statusLang);
    statusBar()->addPermanentWidget(m_statusModified);
    statusBar()->addPermanentWidget(m_statusLSP);
    statusBar()->addPermanentWidget(new QLabel("|"));
    statusBar()->setStyleSheet(
        "QStatusBar { background:#007acc; color:white; font-size:12px; }"
        "QStatusBar QLabel { color:white; padding:0 6px; }");
}

// ============================================================
// Theme system
// ============================================================
void MainWindow::applyTheme(Theme theme)
{
    m_currentTheme = theme;
    switch (theme) {
        case Theme::Dark:       applyDarkTheme();       break;
        case Theme::Light:      applyLightTheme();      break;
        case Theme::Solarized:  applySolarizedTheme();  break;
    }
    // Re-apply editor colours for all open tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i)) {
            applyThemeToEditor(ed);
        }
    }
}

void MainWindow::applyThemeToEditor(Editor* ed)
{
    QPalette p = ed->palette();
    switch (m_currentTheme) {
        case Theme::Dark:
            p.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
            p.setColor(QPalette::Text,            QColor(0xd4, 0xd4, 0xd4));
            p.setColor(QPalette::Highlight,       QColor(0x26, 0x4f, 0x78));
            p.setColor(QPalette::HighlightedText, QColor(0xd4, 0xd4, 0xd4));
            break;
        case Theme::Light:
            p.setColor(QPalette::Base,            QColor(0xff, 0xff, 0xff));
            p.setColor(QPalette::Text,            QColor(0x1e, 0x1e, 0x1e));
            p.setColor(QPalette::Highlight,       QColor(0xad, 0xd6, 0xff));
            p.setColor(QPalette::HighlightedText, QColor(0x00, 0x00, 0x00));
            break;
        case Theme::Solarized:
            p.setColor(QPalette::Base,            QColor(0xfd, 0xf6, 0xe3)); // cream
            p.setColor(QPalette::Text,            QColor(0x65, 0x7b, 0x83)); // slate
            p.setColor(QPalette::Highlight,       QColor(0x26, 0x8b, 0xd2)); // blue
            p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
            break;
    }
    ed->setPalette(p);
}

void MainWindow::applyDarkTheme()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor(0x25, 0x25, 0x26));
    p.setColor(QPalette::WindowText,      QColor(0xcc, 0xcc, 0xcc));
    p.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::AlternateBase,   QColor(0x25, 0x25, 0x26));
    p.setColor(QPalette::Text,            QColor(0xd4, 0xd4, 0xd4));
    p.setColor(QPalette::Button,          QColor(0x3c, 0x3c, 0x3c));
    p.setColor(QPalette::ButtonText,      QColor(0xcc, 0xcc, 0xcc));
    p.setColor(QPalette::Highlight,       QColor(0x00, 0x7a, 0xcc));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor(0x25, 0x25, 0x26));
    p.setColor(QPalette::ToolTipText,     QColor(0xcc, 0xcc, 0xcc));
    qApp->setPalette(p);
    setStyleSheet(
        "QMenuBar { background:#3c3c3c; color:#ccc; }"
        "QMenuBar::item:selected { background:#094771; }"
        "QMenu { background:#252526; color:#ccc; border:1px solid #444; }"
        "QMenu::item:selected { background:#094771; }"
        "QToolBar { background:#333; border:none; spacing:4px; }"
        "QToolBar QToolButton { color:#ccc; }"
        "QToolBar QToolButton:hover { background:#444; }"
        "QDockWidget::title { background:#252526; color:#bbb; padding:4px; }"
        "QScrollBar:vertical { width:10px; background:#1e1e1e; }"
        "QScrollBar::handle:vertical { background:#424242; min-height:20px; }"
        "QScrollBar:horizontal { height:10px; background:#1e1e1e; }"
        "QScrollBar::handle:horizontal { background:#424242; }"
        "QPlainTextEdit { background:#1e1e1e; color:#d4d4d4; }"
        "QStatusBar { background:#007acc; color:white; }"
        "QStatusBar QLabel { color:white; padding:0 6px; }");
    // Terminal
    if (m_terminalOutput)
        m_terminalOutput->setStyleSheet(
            "QPlainTextEdit { background:#0c0c0c; color:#cccccc; border:none; font-size:12px; }");
}

void MainWindow::applyLightTheme()
{
    QPalette p;
    p.setColor(QPalette::Window,          QColor(0xf3, 0xf3, 0xf3));
    p.setColor(QPalette::WindowText,      QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::Base,            QColor(0xff, 0xff, 0xff));
    p.setColor(QPalette::AlternateBase,   QColor(0xf3, 0xf3, 0xf3));
    p.setColor(QPalette::Text,            QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::Button,          QColor(0xe8, 0xe8, 0xe8));
    p.setColor(QPalette::ButtonText,      QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::Highlight,       QColor(0x00, 0x7a, 0xcc));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     Qt::white);
    p.setColor(QPalette::ToolTipText,     QColor(0x1e, 0x1e, 0x1e));
    qApp->setPalette(p);
    setStyleSheet(
        "QMenuBar { background:#f3f3f3; color:#333; border-bottom:1px solid #ddd; }"
        "QMenuBar::item:selected { background:#007acc; color:white; }"
        "QMenu { background:#ffffff; color:#333; border:1px solid #ccc; }"
        "QMenu::item:selected { background:#007acc; color:white; }"
        "QToolBar { background:#f3f3f3; border:none; spacing:4px; }"
        "QToolBar QToolButton { color:#333; }"
        "QToolBar QToolButton:hover { background:#ddd; }"
        "QDockWidget::title { background:#e8e8e8; color:#333; padding:4px; }"
        "QTreeView { background:#f3f3f3; color:#333; border:none; }"
        "QTreeView::item:hover    { background:#e8e8e8; }"
        "QTreeView::item:selected { background:#007acc; color:white; }"
        "QScrollBar:vertical { width:10px; background:#f3f3f3; }"
        "QScrollBar::handle:vertical { background:#c0c0c0; min-height:20px; }"
        "QScrollBar:horizontal { height:10px; background:#f3f3f3; }"
        "QScrollBar::handle:horizontal { background:#c0c0c0; }"
        "QTabWidget::pane   { border:0; background:#ffffff; }"
        "QTabBar::tab       { background:#ececec; color:#555; padding:5px 14px; border-right:1px solid #ddd; }"
        "QTabBar::tab:selected { background:#ffffff; color:#000; border-top:2px solid #007acc; }"
        "QTabBar::tab:hover { background:#e0e0e0; }"
        "QStatusBar { background:#007acc; color:white; }"
        "QStatusBar QLabel { color:white; padding:0 6px; }");
    if (m_terminalOutput)
        m_terminalOutput->setStyleSheet(
            "QPlainTextEdit { background:#ffffff; color:#333333; border:none; font-size:12px; }");
}

void MainWindow::applySolarizedTheme()
{
    // Solarized Light — warm cream background, easy on the eyes
    // Palette: https://ethanschoonover.com/solarized/
    const QColor base03 (0x00, 0x2b, 0x36);
    const QColor base02 (0x07, 0x36, 0x42);
    const QColor base3  (0xfd, 0xf6, 0xe3); // main bg
    const QColor base2  (0xee, 0xe8, 0xd5); // alt bg
    const QColor base1  (0x93, 0xa1, 0xa1); // comments
    const QColor base00 (0x65, 0x7b, 0x83); // body text
    const QColor base01 (0x58, 0x6e, 0x75);
    const QColor blue   (0x26, 0x8b, 0xd2);
    const QColor cyan   (0x2a, 0xa1, 0x98);
    const QColor green  (0x85, 0x99, 0x00);

    QPalette p;
    p.setColor(QPalette::Window,          base2);
    p.setColor(QPalette::WindowText,      base00);
    p.setColor(QPalette::Base,            base3);
    p.setColor(QPalette::AlternateBase,   base2);
    p.setColor(QPalette::Text,            base00);
    p.setColor(QPalette::Button,          base2);
    p.setColor(QPalette::ButtonText,      base00);
    p.setColor(QPalette::Highlight,       blue);
    p.setColor(QPalette::HighlightedText, base3);
    p.setColor(QPalette::ToolTipBase,     base2);
    p.setColor(QPalette::ToolTipText,     base00);
    qApp->setPalette(p);

    const QString bg    = base3.name();
    const QString bg2   = base2.name();
    const QString fg    = base00.name();
    const QString sel   = blue.name();
    setStyleSheet(QString(
        "QMenuBar { background:%2; color:%3; border-bottom:1px solid #c9b89a; }"
        "QMenuBar::item:selected { background:%4; color:#fdf6e3; }"
        "QMenu { background:%1; color:%3; border:1px solid #c9b89a; }"
        "QMenu::item:selected { background:%4; color:#fdf6e3; }"
        "QToolBar { background:%2; border:none; spacing:4px; }"
        "QToolBar QToolButton { color:%3; }"
        "QToolBar QToolButton:hover { background:#d5ccbb; }"
        "QDockWidget::title { background:%2; color:%3; padding:4px; }"
        "QTreeView { background:%2; color:%3; border:none; }"
        "QTreeView::item:hover    { background:#e0d9ca; }"
        "QTreeView::item:selected { background:%4; color:#fdf6e3; }"
        "QScrollBar:vertical { width:10px; background:%1; }"
        "QScrollBar::handle:vertical { background:#c9b89a; min-height:20px; }"
        "QScrollBar:horizontal { height:10px; background:%1; }"
        "QScrollBar::handle:horizontal { background:#c9b89a; }"
        "QTabWidget::pane   { border:0; background:%1; }"
        "QTabBar::tab       { background:%2; color:#888; padding:5px 14px; border-right:1px solid #c9b89a; }"
        "QTabBar::tab:selected { background:%1; color:%3; border-top:2px solid %4; }"
        "QTabBar::tab:hover { background:#e5ddd0; }"
        "QStatusBar { background:#859900; color:white; }"
        "QStatusBar QLabel { color:white; padding:0 6px; }"
    ).arg(bg, bg2, fg, sel));
    if (m_terminalOutput)
        m_terminalOutput->setStyleSheet(QString(
            "QPlainTextEdit { background:%1; color:%2; border:none; font-size:12px; }"
        ).arg(base2.name(), base00.name()));
}

// ============================================================
// Tab / Editor management
// ============================================================
Editor* MainWindow::currentEditor() const
{
    return qobject_cast<Editor*>(m_tabWidget->currentWidget());
}

Editor* MainWindow::editorAt(int index) const
{
    return qobject_cast<Editor*>(m_tabWidget->widget(index));
}

int MainWindow::findTabForPath(const QString& path) const
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i); ed && ed->filePath() == path)
            return i;
    }
    return -1;
}

void MainWindow::addEditorTab(Editor* editor, const QString& title)
{
    editor->setLSPClient(m_lspClient.get());
    applyThemeToEditor(editor);  // apply current theme to new tab

    connect(editor, &Editor::cursorPositionInfo,
            this, &MainWindow::updateStatusBar);
    connect(editor, &Editor::fileModified,
            this, &MainWindow::onFileModified);
    connect(editor, &Editor::fileSaved,
            this, &MainWindow::onFileSaved);

    const int idx = m_tabWidget->addTab(editor, title);
    m_tabWidget->setCurrentIndex(idx);
    // setFocus() deferred — called by Qt automatically when tab becomes visible
}

// ============================================================
// File actions
// ============================================================
void MainWindow::newFile()
{
    ++m_untitledCount;
    auto* editor = new Editor;
    addEditorTab(editor, QString("Untitled %1").arg(m_untitledCount));
}

void MainWindow::openFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open File", QDir::homePath(),
        "All Files (*);;C/C++ (*.cpp *.hpp *.c *.h);;"
        "Python (*.py);;JavaScript (*.js *.ts);;JSON (*.json)");
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::openFile(const QString& path)
{
    // Check if already open
    const int existing = findTabForPath(path);
    if (existing >= 0) {
        m_tabWidget->setCurrentIndex(existing);
        return;
    }

    auto* editor = new Editor;
    if (!editor->openFile(path)) {
        delete editor;
        QMessageBox::critical(this, "Error",
            QString("Cannot open file:\n%1").arg(path));
        return;
    }

    addEditorTab(editor, FileManager::fileName(path));

    // Update sidebar to show the file's directory
    const QString dir = QFileInfo(path).absolutePath();
    m_fileTree->setRootIndex(m_fsModel->index(dir));
}

void MainWindow::saveFile()
{
    auto* ed = currentEditor();
    if (!ed) return;

    if (ed->filePath().isEmpty()) {
        saveFileAs();
        return;
    }
    ed->saveFile();
}

void MainWindow::saveFileAs()
{
    auto* ed = currentEditor();
    if (!ed) return;

    const QString path = QFileDialog::getSaveFileName(
        this, "Save File As",
        ed->filePath().isEmpty() ? QDir::homePath() : ed->filePath(),
        "All Files (*)");
    if (path.isEmpty()) return;

    if (ed->saveFileAs(path)) {
        m_tabWidget->setTabText(m_tabWidget->currentIndex(),
                                 FileManager::fileName(path));
    }
}

void MainWindow::closeTab(int index)
{
    auto* ed = editorAt(index);
    if (ed && ed->isModified()) {
        const auto btn = QMessageBox::question(
            this, "Unsaved changes",
            QString("Save changes to '%1'?")
                .arg(m_tabWidget->tabText(index)),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (btn == QMessageBox::Cancel) return;
        if (btn == QMessageBox::Save)   ed->saveFile();
    }
    m_tabWidget->removeTab(index);
}

// ============================================================
// Edit actions
// ============================================================
void MainWindow::findInFile()
{
    m_findBar->setVisible(!m_findBar->isVisible());
    if (m_findBar->isVisible()) m_findInput->setFocus();
}

void MainWindow::findInFiles()
{
    bool ok;
    const QString text = QInputDialog::getText(
        this, "Find in Files", "Search:", QLineEdit::Normal, "", &ok);
    if (!ok || text.isEmpty()) return;

    // Search all open editors
    QString results;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i)) {
            const QStringList lines = ed->toPlainText().split('\n');
            for (int ln = 0; ln < lines.size(); ++ln) {
                if (lines[ln].contains(text, Qt::CaseInsensitive)) {
                    results += QString("[%1:%2] %3\n")
                        .arg(m_tabWidget->tabText(i))
                        .arg(ln + 1)
                        .arg(lines[ln].trimmed());
                }
            }
        }
    }

    if (results.isEmpty())
        results = "No results found for: " + text;

    m_terminalOutput->appendPlainText("=== Find in Files: " + text + " ===");
    m_terminalOutput->appendPlainText(results);
    m_terminalDock->show();
}

void MainWindow::toggleComment()
{
    // Delegate to the editor key event (Ctrl+/)
    if (auto* ed = currentEditor())
        QApplication::postEvent(ed,
            new QKeyEvent(QEvent::KeyPress, Qt::Key_Slash,
                          Qt::ControlModifier));
}

void MainWindow::onSearchBarReturn()
{
    if (auto* ed = currentEditor())
        ed->findText(m_findInput->text(), false);
}

// ============================================================
// View actions
// ============================================================
void MainWindow::toggleSidebar()
{
    m_sidebarDock->setVisible(!m_sidebarDock->isVisible());
}

void MainWindow::toggleTerminal()
{
    m_terminalDock->setVisible(!m_terminalDock->isVisible());
    if (m_terminalDock->isVisible())
        m_terminalInput->setFocus();
}

void MainWindow::zoomIn()
{
    if (auto* ed = currentEditor()) {
        QFont f = ed->font();
        f.setPointSize(qBound(8, f.pointSize() + 1, 32));
        ed->setFont(f);
        ed->setTabStopDistance(ed->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    }
}

void MainWindow::zoomOut()
{
    if (auto* ed = currentEditor()) {
        QFont f = ed->font();
        f.setPointSize(qBound(8, f.pointSize() - 1, 32));
        ed->setFont(f);
        ed->setTabStopDistance(ed->fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 4);
    }
}

// ============================================================
// Build & Run
// ============================================================
void MainWindow::buildProject()
{
    if (auto* ed = currentEditor()) ed->saveFile();

    m_terminalOutput->appendPlainText("\n$ make");
    m_terminalDock->show();

    auto* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, &QProcess::readyRead, [this, proc]() {
        m_terminalOutput->appendPlainText(
            QString::fromUtf8(proc->readAll()));
    });
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int code, QProcess::ExitStatus) {
        m_terminalOutput->appendPlainText(
            QString("\nBuild finished with exit code %1").arg(code));
        proc->deleteLater();
    });
    proc->start("make");
}

void MainWindow::runProject()
{
    runInTerminal("./MiniCode");
}

void MainWindow::runInTerminal(const QString& cmd)
{
    m_terminalOutput->appendPlainText("\n$ " + cmd);
    m_terminalDock->show();

    if (m_terminalProcess->state() == QProcess::Running)
        m_terminalProcess->kill();

    m_terminalProcess->start("bash", {"-c", cmd});
}

// ============================================================
// Terminal
// ============================================================
void MainWindow::onTerminalCommand()
{
    const QString cmd = m_terminalInput->text().trimmed();
    m_terminalInput->clear();
    if (cmd.isEmpty()) return;

    runInTerminal(cmd);
}

void MainWindow::onTerminalOutput()
{
    const QByteArray data = m_terminalProcess->readAll();
    m_terminalOutput->appendPlainText(QString::fromLocal8Bit(data));
}

void MainWindow::onTerminalFinished()
{
    m_terminalOutput->appendPlainText(
        QString("[Process exited with code %1]")
            .arg(m_terminalProcess->exitCode()));
}

// ============================================================
// Sidebar
// ============================================================
void MainWindow::onSidebarItemActivated(const QModelIndex& index)
{
    const QString path = m_fsModel->filePath(index);
    const QFileInfo info(path);

    if (info.isDir()) {
        // Toggle expand/collapse — don't try to open as file
        if (m_fileTree->isExpanded(index))
            m_fileTree->collapse(index);
        else
            m_fileTree->expand(index);
    } else if (info.isFile()) {
        openFile(path);
    }
    // else: broken symlink etc. — silently ignore
}

void MainWindow::openFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, "Open Folder", QDir::homePath());
    if (dir.isEmpty()) return;
    openFolder(dir);
}

void MainWindow::openFolder(const QString& dir)
{
    if (dir.isEmpty()) return;
    m_fsModel->setRootPath(dir);
    m_fileTree->setRootIndex(m_fsModel->index(dir));
    m_sidebarDock->show();
    setWindowTitle(QString("MiniCode — %1").arg(QFileInfo(dir).fileName()));
}

// ============================================================
// Status bar
// ============================================================
void MainWindow::updateStatusBar(int line, int col)
{
    m_statusPos->setText(QString("Ln %1, Col %2").arg(line).arg(col));
}

void MainWindow::onTabChanged(int index)
{
    if (auto* ed = editorAt(index)) {
        m_statusLang->setText(ed->language().toUpper());
    }
}

void MainWindow::onFileModified(const QString& path)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i); ed && ed->filePath() == path) {
            QString title = m_tabWidget->tabText(i);
            if (!title.endsWith(" ●"))
                m_tabWidget->setTabText(i, title + " ●");
        }
    }
}

void MainWindow::onFileSaved(const QString& path)
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i); ed && ed->filePath() == path) {
            QString title = m_tabWidget->tabText(i);
            title.remove(" ●");
            m_tabWidget->setTabText(i, title);
        }
    }
    statusBar()->showMessage("Saved: " + path, 3000);
}

// ============================================================
// LSP
// ============================================================
void MainWindow::startLSP()
{
    // Try clangd
    const QStringList candidates = {"clangd", "clangd-15", "clangd-14",
                                      "clangd-13", "/usr/bin/clangd"};
    for (const auto& cmd : candidates) {
        QProcess probe;
        probe.start(cmd, {"--version"});
        if (probe.waitForFinished(1000)) {
            m_lspClient->start(cmd, {"--background-index", "--clang-tidy"},
                                QDir::currentPath());
            return;
        }
    }
    m_statusLSP->setText("LSP: clangd not found");
}

void MainWindow::onLSPStarted()
{
    m_statusLSP->setText("LSP: ● clangd");
    m_statusLSP->setStyleSheet("color:#4ec9b0;");
}

void MainWindow::onLSPStopped()
{
    m_statusLSP->setText("LSP: Off");
    m_statusLSP->setStyleSheet("");
}

void MainWindow::onLSPError(const QString& msg)
{
    m_statusLSP->setText("LSP: Error");
    statusBar()->showMessage("LSP Error: " + msg, 5000);
}

// ============================================================
// Window events
// ============================================================
void MainWindow::closeEvent(QCloseEvent* event)
{
    // Check unsaved files
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* ed = editorAt(i); ed && ed->isModified()) {
            const auto btn = QMessageBox::question(
                this, "Unsaved Changes",
                QString("'%1' has unsaved changes. Save before closing?")
                    .arg(m_tabWidget->tabText(i)),
                QMessageBox::SaveAll | QMessageBox::Discard | QMessageBox::Cancel);

            if (btn == QMessageBox::Cancel) { event->ignore(); return; }
            if (btn == QMessageBox::SaveAll) {
                for (int j = i; j < m_tabWidget->count(); ++j)
                    if (auto* e2 = editorAt(j); e2 && e2->isModified())
                        e2->saveFile();
                break;
            }
            break;
        }
    }
    m_lspClient->stop();
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    for (const QUrl& url : event->mimeData()->urls())
        if (url.isLocalFile()) openFile(url.toLocalFile());
}

} // namespace minicode
