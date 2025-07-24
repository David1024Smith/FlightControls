#include "TabBasedLauncher.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QThread>
#include <QTextCursor>
#include <QUrl>

TabBasedLauncher::TabBasedLauncher(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_controlLayout(nullptr)
    , m_tabWidget(nullptr)
    , m_qgcButton(nullptr)
    , m_rvizButton(nullptr)
    , m_stopButton(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_qgcTab(nullptr)
    , m_rvizTab(nullptr)
    , m_dashboardTab(nullptr)
    , m_statusTimer(nullptr)
{
    setupUI();
    setupApplicationTabs();
    setupControlBar();
    
    // 注册应用程序
    registerApplication("QGC", "QGroundControl.AppImage");
    registerApplication("RVIZ", "rosrun", QStringList() << "rviz" << "rviz");
    
    // 设置状态更新定时器
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &TabBasedLauncher::updateApplicationStatus);
    m_statusTimer->start(2000); // 每2秒更新一次状态
    
    // 设置窗口属性
    setWindowTitle("飞行控制应用程序启动器 - Tab版本");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // 居中显示
    QRect screenGeometry = QApplication::desktop()->screenGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    qDebug() << "TabBasedLauncher initialized successfully";
}

TabBasedLauncher::~TabBasedLauncher()
{
    // 停止所有运行中的应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().isRunning && it.value().process) {
            it.value().process->kill();
            it.value().process->waitForFinished(3000);
        }
    }
}

void TabBasedLauncher::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(10, 10, 10, 10);
    m_mainLayout->setSpacing(10);
    
    // 创建Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setMovable(false);
    m_tabWidget->setTabsClosable(false);
    
    // 设置Tab样式
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 2px solid #C2C7CB;"
        "    border-radius: 5px;"
        "}"
        "QTabBar::tab {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,"
        "                                stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);"
        "    border: 2px solid #C4C4C3;"
        "    border-bottom-color: #C2C7CB;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "    min-width: 8ex;"
        "    padding: 8px 20px;"
        "    margin-right: 2px;"
        "    font-weight: bold;"
        "}"
        "QTabBar::tab:selected {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #fafafa, stop: 0.4 #f4f4f4,"
        "                                stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);"
        "    border-color: #9B9B9B;"
        "    border-bottom-color: #C2C7CB;"
        "}"
        "QTabBar::tab:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "                                stop: 0 #f6f7fa, stop: 1 #dadbde);"
        "}"
    );
    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TabBasedLauncher::onTabChanged);
    
    m_mainLayout->addWidget(m_tabWidget);
}

void TabBasedLauncher::setupApplicationTabs()
{
    // 创建仪表板Tab
    m_dashboardTab = new ApplicationTab("Dashboard", this);
    m_dashboardTab->setApplicationInfo("系统控制面板 - 查看和管理所有应用程序");
    m_tabWidget->addTab(m_dashboardTab, "🏠 仪表板");
    
    // 创建QGroundControl Tab
    m_qgcTab = new ApplicationTab("QGC", this);
    m_qgcTab->setApplicationInfo("QGroundControl - 地面控制站软件，用于无人机控制和监控");
    m_tabWidget->addTab(m_qgcTab, "✈️ QGroundControl");
    
    // 创建RVIZ Tab
    m_rvizTab = new ApplicationTab("RVIZ", this);
    m_rvizTab->setApplicationInfo("RVIZ - ROS 3D可视化工具，用于机器人状态显示和调试");
    m_tabWidget->addTab(m_rvizTab, "🤖 RVIZ");
    
    // 连接信号
    connect(m_qgcTab, &ApplicationTab::launchRequested, this, &TabBasedLauncher::onLaunchQGC);
    connect(m_qgcTab, &ApplicationTab::stopRequested, this, &TabBasedLauncher::onStopApplication);
    
    connect(m_rvizTab, &ApplicationTab::launchRequested, this, &TabBasedLauncher::onLaunchRVIZ);
    connect(m_rvizTab, &ApplicationTab::stopRequested, this, &TabBasedLauncher::onStopApplication);
}

