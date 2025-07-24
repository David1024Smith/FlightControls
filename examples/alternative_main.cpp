#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTextEdit>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

// 引入我们的替代方案
#include "../src/WindowManager.h"
#include "../src/VirtualDesktopManager.h"

// 条件包含 TabBasedLauncher
#ifdef QT_WEBENGINEWIDGETS_LIB
#include "../src/TabBasedLauncher.h"
#define HAS_TAB_LAUNCHER
#endif

class AlternativeLauncherDemo : public QMainWindow
{
    Q_OBJECT

public:
    explicit AlternativeLauncherDemo(QWidget *parent = nullptr)
        : QMainWindow(parent)
        , m_windowManager(nullptr)
#ifdef HAS_TAB_LAUNCHER
        , m_tabLauncher(nullptr)
#endif
        , m_virtualDesktopManager(nullptr)
        , m_currentMethod(MethodNone)
    {
        setupUI();
    }

private slots:
    void onMethodChanged(int index)
    {
        Method newMethod = static_cast<Method>(index);
        if (newMethod == m_currentMethod) {
            return;
        }
        
        // 清理旧方法
        cleanupCurrentMethod();
        
        // 初始化新方法
        switch (newMethod) {
        case MethodWindowManager:
            initializeWindowManager();
            break;
        case MethodTabBased:
#ifdef HAS_TAB_LAUNCHER
            initializeTabLauncher();
#else
            QMessageBox::warning(this, "功能不可用", 
                               "Tab界面方案需要Qt WebEngine支持\n"
                               "请安装 libqt5webengine5-dev 后重新编译");
            m_methodCombo->setCurrentIndex(0);
            return;
#endif
            break;
        case MethodVirtualDesktop:
            initializeVirtualDesktop();
            break;
        default:
            break;
        }
        
        m_currentMethod = newMethod;
        updateUI();
    }
    
    void onLaunchQGC()
    {
        switch (m_currentMethod) {
        case MethodWindowManager:
            if (m_windowManager) {
                m_windowManager->switchToApplication("QGC");
            }
            break;
        case MethodTabBased:
#ifdef HAS_TAB_LAUNCHER
            if (m_tabLauncher) {
                // Tab方案有自己的UI，这里只是演示
                addLogMessage("Tab方案：请在Tab界面中操作");
            }
#else
            addLogMessage("Tab方案不可用 - 缺少Qt WebEngine支持");
#endif
            break;
        case MethodVirtualDesktop:
            if (m_virtualDesktopManager) {
                m_virtualDesktopManager->switchToApplication("QGC");
            }
            break;
        default:
            QMessageBox::warning(this, "警告", "请先选择一个方案");
            break;
        }
    }
    
    void onLaunchRVIZ()
    {
        switch (m_currentMethod) {
        case MethodWindowManager:
            if (m_windowManager) {
                m_windowManager->switchToApplication("RVIZ");
            }
            break;
        case MethodTabBased:
#ifdef HAS_TAB_LAUNCHER
            if (m_tabLauncher) {
                addLogMessage("Tab方案：请在Tab界面中操作");
            }
#else
            addLogMessage("Tab方案不可用 - 缺少Qt WebEngine支持");
#endif
            break;
        case MethodVirtualDesktop:
            if (m_virtualDesktopManager) {
                m_virtualDesktopManager->switchToApplication("RVIZ");
            }
            break;
        default:
            QMessageBox::warning(this, "警告", "请先选择一个方案");
            break;
        }
    }
    
    void onApplicationSwitched(const QString &appName)
    {
        addLogMessage(QString("切换到应用程序: %1").arg(appName));
    }
    
    void onErrorOccurred(const QString &error)
    {
        addLogMessage(QString("错误: %1").arg(error));
    }

private:
    enum Method {
        MethodNone = 0,
        MethodWindowManager = 1,
        MethodTabBased = 2,
        MethodVirtualDesktop = 3
    };
    
    void setupUI()
    {
        setWindowTitle("飞行控制应用程序启动器 - 替代方案演示");
        setMinimumSize(800, 600);
        
        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        
        // 方案选择
        QHBoxLayout *methodLayout = new QHBoxLayout();
        methodLayout->addWidget(new QLabel("选择方案:", this));
        
        m_methodCombo = new QComboBox(this);
        m_methodCombo->addItem("请选择方案...");
        m_methodCombo->addItem("方案1: 窗口管理切换");
#ifdef HAS_TAB_LAUNCHER
        m_methodCombo->addItem("方案2: Tab界面组织");
#else
        m_methodCombo->addItem("方案2: Tab界面组织 (不可用 - 需要Qt WebEngine)");
#endif
        m_methodCombo->addItem("方案3: 虚拟桌面");
        
        connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &AlternativeLauncherDemo::onMethodChanged);
        
