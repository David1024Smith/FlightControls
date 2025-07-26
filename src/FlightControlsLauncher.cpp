#include "FlightControlsLauncher.h"
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QDebug>
#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QFileInfo>
#include <QDir>
#include <algorithm>  // 用于std::sort

// 在包含X11头文件之前，需要解决宏冲突
#ifdef Q_OS_LINUX
// 保存Qt可能使用的宏
#ifdef Bool
#define QT_X11_Bool Bool
#undef Bool
#endif

#ifdef Status
#define QT_X11_Status Status
#undef Status
#endif

#ifdef Unsorted
#define QT_X11_Unsorted Unsorted
#undef Unsorted
#endif

#ifdef None
#define QT_X11_None None
#undef None
#endif

// 包含X11头文件
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

// 恢复Qt宏定义
#ifdef QT_X11_Bool
#define Bool QT_X11_Bool
#undef QT_X11_Bool
#endif

#ifdef QT_X11_Status
#define Status QT_X11_Status
#undef QT_X11_Status
#endif

#ifdef QT_X11_Unsorted
#define Unsorted QT_X11_Unsorted
#undef QT_X11_Unsorted
#endif

#ifdef QT_X11_None
#define None QT_X11_None
#undef QT_X11_None
#endif

#endif // Q_OS_LINUX

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
    , m_windowSearchTimer(nullptr)
    , m_retryTimer(nullptr)
    , m_searchRetryCount(0)
    , m_dragging(false)
#ifdef Q_OS_LINUX
    , m_display(nullptr)
#endif
{
    qDebug() << "创建飞行控制应用程序启动器";
    
    // 初始化X11显示连接
#ifdef Q_OS_LINUX
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        qWarning() << "无法连接到X11显示服务器，窗口管理功能可能不可用";
    } else {
        qDebug() << "X11显示服务器连接成功";
    }
#endif
    
    setupUI();
    setupButtons();
    positionWindow();
    applyStyles();
    
    // 设置状态更新定时器
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &FlightControlsLauncher::updateStatus);
    m_statusTimer->start(STATUS_UPDATE_INTERVAL);
    
    // 设置窗口搜索定时器
    m_windowSearchTimer = new QTimer(this);
    m_windowSearchTimer->setSingleShot(true);
    connect(m_windowSearchTimer, &QTimer::timeout, this, &FlightControlsLauncher::findAndMaximizeWindows);
    
    // 设置重试定时器
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);
    connect(m_retryTimer, &QTimer::timeout, this, &FlightControlsLauncher::retryWindowSearch);
    
    // 注册应用程序
    AppProcess qgcApp;
    qgcApp.name = "QGroundControl";
    qgcApp.command = ""; // 将在启动时动态确定路径
    qgcApp.arguments = QStringList();
    qgcApp.process = nullptr;
    qgcApp.isRunning = false;
    qgcApp.windowTitlePattern = "QGroundControl";
    m_applications["QGC"] = qgcApp;
    
    // 注册rviz进程 - 使用终端窗口启动，确保环境变量正确
    AppProcess rvizApp;
    rvizApp.name = "RVIZ";
    rvizApp.windowTitlePattern = "RViz";
    
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
        
        // 根据不同终端设置不同的参数 - 添加隐藏选项
        QString rvizCommand = "echo '正在启动ROS和RVIZ...'; "
                             "source /opt/ros/*/setup.bash 2>/dev/null || echo 'ROS环境已加载'; "
                             "roscore >/dev/null 2>&1 & sleep 3; "
                             "nohup rosrun rviz rviz >/dev/null 2>&1 & "
                             "sleep 1; exit";
        
        if (availableTerminal == "gnome-terminal") {
            // 使用 --geometry 最小化终端大小，并立即退出
            rvizApp.arguments = QStringList() << "--geometry=1x1+0+0" << "--" << "bash" << "-c" << rvizCommand;
        } else if (availableTerminal == "konsole") {
            rvizApp.arguments = QStringList() << "--geometry" << "1x1+0+0" << "-e" << "bash" << "-c" << rvizCommand;
        } else if (availableTerminal == "xfce4-terminal") {
            rvizApp.arguments = QStringList() << "--geometry=1x1+0+0" << "-e" << "bash" << "-c" << rvizCommand;
        } else { // xterm or others
            rvizApp.arguments = QStringList() << "-geometry" << "1x1+0+0" << "-e" << "bash" << "-c" << rvizCommand;
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
    if (m_windowSearchTimer) {
        m_windowSearchTimer->stop();
    }
    if (m_retryTimer) {
        m_retryTimer->stop();
    }
    
    // 停止所有应用程序
    stopAllApplications();
    
    // 关闭X11显示连接
#ifdef Q_OS_LINUX
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
#endif
    
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

void FlightControlsLauncher::stopAllApplications()
{
    qDebug() << "停止所有应用程序...";
    
    // 停止所有运行中的应用程序 - 使用新的增强停止机制
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().isRunning) {
            stopApplication(it.key());
        }
    }
    
    // 额外的系统级清理（保留作为最后保险）
    qDebug() << "执行系统级进程清理...";
    QProcess::execute("pkill", QStringList() << "-f" << "QGroundControl");
    QProcess::execute("pkill", QStringList() << "-f" << "roscore");
    QProcess::execute("pkill", QStringList() << "-f" << "rviz");
    QProcess::execute("pkill", QStringList() << "-f" << "gnome-terminal.*RVIZ");
    
    qDebug() << "所有应用程序已停止";
}

