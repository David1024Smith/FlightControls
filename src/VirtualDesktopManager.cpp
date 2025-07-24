#include "VirtualDesktopManager.h"
#include <QDebug>
#include <QProcessEnvironment>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>

// X11 headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

VirtualDesktopManager::VirtualDesktopManager(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_rootWindow(0)
    , m_screen(0)
    , m_statusTimer(nullptr)
    , m_currentDesktop(1)
    , m_qgcDesktop(QGC_DESKTOP)
    , m_rvizDesktop(RVIZ_DESKTOP)
    , m_isSupported(false)
{
    if (!initializeDesktopEnvironment()) {
        qCritical() << "Failed to initialize desktop environment";
        return;
    }
    
    if (!initializeX11()) {
        qCritical() << "Failed to initialize X11";
        return;
    }
    
    if (!setupDesktops()) {
        qWarning() << "Failed to setup virtual desktops";
    }
    
    // 设置状态检查定时器
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &VirtualDesktopManager::checkApplicationStatus);
    m_statusTimer->start(STATUS_CHECK_INTERVAL);
    
    qDebug() << "VirtualDesktopManager initialized successfully";
    qDebug() << "Desktop Environment:" << m_desktopEnvironment;
    qDebug() << "Virtual Desktop Support:" << (m_isSupported ? "Yes" : "No");
}

VirtualDesktopManager::~VirtualDesktopManager()
{
    // 停止所有应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().isRunning && it.value().process) {
            it.value().process->kill();
            it.value().process->waitForFinished(3000);
        }
    }
    
    cleanupX11();
}

bool VirtualDesktopManager::createVirtualDesktop(const QString &name, int desktopNumber)
{
    if (!m_isSupported) {
        emit errorOccurred("Virtual desktop not supported");
        return false;
    }
    
    DesktopInfo desktop;
    desktop.number = desktopNumber;
    desktop.name = name;
    desktop.isActive = false;
    
    m_desktops[desktopNumber] = desktop;
    
    qDebug() << "Created virtual desktop:" << name << "Number:" << desktopNumber;
    return true;
}

bool VirtualDesktopManager::switchToDesktop(int desktopNumber)
{
    if (!m_isSupported) {
        emit errorOccurred("Virtual desktop not supported");
        return false;
    }
    
    int oldDesktop = m_currentDesktop;
    
    bool success = false;
    
    if (m_desktopEnvironment == "GNOME") {
        success = switchDesktopGnome(desktopNumber);
    } else if (m_desktopEnvironment == "KDE") {
        success = switchDesktopKDE(desktopNumber);
    } else if (m_desktopEnvironment == "XFCE") {
        success = switchDesktopXfce(desktopNumber);
    } else if (m_desktopEnvironment == "i3") {
        success = switchDesktopI3(desktopNumber);
    } else {
        // 通用X11方法
        if (m_display) {
            Display *display = static_cast<Display*>(m_display);
            
            Atom currentDesktopAtom = XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
            if (currentDesktopAtom != None) {
                XEvent event = {};
                event.type = ClientMessage;
                event.xclient.window = m_rootWindow;
                event.xclient.message_type = currentDesktopAtom;
                event.xclient.format = 32;
                event.xclient.data.l[0] = desktopNumber - 1;  // X11使用0-based索引
                event.xclient.data.l[1] = CurrentTime;
                
                success = (XSendEvent(display, m_rootWindow, False, 
                                    SubstructureRedirectMask | SubstructureNotifyMask, &event) != 0);
                XFlush(display);
            }
        }
    }
    
    if (success) {
        // 更新桌面状态
        if (m_desktops.contains(oldDesktop)) {
            m_desktops[oldDesktop].isActive = false;
        }
        if (m_desktops.contains(desktopNumber)) {
            m_desktops[desktopNumber].isActive = true;
        }
        
        m_currentDesktop = desktopNumber;
        emit desktopSwitched(oldDesktop, desktopNumber);
        
        qDebug() << "Switched to desktop:" << desktopNumber;
    } else {
        qWarning() << "Failed to switch to desktop:" << desktopNumber;
    }
    
    return success;
}

