#include "X11Helper.h"
#include <QDebug>

// Include X11 headers in implementation file only
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

X11Helper* X11Helper::s_instance = nullptr;

X11Helper* X11Helper::instance()
{
    if (!s_instance) {
        s_instance = new X11Helper();
    }
    return s_instance;
}

X11Helper::X11Helper()
    : m_display(nullptr)
    , m_rootWindow(0)
    , m_screen(0)
{
}

X11Helper::~X11Helper()
{
    cleanup();
}

bool X11Helper::initialize()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        qCritical() << "Cannot open X11 display";
        return false;
    }
    
    m_screen = DefaultScreen(m_display);
    m_rootWindow = RootWindow(m_display, m_screen);
    
    return true;
}

void X11Helper::cleanup()
{
    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

bool X11Helper::embedWindow(Window windowId, unsigned long containerId)
{
    if (!m_display || !windowId || !containerId) {
        return false;
    }
    
    // Set window attributes before embedding
    setWindowAttributes(windowId);
    
    // Reparent the window to the container
    XReparentWindow(m_display, windowId, containerId, 0, 0);
    XMapWindow(m_display, windowId);
    XFlush(m_display);
    
    return verifyEmbedding(windowId, containerId);
}

void X11Helper::unembedWindow(Window windowId, Window rootWindow)
{
    if (!m_display || !windowId) {
        return;
    }
    
    XReparentWindow(m_display, windowId, rootWindow, 0, 0);
    XFlush(m_display);
}

void X11Helper::showWindow(Window windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    XMapWindow(m_display, windowId);
    XFlush(m_display);
}

void X11Helper::hideWindow(Window windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    XUnmapWindow(m_display, windowId);
    XFlush(m_display);
}

void X11Helper::resizeWindow(Window windowId, int width, int height)
{
    if (!m_display || !windowId) {
        return;
    }
    
    XResizeWindow(m_display, windowId, width, height);
    XFlush(m_display);
}

void X11Helper::moveWindow(Window windowId, int x, int y)
{
    if (!m_display || !windowId) {
        return;
    }
    
    XMoveWindow(m_display, windowId, x, y);
    XFlush(m_display);
}

QList<Window> X11Helper::getAllWindows()
{
    QList<Window> windows;
    if (m_display && m_rootWindow) {
        windows.append(getChildWindows(m_rootWindow));
    }
    return windows;
}

QString X11Helper::getWindowTitle(Window windowId)
{
    if (!m_display || !windowId) {
        return QString();
    }
    
    char *windowName = nullptr;
    
    if (XFetchName(m_display, windowId, &windowName) && windowName) {
        QString title = QString::fromLocal8Bit(windowName);
        XFree(windowName);
        return title;
    }
    
    return QString();
}

QString X11Helper::getWindowClass(Window windowId)
{
    if (!m_display || !windowId) {
        return QString();
    }
    
    XClassHint classHint;
    
    if (XGetClassHint(m_display, windowId, &classHint)) {
        QString className = QString::fromLocal8Bit(classHint.res_class);
        
        if (classHint.res_name) XFree(classHint.res_name);
        if (classHint.res_class) XFree(classHint.res_class);
        
        return className;
    }
    
    return QString();
}

qint64 X11Helper::getWindowPid(Window windowId)
{
    if (!m_display || !windowId) {
        return 0;
    }
    
    Atom pidAtom = XInternAtom(m_display, "_NET_WM_PID", True);
    if (pidAtom == None) {
        return 0;
    }
    
    Atom actualType;
    int actualFormat;
    unsigned long nitems, bytesAfter;
    unsigned char *prop = nullptr;
    
    if (XGetWindowProperty(m_display, windowId, pidAtom, 0, 1, False,
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

QRect X11Helper::getWindowGeometry(Window windowId)
{
    if (!m_display || !windowId) {
        return QRect();
    }
    
    Window root;
    int x, y;
    unsigned int width, height, border, depth;
    
    if (XGetGeometry(m_display, windowId, &root, &x, &y, 
                     &width, &height, &border, &depth)) {
        return QRect(x, y, width, height);
    }
    
    return QRect();
}

bool X11Helper::isValidWindow(Window windowId)
{
    if (!m_display || !windowId) {
        return false;
    }
    
    XWindowAttributes attrs;
    return XGetWindowAttributes(m_display, windowId, &attrs) != 0;
}

bool X11Helper::isApplicationWindow(Window windowId)
{
    if (!m_display || !windowId) {
        return false;
    }
    
    XWindowAttributes attrs;
    if (XGetWindowAttributes(m_display, windowId, &attrs) == 0) {
        return false;
    }
    
    // Check if window is mapped and visible
    if (attrs.map_state != IsViewable) {
        return false;
    }
    
    // Check window size (filter out very small windows)
    if (attrs.width < 100 || attrs.height < 100) {
        return false;
    }
    
    return true;
}

bool X11Helper::verifyEmbedding(Window windowId, unsigned long containerId)
{
    if (!m_display || !windowId) {
        return false;
    }
    
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    
    if (XQueryTree(m_display, windowId, &root, &parent, &children, &nchildren) != 0) {
        if (children) {
            XFree(children);
        }
        
        // Check if the parent is the container's window
        return parent == containerId;
    }
    
    return false;
}

void X11Helper::setWindowAttributes(Window windowId)
{
    if (!m_display || !windowId) {
        return;
    }
    
    // Set override_redirect to prevent window manager interference
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;
    XChangeWindowAttributes(m_display, windowId, CWOverrideRedirect, &attrs);
    
    // Set input focus
    XSetInputFocus(m_display, windowId, RevertToParent, CurrentTime);
    
    XFlush(m_display);
}

QList<Window> X11Helper::getChildWindows(Window parent)
{
    QList<Window> windows;
    
    if (!m_display) {
        return windows;
    }
    
    Window root, parentWindow;
    Window *children;
    unsigned int nchildren;
    
    if (XQueryTree(m_display, parent, &root, &parentWindow, &children, &nchildren) != 0) {
        for (unsigned int i = 0; i < nchildren; i++) {
            windows.append(children[i]);
            windows.append(getChildWindows(children[i]));
        }
        
        if (children) {
            XFree(children);
        }
    }
    
    return windows;
}