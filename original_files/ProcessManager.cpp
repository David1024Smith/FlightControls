#include "ProcessManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QDateTime>

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
    
    setRVIZState(Starting);
    
    // 使用系统环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    // 确保设置了 ROS_MASTER_URI
    if (!env.contains("ROS_MASTER_URI")) {
        env.insert("ROS_MASTER_URI", "http://localhost:11311");
    }
    
    // 启动 roscore
    qDebug() << "Starting roscore...";
    m_roscoreProcess->setProcessEnvironment(env);
    m_roscoreProcess->start("roscore");
    
    if (!m_roscoreProcess->waitForStarted(5000)) {
        setRVIZState(Error);
        emit processError("RVIZ", "Failed to start roscore. Please make sure ROS is properly installed.");
        return false;
    }
    
    qDebug() << "Roscore started with PID:" << m_roscoreProcess->processId();
    qDebug() << "Waiting for roscore to be ready...";
    
    // 启动定时器检查 roscore 是否就绪
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
    static int attempts = 0;
    attempts++;
    
    if (isRoscoreReady()) {
        qDebug() << "Roscore is ready after" << attempts << "attempts, starting RVIZ now";
        attempts = 0; // 重置尝试次数
        startRVIZProcess();
    } else {
        if (attempts > 30) { // 最多尝试30次，约30秒
            qDebug() << "Giving up waiting for roscore after" << attempts << "attempts";
            setRVIZState(Error);
            emit processError("RVIZ", "Roscore did not become ready in time. Please check your ROS installation.");
            attempts = 0; // 重置尝试次数
            return;
        }
        
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
    // 检查 ROS 环境变量
    QString rosDistro = qgetenv("ROS_DISTRO");
    if (rosDistro.isEmpty()) {
        qDebug() << "ROS_DISTRO environment variable not set, trying to find ROS setup file";
        
        // 尝试查找并自动设置 ROS 环境
        QStringList possibleSetupFiles = {
            "/opt/ros/noetic/setup.bash",
            "/opt/ros/melodic/setup.bash",
            "/opt/ros/kinetic/setup.bash",
            QDir::homePath() + "/catkin_ws/devel/setup.bash"
        };
        
        bool foundSetup = false;
        for (const QString &setupFile : possibleSetupFiles) {
            QFileInfo fileInfo(setupFile);
            if (fileInfo.exists()) {
                qDebug() << "Found ROS setup file at:" << setupFile;
                
                // 设置 ROS 环境变量
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                
                // 使用 bash 获取 ROS 环境变量
                QProcess sourceProcess;
                sourceProcess.setProcessChannelMode(QProcess::MergedChannels);
                
                QString command = QString("bash -c \"source %1 && echo ROS_DISTRO=$ROS_DISTRO\"").arg(setupFile);
                sourceProcess.start("bash", QStringList() << "-c" << command);
                sourceProcess.waitForFinished(5000);
                
                QString output = sourceProcess.readAllStandardOutput();
                if (output.contains("ROS_DISTRO=")) {
                    rosDistro = output.split("=").at(1).trimmed();
                    qputenv("ROS_DISTRO", rosDistro.toUtf8());
                    qDebug() << "Set ROS_DISTRO to:" << rosDistro;
                    foundSetup = true;
                    break;
                }
            }
        }
        
        if (!foundSetup) {
            qDebug() << "Could not find ROS setup file";
            return false;
        }
    }
    
    // 检查 roscore 命令是否可用
    QProcess testProcess;
    testProcess.start("which", QStringList() << "roscore");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() != 0) {
        qDebug() << "roscore command not found, trying to find it in common locations";
        
        QStringList possibleRoscore = {
            "/opt/ros/" + rosDistro + "/bin/roscore",
            "/usr/bin/roscore",
            "/usr/local/bin/roscore"
        };
        
        bool foundRoscore = false;
        for (const QString &roscorePath : possibleRoscore) {
            QFileInfo fileInfo(roscorePath);
            if (fileInfo.exists() && fileInfo.isExecutable()) {
                qDebug() << "Found roscore at:" << roscorePath;
                foundRoscore = true;
                break;
            }
        }
        
        if (!foundRoscore) {
            qDebug() << "roscore command not found";
            return false;
        }
    }
    
    // 检查 rviz 命令是否可用
    testProcess.start("which", QStringList() << "rviz");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() != 0) {
        qDebug() << "rviz command not found, trying to find it in common locations";
        
        QStringList possibleRviz = {
            "/opt/ros/" + rosDistro + "/bin/rviz",
            "/usr/bin/rviz",
            "/usr/local/bin/rviz"
        };
        
        bool foundRviz = false;
        for (const QString &rvizPath : possibleRviz) {
            QFileInfo fileInfo(rvizPath);
            if (fileInfo.exists() && fileInfo.isExecutable()) {
                qDebug() << "Found rviz at:" << rvizPath;
                foundRviz = true;
                break;
            }
        }
        
        if (!foundRviz) {
            qDebug() << "rviz command not found";
            return false;
        }
    }
    
    qDebug() << "ROS environment check passed";
    return true;
}

