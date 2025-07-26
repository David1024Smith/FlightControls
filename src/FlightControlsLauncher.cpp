#include "FlightControlsLauncher.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QDebug>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <QDir>

FlightControlsLauncher::FlightControlsLauncher(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_statusLayout(nullptr)
    , m_qgcButton(nullptr)
    , m_rvizButton(nullptr)
    , m_closeButton(nullptr)
    , m_statusLabel(nullptr)
    , m_statusTimer(nullptr)
    , m_dragging(false)
{
    qDebug() << "创建飞行控制应用程序启动器";
    
    setupUI();
    setupButtons();
    positionWindow();
    applyStyles();
    
    // 设置状态更新定时器
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &FlightControlsLauncher::updateStatus);
    m_statusTimer->start(STATUS_UPDATE_INTERVAL);
    
    // 注册应用程序
    AppProcess qgcApp;
    qgcApp.name = "QGroundControl";
    qgcApp.command = ""; // 将在启动时动态确定路径
    qgcApp.arguments = QStringList();
    qgcApp.process = nullptr;
    qgcApp.isRunning = false;
    m_applications["QGC"] = qgcApp;
    
    // 注册rviz进程 - 使用终端窗口启动，确保环境变量正确
    AppProcess rvizApp;
    rvizApp.name = "RVIZ";
    
    // 尝试多种终端，确保兼容性
    QStringList terminals = {"gnome-terminal", "konsole", "xfce4-terminal", "xterm"};
    QString availableTerminal;
    
    for (const QString &terminal : terminals) {
        if (QProcess::execute("which", QStringList() << terminal) == 0) {
            availableTerminal = terminal;
            break;
        }
    }
    
    if (availableTerminal.isEmpty()) {
        qWarning() << "未找到可用的终端程序，RVIZ功能可能不可用";
        rvizApp.command = "echo";
        rvizApp.arguments = QStringList() << "未找到终端程序";
    } else {
        qDebug() << "使用终端程序:" << availableTerminal;
        rvizApp.command = availableTerminal;
        
        // 根据不同终端设置不同的参数
        if (availableTerminal == "gnome-terminal") {
            rvizApp.arguments = QStringList() << "--title=RVIZ" << "--" << "bash" << "-c" 
                               << "echo '正在启动ROS和RVIZ...'; "
                                  "source /opt/ros/*/setup.bash 2>/dev/null || echo 'ROS环境已加载'; "
                                  "roscore & sleep 3; rosrun rviz rviz; "
                                  "echo '按任意键关闭此窗口...'; read";
        } else if (availableTerminal == "konsole") {
            rvizApp.arguments = QStringList() << "--title" << "RVIZ" << "-e" << "bash" << "-c" 
                               << "echo '正在启动ROS和RVIZ...'; "
                                  "source /opt/ros/*/setup.bash 2>/dev/null || echo 'ROS环境已加载'; "
                                  "roscore & sleep 3; rosrun rviz rviz; "
                                  "echo '按任意键关闭此窗口...'; read";
        } else if (availableTerminal == "xfce4-terminal") {
            rvizApp.arguments = QStringList() << "--title=RVIZ" << "-e" << "bash" << "-c" 
                               << "echo '正在启动ROS和RVIZ...'; "
                                  "source /opt/ros/*/setup.bash 2>/dev/null || echo 'ROS环境已加载'; "
                                  "roscore & sleep 3; rosrun rviz rviz; "
                                  "echo '按任意键关闭此窗口...'; read";
        } else { // xterm or others
            rvizApp.arguments = QStringList() << "-title" << "RVIZ" << "-e" << "bash" << "-c" 
                               << "echo '正在启动ROS和RVIZ...'; "
                                  "source /opt/ros/*/setup.bash 2>/dev/null || echo 'ROS环境已加载'; "
                                  "roscore & sleep 3; rosrun rviz rviz; "
                                  "echo '按任意键关闭此窗口...'; read";
        }
    }
    rvizApp.process = nullptr;
    rvizApp.isRunning = false;
    m_applications["RVIZ"] = rvizApp;
    
    qDebug() << "飞行控制启动器初始化完成";
}

