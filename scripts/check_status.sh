#!/bin/bash

# 检查项目状态脚本

echo "========================================="
echo "FlightControls 项目状态检查"
echo "========================================="

# 检查系统环境
echo "系统环境:"
echo "- 操作系统: $(lsb_release -d 2>/dev/null | cut -f2 || echo '未知')"
echo "- Qt版本: $(pkg-config --modversion Qt5Core 2>/dev/null || echo '未安装')"
echo "- CMake版本: $(cmake --version 2>/dev/null | head -1 | cut -d' ' -f3 || echo '未安装')"
echo "- GCC版本: $(gcc --version 2>/dev/null | head -1 | cut -d' ' -f4 || echo '未安装')"
echo

# 检查项目文件
echo "项目文件:"
echo "- CMakeLists.txt: $([ -f CMakeLists.txt ] && echo '✓' || echo '✗')"
echo "- src/main.cpp: $([ -f src/main.cpp ] && echo '✓' || echo '✗')"
echo "- src/FlightControlsLauncher.cpp: $([ -f src/FlightControlsLauncher.cpp ] && echo '✓' || echo '✗')"
echo "- src/FlightControlsLauncher.h: $([ -f src/FlightControlsLauncher.h ] && echo '✓' || echo '✗')"
echo

# 检查构建状态
echo "构建状态:"
if [ -d "build" ]; then
    echo "- 构建目录: ✓ 存在"
    
    EXECUTABLE=$(find build -name "flight_controls_launcher" -type f 2>/dev/null | head -1)
    if [ -n "$EXECUTABLE" ]; then
        echo "- 可执行文件: ✓ $EXECUTABLE"
        echo "- 文件大小: $(ls -lh "$EXECUTABLE" 2>/dev/null | awk '{print $5}' || echo '未知')"
        echo "- 文件权限: $(ls -l "$EXECUTABLE" 2>/dev/null | awk '{print $1}' || echo '未知')"
    else
        echo "- 可执行文件: ✗ 未找到"
    fi
else
    echo "- 构建目录: ✗ 不存在"
fi
echo

# 检查打包状态
echo "打包状态:"
if [ -d "package" ]; then
    echo "- 打包目录: ✓ 存在"
    
    DEB_FILE=$(find package -name "*.deb" -type f 2>/dev/null | head -1)
    if [ -n "$DEB_FILE" ]; then
        echo "- DEB包: ✓ $DEB_FILE"
        echo "- 包大小: $(ls -lh "$DEB_FILE" 2>/dev/null | awk '{print $5}' || echo '未知')"
    else
        echo "- DEB包: ✗ 未找到"
    fi
else
    echo "- 打包目录: ✗ 不存在"
fi
echo

# 检查脚本权限
echo "脚本权限:"
for script in scripts/*.sh; do
    if [ -f "$script" ]; then
        if [ -x "$script" ]; then
            echo "- $(basename "$script"): ✓ 可执行"
        else
            echo "- $(basename "$script"): ✗ 不可执行"
        fi
    fi
done
echo

# 建议的下一步操作
echo "建议的下一步操作:"

if [ ! -d "build" ] || [ -z "$(find build -name "flight_controls_launcher" -type f 2>/dev/null)" ]; then
    echo "1. 编译项目:"
    echo "   mkdir build && cd build"
    echo "   cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS='-Wno-reorder -Wno-unused-parameter' .."
    echo "   make -j\$(nproc)"
    echo
elif [ ! -d "package" ] || [ -z "$(find package -name "*.deb" -type f 2>/dev/null)" ]; then
    echo "1. 创建DEB包:"
    echo "   ./scripts/create_deb_package.sh"
    echo
else
    echo "1. 安装DEB包:"
    DEB_FILE=$(find package -name "*.deb" -type f 2>/dev/null | head -1)
    echo "   sudo dpkg -i $DEB_FILE"
    echo "   sudo apt-get install -f"
    echo
fi

echo "2. 或者使用一键脚本:"
echo "   ./scripts/build_and_package.sh"
echo

echo "========================================="