bool VirtualDesktopManager::moveApplicationToDesktop(const QString &appName, int desktopNumber)
{
    if (!m_applications.contains(appName)) {
        return false;
    }
    
    AppInfo &app = m_applications[appName];
    
    if (app.windowId != 0) {
        if (moveWindowToDesktop(app.windowId, desktopNumber)) {
            app.assignedDesktop = desktopNumber;
            
            // 更新桌面信息
            if (m_desktops.contains(desktopNumber)) {
                if (!m_desktops[desktopNumber].applications.contains(appName)) {
                    m_desktops[desktopNumber].applications.append(appName);
                }
            }
            
            qDebug() << "Moved application" << appName << "to desktop" << desktopNumber;
            return true;
        }
    }
    
    return false;
}

int VirtualDesktopManager::getCurrentDesktop() const
{
    return m_currentDesktop;
}

QStringList VirtualDesktopManager::getAvailableDesktops() const
{
    QStringList desktops;
    for (auto it = m_desktops.begin(); it != m_desktops.end(); ++it) {
        desktops.append(QString("%1: %2").arg(it.key()).arg(it.value().name));
    }
    return desktops;
}

bool VirtualDesktopManager::registerApplication(const QString &appName, const QString &command, 
                                              const QStringList &args, int preferredDesktop)
{
    AppInfo app;
    app.name = appName;
    app.command = command;
    app.arguments = args;
    app.process = nullptr;
    app.assignedDesktop = (preferredDesktop > 0) ? preferredDesktop : 
                         (appName == "QGC" ? m_qgcDesktop : m_rvizDesktop);
    app.isRunning = false;
    app.processId = 0;
    app.windowId = 0;
    
    m_applications[appName] = app;
    
    // 确保目标桌面存在
    if (!m_desktops.contains(app.assignedDesktop)) {
        QString desktopName = (appName == "QGC") ? "QGroundControl" : 
                             (appName == "RVIZ") ? "RVIZ" : appName;
        createVirtualDesktop(desktopName, app.assignedDesktop);
    }
    
    qDebug() << "Registered application:" << appName << "Desktop:" << app.assignedDesktop;
    return true;
}

bool VirtualDesktopManager::startApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        emit errorOccurred("Application not registered: " + appName);
        return false;
    }
    
    AppInfo &app = m_applications[appName];
    
    if (app.isRunning && app.process) {
        qDebug() << "Application already running:" << appName;
        return true;
    }
    
    // 创建新进程
    if (app.process) {
        if (app.process->state() != QProcess::NotRunning) {
            app.process->kill();
            app.process->waitForFinished(3000);
        }
        app.process->deleteLater();
        app.process = nullptr;
    }
    
    app.process = new QProcess(this);
    connect(app.process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VirtualDesktopManager::onProcessFinished);
    
    // 设置环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (appName == "RVIZ") {
        env.insert("ROS_MASTER_URI", "http://localhost:11311");
        env.insert("DISPLAY", ":0");
        
        // 检查并启动roscore
        QProcess roscoreCheck;
        roscoreCheck.start("pgrep", QStringList() << "roscore");
        roscoreCheck.waitForFinished(2000);
        
        if (roscoreCheck.exitCode() != 0) {
            qDebug() << "Starting roscore...";
            QProcess::startDetached("roscore");
            // 使用定时器替代阻塞等待，避免冻结UI
            QTimer::singleShot(3000, this, [this, appName]() {
                if (m_applications.contains(appName)) {
                    // 继续处理启动逻辑
                    qDebug() << "Roscore should be ready now, continuing...";
                }
            });
        }
    }
    
    app.process->setProcessEnvironment(env);
    
    // 启动进程
    qDebug() << "Starting application:" << appName << "on desktop:" << app.assignedDesktop;
    app.process->start(app.command, app.arguments);
    
    if (!app.process->waitForStarted(10000)) {
        QString errorMsg = "Failed to start " + appName + ": " + app.process->errorString();
        emit errorOccurred(errorMsg);
        return false;
    }
    
    app.isRunning = true;
    app.processId = app.process->processId();
    
    // 延迟查找窗口并移动到指定桌面 - 使用QPointer保证线程安全
    QTimer::singleShot(3000, this, [this, appName]() {
        if (!this || !m_applications.contains(appName) || !m_applications[appName].isRunning) {
            return;
        }
        
        AppInfo &app = m_applications[appName];
        app.windowId = findWindowForProcess(app.processId);
        
        if (app.windowId != 0) {
            moveApplicationToDesktop(appName, app.assignedDesktop);
            
            // 如果是当前要显示的应用，切换到对应桌面
            switchToDesktop(app.assignedDesktop);
        }
    });
    
    emit applicationStarted(appName, app.assignedDesktop);
    return true;
}