FlightControlsLauncher::~FlightControlsLauncher()
{
    qDebug() << "销毁飞行控制启动器，清理资源...";
    
    // 停止定时器
    if (m_statusTimer) {
        m_statusTimer->stop();
    }
    
    // 停止所有运行中的应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        AppProcess &app = it.value();
        if (app.isRunning && app.process) {
            qDebug() << "强制终止进程:" << app.name;
            app.process->kill();
            if (!app.process->waitForFinished(PROCESS_KILL_TIMEOUT)) {
                qWarning() << "进程" << app.name << "强制终止超时";
            }
        }
        // QProcess会被Qt的父子关系自动清理
    }
    
    // 清理可能残留的ROS进程
    QProcess::execute("pkill", QStringList() << "-f" << "roscore");
    QProcess::execute("pkill", QStringList() << "-f" << "rviz");
    
    qDebug() << "资源清理完成";
}

QString FlightControlsLauncher::findQGroundControlPath()
{
    QStringList searchPaths = {
        // 当前工作目录
        "./QGroundControl.AppImage",
        // 程序所在目录
        QApplication::applicationDirPath() + "/QGroundControl.AppImage",
        // build目录（开发时）
        "./build/QGroundControl.AppImage",
        "../QGroundControl.AppImage",
        // 常见安装路径
        "/usr/local/bin/QGroundControl.AppImage",
        "/opt/QGroundControl/QGroundControl.AppImage"
    };
    
    for (const QString &path : searchPaths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isExecutable()) {
            qDebug() << "找到QGroundControl:" << fileInfo.absoluteFilePath();
            return fileInfo.absoluteFilePath();
        }
    }
    
    qWarning() << "未找到QGroundControl.AppImage文件";
    return QString();
}

void FlightControlsLauncher::setupUI()
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(LAUNCHER_WIDTH, LAUNCHER_HEIGHT);
    
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 10, 15, 10);
    m_mainLayout->setSpacing(8);
    
    // 创建按钮布局
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(15);
    
    // 创建状态布局
    m_statusLayout = new QHBoxLayout();
    m_statusLayout->setSpacing(10);
    
    m_mainLayout->addLayout(m_buttonLayout);
    m_mainLayout->addLayout(m_statusLayout);
}

void FlightControlsLauncher::setupButtons()
{
    // QGroundControl按钮
    m_qgcButton = new QPushButton("🚁 启动 QGC", this);
    m_qgcButton->setMinimumSize(100, 35);
    connect(m_qgcButton, &QPushButton::clicked, this, &FlightControlsLauncher::onLaunchQGC);
    
    // RVIZ按钮
    m_rvizButton = new QPushButton("🤖 启动 RVIZ", this);
    m_rvizButton->setMinimumSize(100, 35);
    connect(m_rvizButton, &QPushButton::clicked, this, &FlightControlsLauncher::onLaunchRVIZ);
    
    // 关闭按钮
    m_closeButton = new QPushButton("✖", this);
    m_closeButton->setFixedSize(25, 25);
    m_closeButton->setToolTip("关闭启动器");
    connect(m_closeButton, &QPushButton::clicked, this, &QWidget::close);
    
    // 状态标签
    m_statusLabel = new QLabel("🟢 就绪", this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    
    // 添加到布局
    m_buttonLayout->addWidget(m_qgcButton);
    m_buttonLayout->addWidget(m_rvizButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_closeButton);
    
    m_statusLayout->addWidget(m_statusLabel);
}

void FlightControlsLauncher::positionWindow()
{
    // 获取主屏幕
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        
        // 计算居中位置（屏幕顶部，水平居中）
        int x = (screenGeometry.width() - width()) / 2;
        int y = TOP_OFFSET;
        
        move(x, y);
        qDebug() << "启动器位置:" << x << "," << y;
    } else {
        qWarning() << "无法获取主屏幕信息，使用默认位置";
        move(100, TOP_OFFSET);
    }
}

