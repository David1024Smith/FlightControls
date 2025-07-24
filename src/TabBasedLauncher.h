#ifndef TABBASEDLAUNCHER_H
#define TABBASEDLAUNCHER_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QProcess>
#include <QMap>
#include <QTextEdit>

// 条件包含QWebEngineView - 如果不可用则使用QTextEdit替代
#ifdef QT_WEBENGINEWIDGETS_LIB
#include <QWebEngineView>
#endif

class ApplicationTab;

class TabBasedLauncher : public QMainWindow
{
    Q_OBJECT

public:
    explicit TabBasedLauncher(QWidget *parent = nullptr);
    ~TabBasedLauncher();

private slots:
    void onTabChanged(int index);
    void onLaunchQGC();
    void onLaunchRVIZ();
    void onStopApplication(const QString &appName);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void updateApplicationStatus();

private:
    void setupUI();
    void setupApplicationTabs();
    void setupControlBar();
    
    // 应用程序管理
    void registerApplication(const QString &appName, const QString &command, const QStringList &args = QStringList());
    bool startApplication(const QString &appName);
    void stopApplication(const QString &appName);
    bool isApplicationRunning(const QString &appName) const;
    
    // UI组件
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_controlLayout;
    QTabWidget *m_tabWidget;
    
    // 控制栏
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QPushButton *m_stopButton;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    // 应用程序Tab页
    ApplicationTab *m_qgcTab;
    ApplicationTab *m_rvizTab;
    ApplicationTab *m_dashboardTab;
    
    // 应用程序管理
    struct AppInfo {
        QString name;
        QString command;
        QStringList arguments;
        QProcess *process;
        ApplicationTab *tab;
        bool isRunning;
    };
    
    QMap<QString, AppInfo> m_applications;
    QTimer *m_statusTimer;
    QString m_currentApp;
};

class ApplicationTab : public QWidget
{
    Q_OBJECT

public:
    explicit ApplicationTab(const QString &appName, QWidget *parent = nullptr);
    ~ApplicationTab();
    
    void setApplicationInfo(const QString &info);
    void setApplicationStatus(const QString &status);
    void addLogMessage(const QString &message);
    void showApplicationOutput(const QString &output);
    void showWebInterface(const QString &url);
    void setProgressValue(int value);
    
signals:
    void launchRequested(const QString &appName);
    void stopRequested(const QString &appName);
    void refreshRequested(const QString &appName);

private slots:
    void onLaunchClicked();
    void onStopClicked();
    void onRefreshClicked();
    void onClearLogClicked();

private:
    void setupUI();
    void setupInfoSection();
    void setupControlSection();
    void setupContentSection();
    
    QString m_appName;
    
    // UI组件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_controlLayout;
    
    // 信息区域
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    
    // 控制区域
    QPushButton *m_launchButton;
    QPushButton *m_stopButton;
    QPushButton *m_refreshButton;
    QPushButton *m_clearLogButton;
    
    // 内容区域
    QTabWidget *m_contentTabWidget;
    QTextEdit *m_outputText;
    QTextEdit *m_logText;
#ifdef QT_WEBENGINEWIDGETS_LIB
    QWebEngineView *m_webView;
#else
    QTextEdit *m_webView;  // 如果没有WebEngine，使用QTextEdit作为替代
#endif
    
    // 状态
    bool m_isApplicationRunning;
};

#endif // TABBASEDLAUNCHER_H 