void VirtualDesktopManager::stopApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    AppInfo &app = m_applications[appName];
    
    if (!app.isRunning || !app.process) {
        return;
    }
    
    qDebug() << "Stopping application:" << appName;
    
    // 尝试优雅地终止进程
    app.process->terminate();
    
    if (!app.process->waitForFinished(5000)) {
        app.process->kill();
        app.process->waitForFinished(3000);
    }
    
    app.isRunning = false;
    app.windowId = 0;
    
    emit applicationStopped(appName);
}

bool VirtualDesktopManager::isApplicationRunning(const QString &appName) const
{
    if (m_applications.contains(appName)) {
        return m_applications[appName].isRunning;
    }
    return false;
}

void VirtualDesktopManager::switchToApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        emit errorOccurred("Application not registered: " + appName);
        return;
    }
    
    const AppInfo &app = m_applications[appName];
    
    if (!app.isRunning) {
        // 如果应用程序未运行，先启动它
        if (startApplication(appName)) {
            // 启动成功，延迟切换桌面
            QTimer::singleShot(2000, [this, appName]() {
                if (m_applications.contains(appName)) {
                    switchToDesktop(m_applications[appName].assignedDesktop);
                    emit applicationSwitched(appName);
                }
            });
        }
    } else {
        // 应用程序已运行，直接切换桌面
        if (switchToDesktop(app.assignedDesktop)) {
            emit applicationSwitched(appName);
        }
    }
}

void VirtualDesktopManager::setupQuickSwitchKeys()
{
    // 设置全局快捷键（需要桌面环境支持）
    QString qgcShortcut = QString("gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-%1 \"['<Super>1']\"").arg(m_qgcDesktop);
    QString rvizShortcut = QString("gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-%1 \"['<Super>2']\"").arg(m_rvizDesktop);
    
    if (m_desktopEnvironment == "GNOME") {
        QProcess::execute("bash", QStringList() << "-c" << qgcShortcut);
        QProcess::execute("bash", QStringList() << "-c" << rvizShortcut);
        qDebug() << "Setup GNOME quick switch keys";
    }
    // 其他桌面环境的快捷键设置...
}

bool VirtualDesktopManager::isVirtualDesktopSupported() const
{
    return m_isSupported;
}

QString VirtualDesktopManager::getDesktopEnvironment() const
{
    return m_desktopEnvironment;
}

void VirtualDesktopManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }
    
    // 查找对应的应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().process == process) {
            it.value().isRunning = false;
            it.value().windowId = 0;
            
            emit applicationStopped(it.key());
            qDebug() << "Application finished:" << it.key() << "Exit code:" << exitCode;
            break;
        }
    }
}

void VirtualDesktopManager::checkApplicationStatus()
{
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        AppInfo &app = it.value();
        
        if (app.isRunning && app.windowId == 0 && app.processId > 0) {
            // 尝试查找窗口
            app.windowId = findWindowForProcess(app.processId);
            
            if (app.windowId != 0) {
                // 确保窗口在正确的桌面上
                moveApplicationToDesktop(app.name, app.assignedDesktop);
            }
        }
    }
}