unsigned long FlightControlsLauncher::findWindowByTitle(const QString &titlePattern)
{
#ifdef Q_OS_LINUX
    if (!m_display) {
        qDebug() << "X11显示连接无效，无法搜索窗口";
        return 0;
    }
    
    qDebug() << "开始搜索窗口，匹配模式:" << titlePattern;
    
    Window root = DefaultRootWindow(m_display);
    Window parent, *children;
    unsigned int nchildren;
    
    // 用于收集RVIZ候选窗口
    struct RvizCandidate {
        unsigned long windowId;
        QString title;
        int width, height;
        int score; // 评分，越高越好
    };
    QList<RvizCandidate> rvizCandidates;
    
    if (XQueryTree(m_display, root, &root, &parent, &children, &nchildren)) {
        qDebug() << "找到" << nchildren << "个窗口，开始逐个检查...";
        
        for (unsigned int i = 0; i < nchildren; ++i) {
            char *window_name = nullptr;
            if (XFetchName(m_display, children[i], &window_name) && window_name) {
                QString windowTitle = QString::fromUtf8(window_name);
                
                // 增加调试信息 - 显示所有窗口标题
                qDebug() << "检查窗口[" << i << "]:" << windowTitle << "[ID:" << children[i] << "]";
                
                // 更灵活的匹配逻辑
                bool titleMatch = false;
                if (titlePattern == "QGroundControl") {
                    // QGC可能的标题变体
                    titleMatch = windowTitle.contains("QGroundControl", Qt::CaseInsensitive) ||
                                windowTitle.contains("QGC", Qt::CaseInsensitive) ||
                                windowTitle.contains("Ground Control", Qt::CaseInsensitive) ||
                                windowTitle.contains("qgroundcontrol", Qt::CaseInsensitive);
                } else if (titlePattern == "RViz") {
                    // RVIZ可能的标题变体 - 根据实际日志更新，增加更多格式
                    titleMatch = windowTitle.contains("RViz", Qt::CaseInsensitive) ||
                                windowTitle.contains("rviz", Qt::CaseInsensitive) ||
                                windowTitle.contains("ROS Visualization", Qt::CaseInsensitive) ||
                                windowTitle.contains("default.rviz", Qt::CaseInsensitive) ||
                                windowTitle.contains("- RViz", Qt::CaseInsensitive) ||
                                windowTitle.contains(".rviz", Qt::CaseInsensitive) ||
                                // 新增更多可能的标题格式
                                windowTitle.contains("Visualization", Qt::CaseInsensitive) ||
                                windowTitle.contains("ROS", Qt::CaseInsensitive) ||
                                windowTitle.contains("Display", Qt::CaseInsensitive) ||
                                windowTitle.contains("3D View", Qt::CaseInsensitive) ||
                                // Qt应用程序可能的标题
                                (windowTitle.contains("Qt", Qt::CaseInsensitive) && 
                                 (windowTitle.contains("rviz", Qt::CaseInsensitive) || 
                                  windowTitle.contains("RViz", Qt::CaseInsensitive))) ||
                                // 空标题但可能是RVIZ的子窗口（将在获取属性后再检查尺寸）
                                windowTitle.isEmpty();
                } else {
                    // 默认匹配
                    titleMatch = windowTitle.contains(titlePattern, Qt::CaseInsensitive);
                }
                
                if (titleMatch) {
                    qDebug() << "找到匹配的窗口标题:" << windowTitle;
                    
                    // 检查窗口是否可见和有效 - 先获取窗口属性
                    XWindowAttributes attrs;
                    if (XGetWindowAttributes(m_display, children[i], &attrs)) {
                        qDebug() << "窗口属性 - 状态:" << attrs.map_state 
                                << "尺寸:" << attrs.width << "x" << attrs.height
                                << "位置:" << attrs.x << "," << attrs.y;
                        
                        if (titlePattern == "RViz") {
                            // 对于空标题的窗口，需要检查尺寸是否足够大
                            bool isValidEmptyTitle = !windowTitle.isEmpty() || 
                                                   (windowTitle.isEmpty() && attrs.width > 200 && attrs.height > 200);
                            
                            // 收集所有RVIZ候选窗口
                            if (attrs.width > 0 && attrs.height > 0 && isValidEmptyTitle) {
                                RvizCandidate candidate;
                                candidate.windowId = children[i];
                                candidate.title = windowTitle;
                                candidate.width = attrs.width;
                                candidate.height = attrs.height;
                                
                                // 计算评分
                                candidate.score = 0;
                                if (attrs.width >= 800 && attrs.height >= 600) candidate.score += 100; // 大窗口高分
                                else if (attrs.width >= 300 && attrs.height >= 200) candidate.score += 50; // 中等窗口
                                else candidate.score += 10; // 小窗口低分
                                
                                if (attrs.map_state == IsViewable) candidate.score += 30; // 可见窗口加分
                                if (windowTitle.contains("default.rviz", Qt::CaseInsensitive)) candidate.score += 20; // 包含配置文件名加分
                                
                                rvizCandidates.append(candidate);
                                qDebug() << "添加RVIZ候选窗口 - 标题:" << windowTitle 
                                        << "尺寸:" << attrs.width << "x" << attrs.height 
                                        << "评分:" << candidate.score;
                            }
                        } else {
                            // 非RVIZ窗口使用原有逻辑
                            bool windowValid = (attrs.map_state == IsViewable && attrs.width > 50 && attrs.height > 50);
                            if (windowValid) {
                                qDebug() << "✅ 找到有效窗口:" << windowTitle << "[ID:" << children[i] << "]";
                                XFree(window_name);
                                XFree(children);
                                return children[i];
                            } else {
                                qDebug() << "⚠️ 窗口不满足条件 - 状态:" << (attrs.map_state == IsViewable ? "可见" : "不可见")
                                        << "尺寸:" << attrs.width << "x" << attrs.height;
                            }
                        }
                    } else {
                        qDebug() << "❌ 无法获取窗口属性";
                    }
                }
                
                XFree(window_name);
            }
        }
        XFree(children);
        
        // 处理RVIZ候选窗口
        if (titlePattern == "RViz" && !rvizCandidates.isEmpty()) {
            qDebug() << "找到" << rvizCandidates.size() << "个RVIZ候选窗口，选择最佳的...";
            
            // 按评分排序，选择最高分的
            std::sort(rvizCandidates.begin(), rvizCandidates.end(), 
                     [](const RvizCandidate &a, const RvizCandidate &b) {
                         return a.score > b.score;
                     });
            
            auto best = rvizCandidates.first();
            
            // 特殊处理：如果最佳窗口仍然很小，可能RVIZ还没完全启动
            if (best.width <= 50 && best.height <= 50) {
                qDebug() << "⚠️ 最佳RVIZ窗口尺寸很小(" << best.width << "x" << best.height 
                        << ")，可能RVIZ还没完全启动";
                qDebug() << "建议：等待更长时间让RVIZ完全加载";
                
                // 如果评分太低，返回0表示未找到合适窗口，触发重试
                if (best.score < 50) {
                    qDebug() << "❌ 最佳窗口评分过低(" << best.score << ")，返回未找到以触发重试";
                    return 0;
                }
            }
            
            qDebug() << "✅ 选择最佳RVIZ窗口:" << best.title 
                    << "[ID:" << best.windowId << "] 尺寸:" << best.width << "x" << best.height 
                    << "评分:" << best.score;
            return best.windowId;
        }
        
        qDebug() << "窗口搜索完成，未找到匹配的窗口";
    } else {
        qDebug() << "❌ 无法获取窗口树";
    }
#else
    Q_UNUSED(titlePattern)
    qDebug() << "非Linux系统，跳过窗口搜索";
#endif
    
    return 0;
}

