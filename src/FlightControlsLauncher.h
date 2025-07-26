#ifndef FLIGHTCONTROLSLAUNCHER_H
#define FLIGHTCONTROLSLAUNCHER_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProcess>
#include <QMap>
#include <QTimer>
#include <QMouseEvent>
#include <QPoint>
#include <QThread>

// X11前置声明（避免头文件冲突）
#ifdef Q_OS_LINUX
typedef struct _XDisplay Display;
#endif

/**
 * @brief 飞行控制应用程序浮动启动器
 * 
 * 提供一个始终置顶的浮动窗口，包含QGroundControl和RVIZ的启动按钮
 * 悬浮显示在屏幕顶部居中位置，提供统一的程序启动界面
 */
class FlightControlsLauncher : public QWidget
{
    Q_OBJECT

public:
    explicit FlightControlsLauncher(QWidget *parent = nullptr);
    ~FlightControlsLauncher();

    // 配置常量
    static constexpr int LAUNCHER_WIDTH = 360;
    static constexpr int LAUNCHER_HEIGHT = 100;
    static constexpr int TOP_OFFSET = 50;
    static constexpr int STATUS_UPDATE_INTERVAL = 2000; // 毫秒
    static constexpr int PROCESS_KILL_TIMEOUT = 3000;   // 毫秒
    static constexpr int WINDOW_SEARCH_DELAY = 5000;    // 窗口搜索延迟（增加到5秒）
    static constexpr int WINDOW_SEARCH_RETRY_DELAY = 3000; // 重试延迟（增加到3秒）
    static constexpr int WINDOW_SEARCH_MAX_RETRIES = 5;    // 最大重试次数（增加到5次）
    static constexpr int RVIZ_EXTRA_DELAY = 5000;         // RVIZ额外延迟（增加到5秒）

protected:
    // 鼠标事件处理（用于拖拽移动窗口）
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onLaunchQGC();
    void onLaunchRVIZ();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void updateStatus();
    void onCloseButtonClicked();  // 关闭按钮槽函数
    void findAndMaximizeWindows(); // 查找并最大化窗口
    void retryWindowSearch();     // 重试窗口搜索

private:
    void setupUI();
    void setupButtons();
    void positionWindow();
    
    // 应用程序管理
    void startApplication(const QString &appName, const QString &command, const QStringList &args = QStringList());
    void stopApplication(const QString &appName);
    void stopAllApplications();  // 停止所有应用程序
    bool isApplicationRunning(const QString &appName) const;
    
    // QGC特殊处理
    QString findQGroundControlPath();
    
    // 窗口管理
    void maximizeAndRaiseWindow(const QString &appName);
    unsigned long findWindowByTitle(const QString &titlePattern);
    void setWindowMaximized(unsigned long windowId);
    void raiseWindow(unsigned long windowId);
    
    // UI组件
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QHBoxLayout *m_statusLayout;
    
    QPushButton *m_qgcButton;
    QPushButton *m_rvizButton;
    QPushButton *m_closeButton;
    QLabel *m_statusLabel;
    
    // 应用程序进程管理
    struct AppProcess {
        QString name;
        QString command;
        QStringList arguments;
        QProcess *process;
        bool isRunning;
        QString windowTitlePattern;  // 窗口标题匹配模式
    };
    
    QMap<QString, AppProcess> m_applications;
    QTimer *m_statusTimer;
    QTimer *m_windowSearchTimer;  // 窗口搜索定时器
    QTimer *m_retryTimer;         // 重试定时器
    int m_searchRetryCount;       // 当前重试次数
    
    // 窗口拖拽
    bool m_dragging;
    QPoint m_dragPosition;
    
    // X11显示连接（使用前置声明）
#ifdef Q_OS_LINUX
    Display *m_display;
#endif
    
    // 样式设置
    void applyStyles();
};

#endif // FLIGHTCONTROLSLAUNCHER_H 