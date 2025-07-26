#!/bin/bash

# 测试DEB包创建脚本

set -e

echo "========================================="
echo "测试DEB包创建"
echo "========================================="

# 检查构建目录
echo "检查构建目录..."
if [ -d "build" ]; then
    echo "构建目录存在"
    echo "构建目录内容:"
    find build -name "flight_controls_launcher" -type f
    
    # 直接测试可执行文件
    EXECUTABLE=$(find build -name "flight_controls_launcher" -type f | head -1)
    if [ -n "$EXECUTABLE" ]; then
        echo "找到可执行文件: $EXECUTABLE"
        echo "文件信息:"
        ls -la "$EXECUTABLE"
        
        echo "测试运行（2秒后退出）:"
        timeout 2s "$EXECUTABLE" 2>&1 || echo "程序已退出"
    else
        echo "❌ 未找到可执行文件"
        exit 1
    fi
else
    echo "❌ 构建目录不存在，请先运行编译"
    exit 1
fi

echo "========================================="
echo "开始创建DEB包..."
echo "========================================="

# 运行DEB包创建脚本
./scripts/create_deb_package.sh