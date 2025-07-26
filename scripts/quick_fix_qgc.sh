#!/bin/bash

# 快速修复QGroundControl问题

echo "========================================="
echo "快速修复QGroundControl问题"
echo "========================================="

# 方法1: 下载到用户目录
echo "方法1: 快速下载QGroundControl到用户目录"
echo "下载地址: https://github.com/mavlink/qgroundcontrol/releases/download/v4.3.0/QGroundControl.AppImage"
echo

if command -v wget >/dev/null 2>&1; then
    echo "使用wget下载..."
    wget -O ~/QGroundControl.AppImage https://github.com/mavlink/qgroundcontrol/releases/download/v4.3.0/QGroundControl.AppImage
elif command -v curl >/dev/null 2>&1; then
    echo "使用curl下载..."
    curl -L -o ~/QGroundControl.AppImage https://github.com/mavlink/qgroundcontrol/releases/download/v4.3.0/QGroundControl.AppImage
else
    echo "❌ 未找到wget或curl"
    echo "请手动下载QGroundControl.AppImage到: ~/QGroundControl.AppImage"
    exit 1
fi

# 添加执行权限
chmod +x ~/QGroundControl.AppImage

# 验证下载
if [ -f ~/QGroundControl.AppImage ] && [ -x ~/QGroundControl.AppImage ]; then
    echo "✅ QGroundControl下载成功！"
    echo "文件位置: ~/QGroundControl.AppImage"
    echo "文件大小: $(ls -lh ~/QGroundControl.AppImage | awk '{print $5}')"
    
    # 创建符号链接到标准位置
    if [ ! -e /usr/local/bin/QGroundControl.AppImage ]; then
        sudo ln -sf ~/QGroundControl.AppImage /usr/local/bin/QGroundControl.AppImage 2>/dev/null || true
    fi
    
    echo ""
    echo "现在可以启动FlightControls启动器了:"
    echo "flight_controls_launcher"
else
    echo "❌ 下载失败"
    exit 1
fi