bool ProcessManager::isRoscoreReady()
{
    qDebug() << "Checking if roscore is ready...";
    
    // 尝试使用 rostopic list 命令检查 roscore 是否就绪
    QProcess testProcess;
    testProcess.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    testProcess.start("bash", QStringList() << "-c" << "rostopic list");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() == 0) {
        qDebug() << "Roscore is ready (verified with rostopic list)!";
        return true;
    }
    
    // 如果 rostopic list 失败，尝试使用 rosnode list 命令
    testProcess.start("bash", QStringList() << "-c" << "rosnode list");
    testProcess.waitForFinished(3000);
    
    if (testProcess.exitCode() == 0) {
        qDebug() << "Roscore is ready (verified with rosnode list)!";
        return true;
    }
    
    // 如果以上方法都失败，尝试检查 roscore 进程是否在运行
    if (m_roscoreProcess->state() == QProcess::Running) {
        // 使用进程启动时间来计算运行时间
        static QDateTime roscoreStartTime;
        static bool timeInitialized = false;
        
        if (!timeInitialized) {
            roscoreStartTime = QDateTime::currentDateTime();
            timeInitialized = true;
        }
        
        int elapsedSecs = roscoreStartTime.secsTo(QDateTime::currentDateTime());
        
        if (elapsedSecs > 8) {
            qDebug() << "Assuming roscore is ready after" << elapsedSecs << "seconds of running";
            return true;
        }
        
        qDebug() << "Roscore is running for" << elapsedSecs << "seconds, waiting for it to be ready...";
    } else {
        qDebug() << "Roscore process is not running!";
        // 如果进程不在运行，重置计时器
        static bool timeInitialized = false;
        timeInitialized = false;
    }
    
    return false;
}

void ProcessManager::startRVIZProcess()
{
    qDebug() << "Starting RVIZ...";
    
    // 使用系统环境变量
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    // 确保设置了必要的环境变量
    if (!env.contains("ROS_MASTER_URI")) {
        env.insert("ROS_MASTER_URI", "http://localhost:11311");
    }
    env.insert("DISPLAY", qgetenv("DISPLAY"));
    
    m_rvizProcess->setProcessEnvironment(env);
    
    // 获取 ROS_DISTRO
    QString rosDistro = qgetenv("ROS_DISTRO");
    if (rosDistro.isEmpty()) {
        // 尝试从系统中获取 ROS_DISTRO
        QProcess rosVersionProcess;
        rosVersionProcess.start("bash", QStringList() << "-c" << "rosversion -d");
        rosVersionProcess.waitForFinished(3000);
        rosDistro = rosVersionProcess.readAllStandardOutput().trimmed();
        
        if (!rosDistro.isEmpty()) {
            qDebug() << "Detected ROS_DISTRO:" << rosDistro;
        }
    }
    
    // 尝试查找 setup.bash 文件
    QString setupFile;
    QStringList possibleSetupFiles = {
        "/opt/ros/" + rosDistro + "/setup.bash",
        "/opt/ros/noetic/setup.bash",
        "/opt/ros/melodic/setup.bash",
        "/opt/ros/kinetic/setup.bash",
        QDir::homePath() + "/catkin_ws/devel/setup.bash"
    };
    
    for (const QString &file : possibleSetupFiles) {
        if (QFileInfo(file).exists()) {
            setupFile = file;
            qDebug() << "Found ROS setup file:" << setupFile;
            break;
        }
    }
    
    // 使用 bash 启动 RVIZ，确保 ROS 环境变量被正确设置
    if (!setupFile.isEmpty()) {
        qDebug() << "Starting RVIZ with ROS setup file:" << setupFile;
        QString command = QString("source %1 && rosrun rviz rviz").arg(setupFile);
        m_rvizProcess->start("bash", QStringList() << "-c" << command);
    } else {
        // 直接启动 RVIZ
        qDebug() << "Starting RVIZ directly with rosrun";
        m_rvizProcess->start("bash", QStringList() << "-c" << "rosrun rviz rviz");
    }
    
    if (!m_rvizProcess->waitForStarted(5000)) {
        qDebug() << "Failed to start RVIZ with rosrun, trying direct command";
        
        // 尝试直接启动 rviz 命令
        m_rvizProcess->start("rviz");
        
        if (!m_rvizProcess->waitForStarted(5000)) {
            setRVIZState(Error);
            emit processError("RVIZ", "Failed to start RVIZ. Please make sure ROS and RVIZ are properly installed.");
            return;
        }
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