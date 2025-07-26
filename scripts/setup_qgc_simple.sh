#!/bin/bash

# 简单的QGroundControl设置脚本

set -e

echo "========================================="
echo "QGroundControl 快速设置"
echo "========================================="

# 检查是否已经存在QGroundControl
check_existing_qgc() {
    local locations=(
        "$HOME/QGroundControl.AppImage"
        "/usr/local/bin/QGroundControl.AppImage"
        "/opt/QGroundControl/QGroundControl.AppImage"
        "$(pwd)/QGroundControl.AppImage"
        "./QGroundControl.AppImage"
    )
    
    for location in "${locations[@]}"; do
        if [ -f "$location" ] && [ -x "$location" ]; then
            echo "✓ 找到QGroundControl: $location"
            return 0
        fi
    done
    
    return 1
}

if check_existing_qgc; then
    echo "QGroundControl已安装，可以直接使用FlightControls启动器"
    echo "运行: flight_controls_launcher"
else
    echo "未找到QGroundControl，提供以下选项:"
    echo
    echo "选项1: 自动下载安装（推荐）"
    echo "  ./scripts/install_qgroundcontrol.sh"
    echo
    echo "选项2: 手动下载"
    echo "  1. 访问: https://github.com/mavlink/qgroundcontrol/releases"
    echo "  2. 下载最新的QGroundControl.AppImage"
    echo "  3. 将文件放置到以下位置之一:"
    echo "     - $HOME/QGroundControl.AppImage"
    echo "     - /usr/local/bin/QGroundControl.AppImage"
    echo "     - $(pwd)/QGroundControl.AppImage"
    echo "  4. 添加执行权限: chmod +x QGroundControl.AppImage"
    echo
    
    read -p "是否现在自动下载安装QGroundControl? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        ./scripts/install_qgroundcontrol.sh
    else
        echo "请手动安装QGroundControl后再使用FlightControls启动器"
    fi
fi