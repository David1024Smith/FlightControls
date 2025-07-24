#include "WindowManager.h"
#include <QDebug>
#include <QApplication>
#include <QScreen>
#include <QDesktopWidget>
#include <QDateTime>

// X11 headers
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_rootWindow(0)
    , m_screen(0)
    , m_searchTimer(nullptr)
    , m_monitorTimer(nullptr)
{
    if (!initializeX11()) {
        qCritical() << "Failed to initialize X11 for window management";
        return;
    }
    
    // 初始化定时器
    m_searchTimer = new QTimer(this);
    connect(m_searchTimer, &QTimer::timeout, this, &WindowManager::searchForWindows);
    
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &WindowManager::monitorWindows);
    
    // 开始监控
    m_searchTimer->start(SEARCH_INTERVAL);
    m_monitorTimer->start(MONITOR_INTERVAL);
    
    qDebug() << "WindowManager initialized successfully";
}

WindowManager::~WindowManager()
{
    cleanupX11();
}

bool WindowManager::registerApplication(const QString &appName, QProcess *process)
{
    if (appName.isEmpty() || !process) {
        return false;
    }
    
    AppWindow app;
    app.appName = appName;
    app.process = process;
    app.windowId = 0;
    app.isVisible = false;
    app.isMinimized = false;
    app.lastSeen = QDateTime::currentMSecsSinceEpoch();
    
    m_applications[appName] = app;
    
    // 连接进程信号
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WindowManager::handleProcessFinished);
    
    qDebug() << "Registered application:" << appName;
    return true;
}

void WindowManager::unregisterApplication(const QString &appName)
{
    if (m_applications.contains(appName)) {
        m_applications.remove(appName);
        qDebug() << "Unregistered application:" << appName;
    }
}

void WindowManager::switchToApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        emit switchingFailed(appName, "Application not registered");
        return;
    }
    
    AppWindow &app = m_applications[appName];
    
    // 如果没有找到窗口，先尝试查找
    if (app.windowId == 0) {
        if (!findApplicationWindow(appName)) {
            emit switchingFailed(appName, "Window not found");
            return;
        }
    }
    
    // 最小化其他应用程序
    minimizeAllOthers(appName);
    
    // 显示并聚焦当前应用程序
    if (app.windowId != 0) {
        showWindow(app.windowId);
        raiseWindow(app.windowId);
        focusWindow(app.windowId);
        
        app.isVisible = true;
        app.isMinimized = false;
    }
    
    m_currentActiveApp = appName;
    emit applicationSwitched(appName);
    
    qDebug() << "Switched to application:" << appName;
}

void WindowManager::minimizeAllOthers(const QString &activeApp)
{
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.key() != activeApp && it.value().windowId != 0 && it.value().isVisible) {
            minimizeWindow(it.value().windowId);
            it.value().isVisible = false;
            it.value().isMinimized = true;
        }
    }
}

void WindowManager::restoreApplication(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    AppWindow &app = m_applications[appName];
    if (app.windowId != 0) {
        showWindow(app.windowId);
        app.isVisible = true;
        app.isMinimized = false;
    }
}

bool WindowManager::findApplicationWindow(const QString &appName)
{
    if (!m_applications.contains(appName)) {
        return false;
    }
    
    AppWindow &app = m_applications[appName];
    qint64 processPid = app.process ? app.process->processId() : 0;
    
    QList<unsigned long> windows = getAllWindows();
    
    for (unsigned long windowId : windows) {
        if (!isValidWindow(windowId)) {
            continue;
        }
        
        QString title = getWindowTitle(windowId);
        QString className = getWindowClass(windowId);
        qint64 windowPid = getWindowPid(windowId);
        
        // 首先尝试通过PID匹配
        if (processPid > 0 && windowPid == processPid) {
            QString detectedApp = identifyApplication(title, className);
            if (detectedApp == appName) {
                app.windowId = windowId;
                app.windowTitle = title;
                app.windowClass = className;
                app.geometry = getWindowGeometry(windowId);
                app.lastSeen = QDateTime::currentMSecsSinceEpoch();
                
                emit windowFound(appName, windowId);
                qDebug() << "Found window for" << appName << "by PID:" << windowId;
                return true;
            }
        }
        
        // 如果PID匹配失败，尝试通过标题和类名匹配
        QString detectedApp = identifyApplication(title, className);
        if (detectedApp == appName) {
            app.windowId = windowId;
            app.windowTitle = title;
            app.windowClass = className;
            app.geometry = getWindowGeometry(windowId);
            app.lastSeen = QDateTime::currentMSecsSinceEpoch();
            
            emit windowFound(appName, windowId);
            qDebug() << "Found window for" << appName << "by name:" << windowId;
            return true;
        }
    }
    
    return false;
}

