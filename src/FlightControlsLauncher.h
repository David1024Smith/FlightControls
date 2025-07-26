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

private:
    void setupUI();
    void setupButtons();
    void positionWindow();
    
    // 应用程序管理
    void startApplication(const QString &appName, const QString &command, const QStringList &args = QStringList());
    void stopApplication(const QString &appName);
    bool isApplicationRunning(const QString &appName) const;
    
    // QGC特殊处理
    QString findQGroundControlPath();
    
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
    };
    
    QMap<QString, AppProcess> m_applications;
    QTimer *m_statusTimer;
    
    // 窗口拖拽
    bool m_dragging;
    QPoint m_dragPosition;
    
    // 样式设置
    void applyStyles();
};

#endif // FLIGHTCONTROLSLAUNCHER_H 