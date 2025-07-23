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
    // This will be called periodically to find new windows
    // The actual window finding will be triggered by ProcessManager
    // when processes are started
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
    QStringList qgcTitles = {"QGroundControl", "qgroundcontrol", "QGC"};
    QStringList qgcClasses = {"QGroundControl", "qgroundcontrol"};
    
    for (const QString &qgcTitle : qgcTitles) {
        if (title.contains(qgcTitle, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    for (const QString &qgcClass : qgcClasses) {
        if (className.contains(qgcClass, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

bool WindowEmbedder::matchesRVIZ(const QString &title, const QString &className)
{
    QStringList rvizTitles = {"RViz", "rviz", "RVIZ"};
    QStringList rvizClasses = {"rviz", "RViz"};
    
    for (const QString &rvizTitle : rvizTitles) {
        if (title.contains(rvizTitle, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    for (const QString &rvizClass : rvizClasses) {
        if (className.contains(rvizClass, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}