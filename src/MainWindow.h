#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QStatusBar>
#include <QTimer>
#include <QButtonGroup>
#include <QMenuBar>
#include <QAction>

class ProcessManager;
class WindowEmbedder;
class ApplicationSwitcher;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void switchToQGC();
    void switchToRVIZ();
    void updateUI();
    void onApplicationSwitched(const QString &appName);
    void onStatusChanged(const QString &status);
    void onPreloadProgress(int progress);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupToolbar();
    void setupContentArea();
    void setupStatusBar();
    void centerWindow();
    void applyButtonStyles();

    // UI Components
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QWidget *m_toolbarWidget;
    QHBoxLayout *m_toolbarLayout;
    QLabel *m_titleLabel;
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QLabel *m_statusLabel;
    QButtonGroup *m_buttonGroup;
    
    QStackedLayout *m_stackedLayout;
    QWidget *m_qgcPage;
    QWidget *m_rvizPage;
    QWidget *m_qgcContainer;
    QWidget *m_rvizContainer;
    
    // Menu and Actions
    QAction *m_qgcAction;
    QAction *m_rvizAction;
    QAction *m_exitAction;
    
    // Core modules
    ProcessManager *m_processManager;
    WindowEmbedder *m_windowEmbedder;
    ApplicationSwitcher *m_applicationSwitcher;
    
    // State management
    QTimer *m_uiUpdateTimer;
    QString m_currentApp;
    bool m_preloadInProgress;
};

#endif // MAINWINDOW_H