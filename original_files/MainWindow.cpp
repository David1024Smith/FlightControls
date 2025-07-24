#include "MainWindow.h"
#include "ProcessManager.h"
#include "WindowEmbedder.h"
#include "ApplicationSwitcher.h"
#include <QApplication>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QMessageBox>
#include <QKeySequence>
#include <QDesktopWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_toolbarWidget(nullptr)
    , m_toolbarLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_qgcButton(nullptr)
    , m_rvizButton(nullptr)
    , m_statusLabel(nullptr)
    , m_buttonGroup(nullptr)
    , m_stackedLayout(nullptr)
    , m_qgcPage(nullptr)
    , m_rvizPage(nullptr)
    , m_qgcContainer(nullptr)
    , m_rvizContainer(nullptr)
    , m_qgcAction(nullptr)
    , m_rvizAction(nullptr)
    , m_exitAction(nullptr)
    , m_processManager(nullptr)
    , m_windowEmbedder(nullptr)
    , m_applicationSwitcher(nullptr)
    , m_uiUpdateTimer(nullptr)
    , m_currentApp("QGC")
    , m_preloadInProgress(false)
{
    setupUI();
    
    // Initialize core modules
    m_processManager = new ProcessManager(this);
    m_windowEmbedder = new WindowEmbedder(this);
    m_applicationSwitcher = new ApplicationSwitcher(m_processManager, m_windowEmbedder, this);
    
    // Set containers for application switcher
    m_applicationSwitcher->setContainers(m_qgcContainer, m_rvizContainer);
    
    // Connect signals
    connect(m_applicationSwitcher, &ApplicationSwitcher::applicationSwitched,
            this, &MainWindow::onApplicationSwitched);
    connect(m_applicationSwitcher, &ApplicationSwitcher::preloadProgress,
            this, &MainWindow::onPreloadProgress);
    connect(m_processManager, &ProcessManager::qgcStateChanged,
            this, [this](ProcessManager::ProcessState state) {
                QString status = QString("QGC: %1").arg(
                    state == ProcessManager::Running ? "Running" :
                    state == ProcessManager::Starting ? "Starting" :
                    state == ProcessManager::Stopping ? "Stopping" :
                    state == ProcessManager::Error ? "Error" : "Stopped");
                onStatusChanged(status);
            });
    connect(m_processManager, &ProcessManager::rvizStateChanged,
            this, [this](ProcessManager::ProcessState state) {
                QString status = QString("RVIZ: %1").arg(
                    state == ProcessManager::Running ? "Running" :
                    state == ProcessManager::Starting ? "Starting" :
                    state == ProcessManager::Stopping ? "Stopping" :
                    state == ProcessManager::Error ? "Error" : "Stopped");
                onStatusChanged(status);
            });
    
    // Setup UI update timer
    m_uiUpdateTimer = new QTimer(this);
    connect(m_uiUpdateTimer, &QTimer::timeout, this, &MainWindow::updateUI);
    m_uiUpdateTimer->start(500); // Update every 500ms
    
    // Start preload
    m_applicationSwitcher->startPreload();
    m_preloadInProgress = true;
}

