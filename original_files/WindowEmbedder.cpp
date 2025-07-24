#include "WindowEmbedder.h"
#include "X11Helper.h"
#include <QDebug>
#include <QApplication>

WindowEmbedder::WindowEmbedder(QObject *parent)
    : QObject(parent)
    , m_display(nullptr)
    , m_rootWindow(0)
    , m_screen(0)
    , m_searchTimer(nullptr)
    , m_monitorTimer(nullptr)
{
    if (!initializeX11()) {
        qCritical() << "Failed to initialize X11";
        return;
    }
    
    // Setup search timer
    m_searchTimer = new QTimer(this);
    connect(m_searchTimer, &QTimer::timeout, this, &WindowEmbedder::searchForWindows);
    m_searchTimer->start(SEARCH_INTERVAL);
    
    // Setup monitor timer
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &WindowEmbedder::monitorEmbeddedWindows);
    m_monitorTimer->start(MONITOR_INTERVAL);
}

WindowEmbedder::~WindowEmbedder()
{
    unembedAll();
    cleanupX11();
}

bool WindowEmbedder::embedWindow(Window windowId, QWidget *container)
{
    if (!m_display || !windowId || !container) {
        return false;
    }
    
    if (m_embeddedWindows.contains(windowId)) {
        qDebug() << "Window already embedded:" << windowId;
        return true;
    }
    
    // Use X11Helper to embed window
    X11Helper* x11 = X11Helper::instance();
    bool success = x11->embedWindow(windowId, container->winId());
    
    if (!success) {
        qDebug() << "Failed to embed window:" << windowId;
        return false;
    }
    
    // Store window info
    WindowInfo info;
    info.windowId = windowId;
    info.container = container;
    info.isVisible = true;
    info.geometry = x11->getWindowGeometry(windowId);
    
    // Determine app name
    QString title = x11->getWindowTitle(windowId);
    QString className = x11->getWindowClass(windowId);
    
    if (matchesQGC(title, className)) {
        info.appName = "QGC";
    } else if (matchesRVIZ(title, className)) {
        info.appName = "RVIZ";
    } else {
        info.appName = "Unknown";
    }
    
    m_embeddedWindows[windowId] = info;
    
    // Sync window size
    syncWindowSize(windowId);
    
    emit embedSuccess(windowId, info.appName);
    qDebug() << "Successfully embedded window:" << windowId << "for app:" << info.appName;
    
    return true;
}

void WindowEmbedder::unembedWindow(Window windowId)
{
    if (!m_embeddedWindows.contains(windowId)) {
        return;
    }
    
    WindowInfo info = m_embeddedWindows[windowId];
    
    // Use X11Helper to unembed window
    X11Helper* x11 = X11Helper::instance();
    x11->unembedWindow(windowId, m_rootWindow);
    
    m_embeddedWindows.remove(windowId);
    
    emit windowLost(windowId, info.appName);
    qDebug() << "Unembedded window:" << windowId;
}

void WindowEmbedder::unembedAll()
{
    QList<Window> windows = m_embeddedWindows.keys();
    for (Window windowId : windows) {
        unembedWindow(windowId);
    }
}

Window WindowEmbedder::findWindowByPid(qint64 pid)
{
    if (pid <= 0) {
        return 0;
    }
    
    X11Helper* x11 = X11Helper::instance();
    QList<Window> windows = x11->getAllWindows();
    
    for (Window windowId : windows) {
        if (x11->getWindowPid(windowId) == pid && x11->isApplicationWindow(windowId)) {
            return windowId;
        }
    }
    
    return 0;
}

Window WindowEmbedder::findWindowByTitle(const QString &title)
{
    if (title.isEmpty()) {
        return 0;
    }
    
    X11Helper* x11 = X11Helper::instance();
    QList<Window> windows = x11->getAllWindows();
    
    for (Window windowId : windows) {
        QString windowTitle = x11->getWindowTitle(windowId);
        if (windowTitle.contains(title, Qt::CaseInsensitive) && x11->isApplicationWindow(windowId)) {
            return windowId;
        }
    }
    
    return 0;
}

Window WindowEmbedder::findQGCWindow(qint64 pid)
{
    X11Helper* x11 = X11Helper::instance();
    QList<Window> windows = x11->getAllWindows();
    
    for (Window windowId : windows) {
        if (x11->getWindowPid(windowId) == pid) {
            QString title = x11->getWindowTitle(windowId);
            QString className = x11->getWindowClass(windowId);
            
            if (matchesQGC(title, className) && x11->isApplicationWindow(windowId)) {
                return windowId;
            }
        }
    }
    
    return 0;
}