void TabBasedLauncher::setupControlBar()
{
    // 创建控制栏布局
    m_controlLayout = new QHBoxLayout();
    m_controlLayout->setSpacing(10);
    
    // 快捷启动按钮
    m_qgcButton = new QPushButton("启动 QGroundControl", this);
    m_qgcButton->setIcon(QIcon(":/icons/qgc.png"));
    m_qgcButton->setMinimumSize(180, 40);
    m_qgcButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3d8b40;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    m_rvizButton = new QPushButton("启动 RVIZ", this);
    m_rvizButton->setIcon(QIcon(":/icons/rviz.png"));
    m_rvizButton->setMinimumSize(150, 40);
    m_rvizButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    m_stopButton = new QPushButton("停止当前应用", this);
    m_stopButton->setIcon(QIcon(":/icons/stop.png"));
    m_stopButton->setMinimumSize(150, 40);
    m_stopButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #d32f2f;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c62828;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    // 状态显示
    m_statusLabel = new QLabel("就绪", this);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #4CAF50;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    padding: 5px 10px;"
        "    background-color: #E8F5E8;"
        "    border: 1px solid #4CAF50;"
        "    border-radius: 3px;"
        "}"
    );
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressBar->setMinimumWidth(200);
    
    // 连接信号
    connect(m_qgcButton, &QPushButton::clicked, this, &TabBasedLauncher::onLaunchQGC);
    connect(m_rvizButton, &QPushButton::clicked, this, &TabBasedLauncher::onLaunchRVIZ);
    connect(m_stopButton, &QPushButton::clicked, [this]() {
        onStopApplication(m_currentApp);
    });
    
    // 添加到布局
    m_controlLayout->addWidget(m_qgcButton);
    m_controlLayout->addWidget(m_rvizButton);
    m_controlLayout->addWidget(m_stopButton);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_statusLabel);
    m_controlLayout->addWidget(m_progressBar);
    
    m_mainLayout->addLayout(m_controlLayout);
}

void TabBasedLauncher::registerApplication(const QString &appName, const QString &command, const QStringList &args)
{
    AppInfo info;
    info.name = appName;
    info.command = command;
    info.arguments = args;
    info.process = nullptr;
    info.isRunning = false;
    
    if (appName == "QGC") {
        info.tab = m_qgcTab;
    } else if (appName == "RVIZ") {
        info.tab = m_rvizTab;
    } else {
        info.tab = nullptr;
    }
    
    m_applications[appName] = info;
}