MainWindow::~MainWindow()
{
    if (m_processManager) {
        m_processManager->stopAll();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("FlightControls Launcher");
    setMinimumSize(1200, 800);
    resize(1400, 1000);
    
    setupMenuBar();
    setupToolbar();
    setupContentArea();
    setupStatusBar();
    centerWindow();
    applyButtonStyles();
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // Application menu
    QMenu *appMenu = menuBar->addMenu("&Application");
    
    m_qgcAction = new QAction("Switch to &QGroundControl", this);
    m_qgcAction->setShortcut(QKeySequence("Ctrl+1"));
    m_qgcAction->setToolTip("Switch to QGroundControl (Ctrl+1)");
    connect(m_qgcAction, &QAction::triggered, this, &MainWindow::switchToQGC);
    appMenu->addAction(m_qgcAction);
    
    m_rvizAction = new QAction("Switch to &RVIZ", this);
    m_rvizAction->setShortcut(QKeySequence("Ctrl+2"));
    m_rvizAction->setToolTip("Switch to RVIZ (Ctrl+2)");
    connect(m_rvizAction, &QAction::triggered, this, &MainWindow::switchToRVIZ);
    appMenu->addAction(m_rvizAction);
    
    appMenu->addSeparator();
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    appMenu->addAction(m_exitAction);
}

void MainWindow::setupToolbar()
{
    m_toolbarWidget = new QWidget();
    m_toolbarWidget->setFixedHeight(60);
    m_toolbarWidget->setStyleSheet("background-color: #404040; border-bottom: 1px solid #606060;");
    
    m_toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    m_toolbarLayout->setContentsMargins(20, 10, 20, 10);
    
    // Title label
    m_titleLabel = new QLabel("FlightControls Launcher");
    m_titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: white;");
    m_toolbarLayout->addWidget(m_titleLabel);
    
    m_toolbarLayout->addStretch();
    
    // Application buttons
    m_qgcButton = new QPushButton("QGroundControl");
    m_qgcButton->setCheckable(true);
    m_qgcButton->setChecked(true);
    m_qgcButton->setToolTip("Switch to QGroundControl (Ctrl+1)");
    connect(m_qgcButton, &QPushButton::clicked, this, &MainWindow::switchToQGC);
    
    m_rvizButton = new QPushButton("RVIZ");
    m_rvizButton->setCheckable(true);
    m_rvizButton->setToolTip("Switch to RVIZ (Ctrl+2)");
    connect(m_rvizButton, &QPushButton::clicked, this, &MainWindow::switchToRVIZ);
    
    // Button group for mutual exclusion
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->addButton(m_qgcButton);
    m_buttonGroup->addButton(m_rvizButton);
    
    m_toolbarLayout->addWidget(m_qgcButton);
    m_toolbarLayout->addWidget(m_rvizButton);
    
    m_toolbarLayout->addStretch();
    
    // Status label
    m_statusLabel = new QLabel("Initializing...");
    m_statusLabel->setStyleSheet("color: #cccccc; font-size: 12px;");
    m_toolbarLayout->addWidget(m_statusLabel);
}

void MainWindow::setupContentArea()
{
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Add toolbar
    m_mainLayout->addWidget(m_toolbarWidget);
    
    // Create stacked layout for content pages
    QWidget *contentWidget = new QWidget();
    m_stackedLayout = new QStackedLayout(contentWidget);
    
    // QGC page
    m_qgcPage = new QWidget();
    QVBoxLayout *qgcLayout = new QVBoxLayout(m_qgcPage);
    qgcLayout->setContentsMargins(0, 0, 0, 0);
    
    m_qgcContainer = new QWidget();
    m_qgcContainer->setStyleSheet("background-color: #2a2a2a; border: 1px solid #555555;");
    qgcLayout->addWidget(m_qgcContainer);
    
    // RVIZ page
    m_rvizPage = new QWidget();
    QVBoxLayout *rvizLayout = new QVBoxLayout(m_rvizPage);
    rvizLayout->setContentsMargins(0, 0, 0, 0);
    
    m_rvizContainer = new QWidget();
    m_rvizContainer->setStyleSheet("background-color: #2a2a2a; border: 1px solid #555555;");
    rvizLayout->addWidget(m_rvizContainer);
    
    m_stackedLayout->addWidget(m_qgcPage);
    m_stackedLayout->addWidget(m_rvizPage);
    m_stackedLayout->setCurrentWidget(m_qgcPage);
    
    m_mainLayout->addWidget(contentWidget);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
    statusBar()->setStyleSheet("background-color: #353535; color: white; border-top: 1px solid #606060;");
}

void MainWindow::centerWindow()
{
    QDesktopWidget desktop;
    QRect screenGeometry = desktop.screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
}

void MainWindow::applyButtonStyles()
{
    QString buttonStyle = 
        "QPushButton {"
        "    background-color: #505050;"
        "    border: 1px solid #707070;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    color: white;"
        "    font-size: 14px;"
        "    min-width: 120px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #606060;"
        "    border-color: #808080;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #404040;"
        "}"
        "QPushButton:checked {"
        "    background-color: #0078d4;"
        "    border-color: #106ebe;"
        "}";
    
    m_qgcButton->setStyleSheet(buttonStyle);
    m_rvizButton->setStyleSheet(buttonStyle);
}

void MainWindow::switchToQGC()
{
    if (m_preloadInProgress) {
        m_statusLabel->setText("Please wait for preload to complete...");
        return;
    }
    
    if (m_applicationSwitcher->switchToApplication("QGC")) {
        m_qgcButton->setChecked(true);
        m_stackedLayout->setCurrentWidget(m_qgcPage);
    }
}

void MainWindow::switchToRVIZ()
{
    if (m_preloadInProgress) {
        m_statusLabel->setText("Please wait for preload to complete...");
        return;
    }
    
    if (m_applicationSwitcher->switchToApplication("RVIZ")) {
        m_rvizButton->setChecked(true);
        m_stackedLayout->setCurrentWidget(m_rvizPage);
    }
}

void MainWindow::updateUI()
{
    // Update button states based on current application
    if (m_currentApp == "QGC") {
        m_qgcButton->setChecked(true);
        m_stackedLayout->setCurrentWidget(m_qgcPage);
    } else if (m_currentApp == "RVIZ") {
        m_rvizButton->setChecked(true);
        m_stackedLayout->setCurrentWidget(m_rvizPage);
    }
    
    // Update button enabled state
    bool canSwitch = !m_preloadInProgress && 
                    m_applicationSwitcher->getCurrentState() == ApplicationSwitcher::Idle;
    m_qgcButton->setEnabled(canSwitch);
    m_rvizButton->setEnabled(canSwitch);
    m_qgcAction->setEnabled(canSwitch);
    m_rvizAction->setEnabled(canSwitch);
}

void MainWindow::onApplicationSwitched(const QString &appName)
{
    m_currentApp = appName;
    m_statusLabel->setText(QString("Switched to %1").arg(appName));
    statusBar()->showMessage(QString("Current application: %1").arg(appName));
}

void MainWindow::onStatusChanged(const QString &status)
{
    m_statusLabel->setText(status);
}

void MainWindow::onPreloadProgress(int progress)
{
    m_statusLabel->setText(QString("Preloading applications... %1%").arg(progress));
    
    if (progress >= 100) {
        m_preloadInProgress = false;
        m_statusLabel->setText("Preload complete - Ready");
        statusBar()->showMessage("All applications preloaded successfully");
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Exit Application",
        "Are you sure you want to exit?\nThis will close all embedded applications.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        if (m_processManager) {
            m_processManager->stopAll();
        }
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    
    // Sync embedded window sizes
    if (m_windowEmbedder && m_applicationSwitcher) {
        // This will be handled by the WindowEmbedder's monitoring system
    }
}