void FlightControlsLauncher::applyStyles()
{
    // 设置主窗口样式
    setStyleSheet(
        "FlightControlsLauncher {"
        "    background-color: rgba(45, 52, 65, 0.95);"
        "    border-radius: 15px;"
        "    border: 2px solid rgba(255, 255, 255, 0.3);"
        "}"
    );
    
    // QGC按钮样式
    m_qgcButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #4CAF50, stop: 1 #45a049);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "    padding: 5px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #45a049, stop: 1 #3d8b40);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #3d8b40, stop: 1 #357a38);"
        "}"
        "QPushButton:disabled {"
        "    background: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    // RVIZ按钮样式
    m_rvizButton->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #2196F3, stop: 1 #1976D2);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    font-weight: bold;"
        "    font-size: 11px;"
        "    padding: 5px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #1976D2, stop: 1 #1565C0);"
        "}"
        "QPushButton:pressed {"
        "    background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "        stop: 0 #1565C0, stop: 1 #0d47a1);"
        "}"
        "QPushButton:disabled {"
        "    background: #cccccc;"
        "    color: #666666;"
        "}"
    );
    
    // 关闭按钮样式
    m_closeButton->setStyleSheet(
        "QPushButton {"
        "    background: rgba(244, 67, 54, 0.8);"
        "    color: white;"
        "    border: none;"
        "    border-radius: 12px;"
        "    font-weight: bold;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(244, 67, 54, 1.0);"
        "}"
    );
    
    // 状态标签样式
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 12px;"
        "    padding: 3px 8px;"
        "    background-color: rgba(255, 255, 255, 0.1);"
        "    border-radius: 10px;"
        "}"
    );
    
    // 添加阴影效果
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setColor(QColor(0, 0, 0, 160));
    shadowEffect->setBlurRadius(20);
    shadowEffect->setOffset(0, 5);
    setGraphicsEffect(shadowEffect);
}

void FlightControlsLauncher::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void FlightControlsLauncher::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_dragging) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void FlightControlsLauncher::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}

void FlightControlsLauncher::startApplication(const QString &appName, const QString &command, const QStringList &args)
{
    if (!m_applications.contains(appName)) {
        qWarning() << "应用程序未注册:" << appName;
        QMessageBox::warning(this, "启动错误", QString("应用程序 %1 未注册").arg(appName));
        return;
    }
    
    AppProcess &app = m_applications[appName];
    
    if (app.isRunning && app.process && app.process->state() == QProcess::Running) {
        qDebug() << appName << "已在运行中";
        return;
    }
    
    // 清理之前的进程实例
    if (app.process) {
        if (app.process->state() != QProcess::NotRunning) {
            qDebug() << "终止之前的" << appName << "进程";
            app.process->kill();
            app.process->waitForFinished(PROCESS_KILL_TIMEOUT);
        }
        app.process->deleteLater();
        app.process = nullptr;
    }
    
    // 设置命令和参数
    QString actualCommand = command.isEmpty() ? app.command : command;
    QStringList actualArgs = args.isEmpty() ? app.arguments : args;
    
    qDebug() << "启动" << appName << ":" << actualCommand << actualArgs;
    
    // 对于RVIZ，使用startDetached直接启动终端，不需要QProcess管理
    if (appName == "RVIZ") {
        bool success = QProcess::startDetached(actualCommand, actualArgs);
        if (success) {
            app.isRunning = true;
            qDebug() << appName << "终端启动成功";
            updateStatus();
        } else {
            QString errorMsg = QString("启动 %1 失败").arg(appName);
            qWarning() << errorMsg;
            QMessageBox::warning(this, "启动失败", 
                errorMsg + "\n\n提示：\n" +
                "- 请确保系统安装了终端程序（gnome-terminal、konsole、xfce4-terminal或xterm）\n" +
                "- 请确保ROS环境已正确配置\n" +
                "- 可以尝试在终端中手动执行：roscore& rosrun rviz rviz\n\n" +
                "当前使用的终端：" + actualCommand);
        }
        return;
    }
    
    // 对于其他应用程序，使用正常的QProcess管理
    app.process = new QProcess(this);
    connect(app.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &FlightControlsLauncher::onProcessFinished);
    
    // 设置进程环境 - 继承系统环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    app.process->setProcessEnvironment(env);
    
    // 启动进程
    app.process->start(actualCommand, actualArgs);
    
    if (!app.process->waitForStarted(5000)) {
        QString errorMsg = QString("启动 %1 失败: %2").arg(appName).arg(app.process->errorString());
        qWarning() << errorMsg;
        QMessageBox::warning(this, "启动失败", errorMsg);
        
        // 清理失败的进程
        app.process->deleteLater();
        app.process = nullptr;
        return;
    }
    
    app.isRunning = true;
    qDebug() << appName << "启动成功，PID:" << app.process->processId();
    
    updateStatus();
}