bool TabBasedLauncher::startApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return false;
    }
    
    AppInfo &info = m_applications[appName];
    
    if (info.isRunning && info.process) {
        QMessageBox::information(this, "提示", appName + " 已经在运行中");
        return true;
    }
    
    // 创建新进程
    if (info.process) {
        if (info.process->state() != QProcess::NotRunning) {
            info.process->kill();
            info.process->waitForFinished(3000);
        }
        info.process->deleteLater();
        info.process = nullptr;
    }
    
    info.process = new QProcess(this);
    connect(info.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TabBasedLauncher::onProcessFinished);
    
    // 特殊处理RVIZ（需要先启动roscore）
    if (appName == "RVIZ") {
        // 检查ROS环境
        if (qgetenv("ROS_DISTRO").isEmpty()) {
            QMessageBox::warning(this, "错误", "ROS环境未正确设置，请先source ROS环境");
            return false;
        }
        
        // 启动roscore（如果尚未运行）
        QProcess roscoreCheck;
        roscoreCheck.start("pgrep", QStringList() << "roscore");
        roscoreCheck.waitForFinished(2000);
        
        if (roscoreCheck.exitCode() != 0) {
            // roscore未运行，先启动它
            info.tab->addLogMessage("启动roscore...");
            QProcess::startDetached("roscore");
            // 使用定时器替代阻塞等待，避免冻结UI
            info.tab->addLogMessage("等待roscore启动...");
            QTimer::singleShot(3000, this, [this, appName]() {
                if (m_applications.contains(appName)) {
                    m_applications[appName].tab->addLogMessage("Roscore应该已就绪");
                }
            });
        }
    }
    
    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (appName == "RVIZ") {
        env.insert("ROS_MASTER_URI", "http://localhost:11311");
    }
    info.process->setProcessEnvironment(env);
    
    // 启动进程
    m_statusLabel->setText("正在启动 " + appName + "...");
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // 设置为忙指示器
    
    if (info.tab) {
        info.tab->setApplicationStatus("正在启动...");
        info.tab->setProgressValue(0);
        info.tab->addLogMessage("启动命令: " + info.command + " " + info.arguments.join(" "));
    }
    
    info.process->start(info.command, info.arguments);
    
    if (!info.process->waitForStarted(10000)) {
        QString errorMsg = "启动失败: " + info.process->errorString();
        m_statusLabel->setText("启动失败");
        m_progressBar->setVisible(false);
        
        if (info.tab) {
            info.tab->setApplicationStatus("启动失败");
            info.tab->addLogMessage("错误: " + errorMsg);
        }
        
        QMessageBox::critical(this, "启动失败", errorMsg);
        return false;
    }
    
    info.isRunning = true;
    m_currentApp = appName;
    
    // 切换到对应的Tab
    if (appName == "QGC") {
        m_tabWidget->setCurrentWidget(m_qgcTab);
    } else if (appName == "RVIZ") {
        m_tabWidget->setCurrentWidget(m_rvizTab);
    }
    
    // 延迟更新状态（等待应用程序完全启动）- 使用QPointer保证线程安全
    QTimer::singleShot(5000, this, [this, appName]() {
        if (!this || !m_applications.contains(appName) || !m_applications[appName].isRunning) {
            return;
        }
        
        if (m_statusLabel) {
            m_statusLabel->setText(appName + " 运行中");
        }
        if (m_progressBar) {
            m_progressBar->setVisible(false);
        }
        
        if (m_applications[appName].tab) {
            m_applications[appName].tab->setApplicationStatus("运行中");
            m_applications[appName].tab->setProgressValue(100);
        }
    });
    
    return true;
}

void TabBasedLauncher::stopApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    AppInfo &info = m_applications[appName];
    
    if (!info.isRunning || !info.process) {
        return;
    }
    
    m_statusLabel->setText("正在停止 " + appName + "...");
    
    if (info.tab) {
        info.tab->setApplicationStatus("正在停止...");
        info.tab->addLogMessage("停止应用程序...");
    }
    
    // 尝试优雅地终止进程
    info.process->terminate();
    
    if (!info.process->waitForFinished(5000)) {
        // 如果优雅终止失败，强制杀死进程
        info.process->kill();
        info.process->waitForFinished(3000);
    }
    
    info.isRunning = false;
    
    if (m_currentApp == appName) {
        m_currentApp.clear();
    }
    
    m_statusLabel->setText("就绪");
    
    if (info.tab) {
        info.tab->setApplicationStatus("已停止");
        info.tab->setProgressValue(0);
        info.tab->addLogMessage("应用程序已停止");
    }
}

bool TabBasedLauncher::isApplicationRunning(const QString &appName) const
{
    if (m_applications.contains(appName)) {
        return m_applications[appName].isRunning;
    }
    return false;
}

void TabBasedLauncher::onTabChanged(int index)
{
    QString tabText = m_tabWidget->tabText(index);
    qDebug() << "Tab changed to:" << tabText;
    
    // 根据选中的Tab更新控制栏状态
    updateApplicationStatus();
}

void TabBasedLauncher::onLaunchQGC()
{
    startApplication("QGC");
}

void TabBasedLauncher::onLaunchRVIZ()
{
    startApplication("RVIZ");
}

void TabBasedLauncher::onStopApplication(const QString &appName)
{
    stopApplication(appName);
}

