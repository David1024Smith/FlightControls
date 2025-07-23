#ifndef APPLICATIONSWITCHER_H
#define APPLICATIONSWITCHER_H

#include <QObject>
#include <QTimer>
#include <QWidget>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "X11Helper.h" // Include X11Helper.h for Window type definition

class ProcessManager;
class WindowEmbedder;

class ApplicationSwitcher : public QObject
{
    Q_OBJECT

public:
    enum SwitchState {
        Idle,
        Switching,
        WaitingForApp,
        PreloadInProgress,
        Error
    };

    explicit ApplicationSwitcher(ProcessManager *processManager, 
                               WindowEmbedder *windowEmbedder,
                               QObject *parent = nullptr);
    ~ApplicationSwitcher();

    void setContainers(QWidget *qgcContainer, QWidget *rvizContainer);
    
    bool switchToApplication(const QString &appName);
    void startPreload();
    
    SwitchState getCurrentState() const { return m_currentState; }
    QString getCurrentApp() const { return m_currentApp; }
    bool isPreloadComplete() const { return m_preloadComplete; }
    int getPreloadProgress() const { return m_preloadProgress; }

signals:
    void applicationSwitched(const QString &appName);
    void switchFailed(const QString &appName, const QString &error);
    void preloadProgress(int progress);
    void preloadComplete();
    void stateChanged(SwitchState state);

private slots:
    void onProcessStarted(const QString &appName, qint64 pid);
    void onProcessStopped(const QString &appName);
    void onProcessError(const QString &appName, const QString &error);
    void onWindowFound(Window windowId, const QString &appName);
    void onWindowLost(Window windowId, const QString &appName);
    void onEmbedSuccess(Window windowId, const QString &appName);
    void onEmbedFailed(Window windowId, const QString &error);
    void onSwitchTimeout();
    void onPreloadTimeout();
    void updatePreloadProgress();

private:
    void setState(SwitchState state);
    void performSwitch(const QString &appName);
    void showContainer(const QString &appName);
    void hideContainer(const QString &appName);
    void startSwitchAnimation(QWidget *fromContainer, QWidget *toContainer);
    void completeSwitchOperation(const QString &appName);
    void handleSwitchError(const QString &appName, const QString &error);
    
    void startPreloadProcess();
    void checkPreloadStatus();
    void completePreload();
    
    ProcessManager *m_processManager;
    WindowEmbedder *m_windowEmbedder;
    
    QWidget *m_qgcContainer;
    QWidget *m_rvizContainer;
    
    SwitchState m_currentState;
    QString m_currentApp;
    QString m_pendingApp;
    
    // Preload management
    bool m_preloadComplete;
    int m_preloadProgress;
    QTimer *m_preloadTimer;
    QTimer *m_preloadProgressTimer;
    
    // Switch management
    QTimer *m_switchTimer;
    QString m_switchingToApp;
    
    // Animation
    QPropertyAnimation *m_fadeAnimation;
    QGraphicsOpacityEffect *m_opacityEffect;
    
    // Constants
    static const int SWITCH_TIMEOUT = 30000; // 30 seconds
    static const int PRELOAD_TIMEOUT = 60000; // 60 seconds
    static const int PRELOAD_UPDATE_INTERVAL = 500; // 0.5 seconds
    static const int ANIMATION_DURATION = 300; // 0.3 seconds
};

#endif // APPLICATIONSWITCHER_H