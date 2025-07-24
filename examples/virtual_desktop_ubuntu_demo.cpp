#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QProgressBar>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextCursor>

#include "../src/VirtualDesktopManager.h"

class Ubuntu1804VirtualDesktopDemo : public QMainWindow
{
    Q_OBJECT

public:
    explicit Ubuntu1804VirtualDesktopDemo(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_virtualDesktopManager(nullptr)
        , m_setupProgress(0)
    {
        setupUI();
        initializeVirtualDesktop();
    }

private slots:
    void detectDesktopEnvironment()
    {
        addLogMessage("=== Ubuntu 18.04 桌面环境检测 ===");
        
        // 检测桌面环境
        QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
        QString session = qgetenv("DESKTOP_SESSION");
        QString gdmSession = qgetenv("GDMSESSION");
        
        addLogMessage(QString("XDG_CURRENT_DESKTOP: %1").arg(desktop));
        addLogMessage(QString("DESKTOP_SESSION: %1").arg(session));
        addLogMessage(QString("GDMSESSION: %1").arg(gdmSession));
        
        // Ubuntu 18.04 常见桌面环境检测
        if (desktop.contains("ubuntu", Qt::CaseInsensitive) || 
            desktop.contains("GNOME", Qt::CaseInsensitive)) {
            m_desktopType = "Ubuntu GNOME";
            addLogMessage("✅ 检测到 Ubuntu GNOME 桌面环境");
            checkGnomeWorkspaces();
        } else if (desktop.contains("KDE", Qt::CaseInsensitive)) {
            m_desktopType = "KDE Plasma";
            addLogMessage("✅ 检测到 KDE Plasma 桌面环境");
            checkKDEDesktops();
        } else if (desktop.contains("XFCE", Qt::CaseInsensitive)) {
            m_desktopType = "XFCE";
            addLogMessage("✅ 检测到 XFCE 桌面环境");
            checkXfceWorkspaces();
        } else {
            m_desktopType = "Unknown";
            addLogMessage("⚠️ 未知桌面环境，将使用通用X11方法");
        }
        
        updateProgress(25);
    }
    
    void checkGnomeWorkspaces()
    {
        addLogMessage("--- 检查 GNOME 工作区设置 ---");
        
        // 检查当前工作区数量
        QProcess gsettingsCheck;
        gsettingsCheck.start("gsettings", QStringList() << "get" << "org.gnome.desktop.wm.preferences" << "num-workspaces");
        gsettingsCheck.waitForFinished(3000);
        
        if (gsettingsCheck.exitCode() == 0) {
            QString output = gsettingsCheck.readAllStandardOutput().trimmed();
            addLogMessage(QString("当前工作区数量: %1").arg(output));
            
            // 设置为4个工作区
            QProcess gsettingsSet;
            gsettingsSet.start("gsettings", QStringList() << "set" << "org.gnome.desktop.wm.preferences" << "num-workspaces" << "4");
            gsettingsSet.waitForFinished(3000);
            
            if (gsettingsSet.exitCode() == 0) {
                addLogMessage("✅ 成功设置4个工作区");
            } else {
                addLogMessage("❌ 设置工作区失败: " + gsettingsSet.readAllStandardError());
            }
        } else {
            addLogMessage("❌ 无法检查工作区设置: " + gsettingsCheck.readAllStandardError());
        }
        
        // 检查快捷键设置
        checkGnomeShortcuts();
    }
    
    void checkGnomeShortcuts()
    {
        addLogMessage("--- 检查 GNOME 快捷键设置 ---");
        
        QStringList shortcuts = {
            "switch-to-workspace-1",
            "switch-to-workspace-2", 
            "switch-to-workspace-3",
            "switch-to-workspace-4"
        };
        
        for (int i = 0; i < shortcuts.size(); ++i) {
            QProcess shortcutCheck;
            shortcutCheck.start("gsettings", QStringList() << "get" << "org.gnome.desktop.wm.keybindings" << shortcuts[i]);
            shortcutCheck.waitForFinished(2000);
            
            if (shortcutCheck.exitCode() == 0) {
                QString output = shortcutCheck.readAllStandardOutput().trimmed();
                addLogMessage(QString("工作区%1快捷键: %2").arg(i+1).arg(output));
            }
        }
    }
    
    void checkKDEDesktops()
    {
        addLogMessage("--- 检查 KDE 虚拟桌面设置 ---");
        
        QProcess kdeCheck;
        kdeCheck.start("qdbus", QStringList() << "org.kde.KWin" << "/VirtualDesktopManager" 
                      << "org.kde.KWin.VirtualDesktopManager.count");
        kdeCheck.waitForFinished(3000);
        
        if (kdeCheck.exitCode() == 0) {
            QString output = kdeCheck.readAllStandardOutput().trimmed();
            addLogMessage(QString("当前虚拟桌面数量: %1").arg(output));
            
            // 设置为4个桌面
            QProcess kdeSet;
            kdeSet.start("qdbus", QStringList() << "org.kde.KWin" << "/VirtualDesktopManager" 
                        << "setNumberOfDesktops" << "4");
            kdeSet.waitForFinished(3000);
            
            if (kdeSet.exitCode() == 0) {
                addLogMessage("✅ 成功设置4个虚拟桌面");
            }
        } else {
            addLogMessage("❌ 无法检查KDE虚拟桌面: " + kdeCheck.readAllStandardError());
        }
    }
    
    void checkXfceWorkspaces()
    {
        addLogMessage("--- 检查 XFCE 工作区设置 ---");
        
        QProcess xfceCheck;
        xfceCheck.start("xfconf-query", QStringList() << "-c" << "xfwm4" 
                       << "-p" << "/general/workspace_count");
        xfceCheck.waitForFinished(3000);
        
        if (xfceCheck.exitCode() == 0) {
            QString output = xfceCheck.readAllStandardOutput().trimmed();
            addLogMessage(QString("当前工作区数量: %1").arg(output));
            
            // 设置为4个工作区
            QProcess xfceSet;
            xfceSet.start("xfconf-query", QStringList() << "-c" << "xfwm4" 
                         << "-p" << "/general/workspace_count" << "-s" << "4");
            xfceSet.waitForFinished(3000);
            
            if (xfceSet.exitCode() == 0) {
                addLogMessage("✅ 成功设置4个工作区");
            }
        } else {
            addLogMessage("❌ 无法检查XFCE工作区: " + xfceCheck.readAllStandardError());
        }
    }
    
    void setupVirtualDesktops()
    {
        addLogMessage("=== 配置虚拟桌面环境 ===");
        
        if (!m_virtualDesktopManager) {
            addLogMessage("❌ 虚拟桌面管理器未初始化");
            return;
        }
        
        if (!m_virtualDesktopManager->isVirtualDesktopSupported()) {
            addLogMessage("❌ 当前环境不支持虚拟桌面功能");
            return;
        }
        
        // 创建专用桌面
        m_virtualDesktopManager->createVirtualDesktop("主控制台", 1);
        m_virtualDesktopManager->createVirtualDesktop("QGroundControl", 2);
        m_virtualDesktopManager->createVirtualDesktop("RVIZ可视化", 3);
        m_virtualDesktopManager->createVirtualDesktop("备用工作区", 4);
        
        addLogMessage("✅ 虚拟桌面配置完成");
        addLogMessage("桌面布局:");
        addLogMessage("  桌面1: 主控制台 (当前)");
        addLogMessage("  桌面2: QGroundControl专用");
        addLogMessage("  桌面3: RVIZ专用");
        addLogMessage("  桌面4: 备用工作区");
        
        updateProgress(50);
    }
    
    void registerApplications()
    {
        addLogMessage("=== 注册飞行控制应用程序 ===");
        
        // 检查QGroundControl
        QString qgcPath = findQGroundControl();
        if (!qgcPath.isEmpty()) {
            m_virtualDesktopManager->registerApplication("QGC", qgcPath, QStringList(), 2);
            addLogMessage("✅ QGroundControl 已注册到桌面2");
            m_qgcAvailable = true;
        } else {
            addLogMessage("⚠️ 未找到QGroundControl.AppImage");
            m_qgcAvailable = false;
        }
        
        // 检查RVIZ
        if (this->checkRVIZAvailable()) {
            m_virtualDesktopManager->registerApplication("RVIZ", "rosrun", QStringList() << "rviz" << "rviz", 3);
            addLogMessage("✅ RVIZ 已注册到桌面3");
            m_rvizAvailable = true;
        } else {
            addLogMessage("⚠️ RVIZ不可用，请检查ROS安装");
            m_rvizAvailable = false;
        }
        
        updateProgress(75);
    }
    
    void setupShortcuts()
    {
        addLogMessage("=== 配置快捷键 ===");
        
        if (m_desktopType == "Ubuntu GNOME") {
            setupGnomeShortcuts();
        } else if (m_desktopType == "KDE Plasma") {
            setupKDEShortcuts();
        } else if (m_desktopType == "XFCE") {
            setupXfceShortcuts();
        }
        
        addLogMessage("💡 建议快捷键:");
        addLogMessage("  Super+1: 切换到主控制台");
        addLogMessage("  Super+2: 切换到QGroundControl");
        addLogMessage("  Super+3: 切换到RVIZ");
        addLogMessage("  Super+4: 切换到备用工作区");
        
        updateProgress(100);
        addLogMessage("🎉 虚拟桌面环境配置完成！");
    }
    
    void setupGnomeShortcuts()
    {
        addLogMessage("配置GNOME快捷键...");
        
        QStringList commands = {
            "gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-1 \"['<Super>1']\"",
            "gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-2 \"['<Super>2']\"",
            "gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-3 \"['<Super>3']\"",
            "gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-4 \"['<Super>4']\""
        };
        
        for (const QString &cmd : commands) {
            QProcess::execute("bash", QStringList() << "-c" << cmd);
        }
        
        addLogMessage("✅ GNOME快捷键配置完成");
    }
    
    void setupKDEShortcuts()
    {
        addLogMessage("配置KDE快捷键...");
        addLogMessage("⚠️ KDE快捷键需要手动在系统设置中配置");
    }
    
    void setupXfceShortcuts()
    {
        addLogMessage("配置XFCE快捷键...");
        addLogMessage("⚠️ XFCE快捷键需要手动在设置管理器中配置");
    }
    
    void onSwitchToQGC()
    {
        if (!m_virtualDesktopManager || !m_qgcAvailable) {
            QMessageBox::warning(this, "警告", "QGroundControl不可用");
            return;
        }
        
        addLogMessage("🚀 切换到QGroundControl...");
        m_virtualDesktopManager->switchToApplication("QGC");
    }
    
    void onSwitchToRVIZ()
    {
        if (!m_virtualDesktopManager || !m_rvizAvailable) {
            QMessageBox::warning(this, "警告", "RVIZ不可用");
            return;
        }
        
        addLogMessage("🚀 切换到RVIZ...");
        m_virtualDesktopManager->switchToApplication("RVIZ");
    }
    
    void onReturnToMain()
    {
        if (!m_virtualDesktopManager) {
            return;
        }
        
        addLogMessage("🏠 返回主控制台");
        m_virtualDesktopManager->switchToDesktop(1);
    }
    
    void onApplicationSwitched(const QString &appName)
    {
        addLogMessage(QString("✅ 已切换到: %1").arg(appName));
        
        // 更新当前桌面显示
        int currentDesktop = m_virtualDesktopManager->getCurrentDesktop();
        m_currentDesktopLabel->setText(QString("当前桌面: %1").arg(currentDesktop));
    }
    
    void onDesktopSwitched(int fromDesktop, int toDesktop)
    {
        addLogMessage(QString("🔄 桌面切换: %1 → %2").arg(fromDesktop).arg(toDesktop));
        m_currentDesktopLabel->setText(QString("当前桌面: %1").arg(toDesktop));
    }

private:
    void setupUI()
    {
        setWindowTitle("虚拟桌面方案演示 - Ubuntu 18.04 LTS");
        setMinimumSize(900, 700);
        
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // 标题
        QLabel *titleLabel = new QLabel("🖥️ 虚拟桌面方案演示", this);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);
        
        // 环境信息
        QGroupBox *envGroup = new QGroupBox("环境信息", this);
        QVBoxLayout *envLayout = new QVBoxLayout(envGroup);
        
        m_desktopEnvLabel = new QLabel("桌面环境: 检测中...", this);
        m_currentDesktopLabel = new QLabel("当前桌面: 1", this);
        m_supportStatusLabel = new QLabel("支持状态: 检测中...", this);
        
        envLayout->addWidget(m_desktopEnvLabel);
        envLayout->addWidget(m_currentDesktopLabel);
        envLayout->addWidget(m_supportStatusLabel);
        
        mainLayout->addWidget(envGroup);
        
        // 进度显示
        QGroupBox *progressGroup = new QGroupBox("配置进度", this);
        QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
        
        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        
        progressLayout->addWidget(m_progressBar);
        mainLayout->addWidget(progressGroup);
        
        // 控制按钮
        QGroupBox *controlGroup = new QGroupBox("应用程序控制", this);
        QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
        
        m_qgcButton = new QPushButton("切换到 QGroundControl\n(桌面2)", this);
        m_qgcButton->setEnabled(false);
        m_qgcButton->setMinimumHeight(60);
        m_qgcButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #4CAF50;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #45a049;"
            "}"
            "QPushButton:disabled {"
            "    background-color: #cccccc;"
            "}"
        );
        