        methodLayout->addWidget(m_methodCombo);
        methodLayout->addStretch();
        
        mainLayout->addLayout(methodLayout);
        
        // 控制按钮
        QHBoxLayout *controlLayout = new QHBoxLayout();
        
        m_qgcButton = new QPushButton("启动 QGroundControl", this);
        m_qgcButton->setEnabled(false);
        connect(m_qgcButton, &QPushButton::clicked, this, &AlternativeLauncherDemo::onLaunchQGC);
        
        m_rvizButton = new QPushButton("启动 RVIZ", this);
        m_rvizButton->setEnabled(false);
        connect(m_rvizButton, &QPushButton::clicked, this, &AlternativeLauncherDemo::onLaunchRVIZ);
        
        controlLayout->addWidget(m_qgcButton);
        controlLayout->addWidget(m_rvizButton);
        controlLayout->addStretch();
        
        mainLayout->addLayout(controlLayout);
        
        // 状态显示
        m_statusLabel = new QLabel("状态: 未选择方案", this);
        mainLayout->addWidget(m_statusLabel);
        
        // 日志显示
        m_logText = new QTextEdit(this);
        m_logText->setReadOnly(true);
        m_logText->setMaximumHeight(200);
        mainLayout->addWidget(new QLabel("日志:"));
        mainLayout->addWidget(m_logText);
        
        // 方案说明
        QTabWidget *infoTabs = new QTabWidget(this);
        
        // 方案1说明
        QTextEdit *method1Info = new QTextEdit(this);
        method1Info->setReadOnly(true);
        method1Info->setHtml(
            "<h3>方案1: 基于窗口管理的快速切换</h3>"
            "<p><b>优点:</b></p>"
            "<ul>"
            "<li>无需窗口嵌入，兼容性更好</li>"
            "<li>支持多种应用程序类型</li>"
            "<li>切换速度快</li>"
            "<li>资源占用低</li>"
            "</ul>"
            "<p><b>缺点:</b></p>"
            "<ul>"
            "<li>应用程序仍然独立运行</li>"
            "<li>需要依赖X11窗口管理</li>"
            "</ul>"
            "<p><b>适用场景:</b> 需要快速切换但不要求完全集成的场景</p>"
        );
        infoTabs->addTab(method1Info, "窗口管理方案");
        
        // 方案2说明
        QTextEdit *method2Info = new QTextEdit(this);
        method2Info->setReadOnly(true);
        method2Info->setHtml(
            "<h3>方案2: 基于Tab Widget的界面组织</h3>"
            "<p><b>优点:</b></p>"
            "<ul>"
            "<li>用户界面友好</li>"
            "<li>完全原生Qt实现</li>"
            "<li>支持日志和状态监控</li>"
            "<li>可扩展性好</li>"
            "</ul>"
            "<p><b>缺点:</b></p>"
            "<ul>"
            "<li>不是真正的应用程序集成</li>"
            "<li>需要为每个应用程序设计专门的界面</li>"
            "</ul>"
            "<p><b>适用场景:</b> 注重用户体验和界面美观的场景</p>"
        );
        infoTabs->addTab(method2Info, "Tab界面方案");
        
        // 方案3说明
        QTextEdit *method3Info = new QTextEdit(this);
        method3Info->setReadOnly(true);
        method3Info->setHtml(
            "<h3>方案3: 基于虚拟桌面的解决方案</h3>"
            "<p><b>优点:</b></p>"
            "<ul>"
            "<li>完全利用系统原生功能</li>"
            "<li>支持多种桌面环境</li>"
            "<li>可设置全局快捷键</li>"
            "<li>应用程序完全独立</li>"
            "</ul>"
            "<p><b>缺点:</b></p>"
            "<ul>"
            "<li>依赖桌面环境支持</li>"
            "<li>在某些环境下可能不可用</li>"
            "</ul>"
            "<p><b>适用场景:</b> 在支持虚拟桌面的环境下提供最佳体验</p>"
        );
        infoTabs->addTab(method3Info, "虚拟桌面方案");
        
        mainLayout->addWidget(infoTabs);
        
