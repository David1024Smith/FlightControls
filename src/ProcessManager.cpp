#include "ProcessManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QThread>

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_qgcProcess(nullptr)
    , m_qgcState(Stopped)
    , m_qgcRestartCount(0)
    , m_roscoreProcess(nullptr)
    , m_rvizProcess(nullptr)
    , m_rvizState(Stopped)
    , m_rvizRestartCount(0)
    , m_roscoreReadyTimer(nullptr)
    , m_healthCheckTimer(nullptr)
{
    // Initialize QGC process
    m_qgcProcess = new QProcess(this);
    connect(m_qgcProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onQGCFinished);
    connect(m_qgcProcess, &QProcess::errorOccurred,
            this, &ProcessManager::onQGCError);
    
    // Initialize RVIZ processes
    m_roscoreProcess = new QProcess(this);
    connect(m_roscoreProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onRoscoreFinished);
    connect(m_roscoreProcess, &QProcess::errorOccurred,
            this, &ProcessManager::onRoscoreError);
    
    m_rvizProcess = new QProcess(this);
    connect(m_rvizProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ProcessManager::onRVIZFinished);
    connect(m_rvizProcess, &QProcess::errorOccurred,
            this, &ProcessManager::onRVIZError);
    
    // Initialize timers
    m_roscoreReadyTimer = new QTimer(this);
    m_roscoreReadyTimer->setSingleShot(true);
    m_roscoreReadyTimer->setInterval(ROSCORE_READY_TIMEOUT);
    connect(m_roscoreReadyTimer, &QTimer::timeout, this, &ProcessManager::checkRoscoreReady);
    
    m_healthCheckTimer = new QTimer(this);
    m_healthCheckTimer->setInterval(HEALTH_CHECK_INTERVAL);
    connect(m_healthCheckTimer, &QTimer::timeout, this, &ProcessManager::performHealthCheck);
    m_healthCheckTimer->start();
    
    // Find QGC AppImage
    m_qgcPath = findQGCAppImage();
}

ProcessManager::~ProcessManager()
{
    stopAll();
}

bool ProcessManager::startQGC()
{
    if (m_qgcState == Running || m_qgcState == Starting) {
        return true;
    }
    
    if (m_qgcPath.isEmpty()) {
        emit processError("QGC", "QGroundControl.AppImage not found");
        return false;
    }
    
    setQGCState(Starting);
    
    // Set environment variables
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_X11_NO_MITSHM", "1"); // Prevent X11 shared memory issues
    m_qgcProcess->setProcessEnvironment(env);
    
    m_qgcProcess->start(m_qgcPath);
    
    if (!m_qgcProcess->waitForStarted(5000)) {
        setQGCState(Error);
        emit processError("QGC", "Failed to start QGroundControl");
        return false;
    }
    
    setQGCState(Running);
    emit processStarted("QGC", m_qgcProcess->processId());
    m_qgcRestartCount = 0;
    
    qDebug() << "QGroundControl started with PID:" << m_qgcProcess->processId();
    return true;
}

bool ProcessManager::startRVIZ()
{
    if (m_rvizState == Running || m_rvizState == Starting) {
        return true;
    }
    
    if (!checkROSEnvironment()) {
        emit processError("RVIZ", "ROS environment not properly configured");
        return false;
    }
    
    setRVIZState(Starting);
    
    // Start roscore first
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ROS_MASTER_URI", "http://localhost:11311");
    
    m_roscoreProcess->setProcessEnvironment(env);
    m_roscoreProcess->start("roscore");
    
    if (!m_roscoreProcess->waitForStarted(5000)) {
        setRVIZState(Error);
        emit processError("RVIZ", "Failed to start roscore");
        return false;
    }
    
    // Start timer to check when roscore is ready
    m_roscoreReadyTimer->start();
    
    qDebug() << "Roscore started, waiting for it to be ready...";
    return true;
}

void ProcessManager::stopQGC()
{
    if (m_qgcState == Stopped || m_qgcState == Stopping) {
        return;
    }
    
    setQGCState(Stopping);
    killProcess(m_qgcProcess);
}

