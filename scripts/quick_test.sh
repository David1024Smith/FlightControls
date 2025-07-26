#!/bin/bash

# 快速测试脚本 - 专为Ubuntu 18.04优化

set -e

echo "========================================="
echo "FlightControls 快速编译测试"
echo "========================================="

# 检查Qt版本
echo "检查Qt版本..."
QT_VERSION=$(pkg-config --modversion Qt5Core 2>/dev/null || echo "未安装")
echo "Qt版本: $QT_VERSION"

# 清理并创建构建目录
echo "准备构建环境..."
rm -rf build_quick
mkdir build_quick
cd build_quick

# 配置CMake（使用宽松的编译选项）
echo "配置CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-Wno-error -Wno-reorder -Wno-unused-parameter -std=c++17" \
      ..

# 编译
echo "开始编译..."
make -j$(nproc) 2>&1 | tee compile.log

if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo "========================================="
    echo "✅ 编译成功！"
    echo "========================================="
    
    # 查找可执行文件
    EXECUTABLE=""
    if [ -f "bin/flight_controls_launcher" ]; then
        EXECUTABLE="bin/flight_controls_launcher"
    elif [ -f "flight_controls_launcher" ]; then
        EXECUTABLE="flight_controls_launcher"
    else
        echo "查找可执行文件..."
        EXECUTABLE=$(find . -name "flight_controls_launcher" -type f | head -1)
    fi
    
    if [ -n "$EXECUTABLE" ]; then
        echo "可执行文件信息:"
        ls -la "$EXECUTABLE"
        file "$EXECUTABLE"
        
        echo "依赖库检查:"
        ldd "$EXECUTABLE" | grep -E "(Qt|X11)"
        
        echo "快速测试运行（3秒后自动退出）:"
        timeout 3s "./$EXECUTABLE" 2>&1 || echo "程序已退出"
    else
        echo "❌ 找不到可执行文件"
        exit 1
    fi
    
    echo "========================================="
    echo "测试完成！可以继续创建安装包。"
    echo "运行: ./scripts/create_deb_package.sh"
    echo "========================================="
else
    echo "========================================="
    echo "❌ 编译失败"
    echo "========================================="
    echo "错误日志:"
    tail -20 compile.log
    exit 1
fi