Window WindowEmbedder::findRVIZWindow(qint64 pid)
{
    X11Helper* x11 = X11Helper::instance();
    QList<Window> windows = x11->getAllWindows();
    
    for (Window windowId : windows) {
        if (x11->getWindowPid(windowId) == pid) {
            QString title = x11->getWindowTitle(windowId);
            QString className = x11->getWindowClass(windowId);
            
            if (matchesRVIZ(title, className) && x11->isApplicationWindow(windowId)) {
                return windowId;
            }
        }
    }
    
    return 0;
}

bool WindowEmbedder::isWindowEmbedded(Window windowId) const
{
    return m_embeddedWindows.contains(windowId);
}

QWidget* WindowEmbedder::getContainer(Window windowId) const
{
    if (m_embeddedWindows.contains(windowId)) {
        return m_embeddedWindows[windowId].container;
    }
    return nullptr;
}

void WindowEmbedder::syncWindowSize(Window windowId)
{
    if (!m_embeddedWindows.contains(windowId)) {
        return;
    }
    
    WindowInfo &info = m_embeddedWindows[windowId];
    QWidget *container = info.container;
    
    if (!container) {
        return;
    }
    
    QSize containerSize = container->size();
    
    X11Helper* x11 = X11Helper::instance();
    x11->resizeWindow(windowId, containerSize.width(), containerSize.height());
    x11->moveWindow(windowId, 0, 0);
    
    info.geometry = QRect(0, 0, containerSize.width(), containerSize.height());
}

void WindowEmbedder::showEmbeddedWindow(Window windowId)
{
    if (!m_embeddedWindows.contains(windowId)) {
        return;
    }
    
    WindowInfo &info = m_embeddedWindows[windowId];
    
    X11Helper* x11 = X11Helper::instance();
    x11->showWindow(windowId);
    
    info.isVisible = true;
}

void WindowEmbedder::hideEmbeddedWindow(Window windowId)
{
    if (!m_embeddedWindows.contains(windowId)) {
        return;
    }
    
    WindowInfo &info = m_embeddedWindows[windowId];
    
    X11Helper* x11 = X11Helper::instance();
    x11->hideWindow(windowId);
    
    info.isVisible = false;
}

void WindowEmbedder::searchForWindows()
{
    // 主动查找所有窗口，打印调试信息
    X11Helper* x11 = X11Helper::instance();
    QList<Window> windows = x11->getAllWindows();
    
    static int searchCount = 0;
    searchCount++;
    
    // 每10次搜索打印一次详细信息（避免日志过多）
    bool printDetails = (searchCount % 10 == 0);
    
    if (printDetails) {
        qDebug() << "Searching for windows, found" << windows.size() << "windows";
    }
    
    for (Window windowId : windows) {
        if (!x11->isApplicationWindow(windowId)) {
            continue;
        }
        
        QString title = x11->getWindowTitle(windowId);
        QString className = x11->getWindowClass(windowId);
        
        // 只打印有标题或类名的窗口
        if (!title.isEmpty() || !className.isEmpty()) {
            if (printDetails) {
                qDebug() << "Window:" << windowId << "Title:" << title << "Class:" << className;
            }
            
            // 检查是否是我们要找的窗口
            if (matchesQGC(title, className)) {
                qDebug() << "Found QGC window:" << windowId << "Title:" << title << "Class:" << className;
                emit windowFound(windowId, "QGC");
            } else if (matchesRVIZ(title, className)) {
                qDebug() << "Found RVIZ window:" << windowId << "Title:" << title << "Class:" << className;
                emit windowFound(windowId, "RVIZ");
            }
        }
    }
}

void WindowEmbedder::monitorEmbeddedWindows()
{
    X11Helper* x11 = X11Helper::instance();
    QList<Window> toRemove;
    
    for (auto it = m_embeddedWindows.begin(); it != m_embeddedWindows.end(); ++it) {
        Window windowId = it.key();
        WindowInfo &info = it.value();
        
        if (!x11->isValidWindow(windowId)) {
            toRemove.append(windowId);
            continue;
        }
        
        // Sync window size if container size changed
        if (info.container) {
            QSize containerSize = info.container->size();
            QRect currentGeometry = x11->getWindowGeometry(windowId);
            
            if (containerSize.width() != currentGeometry.width() || 
                containerSize.height() != currentGeometry.height()) {
                syncWindowSize(windowId);
            }
        }
    }
    
    // Remove invalid windows
    for (Window windowId : toRemove) {
        WindowInfo info = m_embeddedWindows[windowId];
        m_embeddedWindows.remove(windowId);
        emit windowLost(windowId, info.appName);
    }
}