void WindowManager::moveWindowToPosition(const QString &appName, const QRect &geometry)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    AppWindow &app = m_applications[appName];
    if (app.windowId != 0) {
        moveWindow(app.windowId, geometry.x(), geometry.y());
        resizeWindow(app.windowId, geometry.width(), geometry.height());
        app.geometry = geometry;
    }
}

void WindowManager::setWindowAlwaysOnTop(const QString &appName, bool onTop)
{
    if (!m_applications.contains(appName)) {
        return;
    }
    
    AppWindow &app = m_applications[appName];
    if (app.windowId != 0 && m_display) {
        Display *display = static_cast<Display*>(m_display);
        
        // 使用 _NET_WM_STATE 属性设置窗口始终在最前面
        Atom netWmState = XInternAtom(display, "_NET_WM_STATE", False);
        Atom netWmStateAbove = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
        
        XEvent event = {};
        event.type = ClientMessage;
        event.xclient.window = app.windowId;
        event.xclient.message_type = netWmState;
        event.xclient.format = 32;
        event.xclient.data.l[0] = onTop ? 1 : 0;  // 1 = add, 0 = remove
        event.xclient.data.l[1] = netWmStateAbove;
        event.xclient.data.l[2] = 0;
        event.xclient.data.l[3] = 0;
        event.xclient.data.l[4] = 0;
        
        XSendEvent(display, m_rootWindow, False, 
                   SubstructureRedirectMask | SubstructureNotifyMask, &event);
        XFlush(display);
    }
}

bool WindowManager::isApplicationVisible(const QString &appName) const
{
    if (m_applications.contains(appName)) {
        return m_applications[appName].isVisible;
    }
    return false;
}

QString WindowManager::getCurrentActiveApp() const
{
    return m_currentActiveApp;
}

QStringList WindowManager::getRegisteredApps() const
{
    return m_applications.keys();
}

void WindowManager::searchForWindows()
{
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().windowId == 0) {
            findApplicationWindow(it.key());
        }
    }
}

void WindowManager::monitorWindows()
{
    QStringList lostWindows;
    
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().windowId != 0) {
            if (!isValidWindow(it.value().windowId)) {
                lostWindows.append(it.key());
            } else {
                // 更新窗口信息
                it.value().geometry = getWindowGeometry(it.value().windowId);
                it.value().lastSeen = QDateTime::currentMSecsSinceEpoch();
            }
        }
    }
    
    // 处理丢失的窗口
    for (const QString &appName : lostWindows) {
        m_applications[appName].windowId = 0;
        m_applications[appName].isVisible = false;
        emit windowLost(appName);
        qDebug() << "Lost window for application:" << appName;
    }
}

void WindowManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (!process) {
        return;
    }
    
    // 查找对应的应用程序
    for (auto it = m_applications.begin(); it != m_applications.end(); ++it) {
        if (it.value().process == process) {
            it.value().windowId = 0;
            it.value().isVisible = false;
            emit windowLost(it.key());
            qDebug() << "Process finished for application:" << it.key() 
                     << "Exit code:" << exitCode;
            break;
        }
    }
}

// X11 相关实现
bool WindowManager::initializeX11()
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
        qWarning() << "X11 Error:" << errorString 
                   << "Request code:" << error->request_code
                   << "Minor code:" << error->minor_code;
        return 0; // 继续执行，不要终止程序
    });
    
    return true;
}

void WindowManager::cleanupX11()
{
    if (m_display) {
        XCloseDisplay(static_cast<Display*>(m_display));
        m_display = nullptr;
    }
}

QList<unsigned long> WindowManager::getAllWindows()
{
    QList<unsigned long> windows;
    
    if (!m_display) {
        return windows;
    }
    
    Display *display = static_cast<Display*>(m_display);
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    
    if (XQueryTree(display, m_rootWindow, &root, &parent, &children, &nchildren)) {
        for (unsigned int i = 0; i < nchildren; i++) {
            windows.append(children[i]);
        }
        
        if (children) {
            XFree(children);
        }
    }
    
    return windows;
}

