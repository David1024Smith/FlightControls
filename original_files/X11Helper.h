#ifndef X11HELPER_H
#define X11HELPER_H

// Qt includes
#include <QString>
#include <QRect>
#include <QList>

// Forward declare X11 types to avoid including X11 headers in this header
typedef unsigned long Window;
typedef struct _XDisplay Display;

// This header handles X11/Qt conflicts by including X11 headers
// in the implementation file only and providing a clean interface

class X11Helper
{
public:
    static X11Helper* instance();
    ~X11Helper();

    bool initialize();
    void cleanup();
    
    bool embedWindow(Window windowId, unsigned long containerId);
    void unembedWindow(Window windowId, Window rootWindow);
    
    void showWindow(Window windowId);
    void hideWindow(Window windowId);
    void resizeWindow(Window windowId, int width, int height);
    void moveWindow(Window windowId, int x, int y);
    
    QList<Window> getAllWindows();
    QString getWindowTitle(Window windowId);
    QString getWindowClass(Window windowId);
    qint64 getWindowPid(Window windowId);
    QRect getWindowGeometry(Window windowId);
    
    bool isValidWindow(Window windowId);
    bool isApplicationWindow(Window windowId);
    
    Window getRootWindow() const { return m_rootWindow; }
    Display* getDisplay() const { return m_display; }

private:
    X11Helper();
    
    bool verifyEmbedding(Window windowId, unsigned long containerId);
    void setWindowAttributes(Window windowId);
    QList<Window> getChildWindows(Window parent);
    
    Display *m_display;
    Window m_rootWindow;
    int m_screen;
    
    static X11Helper* s_instance;
};

// Undefine X11 macros that conflict with Qt
#ifdef Bool
#undef Bool
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Success
#undef Success
#endif
#ifdef Always
#undef Always
#endif
#ifdef Complex
#undef Complex
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef ButtonPress
#undef ButtonPress
#endif
#ifdef ButtonRelease
#undef ButtonRelease
#endif
#ifdef MotionNotify
#undef MotionNotify
#endif
#ifdef EnterNotify
#undef EnterNotify
#endif
#ifdef LeaveNotify
#undef LeaveNotify
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef GraphicsExpose
#undef GraphicsExpose
#endif
#ifdef NoExpose
#undef NoExpose
#endif
#ifdef VisibilityNotify
#undef VisibilityNotify
#endif
#ifdef CreateNotify
#undef CreateNotify
#endif
#ifdef DestroyNotify
#undef DestroyNotify
#endif
#ifdef UnmapNotify
#undef UnmapNotify
#endif
#ifdef MapNotify
#undef MapNotify
#endif
#ifdef MapRequest
#undef MapRequest
#endif
#ifdef ReparentNotify
#undef ReparentNotify
#endif
#ifdef ConfigureNotify
#undef ConfigureNotify
#endif
#ifdef ConfigureRequest
#undef ConfigureRequest
#endif
#ifdef GravityNotify
#undef GravityNotify
#endif
#ifdef ResizeRequest
#undef ResizeRequest
#endif
#ifdef CirculateNotify
#undef CirculateNotify
#endif
#ifdef CirculateRequest
#undef CirculateRequest
#endif
#ifdef PropertyNotify
#undef PropertyNotify
#endif
#ifdef SelectionClear
#undef SelectionClear
#endif
#ifdef SelectionRequest
#undef SelectionRequest
#endif
#ifdef SelectionNotify
#undef SelectionNotify
#endif
#ifdef ColormapNotify
#undef ColormapNotify
#endif
#ifdef ClientMessage
#undef ClientMessage
#endif
#ifdef MappingNotify
#undef MappingNotify
#endif

// Define our own constants to avoid conflicts
namespace X11 {
    const int X11_None = 0L;
    const int X11_Success = 0;
    const int X11_True = 1;
    const int X11_False = 0;
}

#endif // X11HELPER_H