bool WindowEmbedder::initializeX11()
{
    X11Helper* x11 = X11Helper::instance();
    if (!x11->initialize()) {
        return false;
    }
    
    m_display = x11->getDisplay();
    m_rootWindow = x11->getRootWindow();
    
    return true;
}

void WindowEmbedder::cleanupX11()
{
    X11Helper* x11 = X11Helper::instance();
    x11->cleanup();
    m_display = nullptr;
}

bool WindowEmbedder::matchesQGC(const QString &title, const QString &className)
{
    // 打印所有窗口的标题和类名，帮助调试
    qDebug() << "Checking window with title:" << title << "and class:" << className;
    
    // 如果标题或类名为空，不要立即排除
    if (title.isEmpty() && className.isEmpty()) {
        return false;
    }
    
    // 非常宽松的匹配条件
    QStringList qgcTitles = {
        "QGroundControl", "qgroundcontrol", "QGC", "Ground", "Control", 
        "AppImage", "GCS", "UAV", "Drone", "Flight", "PX4", "Ardupilot"
    };
    
    QStringList qgcClasses = {
        "QGroundControl", "qgroundcontrol", "AppImage", "QGC", 
        "org.qgroundcontrol", "org.px4", "px4", "gcs"
    };
    
    // 检查标题
    if (!title.isEmpty()) {
        // 完全匹配
        if (title == "QGroundControl") {
            qDebug() << "Exact match for QGC title:" << title;
            return true;
        }
        
        // 部分匹配
        for (const QString &qgcTitle : qgcTitles) {
            if (title.contains(qgcTitle, Qt::CaseInsensitive)) {
                qDebug() << "Matched QGC by title:" << title << "with pattern:" << qgcTitle;
                return true;
            }
        }
    }
    
    // 检查类名
    if (!className.isEmpty()) {
        // 完全匹配
        if (className == "QGroundControl") {
            qDebug() << "Exact match for QGC class:" << className;
            return true;
        }
        
        // 部分匹配
        for (const QString &qgcClass : qgcClasses) {
            if (className.contains(qgcClass, Qt::CaseInsensitive)) {
                qDebug() << "Matched QGC by class:" << className << "with pattern:" << qgcClass;
                return true;
            }
        }
    }
    
    // 特殊情况：如果标题包含"ground"和"control"，很可能是QGroundControl
    if (!title.isEmpty() && 
        title.contains("ground", Qt::CaseInsensitive) && 
        title.contains("control", Qt::CaseInsensitive)) {
        qDebug() << "Matched QGC by combined keywords in title:" << title;
        return true;
    }
    
    return false;
}

bool WindowEmbedder::matchesRVIZ(const QString &title, const QString &className)
{
    // 打印窗口信息，帮助调试
    qDebug() << "Checking if window matches RVIZ - Title:" << title << "Class:" << className;
    
    // 如果标题或类名为空，不要立即排除
    if (title.isEmpty() && className.isEmpty()) {
        return false;
    }
    
    // 非常宽松的匹配条件
    QStringList rvizTitles = {
        "RViz", "rviz", "RVIZ", "ROS", "Visualization", 
        "Robot", "Visualizer", "3D", "View"
    };
    
    QStringList rvizClasses = {
        "rviz", "RViz", "RVIZ", "org.ros", "ros-visualization"
    };
    
    // 检查标题
    if (!title.isEmpty()) {
        // 完全匹配
        if (title == "rviz" || title == "RViz" || title == "RVIZ") {
            qDebug() << "Exact match for RVIZ title:" << title;
            return true;
        }
        
        // 部分匹配
        for (const QString &rvizTitle : rvizTitles) {
            if (title.contains(rvizTitle, Qt::CaseInsensitive)) {
                qDebug() << "Matched RVIZ by title:" << title << "with pattern:" << rvizTitle;
                return true;
            }
        }
    }
    
    // 检查类名
    if (!className.isEmpty()) {
        // 完全匹配
        if (className == "rviz" || className == "RViz" || className == "RVIZ") {
            qDebug() << "Exact match for RVIZ class:" << className;
            return true;
        }
        
        // 部分匹配
        for (const QString &rvizClass : rvizClasses) {
            if (className.contains(rvizClass, Qt::CaseInsensitive)) {
                qDebug() << "Matched RVIZ by class:" << className << "with pattern:" << rvizClass;
                return true;
            }
        }
    }
    
    return false;
}