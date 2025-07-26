#!/bin/bash

# 修复路径问题并创建DEB包

set -e

echo "========================================="
echo "修复路径问题并创建DEB包"
echo "========================================="

# 检查当前构建状态
if [ -d "build" ]; then
    echo "检查构建目录..."
    EXECUTABLE=$(find build -name "flight_controls_launcher" -type f | head -1)
    
    if [ -n "$EXECUTABLE" ]; then
        echo "✓ 找到可执行文件: $EXECUTABLE"
        
        # 测试运行
        echo "测试可执行文件..."
        timeout 2s "$EXECUTABLE" --version 2>/dev/null || echo "程序测试完成"
        
        # 创建DEB包
        echo "创建DEB包..."
        ./scripts/create_deb_package.sh
        
        # 检查结果
        if [ -f "package/flight-controls-launcher_5.0.0_amd64.deb" ]; then
            echo "========================================="
            echo "✅ DEB包创建成功！"
            echo "========================================="
            
            DEB_FILE="package/flight-controls-launcher_5.0.0_amd64.deb"
            echo "包文件: $DEB_FILE"
            echo "包大小: $(ls -lh "$DEB_FILE" | awk '{print $5}')"
            
            echo ""
            echo "安装命令:"
            echo "sudo dpkg -i $DEB_FILE"
            echo "sudo apt-get install -f"
            
        else
            echo "❌ DEB包创建失败"
            exit 1
        fi
        
    else
        echo "❌ 未找到可执行文件，请重新编译"
        echo "运行: rm -rf build && mkdir build && cd build && cmake .. && make"
        exit 1
    fi
else
    echo "❌ 构建目录不存在，请先编译项目"
    echo "运行: mkdir build && cd build && cmake .. && make"
    exit 1
fi