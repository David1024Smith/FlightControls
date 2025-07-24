#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QMap>
#include <QProcess>
#include <QRect>
#include <QScreen>
#include <QApplication>

class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QObject *parent = nullptr);
    ~WindowManager();

    // 应用程序管理
    bool registerApplication(const QString &appName, QProcess *process);
    void unregisterApplication(const QString &appName);
    
    // 窗口切换
    void switchToApplication(const QString &appName);
    void minimizeAllOthers(const QString &activeApp);
    void restoreApplication(const QString &appName);
    
    // 窗口查找和管理
    bool findApplicationWindow(const QString &appName);
    void moveWindowToPosition(const QString &appName, const QRect &geometry);
    void setWindowAlwaysOnTop(const QString &appName, bool onTop);
    
    // 状态查询
    bool isApplicationVisible(const QString &appName) const;
    QString getCurrentActiveApp() const;
    QStringList getRegisteredApps() const;

signals:
    void applicationSwitched(const QString &appName);
    void windowFound(const QString &appName, unsigned long windowId);
    void windowLost(const QString &appName);
    void switchingFailed(const QString &appName, const QString &error);

private slots:
    void searchForWindows();
    void monitorWindows();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    struct AppWindow {
        QString appName;
        QProcess *process;
        unsigned long windowId;
        QRect geometry;
        bool isVisible;
        bool isMinimized;
        QString windowTitle;
        QString windowClass;
        qint64 lastSeen;
    };

    // X11 相关方法
    bool initializeX11();
    void cleanupX11();
    QList<unsigned long> getAllWindows();
    QString getWindowTitle(unsigned long windowId);
    QString getWindowClass(unsigned long windowId);
    qint64 getWindowPid(unsigned long windowId);
    QRect getWindowGeometry(unsigned long windowId);
    bool isValidWindow(unsigned long windowId);
    
    // 窗口操作方法
    void showWindow(unsigned long windowId);
    void hideWindow(unsigned long windowId);
    void minimizeWindow(unsigned long windowId);
    void maximizeWindow(unsigned long windowId);
    void raiseWindow(unsigned long windowId);
    void focusWindow(unsigned long windowId);
    void moveWindow(unsigned long windowId, int x, int y);
    void resizeWindow(unsigned long windowId, int width, int height);
    
    // 应用程序识别方法
    bool matchesQGC(const QString &title, const QString &className);
    bool matchesRVIZ(const QString &title, const QString &className);
    QString identifyApplication(const QString &title, const QString &className);
    
    // 内部状态
    void *m_display;  // Display* 指针，避免头文件依赖
    unsigned long m_rootWindow;
    int m_screen;
    
    QMap<QString, AppWindow> m_applications;
    QTimer *m_searchTimer;
    QTimer *m_monitorTimer;
    QString m_currentActiveApp;
    
    static const int SEARCH_INTERVAL = 1000;   // 1 second
    static const int MONITOR_INTERVAL = 2000;  // 2 seconds
};

#endif // WINDOWMANAGER_H 