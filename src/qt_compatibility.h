#ifndef QT_COMPATIBILITY_H
#define QT_COMPATIBILITY_H

#include <QtGlobal>

// Qt版本兼容性处理
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
    #ifndef QT_LEGACY_VERSION
        #define QT_LEGACY_VERSION
    #endif
    
    // Qt 5.9兼容性定义
    #include <QProcess>
    
    // QProcess::finished信号在Qt 5.6+中有两个重载版本
    // 在Qt 5.15中，旧版本被标记为deprecated
    // 为了兼容Qt 5.9，我们使用旧版本的信号连接方式
    
    namespace QtCompat {
        // 兼容性函数，用于连接QProcess::finished信号
        template<typename Receiver, typename Func>
        inline void connectProcessFinished(QProcess* process, Receiver* receiver, Func func) {
            // Qt 5.9使用旧版本的finished信号
            QObject::connect(process, 
                           static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                           receiver, func);
        }
    }
    
#else
    // Qt 5.12+版本
    namespace QtCompat {
        template<typename Receiver, typename Func>
        inline void connectProcessFinished(QProcess* process, Receiver* receiver, Func func) {
            // Qt 5.12+使用QOverload
            QObject::connect(process, 
                           QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                           receiver, func);
        }
    }
#endif

#endif // QT_COMPATIBILITY_H