QString WindowManager::getWindowTitle(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return QString();
    }
    
    Display *display = static_cast<Display*>(m_display);
    char *windowName = nullptr;
    
    if (XFetchName(display, windowId, &windowName) && windowName) {
        QString title = QString::fromLocal8Bit(windowName);
        XFree(windowName);
        return title;
    }
    
    return QString();
}

QString WindowManager::getWindowClass(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return QString();
    }
    
    Display *display = static_cast<Display*>(m_display);
    XClassHint classHint;
    
    if (XGetClassHint(display, windowId, &classHint)) {
        QString className = QString::fromLocal8Bit(classHint.res_class);
        
        if (classHint.res_name) XFree(classHint.res_name);
        if (classHint.res_class) XFree(classHint.res_class);
        
        return className;
    }
    
    return QString();
}

qint64 WindowManager::getWindowPid(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return 0;
    }
    
    Display *display = static_cast<Display*>(m_display);
    Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        return 0;
    }
    
    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char *prop = nullptr;
    
    if (XGetWindowProperty(display, windowId, pidAtom, 0, 1, False,
                          XA_CARDINAL, &actualType, &actualFormat,
                          &nitems, &bytesAfter, &prop) == Success) {
        
        if (prop && nitems > 0) {
            qint64 pid = *((qint64*)prop);
            XFree(prop);
            return pid;
        }
        
        if (prop) {
            XFree(prop);
        }
    }
    
    return 0;
}

QRect WindowManager::getWindowGeometry(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return QRect();
    }
    
    Display *display = static_cast<Display*>(m_display);
    Window root;
    int x, y;
    unsigned int width, height, border, depth;
    
    if (XGetGeometry(display, windowId, &root, &x, &y, 
                     &width, &height, &border, &depth)) {
        return QRect(x, y, width, height);
    }
    
    return QRect();
}

bool WindowManager::isValidWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return false;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XWindowAttributes attrs;
    return XGetWindowAttributes(display, windowId, &attrs) != 0;
}

void WindowManager::showWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XMapWindow(display, windowId);
    XFlush(display);
}

void WindowManager::hideWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XUnmapWindow(display, windowId);
    XFlush(display);
}

void WindowManager::minimizeWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XIconifyWindow(display, windowId, m_screen);
    XFlush(display);
}

void WindowManager::maximizeWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    
    // 使用 _NET_WM_STATE 设置最大化
    Atom netWmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom netWmStateMaxVert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    Atom netWmStateMaxHorz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    
    XEvent event = {};
    event.type = ClientMessage;
    event.xclient.window = windowId;
    event.xclient.message_type = netWmState;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 1;  // add
    event.xclient.data.l[1] = netWmStateMaxVert;
    event.xclient.data.l[2] = netWmStateMaxHorz;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;
    
    XSendEvent(display, m_rootWindow, False, 
               SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XFlush(display);
}

void WindowManager::raiseWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XRaiseWindow(display, windowId);
    XFlush(display);
}

void WindowManager::focusWindow(unsigned long windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XSetInputFocus(display, windowId, RevertToParent, CurrentTime);
    XFlush(display);
}

void WindowManager::moveWindow(unsigned long windowId, int x, int y)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XMoveWindow(display, windowId, x, y);
    XFlush(display);
}

void WindowManager::resizeWindow(unsigned long windowId, int width, int height)
{
    if (!m_display || !windowId) {
        return;
    }
    
    Display *display = static_cast<Display*>(m_display);
    XResizeWindow(display, windowId, width, height);
    XFlush(display);
}

bool WindowManager::matchesQGC(const QString &title, const QString &className)
{
    QStringList qgcPatterns = {
        "QGroundControl", "qgroundcontrol", "QGC", "Ground Control"
    };
    
    for (const QString &pattern : qgcPatterns) {
        if (title.contains(pattern, Qt::CaseInsensitive) || 
            className.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

bool WindowManager::matchesRVIZ(const QString &title, const QString &className)
{
    QStringList rvizPatterns = {
        "RViz", "rviz", "RVIZ", "Visualization"
    };
    
    for (const QString &pattern : rvizPatterns) {
        if (title.contains(pattern, Qt::CaseInsensitive) || 
            className.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

QString WindowManager::identifyApplication(const QString &title, const QString &className)
{
    if (matchesQGC(title, className)) {
        return "QGC";
    } else if (matchesRVIZ(title, className)) {
        return "RVIZ";
    }
    
    return "Unknown";
} 