#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QTimer> // Added for QTimer::singleShot

#include "../src/TabBasedLauncher.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序属性
    app.setApplicationName("TabBasedLauncher");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("FlightControls");
    app.setOrganizationDomain("flightcontrols.example.com");
    
    // 检查X11环境（Linux）
#ifdef Q_OS_LINUX
    if (qgetenv("DISPLAY").isEmpty()) {
        QMessageBox::critical(nullptr, "错误", 
            "未检测到X11显示环境\n"
            "请确保：\n"
            "1. 在图形界面环境中运行\n"
            "2. DISPLAY环境变量已设置\n"
            "3. 具有X11显示权限");
        return 1;
    }
#endif
    
    // 创建数据目录
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    
    qDebug() << "TabBasedLauncher启动...";
    qDebug() << "数据目录:" << dataDir;
    qDebug() << "Qt版本:" << qVersion();
    
    // 检查WebEngine支持
#ifdef QT_WEBENGINEWIDGETS_LIB
    qDebug() << "WebEngine支持: 启用";
#else
    qDebug() << "WebEngine支持: 禁用（降级模式）";
#endif
    
    // 创建并显示主窗口
    TabBasedLauncher launcher;
    
    // 检查命令行参数
    QStringList args = app.arguments();
    if (args.contains("--help") || args.contains("-h")) {
        QMessageBox::information(nullptr, "帮助", 
            "TabBasedLauncher v2.0.0\n"
            "飞行控制应用程序标签式启动器\n\n"
            "使用方法:\n"
            "  tab_launcher_demo [选项]\n\n"
            "选项:\n"
            "  -h, --help     显示此帮助信息\n"
            "  --version      显示版本信息\n"
            "  --fullscreen   全屏模式启动\n\n"
            "功能:\n"
            "• QGroundControl集成\n"
            "• RVIZ集成\n"
            "• 应用状态监控\n"
            "• 统一日志查看\n"
            "• Web界面支持（如果可用）");
        return 0;
    }
    
    if (args.contains("--version")) {
        QMessageBox::information(nullptr, "版本信息",
            "TabBasedLauncher v2.0.0\n"
            "构建日期: " __DATE__ "\n"
            "Qt版本: " + QString(qVersion()) + "\n"
            "FlightControls替代方案项目");
        return 0;
    }
    
    // 全屏模式
    if (args.contains("--fullscreen")) {
        launcher.showFullScreen();
    } else {
        launcher.show();
    }
    
    // 显示欢迎信息
    QTimer::singleShot(1000, [&launcher]() {
        // 这里可以添加启动后的初始化操作
        qDebug() << "TabBasedLauncher就绪";
    });
    
    return app.exec();
} 