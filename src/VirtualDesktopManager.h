#ifndef VIRTUALDESKTOPMANAGER_H
#define VIRTUALDESKTOPMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QStringList>

class VirtualDesktopManager : public QObject
{
    Q_OBJECT

public:
    explicit VirtualDesktopManager(QObject *parent = nullptr);
    ~VirtualDesktopManager();

    // 虚拟桌面管理
    bool createVirtualDesktop(const QString &name, int desktopNumber);
    bool switchToDesktop(int desktopNumber);
    bool moveApplicationToDesktop(const QString &appName, int desktopNumber);
    int getCurrentDesktop() const;
    QStringList getAvailableDesktops() const;
    
    // 应用程序管理
    bool registerApplication(const QString &appName, const QString &command, 
                           const QStringList &args = QStringList(), int preferredDesktop = -1);
    bool startApplication(const QString &appName);
    void stopApplication(const QString &appName);
    bool isApplicationRunning(const QString &appName) const;
    
    // 快速切换
    void switchToApplication(const QString &appName);
    void setupQuickSwitchKeys();
    
    // 系统检查
    bool isVirtualDesktopSupported() const;
    QString getDesktopEnvironment() const;

signals:
    void applicationStarted(const QString &appName, int desktop);
    void applicationStopped(const QString &appName);
    void desktopSwitched(int fromDesktop, int toDesktop);
    void applicationSwitched(const QString &appName);
    void errorOccurred(const QString &error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void checkApplicationStatus();

private:
    struct AppInfo {
        QString name;
        QString command;
        QStringList arguments;
        QProcess *process;
        int assignedDesktop;
        bool isRunning;
        qint64 processId;
        unsigned long windowId;
    };

    struct DesktopInfo {
        int number;
        QString name;
        bool isActive;
        QStringList applications;
    };

    // 内部方法
    bool initializeDesktopEnvironment();
    bool setupDesktops();
    
    // 窗口管理方法
    unsigned long findWindowForProcess(qint64 pid);
    bool moveWindowToDesktop(unsigned long windowId, int desktop);
    QList<unsigned long> getWindowsOnDesktop(int desktop);
    
    // 不同桌面环境的支持
    bool setupGnomeDesktops();
    bool setupKDEDesktops();
    bool setupXfceDesktops();
    bool setupI3Desktops();
    
    bool switchDesktopGnome(int desktop);
    bool switchDesktopKDE(int desktop);
    bool switchDesktopXfce(int desktop);
    bool switchDesktopI3(int desktop);
    
    // X11相关
    void *m_display;  // Display* 指针
    unsigned long m_rootWindow;
    int m_screen;
    bool initializeX11();
    void cleanupX11();
    
    // 内部状态
    QMap<QString, AppInfo> m_applications;
    QMap<int, DesktopInfo> m_desktops;
    QTimer *m_statusTimer;
    
    QString m_desktopEnvironment;
    int m_currentDesktop;
    int m_qgcDesktop;
    int m_rvizDesktop;
    bool m_isSupported;
    
    static const int QGC_DESKTOP = 2;
    static const int RVIZ_DESKTOP = 3;
    static const int STATUS_CHECK_INTERVAL = 3000;
};

#endif // VIRTUALDESKTOPMANAGER_H 