bool VirtualDesktopManager::initializeDesktopEnvironment()
{
    // 检测桌面环境
    QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
    QString session = qgetenv("DESKTOP_SESSION");
    QString gdmSession = qgetenv("GDMSESSION");
    
    if (desktop.contains("GNOME", Qt::CaseInsensitive) || 
        session.contains("gnome", Qt::CaseInsensitive)) {
        m_desktopEnvironment = "GNOME";
        m_isSupported = true;
    } else if (desktop.contains("KDE", Qt::CaseInsensitive) || 
               session.contains("kde", Qt::CaseInsensitive)) {
        m_desktopEnvironment = "KDE";
        m_isSupported = true;
    } else if (desktop.contains("XFCE", Qt::CaseInsensitive) || 
               session.contains("xfce", Qt::CaseInsensitive)) {
        m_desktopEnvironment = "XFCE";
        m_isSupported = true;
    } else if (desktop.contains("i3", Qt::CaseInsensitive)) {
        m_desktopEnvironment = "i3";
        m_isSupported = true;
    } else {
        m_desktopEnvironment = "Unknown";
        m_isSupported = false;
        qWarning() << "Unknown desktop environment:" << desktop;
    }
    
    return true;
}

bool VirtualDesktopManager::setupDesktops()
{
    if (!m_isSupported) {
        return false;
    }
    
    // 创建预定义的桌面
    createVirtualDesktop("Main", 1);
    createVirtualDesktop("QGroundControl", m_qgcDesktop);
    createVirtualDesktop("RVIZ", m_rvizDesktop);
    
    // 根据桌面环境进行特定设置
    if (m_desktopEnvironment == "GNOME") {
        return setupGnomeDesktops();
    } else if (m_desktopEnvironment == "KDE") {
        return setupKDEDesktops();
    } else if (m_desktopEnvironment == "XFCE") {
        return setupXfceDesktops();
    } else if (m_desktopEnvironment == "i3") {
        return setupI3Desktops();
    }
    
    return true;
}

unsigned long VirtualDesktopManager::findWindowForProcess(qint64 pid)
{
    if (!m_display || pid <= 0) {
        return 0;
    }
    
    Display *display = static_cast<Display*>(m_display);
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    
    if (!XQueryTree(display, m_rootWindow, &root, &parent, &children, &nchildren)) {
        return 0;
    }
    
    for (unsigned int i = 0; i < nchildren; i++) {
        // 获取窗口的PID
        Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
        if (pidAtom != None) {
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char *prop = nullptr;
            
            if (XGetWindowProperty(display, children[i], pidAtom, 0, 1, False,
                                  XA_CARDINAL, &actualType, &actualFormat,
                                  &nitems, &bytesAfter, &prop) == Success) {
                
                if (prop && nitems > 0) {
                    qint64 windowPid = *((qint64*)prop);
                    XFree(prop);
                    
                    if (windowPid == pid) {
                        // 检查是否是应用程序窗口
                        XWindowAttributes attrs;
                        if (XGetWindowAttributes(display, children[i], &attrs) &&
                            attrs.map_state == IsViewable && 
                            attrs.width > 100 && attrs.height > 100) {
                            
                            if (children) XFree(children);
                            return children[i];
                        }
                    }
                }
                
                if (prop) XFree(prop);
            }
        }
    }
    
    if (children) XFree(children);
    return 0;
}

bool VirtualDesktopManager::moveWindowToDesktop(unsigned long windowId, int desktop)
{
    if (!m_display || windowId == 0) {
        return false;
    }
    
    Display *display = static_cast<Display*>(m_display);
    
    Atom desktopAtom = XInternAtom(display, "_NET_WM_DESKTOP", False);
    if (desktopAtom == None) {
        return false;
    }
    
    // 设置窗口的桌面属性
    long desktopValue = desktop - 1;  // X11使用0-based索引
    
    XChangeProperty(display, windowId, desktopAtom, XA_CARDINAL, 32,
                   PropModeReplace, (unsigned char*)&desktopValue, 1);
    
    XFlush(display);
    return true;
}

QList<unsigned long> VirtualDesktopManager::getWindowsOnDesktop(int desktop)
{
    QList<unsigned long> windows;
    
    if (!m_display) {
        return windows;
    }
    
    Display *display = static_cast<Display*>(m_display);
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    
    if (!XQueryTree(display, m_rootWindow, &root, &parent, &children, &nchildren)) {
        return windows;
    }
    
    Atom desktopAtom = XInternAtom(display, "_NET_WM_DESKTOP", False);
    
    for (unsigned int i = 0; i < nchildren; i++) {
        if (desktopAtom != None) {
            Atom actualType;
            int actualFormat;
            unsigned long nitems, bytesAfter;
            unsigned char *prop = nullptr;
            
            if (XGetWindowProperty(display, children[i], desktopAtom, 0, 1, False,
                                  XA_CARDINAL, &actualType, &actualFormat,
                                  &nitems, &bytesAfter, &prop) == Success) {
                
                if (prop && nitems > 0) {
                    long windowDesktop = *((long*)prop);
                    if (windowDesktop == desktop - 1) {  // X11使用0-based索引
                        windows.append(children[i]);
                    }
                    XFree(prop);
                }
            }
        }
    }
    
    if (children) XFree(children);
    return windows;
}

