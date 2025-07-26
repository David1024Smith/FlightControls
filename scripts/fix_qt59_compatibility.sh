#!/bin/bash

# Qt 5.9兼容性修复脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Qt 5.9兼容性修复工具${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查Qt版本
check_qt_version() {
    if pkg-config --exists Qt5Core; then
        QT_VERSION=$(pkg-config --modversion Qt5Core)
        echo -e "${BLUE}检测到Qt版本: $QT_VERSION${NC}"
        
        if [[ "$QT_VERSION" < "5.12.0" ]]; then
            echo -e "${YELLOW}需要应用Qt 5.9兼容性修复${NC}"
            return 0
        else
            echo -e "${GREEN}Qt版本足够新，无需兼容性修复${NC}"
            return 1
        fi
    else
        echo -e "${RED}未检测到Qt5${NC}"
        exit 1
    fi
}

# 创建兼容性补丁
create_compatibility_patch() {
    echo -e "${YELLOW}创建Qt 5.9兼容性补丁...${NC}"
    
    # 创建临时的兼容性头文件
    cat > "$PROJECT_ROOT/src/qt59_compat.h" << 'EOF'
#ifndef QT59_COMPAT_H
#define QT59_COMPAT_H

#include <QtGlobal>
#include <QLoggingCategory>

// Qt 5.9兼容性处理
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)

// 简化的日志宏，避免QLoggingCategory参数问题
#define qDebugCompat() qDebug()
#define qWarningCompat() qWarning()
#define qCriticalCompat() qCritical()

// QProcess信号连接兼容性
#include <QProcess>
namespace Qt59Compat {
    template<typename Receiver, typename Func>
    inline void connectProcessFinished(QProcess* process, Receiver* receiver, Func func) {
        QObject::connect(process, 
                       static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                       receiver, func);
    }
}

#else

// Qt 5.12+版本使用正常的日志
#define qDebugCompat() qDebug()
#define qWarningCompat() qWarning()
#define qCriticalCompat() qCritical()

namespace Qt59Compat {
    template<typename Receiver, typename Func>
    inline void connectProcessFinished(QProcess* process, Receiver* receiver, Func func) {
        QObject::connect(process, 
                       QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                       receiver, func);
    }
}

#endif

#endif // QT59_COMPAT_H
EOF
    
    echo -e "${GREEN}✓ 兼容性头文件创建完成${NC}"
}

# 修复main.cpp
fix_main_cpp() {
    echo -e "${YELLOW}修复main.cpp...${NC}"
    
    # 备份原文件
    cp "$PROJECT_ROOT/src/main.cpp" "$PROJECT_ROOT/src/main.cpp.backup"
    
    # 替换日志调用
    sed -i 's/qDebugCat(launcher)/qDebugCompat()/g' "$PROJECT_ROOT/src/main.cpp"
    sed -i 's/qWarningCat(launcher)/qWarningCompat()/g' "$PROJECT_ROOT/src/main.cpp"
    sed -i 's/qCriticalCat(launcher)/qCriticalCompat()/g' "$PROJECT_ROOT/src/main.cpp"
    
    # 添加兼容性头文件包含
    sed -i '1i#include "qt59_compat.h"' "$PROJECT_ROOT/src/main.cpp"
    
    echo -e "${GREEN}✓ main.cpp修复完成${NC}"
}

# 修复FlightControlsLauncher.cpp
fix_launcher_cpp() {
    echo -e "${YELLOW}修复FlightControlsLauncher.cpp...${NC}"
    
    # 备份原文件
    cp "$PROJECT_ROOT/src/FlightControlsLauncher.cpp" "$PROJECT_ROOT/src/FlightControlsLauncher.cpp.backup"
    
    # 替换QOverload调用
    sed -i 's/QtCompat::connectProcessFinished/Qt59Compat::connectProcessFinished/g' "$PROJECT_ROOT/src/FlightControlsLauncher.cpp"
    
    # 添加兼容性头文件包含
    sed -i 's/#include "qt_compatibility.h"/#include "qt59_compat.h"/g' "$PROJECT_ROOT/src/FlightControlsLauncher.cpp"
    
    echo -e "${GREEN}✓ FlightControlsLauncher.cpp修复完成${NC}"
}

# 测试编译
test_compilation() {
    echo -e "${YELLOW}测试编译...${NC}"
    
    cd "$PROJECT_ROOT"
    rm -rf build_test
    mkdir build_test
    cd build_test
    
    # 使用宽松的编译选项
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="-Wno-error -Wno-reorder -Wno-unused-parameter" \
          ..
    
    if make -j$(nproc); then
        echo -e "${GREEN}✅ 编译测试成功！${NC}"
        return 0
    else
        echo -e "${RED}❌ 编译测试失败${NC}"
        return 1
    fi
}

# 恢复备份
restore_backup() {
    echo -e "${YELLOW}恢复备份文件...${NC}"
    
    if [ -f "$PROJECT_ROOT/src/main.cpp.backup" ]; then
        mv "$PROJECT_ROOT/src/main.cpp.backup" "$PROJECT_ROOT/src/main.cpp"
    fi
    
    if [ -f "$PROJECT_ROOT/src/FlightControlsLauncher.cpp.backup" ]; then
        mv "$PROJECT_ROOT/src/FlightControlsLauncher.cpp.backup" "$PROJECT_ROOT/src/FlightControlsLauncher.cpp"
    fi
    
    rm -f "$PROJECT_ROOT/src/qt59_compat.h"
    
    echo -e "${GREEN}✓ 备份恢复完成${NC}"
}

# 主函数
main() {
    cd "$PROJECT_ROOT"
    
    if check_qt_version; then
        create_compatibility_patch
        fix_main_cpp
        fix_launcher_cpp
        
        if test_compilation; then
            echo -e "${GREEN}========================================${NC}"
            echo -e "${GREEN}✅ Qt 5.9兼容性修复成功！${NC}"
            echo -e "${GREEN}========================================${NC}"
            echo -e "${YELLOW}现在可以运行:${NC}"
            echo "cd build_test && ./flight_controls_launcher"
        else
            echo -e "${RED}编译失败，恢复原始文件...${NC}"
            restore_backup
            exit 1
        fi
    else
        echo -e "${BLUE}无需兼容性修复${NC}"
    fi
}

# 显示帮助
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Qt 5.9兼容性修复脚本"
    echo
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  --restore      恢复备份文件"
    echo
    echo "此脚本将自动检测Qt版本并应用必要的兼容性修复"
    exit 0
fi

# 恢复选项
if [ "$1" = "--restore" ]; then
    restore_backup
    exit 0
fi

main "$@"