void FlightControlsLauncher::setWindowMaximized(unsigned long windowId)
{
#ifdef Q_OS_LINUX
    if (!m_display || windowId == 0) {
        return;
    }
    
    // 设置窗口最大化状态
    Atom wmState = XInternAtom(m_display, "_NET_WM_STATE", False);
    Atom maxHorz = XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom maxVert = XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = windowId;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 1; // _NET_WM_STATE_ADD
    xev.xclient.data.l[1] = maxHorz;
    xev.xclient.data.l[2] = maxVert;
    
    XSendEvent(m_display, DefaultRootWindow(m_display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    
    XFlush(m_display);
    qDebug() << "设置窗口最大化:" << windowId;
#else
    Q_UNUSED(windowId)
#endif
}

void FlightControlsLauncher::raiseWindow(unsigned long windowId)
{
#ifdef Q_OS_LINUX
    if (!m_display || windowId == 0) {
        return;
    }
    
    // 将窗口置前
    XRaiseWindow(m_display, windowId);
    
    // 设置输入焦点
    XSetInputFocus(m_display, windowId, RevertToPointerRoot, CurrentTime);
    
    // 激活窗口
    Atom activeWindow = XInternAtom(m_display, "_NET_ACTIVE_WINDOW", False);
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = windowId;
    xev.xclient.message_type = activeWindow;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 2; // 来自应用程序的请求
    xev.xclient.data.l[1] = CurrentTime;
    
    XSendEvent(m_display, DefaultRootWindow(m_display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    
    XFlush(m_display);
    qDebug() << "窗口已置前:" << windowId;
#else
    Q_UNUSED(windowId)
#endif
}

void FlightControlsLauncher::maximizeAndRaiseWindow(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    const AppProcess &app = m_applications[appName];
    unsigned long windowId = findWindowByTitle(app.windowTitlePattern);
    
    if (windowId > 0) {
        setWindowMaximized(windowId);
        raiseWindow(windowId);
        qDebug() << appName << "窗口已最大化并置前";
    } else {
        qDebug() << "未找到" << appName << "窗口";
    }
}

void FlightControlsLauncher::findAndMaximizeWindows()
{
    qDebug() << "搜索并最大化应用程序窗口...（尝试次数:" << (m_searchRetryCount + 1) << "/" << (WINDOW_SEARCH_MAX_RETRIES + 1) << ")";
    
    bool foundAnyWindow = false;
    bool foundSmallRvizWindow = false;
    
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().isRunning) {
            qDebug() << "搜索应用程序:" << it.key() << "窗口模式:" << it.value().windowTitlePattern;
            
            unsigned long windowId = findWindowByTitle(it.value().windowTitlePattern);
            if (windowId > 0) {
                setWindowMaximized(windowId);
                raiseWindow(windowId);
                qDebug() << "✅" << it.key() << "窗口已最大化并置前";
                foundAnyWindow = true;
            } else {
                qDebug() << "❌ 未找到" << it.key() << "窗口";
                
                // 特殊处理RVIZ：检查是否因为小窗口被拒绝
                if (it.key() == "RVIZ") {
                    // 这里windowId为0表示findWindowByTitle返回了0，可能是因为小窗口被拒绝
                    foundSmallRvizWindow = true;
                    qDebug() << "RVIZ窗口可能存在但尺寸过小，需要更多时间启动";
                }
            }
        }
    }
    
    // 如果没有找到任何窗口且重试次数未达到最大值，启动重试
    if (!foundAnyWindow && m_searchRetryCount < WINDOW_SEARCH_MAX_RETRIES) {
        m_searchRetryCount++;
        
        // 为RVIZ小窗口情况提供更长的重试延迟
        int retryDelay = WINDOW_SEARCH_RETRY_DELAY;
        if (foundSmallRvizWindow) {
            retryDelay = WINDOW_SEARCH_RETRY_DELAY + 2000; // RVIZ额外等待2秒
            qDebug() << "检测到RVIZ小窗口，延长重试间隔至" << retryDelay << "毫秒";
        }
        
        qDebug() << "未找到窗口，" << retryDelay << "毫秒后进行第" << m_searchRetryCount << "次重试...";
        m_retryTimer->start(retryDelay);
    } else {
        // 重置重试计数器
        m_searchRetryCount = 0;
        if (foundAnyWindow) {
            qDebug() << "🎉 窗口搜索和管理完成！";
        } else {
            qDebug() << "⚠️ 达到最大重试次数，窗口搜索结束";
            if (foundSmallRvizWindow) {
                qDebug() << "💡 建议：手动检查RVIZ是否正在启动中，或尝试重新启动RVIZ";
            }
        }
    }
}

void FlightControlsLauncher::retryWindowSearch()
{
    qDebug() << "执行窗口搜索重试...";
    findAndMaximizeWindows();
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
    
    // 关闭按钮 - 修改为清理所有应用程序
    m_closeButton = new QPushButton("✖", this);
    m_closeButton->setFixedSize(25, 25);
    m_closeButton->setToolTip("关闭启动器并停止所有应用程序");
    connect(m_closeButton, &QPushButton::clicked, this, &FlightControlsLauncher::onCloseButtonClicked);
    
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
            
            // 启动窗口搜索定时器
            m_windowSearchTimer->start(WINDOW_SEARCH_DELAY);
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
    
    // 重置重试计数器，准备新的窗口搜索
    m_searchRetryCount = 0;
    
    // 启动窗口搜索定时器 - RVIZ需要更长的延迟
    int searchDelay = WINDOW_SEARCH_DELAY;
    if (appName == "RVIZ") {
        searchDelay = WINDOW_SEARCH_DELAY + RVIZ_EXTRA_DELAY; // RVIZ额外等待5秒，总共10秒
        qDebug() << "RVIZ需要更长的启动时间，将在" << searchDelay << "毫秒后开始搜索窗口...";
    } else {
        qDebug() << "将在" << searchDelay << "毫秒后开始搜索窗口...";
    }
    m_windowSearchTimer->start(searchDelay);
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
        QProcess::execute("pkill", QStringList() << "-f" << "gnome-terminal.*geometry.*1x1");
        
        app.isRunning = false;
        qDebug() << appName << "已停止";
        updateStatus();
        return;
    }
    
    // 对于QGC，使用增强的停止机制
    if (appName == "QGC") {
        bool processStoppedNormally = false;
        
        // 第一步：尝试通过QProcess优雅停止
        if (app.process) {
            qint64 pid = app.process->processId();
            qDebug() << "尝试优雅停止" << appName << "，PID:" << pid;
            
            app.process->terminate();
            if (app.process->waitForFinished(3000)) {
                qDebug() << appName << "优雅停止成功";
                processStoppedNormally = true;
            } else {
                qWarning() << appName << "优雅终止超时，尝试强制杀死进程";
                app.process->kill();
                if (app.process->waitForFinished(2000)) {
                    qDebug() << appName << "强制停止成功";
                    processStoppedNormally = true;
                } else {
                    qCritical() << appName << "进程可能已僵死，无法通过QProcess停止";
                }
            }
        } else {
            qDebug() << appName << "没有关联的QProcess，将直接执行系统级清理";
        }
        
        // 第二步：使用pkill确保清理所有QGC相关进程
        if (processStoppedNormally) {
            qDebug() << "QProcess停止成功，执行安全性pkill清理...";
        } else {
            qDebug() << "QProcess停止失败或不存在，执行强制pkill清理...";
        }
        
        // 清理主QGC进程
        int result1 = QProcess::execute("pkill", QStringList() << "-f" << "QGroundControl");
        int result2 = QProcess::execute("pkill", QStringList() << "-f" << "qgroundcontrol");
        int result3 = QProcess::execute("pkill", QStringList() << "-f" << "QGC");
        
        // 清理可能的AppImage进程
        int result4 = QProcess::execute("pkill", QStringList() << "-f" << ".AppImage");
        
        qDebug() << "pkill清理结果:" 
                << "QGroundControl(" << result1 << ")"
                << "qgroundcontrol(" << result2 << ")"
                << "QGC(" << result3 << ")"
                << ".AppImage(" << result4 << ")";
        
        // 第三步：等待时间基于停止方式调整
        int waitTime = processStoppedNormally ? 500 : 1000; // 正常停止等待时间更短
        qDebug() << "等待" << waitTime << "毫秒确保进程完全退出...";
        QThread::msleep(waitTime);
        
        // 第四步：验证进程是否真的停止了
        QProcess checkProcess;
        checkProcess.start("pgrep", QStringList() << "-f" << "QGroundControl");
        if (checkProcess.waitForFinished(2000)) {
            QByteArray output = checkProcess.readAllStandardOutput();
            if (output.isEmpty()) {
                qDebug() << "✅ 确认所有QGC进程已完全停止";
            } else {
                qWarning() << "⚠️ 检测到残留的QGC进程，PID:" << output.trimmed();
                // 最后手段：强制杀死残留进程
                QStringList pids = QString::fromLocal8Bit(output).split('\n', QString::SkipEmptyParts);
                for (const QString &pid : pids) {
                    if (!pid.trimmed().isEmpty()) {
                        qDebug() << "强制杀死残留进程PID:" << pid.trimmed();
                        QProcess::execute("kill", QStringList() << "-9" << pid.trimmed());
                    }
                }
            }
        }
        
        app.isRunning = false;
        if (processStoppedNormally) {
            qDebug() << "🎉" << appName << "正常停止流程完成";
        } else {
            qDebug() << "🎉" << appName << "强制停止流程完成";
        }
        updateStatus();
        return;
    }
    
    // 对于其他应用程序，使用原有机制
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

void FlightControlsLauncher::onCloseButtonClicked()
{
    qDebug() << "关闭按钮被点击，停止所有应用程序并关闭启动器";
    
    // 停止所有应用程序
    stopAllApplications();
    
    // 关闭启动器
    close();
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