void ProcessManager::stopRVIZ()
{
    if (m_rvizState == Stopped || m_rvizState == Stopping) {
        return;
    }
    
    setRVIZState(Stopping);
    
    // Stop RVIZ first, then roscore
    if (m_rvizProcess->state() != QProcess::NotRunning) {
        killProcess(m_rvizProcess);
    }
    
    if (m_roscoreProcess->state() != QProcess::NotRunning) {
        killProcess(m_roscoreProcess);
    }
}

void ProcessManager::stopAll()
{
    stopQGC();
    stopRVIZ();
    
    // Wait for processes to terminate
    if (m_qgcProcess->state() != QProcess::NotRunning) {
        m_qgcProcess->waitForFinished(3000);
    }
    if (m_rvizProcess->state() != QProcess::NotRunning) {
        m_rvizProcess->waitForFinished(3000);
    }
    if (m_roscoreProcess->state() != QProcess::NotRunning) {
        m_roscoreProcess->waitForFinished(3000);
    }
}

qint64 ProcessManager::getQGCPid() const
{
    return (m_qgcProcess && m_qgcProcess->state() == QProcess::Running) ? 
           m_qgcProcess->processId() : 0;
}

qint64 ProcessManager::getRVIZPid() const
{
    return (m_rvizProcess && m_rvizProcess->state() == QProcess::Running) ? 
           m_rvizProcess->processId() : 0;
}

void ProcessManager::onQGCFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "QGroundControl finished with exit code:" << exitCode;
    
    if (exitStatus == QProcess::CrashExit && m_qgcRestartCount < MAX_RESTART_COUNT) {
        qDebug() << "QGroundControl crashed, attempting restart...";
        m_qgcRestartCount++;
        QTimer::singleShot(2000, this, &ProcessManager::startQGC);
        return;
    }
    
    setQGCState(Stopped);
    emit processStopped("QGC");
}

void ProcessManager::onQGCError(QProcess::ProcessError error)
{
    QString errorString;
    switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start QGroundControl";
            break;
        case QProcess::Crashed:
            errorString = "QGroundControl crashed";
            break;
        case QProcess::Timedout:
            errorString = "QGroundControl timed out";
            break;
        default:
            errorString = "Unknown QGroundControl error";
    }
    
    setQGCState(Error);
    emit processError("QGC", errorString);
}

void ProcessManager::onRoscoreFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "Roscore finished with exit code:" << exitCode;
    
    // If roscore stops, stop RVIZ too
    if (m_rvizProcess->state() != QProcess::NotRunning) {
        killProcess(m_rvizProcess);
    }
    
    if (exitStatus == QProcess::CrashExit && m_rvizRestartCount < MAX_RESTART_COUNT) {
        qDebug() << "Roscore crashed, attempting restart...";
        m_rvizRestartCount++;
        QTimer::singleShot(2000, this, &ProcessManager::startRVIZ);
        return;
    }
    
    setRVIZState(Stopped);
    emit processStopped("RVIZ");
}

void ProcessManager::onRoscoreError(QProcess::ProcessError error)
{
    QString errorString = QString("Roscore error: %1").arg(error);
    setRVIZState(Error);
    emit processError("RVIZ", errorString);
}

void ProcessManager::onRVIZFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "RVIZ finished with exit code:" << exitCode;
    
    if (exitStatus == QProcess::CrashExit && m_rvizRestartCount < MAX_RESTART_COUNT) {
        qDebug() << "RVIZ crashed, attempting restart...";
        m_rvizRestartCount++;
        QTimer::singleShot(2000, this, &ProcessManager::startRVIZProcess);
        return;
    }
}

void ProcessManager::onRVIZError(QProcess::ProcessError error)
{
    QString errorString = QString("RVIZ error: %1").arg(error);
    emit processError("RVIZ", errorString);
}

void ProcessManager::checkRoscoreReady()
{
    if (isRoscoreReady()) {
        startRVIZProcess();
    } else {
        // Check again in 1 second
        QTimer::singleShot(1000, this, &ProcessManager::checkRoscoreReady);
    }
}

