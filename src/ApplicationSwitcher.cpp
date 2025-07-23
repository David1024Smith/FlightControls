#include "ApplicationSwitcher.h"
#include "ProcessManager.h"
#include "WindowEmbedder.h"
#include "X11Helper.h"
#include <QDebug>

ApplicationSwitcher::ApplicationSwitcher(ProcessManager *processManager, 
                                       WindowEmbedder *windowEmbedder,
                                       QObject *parent)
    : QObject(parent)
    , m_processManager(processManager)
    , m_windowEmbedder(windowEmbedder)
    , m_qgcContainer(nullptr)
    , m_rvizContainer(nullptr)
    , m_currentState(Idle)
    , m_currentApp("QGC")
    , m_preloadComplete(false)
    , m_preloadProgress(0)
    , m_preloadTimer(nullptr)
    , m_preloadProgressTimer(nullptr)
    , m_switchTimer(nullptr)
    , m_fadeAnimation(nullptr)
    , m_opacityEffect(nullptr)
{
    // Connect ProcessManager signals
    connect(m_processManager, &ProcessManager::processStarted,
            this, &ApplicationSwitcher::onProcessStarted);
    connect(m_processManager, &ProcessManager::processStopped,
            this, &ApplicationSwitcher::onProcessStopped);
    connect(m_processManager, &ProcessManager::processError,
            this, &ApplicationSwitcher::onProcessError);
    
    // Connect WindowEmbedder signals
    connect(m_windowEmbedder, &WindowEmbedder::windowFound,
            this, &ApplicationSwitcher::onWindowFound);
    connect(m_windowEmbedder, &WindowEmbedder::windowLost,
            this, &ApplicationSwitcher::onWindowLost);
    connect(m_windowEmbedder, &WindowEmbedder::embedSuccess,
            this, &ApplicationSwitcher::onEmbedSuccess);
    connect(m_windowEmbedder, &WindowEmbedder::embedFailed,
            this, &ApplicationSwitcher::onEmbedFailed);
    
    // Initialize timers
    m_preloadTimer = new QTimer(this);
    m_preloadTimer->setSingleShot(true);
    m_preloadTimer->setInterval(PRELOAD_TIMEOUT);
    connect(m_preloadTimer, &QTimer::timeout, this, &ApplicationSwitcher::onPreloadTimeout);
    
    m_preloadProgressTimer = new QTimer(this);
    m_preloadProgressTimer->setInterval(PRELOAD_UPDATE_INTERVAL);
    connect(m_preloadProgressTimer, &QTimer::timeout, this, &ApplicationSwitcher::updatePreloadProgress);
    
    m_switchTimer = new QTimer(this);
    m_switchTimer->setSingleShot(true);
    m_switchTimer->setInterval(SWITCH_TIMEOUT);
    connect(m_switchTimer, &QTimer::timeout, this, &ApplicationSwitcher::onSwitchTimeout);
    
    // Initialize animation
    m_fadeAnimation = new QPropertyAnimation(this);
    m_fadeAnimation->setDuration(ANIMATION_DURATION);
    m_opacityEffect = new QGraphicsOpacityEffect(this);
}

ApplicationSwitcher::~ApplicationSwitcher()
{
}

void ApplicationSwitcher::setContainers(QWidget *qgcContainer, QWidget *rvizContainer)
{
    m_qgcContainer = qgcContainer;
    m_rvizContainer = rvizContainer;
    
    // Initially show QGC container
    if (m_qgcContainer) {
        m_qgcContainer->show();
    }
    if (m_rvizContainer) {
        m_rvizContainer->hide();
    }
}

bool ApplicationSwitcher::switchToApplication(const QString &appName)
{
    if (m_currentState != Idle) {
        qDebug() << "Cannot switch - current state:" << m_currentState;
        return false;
    }
    
    if (m_currentApp == appName) {
        qDebug() << "Already showing" << appName;
        return true;
    }
    
    if (!m_preloadComplete) {
        qDebug() << "Cannot switch - preload not complete";
        m_pendingApp = appName;
        return false;
    }
    
    setState(Switching);
    m_switchingToApp = appName;
    m_switchTimer->start();
    
    performSwitch(appName);
    return true;
}

void ApplicationSwitcher::startPreload()
{
    if (m_currentState == PreloadInProgress) {
        return;
    }
    
    setState(PreloadInProgress);
    m_preloadProgress = 0;
    m_preloadTimer->start();
    m_preloadProgressTimer->start();
    
    startPreloadProcess();
}