// 桌面环境特定的实现
bool VirtualDesktopManager::setupGnomeDesktops()
{
    // 设置GNOME工作区数量
    QProcess::execute("gsettings", QStringList() << "set" << "org.gnome.desktop.wm.preferences" 
                     << "num-workspaces" << "4");
    
    // 启用工作区切换动画
    QProcess::execute("gsettings", QStringList() << "set" << "org.gnome.desktop.interface" 
                     << "enable-animations" << "true");
    
    return true;
}

bool VirtualDesktopManager::setupKDEDesktops()
{
    // 使用KDE的qdbus设置虚拟桌面
    QProcess::execute("qdbus", QStringList() << "org.kde.KWin" << "/VirtualDesktopManager" 
                     << "setNumberOfDesktops" << "4");
    
    return true;
}

bool VirtualDesktopManager::setupXfceDesktops()
{
    // 使用xfconf设置XFCE工作区
    QProcess::execute("xfconf-query", QStringList() << "-c" << "xfwm4" << "-p" 
                     << "/general/workspace_count" << "-s" << "4");
    
    return true;
}

bool VirtualDesktopManager::setupI3Desktops()
{
    // i3窗口管理器通常通过配置文件管理工作区
    // 这里可以发送i3-msg命令来管理工作区
    return true;
}

bool VirtualDesktopManager::switchDesktopGnome(int desktop)
{
    QProcess process;
    process.start("gdbus", QStringList() << "call" << "--session" 
                 << "--dest" << "org.gnome.Shell" 
                 << "--object-path" << "/org/gnome/Shell" 
                 << "--method" << "org.gnome.Shell.Eval" 
                 << QString("global.workspace_manager.get_workspace_by_index(%1).activate(global.get_current_time())").arg(desktop - 1));
    
    return process.waitForFinished(3000) && process.exitCode() == 0;
}

bool VirtualDesktopManager::switchDesktopKDE(int desktop)
{
    QProcess process;
    process.start("qdbus", QStringList() << "org.kde.KWin" << "/VirtualDesktopManager" 
                 << "setCurrent" << QString::number(desktop));
    
    return process.waitForFinished(3000) && process.exitCode() == 0;
}

bool VirtualDesktopManager::switchDesktopXfce(int desktop)
{
    QProcess process;
    process.start("xdotool", QStringList() << "set_desktop" << QString::number(desktop - 1));
    
    return process.waitForFinished(3000) && process.exitCode() == 0;
}

bool VirtualDesktopManager::switchDesktopI3(int desktop)
{
    QProcess process;
    process.start("i3-msg", QStringList() << "workspace" << QString::number(desktop));
    
    return process.waitForFinished(3000) && process.exitCode() == 0;
}

bool VirtualDesktopManager::initializeX11()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        qCritical() << "Cannot open X11 display - check DISPLAY environment variable";
        return false;
    }
    
    Display *display = static_cast<Display*>(m_display);
    m_screen = DefaultScreen(display);
    m_rootWindow = RootWindow(display, m_screen);
    
    // 设置X11错误处理
    XSetErrorHandler([](Display*, XErrorEvent* error) -> int {
        char errorString[256];
        XGetErrorText(error->display, error->error_code, errorString, sizeof(errorString));
        qWarning() << "X11 Error in VirtualDesktopManager:" << errorString 
                   << "Request code:" << error->request_code
                   << "Minor code:" << error->minor_code;
        return 0; // 继续执行，不要终止程序
    });
    
    return true;
}

void VirtualDesktopManager::cleanupX11()
{
    if (m_display) {
        XCloseDisplay(static_cast<Display*>(m_display));
        m_display = nullptr;
    }
} 