void ProcessManager::performHealthCheck()
{
    // Check QGC health
    if (m_qgcState == Running && m_qgcProcess->state() != QProcess::Running) {
        qDebug() << "QGC health check failed - process not running";
        setQGCState(Error);
        emit processError("QGC", "Process unexpectedly stopped");
    }
    
    // Check RVIZ health
    if (m_rvizState == Running) {
        if (m_roscoreProcess->state() != QProcess::Running) {
            qDebug() << "Roscore health check failed";
            setRVIZState(Error);
            emit processError("RVIZ", "Roscore unexpectedly stopped");
        }
        if (m_rvizProcess->state() != QProcess::Running) {
            qDebug() << "RVIZ health check failed";
            setRVIZState(Error);
            emit processError("RVIZ", "RVIZ process unexpectedly stopped");
        }
    }
}

QString ProcessManager::findQGCAppImage()
{
    QStringList searchPaths = {
        QDir::currentPath(),
        QDir::homePath(),
        QDir::homePath() + "/Downloads",
        "/opt",
        "/usr/local/bin"
    };
    
    QStringList possibleNames = {
        "QGroundControl.AppImage",
        "qgroundcontrol.AppImage",
        "QGC.AppImage"
    };
    
    for (const QString &path : searchPaths) {
        for (const QString &name : possibleNames) {
            QString fullPath = QDir(path).absoluteFilePath(name);
            QFileInfo fileInfo(fullPath);
            if (fileInfo.exists() && fileInfo.isExecutable()) {
                qDebug() << "Found QGroundControl at:" << fullPath;
                return fullPath;
            }
        }
    }
    
    qDebug() << "QGroundControl.AppImage not found in search paths";
    return QString();
}

bool ProcessManager::checkROSEnvironment()
{
    // Check if ROS_DISTRO is set
    QString rosDistro = qgetenv("ROS_DISTRO");
    if (rosDistro.isEmpty()) {
        qDebug() << "ROS_DISTRO environment variable not set";
        return false;
    }
    
    // Check if roscore command is available
    QProcess testProcess;
    testProcess.start("which", QStringList() << "roscore");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() != 0) {
        qDebug() << "roscore command not found";
        return false;
    }
    
    // Check if rviz command is available
    testProcess.start("which", QStringList() << "rviz");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() != 0) {
        qDebug() << "rviz command not found";
        return false;
    }
    
    return true;
}

bool ProcessManager::isRoscoreReady()
{
    QProcess testProcess;
    testProcess.start("rostopic", QStringList() << "list");
    testProcess.waitForFinished(3000);
    
    return testProcess.exitCode() == 0;
}

void ProcessManager::startRVIZProcess()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("ROS_MASTER_URI", "http://localhost:11311");
    env.insert("DISPLAY", qgetenv("DISPLAY"));
    
    m_rvizProcess->setProcessEnvironment(env);
    m_rvizProcess->start("rviz");
    
    if (!m_rvizProcess->waitForStarted(5000)) {
        setRVIZState(Error);
        emit processError("RVIZ", "Failed to start RVIZ");
        return;
    }
    
    setRVIZState(Running);
    emit processStarted("RVIZ", m_rvizProcess->processId());
    m_rvizRestartCount = 0;
    
    qDebug() << "RVIZ started with PID:" << m_rvizProcess->processId();
}

void ProcessManager::setQGCState(ProcessState state)
{
    if (m_qgcState != state) {
        m_qgcState = state;
        emit qgcStateChanged(state);
    }
}

void ProcessManager::setRVIZState(ProcessState state)
{
    if (m_rvizState != state) {
        m_rvizState = state;
        emit rvizStateChanged(state);
    }
}

void ProcessManager::killProcess(QProcess *process)
{
    if (!process || process->state() == QProcess::NotRunning) {
        return;
    }
    
    // Try graceful termination first
    process->terminate();
    
    if (!process->waitForFinished(5000)) {
        // Force kill if graceful termination fails
        process->kill();
        process->waitForFinished(3000);
    }
}