void ApplicationSwitcher::onProcessStarted(const QString &appName, qint64 pid)
{
    qDebug() << "Process started:" << appName << "PID:" << pid;
    
    // Find and embed the window
    QTimer::singleShot(2000, this, [this, appName, pid]() {
        Window windowId = 0;
        
        if (appName == "QGC") {
            windowId = m_windowEmbedder->findQGCWindow(pid);
            if (windowId && m_qgcContainer) {
                m_windowEmbedder->embedWindow(windowId, m_qgcContainer);
            }
        } else if (appName == "RVIZ") {
            windowId = m_windowEmbedder->findRVIZWindow(pid);
            if (windowId && m_rvizContainer) {
                m_windowEmbedder->embedWindow(windowId, m_rvizContainer);
            }
        }
        
        if (windowId == 0) {
            qDebug() << "Could not find window for" << appName;
        }
    });
}

void ApplicationSwitcher::onProcessStopped(const QString &appName)
{
    qDebug() << "Process stopped:" << appName;
    
    if (m_currentState == Switching && m_switchingToApp == appName) {
        handleSwitchError(appName, "Process stopped during switch");
    }
}

void ApplicationSwitcher::onProcessError(const QString &appName, const QString &error)
{
    qDebug() << "Process error:" << appName << error;
    
    if (m_currentState == Switching && m_switchingToApp == appName) {
        handleSwitchError(appName, error);
    }
    
    emit switchFailed(appName, error);
}

void ApplicationSwitcher::onWindowFound(Window windowId, const QString &appName)
{
    qDebug() << "Window found:" << windowId << "for" << appName;
}

void ApplicationSwitcher::onWindowLost(Window windowId, const QString &appName)
{
    qDebug() << "Window lost:" << windowId << "for" << appName;
    
    if (m_currentApp == appName) {
        // Try to restart the application
        if (appName == "QGC") {
            m_processManager->startQGC();
        } else if (appName == "RVIZ") {
            m_processManager->startRVIZ();
        }
    }
}

void ApplicationSwitcher::onEmbedSuccess(Window windowId, const QString &appName)
{
    qDebug() << "Embed success:" << windowId << "for" << appName;
    
    if (m_currentState == Switching && m_switchingToApp == appName) {
        completeSwitchOperation(appName);
    }
    
    // Hide window if it's not the current app
    if (appName != m_currentApp) {
        m_windowEmbedder->hideEmbeddedWindow(windowId);
    }
}

void ApplicationSwitcher::onEmbedFailed(Window windowId, const QString &error)
{
    qDebug() << "Embed failed:" << windowId << error;
    
    if (m_currentState == Switching) {
        handleSwitchError(m_switchingToApp, error);
    }
}

void ApplicationSwitcher::onSwitchTimeout()
{
    if (m_currentState == Switching) {
        handleSwitchError(m_switchingToApp, "Switch operation timed out");
    }
}

void ApplicationSwitcher::onPreloadTimeout()
{
    if (m_currentState == PreloadInProgress) {
        qDebug() << "Preload timed out";
        completePreload();
    }
}

void ApplicationSwitcher::updatePreloadProgress()
{
    if (m_currentState != PreloadInProgress) {
        return;
    }
    
    checkPreloadStatus();
}

void ApplicationSwitcher::setState(SwitchState state)
{
    if (m_currentState != state) {
        m_currentState = state;
        emit stateChanged(state);
    }
}

void ApplicationSwitcher::performSwitch(const QString &appName)
{
    // Ensure the target application is running
    bool appRunning = false;
    
    if (appName == "QGC") {
        appRunning = m_processManager->isQGCRunning();
        if (!appRunning) {
            m_processManager->startQGC();
        }
    } else if (appName == "RVIZ") {
        appRunning = m_processManager->isRVIZRunning();
        if (!appRunning) {
            m_processManager->startRVIZ();
        }
    }
    
    if (appRunning) {
        // Application is already running, just switch containers
        showContainer(appName);
        hideContainer(m_currentApp);
        completeSwitchOperation(appName);
    }
    // If not running, wait for onProcessStarted and onEmbedSuccess
}