        addLogMessage("演示程序启动完成，请选择一个方案开始测试");
    }
    
    void cleanupCurrentMethod()
    {
        if (m_windowManager) {
            delete m_windowManager;
            m_windowManager = nullptr;
        }
        
#ifdef HAS_TAB_LAUNCHER
        if (m_tabLauncher) {
            m_tabLauncher->close();
            delete m_tabLauncher;
            m_tabLauncher = nullptr;
        }
#endif
        
        if (m_virtualDesktopManager) {
            delete m_virtualDesktopManager;
            m_virtualDesktopManager = nullptr;
        }
    }
    
    void initializeWindowManager()
    {
        addLogMessage("初始化窗口管理方案...");
        
        m_windowManager = new WindowManager(this);
        
        // 注册应用程序
        QProcess *qgcProcess = new QProcess(this);
        QProcess *rvizProcess = new QProcess(this);
        
        m_windowManager->registerApplication("QGC", qgcProcess);
        m_windowManager->registerApplication("RVIZ", rvizProcess);
        
        // 连接信号
        connect(m_windowManager, &WindowManager::applicationSwitched,
                this, &AlternativeLauncherDemo::onApplicationSwitched);
        connect(m_windowManager, &WindowManager::switchingFailed,
                this, &AlternativeLauncherDemo::onErrorOccurred);
        
        addLogMessage("窗口管理方案初始化完成");
    }
    
    void initializeTabLauncher()
    {
#ifdef HAS_TAB_LAUNCHER
        addLogMessage("初始化Tab界面方案...");
        
        m_tabLauncher = new TabBasedLauncher();
        m_tabLauncher->show();
        
        addLogMessage("Tab界面方案已启动，请在新窗口中操作");
#else
        addLogMessage("错误: Tab界面方案不可用，缺少Qt WebEngine支持");
#endif
    }
    
    void initializeVirtualDesktop()
    {
        addLogMessage("初始化虚拟桌面方案...");
        
        m_virtualDesktopManager = new VirtualDesktopManager(this);
        
        if (!m_virtualDesktopManager->isVirtualDesktopSupported()) {
            addLogMessage("警告: 当前桌面环境不支持虚拟桌面功能");
            addLogMessage("桌面环境: " + m_virtualDesktopManager->getDesktopEnvironment());
        } else {
            // 注册应用程序
            m_virtualDesktopManager->registerApplication("QGC", "QGroundControl.AppImage");
            m_virtualDesktopManager->registerApplication("RVIZ", "rosrun", QStringList() << "rviz" << "rviz");
            
            // 连接信号
            connect(m_virtualDesktopManager, &VirtualDesktopManager::applicationSwitched,
                    this, &AlternativeLauncherDemo::onApplicationSwitched);
            connect(m_virtualDesktopManager, &VirtualDesktopManager::errorOccurred,
                    this, &AlternativeLauncherDemo::onErrorOccurred);
            
            addLogMessage("虚拟桌面方案初始化完成");
            addLogMessage("支持的桌面: " + m_virtualDesktopManager->getAvailableDesktops().join(", "));
        }
    }
    
    void updateUI()
    {
        bool hasMethod = (m_currentMethod != MethodNone);
        m_qgcButton->setEnabled(hasMethod);
        m_rvizButton->setEnabled(hasMethod);
        
        QString statusText = "状态: ";
        switch (m_currentMethod) {
        case MethodWindowManager:
            statusText += "窗口管理方案已激活";
            break;
        case MethodTabBased:
            statusText += "Tab界面方案已激活";
            break;
        case MethodVirtualDesktop:
            statusText += "虚拟桌面方案已激活";
            break;
        default:
            statusText += "未选择方案";
            break;
        }
        
        m_statusLabel->setText(statusText);
    }
    
    void addLogMessage(const QString &message)
    {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_logText->append(QString("[%1] %2").arg(timestamp, message));
    }
    
    // UI 组件
    QComboBox *m_methodCombo;
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QLabel *m_statusLabel;
    QTextEdit *m_logText;
    
    // 方案实例
    WindowManager *m_windowManager;
#ifdef HAS_TAB_LAUNCHER
    TabBasedLauncher *m_tabLauncher;
#endif
    VirtualDesktopManager *m_virtualDesktopManager;
    
    Method m_currentMethod;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("FlightControls Alternative Launcher Demo");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("FlightControls");
    
    AlternativeLauncherDemo demo;
    demo.show();
    
    return app.exec();
}

#include "alternative_main.moc" 