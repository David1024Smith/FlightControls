#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QLoggingCategory>
#include "FlightControlsLauncher.h"

// 启用调试日志
Q_LOGGING_CATEGORY(launcher, "flightcontrols.launcher")

bool initializeApplication()
{
    // 检查应用程序数据目录
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(appDataPath);
    if (!appDataDir.exists()) {
        if (!appDataDir.mkpath(".")) {
            qCritical(launcher) << "无法创建应用程序数据目录:" << appDataPath;
            return false;
        }
    }
    
    qDebug(launcher) << "应用程序数据目录:" << appDataPath;
    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("FlightControls Launcher");
    app.setApplicationDisplayName("飞行控制应用程序启动器");
    app.setApplicationVersion("5.0");
    app.setOrganizationName("FlightControls");
    app.setOrganizationDomain("flightcontrols.org");
    
    // 设置应用程序样式
    app.setStyle("Fusion");
    
    // 初始化日志
    qDebug(launcher) << "========================================";
    qDebug(launcher) << "启动飞行控制应用程序启动器 v5.0";
    qDebug(launcher) << "Qt版本:" << QT_VERSION_STR;
    qDebug(launcher) << "编译时间:" << __DATE__ << __TIME__;
    qDebug(launcher) << "启动器类型: 浮动启动器";
    qDebug(launcher) << "========================================";
    
    // 检查基本环境
    if (!initializeApplication()) {
        QMessageBox::critical(nullptr, "初始化错误", "应用程序初始化失败，请检查权限设置。");
        return 1;
    }
    
    try {
        // 创建浮动启动器
        FlightControlsLauncher launcher;
        
        // 设置窗口标题
        launcher.setWindowTitle("飞行控制应用程序启动器 v5.0");
        
        // 显示启动器
        launcher.show();
        
        
        int result = app.exec();
        
        return result;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("启动器初始化失败:\n%1").arg(e.what());
        qCritical(launcher) << "标准异常:" << e.what();
        QMessageBox::critical(nullptr, "启动错误", errorMsg);
        return 1;
    } catch (...) {
        QString errorMsg = "启动器初始化失败: 未知错误";
        qCritical(launcher) << "未知异常发生";
        QMessageBox::critical(nullptr, "启动错误", errorMsg);
        return 1;
    }
} 