void ApplicationSwitcher::showContainer(const QString &appName)
{
    QWidget *container = nullptr;
    
    if (appName == "QGC") {
        container = m_qgcContainer;
    } else if (appName == "RVIZ") {
        container = m_rvizContainer;
    }
    
    if (container) {
        container->show();
        container->raise();
        
        // Show embedded windows
        if (appName == "QGC") {
            qint64 pid = m_processManager->getQGCPid();
            if (pid > 0) {
                Window windowId = m_windowEmbedder->findQGCWindow(pid);
                if (windowId) {
                    m_windowEmbedder->showEmbeddedWindow(windowId);
                    m_windowEmbedder->syncWindowSize(windowId);
                }
            }
        } else if (appName == "RVIZ") {
            qint64 pid = m_processManager->getRVIZPid();
            if (pid > 0) {
                Window windowId = m_windowEmbedder->findRVIZWindow(pid);
                if (windowId) {
                    m_windowEmbedder->showEmbeddedWindow(windowId);
                    m_windowEmbedder->syncWindowSize(windowId);
                }
            }
        }
    }
}

void ApplicationSwitcher::hideContainer(const QString &appName)
{
    QWidget *container = nullptr;
    
    if (appName == "QGC") {
        container = m_qgcContainer;
    } else if (appName == "RVIZ") {
        container = m_rvizContainer;
    }
    
    if (container) {
        // Hide embedded windows
        if (appName == "QGC") {
            qint64 pid = m_processManager->getQGCPid();
            if (pid > 0) {
                Window windowId = m_windowEmbedder->findQGCWindow(pid);
                if (windowId) {
                    m_windowEmbedder->hideEmbeddedWindow(windowId);
                }
            }
        } else if (appName == "RVIZ") {
            qint64 pid = m_processManager->getRVIZPid();
            if (pid > 0) {
                Window windowId = m_windowEmbedder->findRVIZWindow(pid);
                if (windowId) {
                    m_windowEmbedder->hideEmbeddedWindow(windowId);
                }
            }
        }
        
        container->hide();
    }
}

void ApplicationSwitcher::startSwitchAnimation(QWidget *fromContainer, QWidget *toContainer)
{
    // Simple fade animation (placeholder for more complex animations)
    if (fromContainer && toContainer) {
        fromContainer->hide();
        toContainer->show();
    }
}

void ApplicationSwitcher::completeSwitchOperation(const QString &appName)
{
    m_switchTimer->stop();
    m_currentApp = appName;
    setState(Idle);
    
    emit applicationSwitched(appName);
    qDebug() << "Successfully switched to" << appName;
}

void ApplicationSwitcher::handleSwitchError(const QString &appName, const QString &error)
{
    m_switchTimer->stop();
    setState(Error);
    
    emit switchFailed(appName, error);
    qDebug() << "Switch failed for" << appName << ":" << error;
    
    // Reset to idle state after a delay
    QTimer::singleShot(2000, this, [this]() {
        setState(Idle);
    });
}

void ApplicationSwitcher::startPreloadProcess()
{
    qDebug() << "Starting preload process";
    
    // Start both applications
    m_processManager->startQGC();
    m_processManager->startRVIZ();
}

void ApplicationSwitcher::checkPreloadStatus()
{
    bool qgcReady = m_processManager->isQGCRunning();
    bool rvizReady = m_processManager->isRVIZRunning();
    
    // Check if windows are embedded
    bool qgcEmbedded = false;
    bool rvizEmbedded = false;
    
    if (qgcReady) {
        qint64 qgcPid = m_processManager->getQGCPid();
        Window qgcWindow = m_windowEmbedder->findQGCWindow(qgcPid);
        qgcEmbedded = m_windowEmbedder->isWindowEmbedded(qgcWindow);
    }
    
    if (rvizReady) {
        qint64 rvizPid = m_processManager->getRVIZPid();
        Window rvizWindow = m_windowEmbedder->findRVIZWindow(rvizPid);
        rvizEmbedded = m_windowEmbedder->isWindowEmbedded(rvizWindow);
    }
    
    // Calculate progress
    int progress = 0;
    if (qgcReady) progress += 25;
    if (rvizReady) progress += 25;
    if (qgcEmbedded) progress += 25;
    if (rvizEmbedded) progress += 25;
    
    if (progress != m_preloadProgress) {
        m_preloadProgress = progress;
        emit preloadProgress(progress);
    }
    
    if (progress >= 100) {
        completePreload();
    }
}

void ApplicationSwitcher::completePreload()
{
    m_preloadTimer->stop();
    m_preloadProgressTimer->stop();
    
    m_preloadComplete = true;
    m_preloadProgress = 100;
    
    setState(Idle);
    emit preloadComplete();
    
    qDebug() << "Preload completed successfully";
    
    // Hide non-current application windows
    hideContainer("RVIZ"); // Start with QGC visible
    
    // Handle pending switch
    if (!m_pendingApp.isEmpty()) {
        QString pendingApp = m_pendingApp;
        m_pendingApp.clear();
        switchToApplication(pendingApp);
    }
}