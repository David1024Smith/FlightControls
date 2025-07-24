#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QTextCursor>

#include "../src/WindowManager.h"

class SimpleWindowManagerDemo : public QMainWindow
{
    Q_OBJECT

public:
    explicit SimpleWindowManagerDemo(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_windowManager(nullptr)
        , m_qgcProcess(nullptr)
        , m_rvizProcess(nullptr)
    {
        setupUI();
        initializeWindowManager();
    }

    ~SimpleWindowManagerDemo()
    {
        // 清理进程
        if (m_qgcProcess) {
            m_qgcProcess->kill();
            m_qgcProcess->waitForFinished(3000);
        }
        if (m_rvizProcess) {
            m_rvizProcess->kill();
            m_rvizProcess->waitForFinished(3000);
        }
    }

private slots:
    void onSwitchToQGC()
    {
        if (!m_windowManager) {
            return;
        }
        
        addLogMessage("切换到QGroundControl...");
        
        if (!m_qgcProcess || m_qgcProcess->state() == QProcess::NotRunning) {
            // 启动QGC进程
            if (!m_qgcProcess) {
                m_qgcProcess = new QProcess(this);
            }
            
            QString qgcPath = findQGroundControl();
            if (qgcPath.isEmpty()) {
                addLogMessage("❌ 未找到QGroundControl.AppImage");
                QMessageBox::warning(this, "错误", "未找到QGroundControl.AppImage文件");
                return;
            }
            
            addLogMessage("启动QGroundControl: " + qgcPath);
            m_qgcProcess->start(qgcPath);
            
            if (!m_qgcProcess->waitForStarted(5000)) {
                addLogMessage("❌ QGroundControl启动失败: " + m_qgcProcess->errorString());
                return;
            }
            
            // 延迟切换，等待窗口出现
            QTimer::singleShot(3000, this, [this]() {
                m_windowManager->switchToApplication("QGC");
            });
        } else {
            m_windowManager->switchToApplication("QGC");
        }
    }
    
    void onSwitchToRVIZ()
    {
        if (!m_windowManager) {
            return;
        }
        
        addLogMessage("切换到RVIZ...");
        
        if (!m_rvizProcess || m_rvizProcess->state() == QProcess::NotRunning) {
            // 检查ROS环境
            if (qgetenv("ROS_DISTRO").isEmpty()) {
                addLogMessage("❌ ROS环境未设置");
                QMessageBox::warning(this, "错误", "ROS环境未设置，请先设置ROS环境变量");
                return;
            }
            
            // 检查roscore
            QProcess roscoreCheck;
            roscoreCheck.start("pgrep", QStringList() << "roscore");
            roscoreCheck.waitForFinished(2000);
            
            if (roscoreCheck.exitCode() != 0) {
                addLogMessage("启动roscore...");
                QProcess::startDetached("roscore");
                QThread::sleep(2);
            }
            
            // 启动RVIZ进程
            if (!m_rvizProcess) {
                m_rvizProcess = new QProcess(this);
            }
            
            addLogMessage("启动RVIZ...");
            m_rvizProcess->start("rosrun", QStringList() << "rviz" << "rviz");
            
            if (!m_rvizProcess->waitForStarted(5000)) {
                addLogMessage("❌ RVIZ启动失败: " + m_rvizProcess->errorString());
                return;
            }
            
            // 延迟切换，等待窗口出现
            QTimer::singleShot(5000, this, [this]() {
                m_windowManager->switchToApplication("RVIZ");
            });
        } else {
            m_windowManager->switchToApplication("RVIZ");
        }
    }
    
    void onMinimizeAll()
    {
        if (!m_windowManager) {
            return;
        }
        
        addLogMessage("最小化所有应用程序...");
        m_windowManager->minimizeAllOthers("");
    }
    
    void onApplicationSwitched(const QString &appName)
    {
        addLogMessage("✅ 成功切换到: " + appName);
        m_statusLabel->setText("当前应用: " + appName);
    }
    
    void onWindowFound(const QString &appName, unsigned long windowId)
    {
        addLogMessage(QString("找到窗口: %1 (ID: %2)").arg(appName).arg(windowId));
    }
    
    void onSwitchingFailed(const QString &appName, const QString &error)
    {
        addLogMessage(QString("❌ 切换失败: %1 - %2").arg(appName, error));
    }

private:
    void setupUI()
    {
        setWindowTitle("窗口管理方案演示 - 简化版");
        setMinimumSize(800, 600);
        
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // 标题
        QLabel *titleLabel = new QLabel("🖥️ 窗口管理方案演示", this);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);
        
        // 状态显示
        QGroupBox *statusGroup = new QGroupBox("状态信息", this);
        QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
        
        m_statusLabel = new QLabel("当前应用: 无", this);
        m_statusLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
        statusLayout->addWidget(m_statusLabel);
        
        mainLayout->addWidget(statusGroup);
        
        // 控制按钮
        QGroupBox *controlGroup = new QGroupBox("应用程序控制", this);
        QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
        
        m_qgcButton = new QPushButton("切换到\nQGroundControl", this);
        m_qgcButton->setMinimumHeight(60);
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
        );
        