        m_rvizButton = new QPushButton("切换到 RVIZ\n(桌面3)", this);
        m_rvizButton->setEnabled(false);
        m_rvizButton->setMinimumHeight(60);
        m_rvizButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #2196F3;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #1976D2;"
            "}"
            "QPushButton:disabled {"
            "    background-color: #cccccc;"
            "}"
        );
        
        m_mainButton = new QPushButton("返回主控制台\n(桌面1)", this);
        m_mainButton->setEnabled(false);
        m_mainButton->setMinimumHeight(60);
        m_mainButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #FF9800;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 5px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: #F57C00;"
            "}"
            "QPushButton:disabled {"
            "    background-color: #cccccc;"
            "}"
        );
        
        connect(m_qgcButton, &QPushButton::clicked, this, &Ubuntu1804VirtualDesktopDemo::onSwitchToQGC);
        connect(m_rvizButton, &QPushButton::clicked, this, &Ubuntu1804VirtualDesktopDemo::onSwitchToRVIZ);
        connect(m_mainButton, &QPushButton::clicked, this, &Ubuntu1804VirtualDesktopDemo::onReturnToMain);
        
        controlLayout->addWidget(m_qgcButton);
        controlLayout->addWidget(m_rvizButton);
        controlLayout->addWidget(m_mainButton);
        
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
        
        // 信息提示
        QLabel *infoLabel = new QLabel(
            "💡 提示: 虚拟桌面方案将为每个应用程序分配独立的桌面空间，"
            "通过快捷键(Super+数字)或按钮进行切换。", this);
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("color: #666; font-size: 11px; margin: 10px;");
        mainLayout->addWidget(infoLabel);
    }
    
    void initializeVirtualDesktop()
    {
        // 延迟初始化，让UI先显示
        QTimer::singleShot(500, this, &Ubuntu1804VirtualDesktopDemo::detectDesktopEnvironment);
        QTimer::singleShot(2000, this, &Ubuntu1804VirtualDesktopDemo::createVirtualDesktopManager);
        QTimer::singleShot(3000, this, &Ubuntu1804VirtualDesktopDemo::setupVirtualDesktops);
        QTimer::singleShot(4000, this, &Ubuntu1804VirtualDesktopDemo::registerApplications);
        QTimer::singleShot(5000, this, &Ubuntu1804VirtualDesktopDemo::setupShortcuts);
    }
    
    void createVirtualDesktopManager()
    {
        addLogMessage("=== 初始化虚拟桌面管理器 ===");
        
        m_virtualDesktopManager = new VirtualDesktopManager(this);
        
        // 连接信号
        connect(m_virtualDesktopManager, &VirtualDesktopManager::applicationSwitched,
                this, &Ubuntu1804VirtualDesktopDemo::onApplicationSwitched);
        connect(m_virtualDesktopManager, &VirtualDesktopManager::desktopSwitched,
                this, &Ubuntu1804VirtualDesktopDemo::onDesktopSwitched);
        connect(m_virtualDesktopManager, &VirtualDesktopManager::errorOccurred,
                [this](const QString &error) {
            addLogMessage("❌ 错误: " + error);
        });
        
        // 更新UI状态
        bool supported = m_virtualDesktopManager->isVirtualDesktopSupported();
        m_supportStatusLabel->setText(QString("支持状态: %1").arg(supported ? "✅ 支持" : "❌ 不支持"));
        m_desktopEnvLabel->setText(QString("桌面环境: %1").arg(m_virtualDesktopManager->getDesktopEnvironment()));
        
        if (supported) {
            addLogMessage("✅ 虚拟桌面管理器初始化成功");
        } else {
            addLogMessage("❌ 当前环境不支持虚拟桌面功能");
        }
    }
    
    QString findQGroundControl()
    {
        QStringList searchPaths = {
            "QGroundControl.AppImage",
            "./QGroundControl.AppImage",
            "../QGroundControl.AppImage",
            "/usr/local/bin/QGroundControl.AppImage",
            QDir::homePath() + "/QGroundControl.AppImage",
            QDir::homePath() + "/Downloads/QGroundControl.AppImage"
        };
        
        for (const QString &path : searchPaths) {
            if (QFile::exists(path)) {
                addLogMessage(QString("找到QGroundControl: %1").arg(path));
                return path;
            }
        }
        
        return QString();
    }
    
    bool checkRVIZAvailable() const
    {
        // 检查ROS环境
        QString rosDistro = qgetenv("ROS_DISTRO");
        if (rosDistro.isEmpty()) {
            addLogMessage("ROS环境变量未设置");
            return false;
        }
        
        addLogMessage(QString("检测到ROS发行版: %1").arg(rosDistro));
        
        // 检查rviz命令
        QProcess rvizCheck;
        rvizCheck.start("which", QStringList() << "rviz");
        rvizCheck.waitForFinished(3000);
        
        if (rvizCheck.exitCode() == 0) {
            QString rvizPath = rvizCheck.readAllStandardOutput().trimmed();
            addLogMessage(QString("找到RVIZ: %1").arg(rvizPath));
            return true;
        }
        
        return false;
    }
    
    void updateProgress(int value)
    {
        m_setupProgress = value;
        m_progressBar->setValue(value);
        
        if (value == 100) {
            // 启用控制按钮
            m_qgcButton->setEnabled(m_qgcAvailable);
            m_rvizButton->setEnabled(m_rvizAvailable);
            m_mainButton->setEnabled(true);
        }
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
    QLabel *m_desktopEnvLabel;
    QLabel *m_currentDesktopLabel;
    QLabel *m_supportStatusLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QPushButton *m_mainButton;
    QTextEdit *m_logText;
    
    // 状态变量
    VirtualDesktopManager *m_virtualDesktopManager;
    QString m_desktopType;
    int m_setupProgress;
    bool m_qgcAvailable = false;
    bool m_rvizAvailable = false;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Ubuntu 18.04 Virtual Desktop Demo");
    app.setApplicationVersion("1.0");
    
    Ubuntu1804VirtualDesktopDemo demo;
    demo.show();
    
    return app.exec();
}

#include "virtual_desktop_ubuntu_demo.moc" 