#ifndef WINDOWEMBEDDER_H
#define WINDOWEMBEDDER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMap>
#include "X11Helper.h"

class WindowEmbedder : public QObject
{
    Q_OBJECT

public:
    explicit WindowEmbedder(QObject *parent = nullptr);
    ~WindowEmbedder();

    bool embedWindow(Window windowId, QWidget *container);
    void unembedWindow(Window windowId);
    void unembedAll();
    
    Window findWindowByPid(qint64 pid);
    Window findWindowByTitle(const QString &title);
    Window findQGCWindow(qint64 pid);
    Window findRVIZWindow(qint64 pid);
    
    bool isWindowEmbedded(Window windowId) const;
    QWidget* getContainer(Window windowId) const;
    
    void syncWindowSize(Window windowId);
    void showEmbeddedWindow(Window windowId);
    void hideEmbeddedWindow(Window windowId);

signals:
    void windowFound(Window windowId, const QString &appName);
    void windowLost(Window windowId, const QString &appName);
    void embedSuccess(Window windowId, const QString &appName);
    void embedFailed(Window windowId, const QString &error);

private slots:
    void searchForWindows();
    void monitorEmbeddedWindows();

private:
    struct WindowInfo {
        Window windowId;
        QWidget *container;
        QString appName;
        bool isVisible;
        QRect geometry;
    };

    bool initializeX11();
    void cleanupX11();
    
    QList<Window> getAllWindows();
    QList<Window> getChildWindows(Window parent);
    
    QString getWindowTitle(Window windowId);
    QString getWindowClass(Window windowId);
    qint64 getWindowPid(Window windowId);
    QRect getWindowGeometry(Window windowId);
    
    bool isValidWindow(Window windowId);
    bool isApplicationWindow(Window windowId);
    bool matchesQGC(const QString &title, const QString &className);
    bool matchesRVIZ(const QString &title, const QString &className);
    
    void setWindowAttributes(Window windowId);
    bool verifyEmbedding(Window windowId, QWidget *container);
    
    Display *m_display;
    Window m_rootWindow;
    int m_screen;
    
    QMap<Window, WindowInfo> m_embeddedWindows;
    QTimer *m_searchTimer;
    QTimer *m_monitorTimer;
    
    static const int SEARCH_INTERVAL = 2000; // 2 seconds
    static const int MONITOR_INTERVAL = 1000; // 1 second
};

#endif // WINDOWEMBEDDER_H