void TabBasedLauncher::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }
    
    // 查找对应的应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().process == process) {
            it.value().isRunning = false;
            
            if (m_currentApp == it.key()) {
                m_currentApp.clear();
            }
            
            QString statusText = QString("进程退出 (代码: %1)").arg(exitCode);
            m_statusLabel->setText("就绪");
            
            if (it.value().tab) {
                it.value().tab->setApplicationStatus("已停止");
                it.value().tab->setProgressValue(0);
                it.value().tab->addLogMessage(statusText);
            }
            
            qDebug() << "Process finished:" << it.key() << "Exit code:" << exitCode;
            break;
        }
    }
    
    updateApplicationStatus();
}

void TabBasedLauncher::updateApplicationStatus()
{
    // 更新按钮状态
    bool qgcRunning = isApplicationRunning("QGC");
    bool rvizRunning = isApplicationRunning("RVIZ");
    
    m_qgcButton->setEnabled(!qgcRunning);
    m_rvizButton->setEnabled(!rvizRunning);
    m_stopButton->setEnabled(qgcRunning || rvizRunning);
    
    // 更新按钮文本
    m_qgcButton->setText(qgcRunning ? "QGC 运行中" : "启动 QGroundControl");
    m_rvizButton->setText(rvizRunning ? "RVIZ 运行中" : "启动 RVIZ");
}

// ApplicationTab 实现
ApplicationTab::ApplicationTab(const QString &appName, QWidget *parent)
    : QWidget(parent)
    , m_appName(appName)
    , m_isApplicationRunning(false)
{
    setupUI();
}

ApplicationTab::~ApplicationTab()
{
}

void ApplicationTab::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(15);
    
    setupInfoSection();
    setupControlSection();
    setupContentSection();
}

void ApplicationTab::setupInfoSection()
{
    // 标题
    m_titleLabel = new QLabel(m_appName, this);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    margin-bottom: 10px;"
        "}"
    );
    
    // 状态
    m_statusLabel = new QLabel("未启动", this);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    padding: 5px 10px;"
        "    border-radius: 3px;"
        "    background-color: #ecf0f1;"
        "    color: #7f8c8d;"
        "}"
    );
    
    // 信息
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #7f8c8d;"
        "    padding: 10px;"
        "    background-color: #f8f9fa;"
        "    border-radius: 5px;"
        "    border-left: 4px solid #3498db;"
        "}"
    );
    
    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addLayout(statusLayout);
    m_mainLayout->addWidget(m_infoLabel);
    m_mainLayout->addWidget(m_progressBar);
}

void ApplicationTab::setupControlSection()
{
    m_controlLayout = new QHBoxLayout();
    m_controlLayout->setSpacing(10);
    
    m_launchButton = new QPushButton("启动", this);
    m_launchButton->setMinimumSize(100, 35);
    m_launchButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2ecc71;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}"
    );
    
    m_stopButton = new QPushButton("停止", this);
    m_stopButton->setMinimumSize(100, 35);
    m_stopButton->setEnabled(false);
    m_stopButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #e74c3c;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c0392b;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}"
    );
    
    m_refreshButton = new QPushButton("刷新", this);
    m_refreshButton->setMinimumSize(100, 35);
    
    m_clearLogButton = new QPushButton("清除日志", this);
    m_clearLogButton->setMinimumSize(100, 35);
    
    connect(m_launchButton, &QPushButton::clicked, this, &ApplicationTab::onLaunchClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &ApplicationTab::onStopClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &ApplicationTab::onRefreshClicked);
    connect(m_clearLogButton, &QPushButton::clicked, this, &ApplicationTab::onClearLogClicked);
    
    m_controlLayout->addWidget(m_launchButton);
    m_controlLayout->addWidget(m_stopButton);
    m_controlLayout->addWidget(m_refreshButton);
    m_controlLayout->addWidget(m_clearLogButton);
    m_controlLayout->addStretch();
    
    m_mainLayout->addLayout(m_controlLayout);
}

