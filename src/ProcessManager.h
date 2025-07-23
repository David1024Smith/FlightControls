#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QMap>
#include <QString>
#include <QStringList>

class ProcessManager : public QObject
{
    Q_OBJECT

public:
    enum ProcessState {
        Stopped,
        Starting,
        Running,
        Stopping,
        Error
    };

    explicit ProcessManager(QObject *parent = nullptr);
    ~ProcessManager();

    bool startQGC();
    bool startRVIZ();
    void stopQGC();
    void stopRVIZ();
    void stopAll();
    
    ProcessState getQGCState() const { return m_qgcState; }
    ProcessState getRVIZState() const { return m_rvizState; }
    
    qint64 getQGCPid() const;
    qint64 getRVIZPid() const;
    
    bool isQGCRunning() const { return m_qgcState == Running; }
    bool isRVIZRunning() const { return m_rvizState == Running; }

signals:
    void qgcStateChanged(ProcessState state);
    void rvizStateChanged(ProcessState state);
    void processError(const QString &appName, const QString &error);
    void processStarted(const QString &appName, qint64 pid);
    void processStopped(const QString &appName);

private slots:
    void onQGCFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onQGCError(QProcess::ProcessError error);
    void onRoscoreFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRoscoreError(QProcess::ProcessError error);
    void onRVIZFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onRVIZError(QProcess::ProcessError error);
    void checkRoscoreReady();
    void performHealthCheck();

private:
    QString findQGCAppImage();
    bool checkROSEnvironment();
    bool isRoscoreReady();
    void startRVIZProcess();
    void setQGCState(ProcessState state);
    void setRVIZState(ProcessState state);
    void killProcess(QProcess *process);

    // QGC Process
    QProcess *m_qgcProcess;
    ProcessState m_qgcState;
    QString m_qgcPath;
    int m_qgcRestartCount;
    
    // RVIZ Processes
    QProcess *m_roscoreProcess;
    QProcess *m_rvizProcess;
    ProcessState m_rvizState;
    int m_rvizRestartCount;
    
    // Timers
    QTimer *m_roscoreReadyTimer;
    QTimer *m_healthCheckTimer;
    
    // Constants
    static const int MAX_RESTART_COUNT = 3;
    static const int ROSCORE_READY_TIMEOUT = 30000; // 30 seconds
    static const int HEALTH_CHECK_INTERVAL = 10000; // 10 seconds
};

#endif // PROCESSMANAGER_H