void FlightControlsLauncher::stopApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        qWarning() << "尝试停止未注册的应用程序:" << appName;
        return;
    }
    
    AppProcess &app = m_applications[appName];
    
    if (!app.isRunning) {
        qDebug() << appName << "未在运行";
        return;
    }
    
    qDebug() << "停止" << appName;
    
    // 对于RVIZ，直接清理ROS进程
    if (appName == "RVIZ") {
        // 清理可能的ROS进程
        QProcess::execute("pkill", QStringList() << "-f" << "roscore");
        QProcess::execute("pkill", QStringList() << "-f" << "rviz");
        QProcess::execute("pkill", QStringList() << "-f" << "gnome-terminal.*RVIZ");
        
        app.isRunning = false;
        qDebug() << appName << "已停止";
        updateStatus();
        return;
    }
    
    // 对于其他应用程序
    if (app.process) {
        qDebug() << "停止" << appName << "，PID:" << app.process->processId();
        
        // 尝试优雅终止
        app.process->terminate();
        if (!app.process->waitForFinished(3000)) {
            qWarning() << appName << "优雅终止超时，强制杀死进程";
            app.process->kill();
            app.process->waitForFinished(1000);
        }
    }
    
    app.isRunning = false;
    qDebug() << appName << "已停止";
    updateStatus();
}

bool FlightControlsLauncher::isApplicationRunning(const QString &appName) const
{
    if (m_applications.contains(appName)) {
        const AppProcess &app = m_applications[appName];
        if (appName == "RVIZ") {
            // 对于RVIZ，检查进程是否仍在运行
            return app.isRunning;
        } else {
            return app.isRunning && app.process && (app.process->state() == QProcess::Running);
        }
    }
    return false;
}

void FlightControlsLauncher::onLaunchQGC()
{
    if (isApplicationRunning("QGC")) {
        stopApplication("QGC");
    } else {
        QString qgcPath = findQGroundControlPath();
        if (qgcPath.isEmpty()) {
            QMessageBox::warning(this, "启动失败", 
                "未找到QGroundControl.AppImage文件！\n\n"
                "请确保QGroundControl.AppImage文件在以下位置之一：\n"
                "• 当前工作目录\n"
                "• 程序所在目录\n"
                "• ./build/ 目录");
            return;
        }
        startApplication("QGC", qgcPath);
    }
}

void FlightControlsLauncher::onLaunchRVIZ()
{
    if (isApplicationRunning("RVIZ")) {
        stopApplication("RVIZ");
    } else {
        // 使用终端窗口启动RVIZ
        startApplication("RVIZ", "", QStringList());
    }
}

void FlightControlsLauncher::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        qWarning() << "进程结束信号发送者无效";
        return;
    }
    
    // 查找对应的应用程序
    QString appName;
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().process == process) {
            appName = it.key();
            AppProcess &app = it.value();
            
            QString statusText = (exitStatus == QProcess::NormalExit) ? "正常退出" : "异常终止";
            qDebug() << appName << "进程结束 -" << statusText << "，退出代码:" << exitCode;
            
            app.isRunning = false;
            break;
        }
    }
    
    if (appName.isEmpty()) {
        qWarning() << "无法找到对应的应用程序进程";
    }
    
    updateStatus();
}

void FlightControlsLauncher::updateStatus()
{
    bool qgcRunning = isApplicationRunning("QGC");
    bool rvizRunning = isApplicationRunning("RVIZ");
    
    // 更新QGC按钮
    if (qgcRunning) {
        m_qgcButton->setText("🚁 停止 QGC");
        m_qgcButton->setEnabled(true);
    } else {
        m_qgcButton->setText("🚁 启动 QGC");
        m_qgcButton->setEnabled(true);
    }
    
    // 更新RVIZ按钮
    if (rvizRunning) {
        m_rvizButton->setText("🤖 停止 RVIZ");
        m_rvizButton->setEnabled(true);
    } else {
        m_rvizButton->setText("🤖 启动 RVIZ");
        m_rvizButton->setEnabled(true);
    }
    
    // 更新状态标签
    if (qgcRunning && rvizRunning) {
        m_statusLabel->setText("🟡 QGC + RVIZ 运行中");
    } else if (qgcRunning) {
        m_statusLabel->setText("🟢 QGC 运行中");
    } else if (rvizRunning) {
        m_statusLabel->setText("🔵 RVIZ 运行中");
    } else {
        m_statusLabel->setText("🟢 就绪");
    }
} 