void ApplicationTab::setupContentSection()
{
    m_contentTabWidget = new QTabWidget(this);
    
    // 输出Tab
    m_outputText = new QTextEdit(this);
    m_outputText->setReadOnly(true);
    m_outputText->setFont(QFont("Consolas", 10));
    m_contentTabWidget->addTab(m_outputText, "应用输出");
    
    // 日志Tab
    m_logText = new QTextEdit(this);
    m_logText->setReadOnly(true);
    m_logText->setFont(QFont("Consolas", 10));
    m_contentTabWidget->addTab(m_logText, "系统日志");
    
    // Web界面Tab（如果支持）
#ifdef QT_WEBENGINEWIDGETS_LIB
    m_webView = new QWebEngineView(this);
    m_contentTabWidget->addTab(m_webView, "Web界面");
#else
    m_webView = new QTextEdit(this);
    m_webView->setReadOnly(true);
    m_webView->setText("Web界面功能需要Qt WebEngine支持\n当前系统未安装Qt WebEngine组件");
    m_contentTabWidget->addTab(m_webView, "Web界面（不可用）");
#endif
    
    m_mainLayout->addWidget(m_contentTabWidget);
}

void ApplicationTab::setApplicationInfo(const QString &info)
{
    m_infoLabel->setText(info);
}

void ApplicationTab::setApplicationStatus(const QString &status)
{
    m_statusLabel->setText(status);
    
    // 根据状态更新样式
    if (status.contains("运行中")) {
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 14px;"
            "    padding: 5px 10px;"
            "    border-radius: 3px;"
            "    background-color: #d5edda;"
            "    color: #155724;"
            "}"
        );
        m_isApplicationRunning = true;
        m_launchButton->setEnabled(false);
        m_stopButton->setEnabled(true);
        m_progressBar->setVisible(false);
    } else if (status.contains("正在启动") || status.contains("正在停止")) {
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 14px;"
            "    padding: 5px 10px;"
            "    border-radius: 3px;"
            "    background-color: #fff3cd;"
            "    color: #856404;"
            "}"
        );
        m_launchButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_progressBar->setVisible(true);
    } else {
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "    font-size: 14px;"
            "    padding: 5px 10px;"
            "    border-radius: 3px;"
            "    background-color: #ecf0f1;"
            "    color: #7f8c8d;"
            "}"
        );
        m_isApplicationRunning = false;
        m_launchButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        m_progressBar->setVisible(false);
    }
}

void ApplicationTab::addLogMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp, message);
    
    m_logText->append(logEntry);
    
    // 自动滚动到底部
    QTextCursor cursor = m_logText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logText->setTextCursor(cursor);
}

void ApplicationTab::showApplicationOutput(const QString &output)
{
    m_outputText->append(output);
    
    // 自动滚动到底部
    QTextCursor cursor = m_outputText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_outputText->setTextCursor(cursor);
}

void ApplicationTab::showWebInterface(const QString &url)
{
    if (m_webView && !url.isEmpty()) {
#ifdef QT_WEBENGINEWIDGETS_LIB
        m_webView->load(QUrl(url));
#else
        m_webView->setText(QString("Web界面功能不可用\n请求的URL: %1\n\n需要安装Qt WebEngine组件才能使用此功能").arg(url));
#endif
        m_contentTabWidget->setCurrentWidget(m_webView);
    }
}

void ApplicationTab::setProgressValue(int value)
{
    m_progressBar->setValue(value);
    if (value > 0 && value < 100) {
        m_progressBar->setVisible(true);
    } else if (value >= 100) {
        m_progressBar->setVisible(false);
    }
}

void ApplicationTab::onLaunchClicked()
{
    emit launchRequested(m_appName);
}

void ApplicationTab::onStopClicked()
{
    emit stopRequested(m_appName);
}

void ApplicationTab::onRefreshClicked()
{
    emit refreshRequested(m_appName);
}

void ApplicationTab::onClearLogClicked()
{
    m_logText->clear();
    m_outputText->clear();
} 