        m_rvizButton = new QPushButton("切换到\nRVIZ", this);
        m_rvizButton->setMinimumHeight(60);
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
        );
        
        m_minimizeButton = new QPushButton("最小化\n所有应用", this);
        m_minimizeButton->setMinimumHeight(60);
        m_minimizeButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF9800;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "    font-weight: bold;"
            "    font-size: 12px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #F57C00;"
            "}"
        );
        
        connect(m_qgcButton, &QPushButton::clicked, this, &SimpleWindowManagerDemo::onSwitchToQGC);
        connect(m_rvizButton, &QPushButton::clicked, this, &SimpleWindowManagerDemo::onSwitchToRVIZ);
        connect(m_minimizeButton, &QPushButton::clicked, this, &SimpleWindowManagerDemo::onMinimizeAll);
        
        controlLayout->addWidget(m_qgcButton);
        controlLayout->addWidget(m_rvizButton);
        controlLayout->addWidget(m_minimizeButton);
        
        mainLayout->addWidget(controlGroup);
        
        // 日志显示
        QGroupBox *logGroup = new QGroupBox("系统日志", this);
        QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
        
        m_logText = new QTextEdit(this);
        m_logText->setReadOnly(true);
        m_logText->setMaximumHeight(200);
        m_logText->setStyleSheet("font-family: monospace; font-size: 11px;");
        
        logLayout->addWidget(m_logText);
        mainLayout->addWidget(logGroup);
        
        // 说明文本
        QLabel *infoLabel = new QLabel(
            "💡 说明: 这是窗口管理方案的简化演示。点击按钮启动并切换到对应的应用程序。\n"
            "应用程序将在独立窗口中运行，通过智能窗口管理实现快速切换。"
        );
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("color: #666; font-size: 11px; margin: 10px;");
        mainLayout->addWidget(infoLabel);
    }
    
    void initializeWindowManager()
    {
        addLogMessage("=== 初始化窗口管理器 ===");
        
        m_windowManager = new WindowManager(this);
        
        // 连接信号
        connect(m_windowManager, &WindowManager::applicationSwitched,
                this, &SimpleWindowManagerDemo::onApplicationSwitched);
        connect(m_windowManager, &WindowManager::windowFound,
                this, &SimpleWindowManagerDemo::onWindowFound);
        connect(m_windowManager, &WindowManager::switchingFailed,
                this, &SimpleWindowManagerDemo::onSwitchingFailed);
        
        addLogMessage("✅ 窗口管理器初始化完成");
        
        // 预注册应用程序（使用空的进程指针，稍后动态创建）
        if (m_qgcProcess) {
            m_windowManager->registerApplication("QGC", m_qgcProcess);
        }
        if (m_rvizProcess) {
            m_windowManager->registerApplication("RVIZ", m_rvizProcess);
        }
    }
    
    QString findQGroundControl()
    {
        QStringList searchPaths = {
            "QGroundControl.AppImage",
            "./QGroundControl.AppImage",
            "../QGroundControl.AppImage",
            QDir::homePath() + "/QGroundControl.AppImage",
            QDir::homePath() + "/Downloads/QGroundControl.AppImage",
            "/usr/local/bin/QGroundControl.AppImage"
        };
        
        for (const QString &path : searchPaths) {
            if (QFile::exists(path)) {
                return QDir(path).absolutePath();
            }
        }
        
        return QString();
    }
    
    void addLogMessage(const QString &message)
    {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_logText->append(QString("[%1] %2").arg(timestamp, message));
        
        // 自动滚动到底部
        QTextCursor cursor = m_logText->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_logText->setTextCursor(cursor);
    }
    
    // UI组件
    QLabel *m_statusLabel;
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QPushButton *m_minimizeButton;
    QTextEdit *m_logText;
    
    // 核心组件
    WindowManager *m_windowManager;
    QProcess *m_qgcProcess;
    QProcess *m_rvizProcess;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Simple Window Manager Demo");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("FlightControls");
    
    SimpleWindowManagerDemo demo;
    demo.show();
    
    return app.exec();
}

#include "window_manager_simple.moc" 