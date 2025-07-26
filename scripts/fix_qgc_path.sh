#!/bin/bash

# 修复QGroundControl路径问题

set -e

echo "========================================="
echo "修复QGroundControl路径问题"
echo "========================================="

# 检查FlightControls启动器是否已安装
if ! command -v flight_controls_launcher >/dev/null 2>&1; then
    echo "❌ FlightControls启动器未安装"
    echo "请先安装DEB包: sudo dpkg -i package/flight-controls-launcher_*.deb"
    exit 1
fi

echo "✓ FlightControls启动器已安装"

# 查找现有的QGroundControl
find_existing_qgc() {
    local search_paths=(
        "$HOME/QGroundControl.AppImage"
        "$HOME/Applications/QGroundControl.AppImage"
        "$HOME/Downloads/QGroundControl.AppImage"
        "/usr/local/bin/QGroundControl.AppImage"
        "/opt/QGroundControl/QGroundControl.AppImage"
        "$(pwd)/QGroundControl.AppImage"
        "./QGroundControl.AppImage"
    )
    
    echo "搜索现有的QGroundControl..."
    for path in "${search_paths[@]}"; do
        if [ -f "$path" ]; then
            echo "找到: $path"
            if [ -x "$path" ]; then
                echo "✓ 文件可执行"
                return 0
            else
                echo "⚠️ 文件不可执行，正在修复权限..."
                chmod +x "$path"
                echo "✓ 权限已修复"
                return 0
            fi
        fi
    done
    
    return 1
}

# 创建符号链接到标准位置
create_standard_links() {
    local source_file="$1"
    local target_locations=(
        "$HOME/QGroundControl.AppImage"
        "/usr/local/bin/QGroundControl.AppImage"
    )
    
    echo "创建标准位置的符号链接..."
    for target in "${target_locations[@]}"; do
        if [ ! -e "$target" ]; then
            if [[ "$target" == /usr/* ]]; then
                sudo ln -sf "$source_file" "$target" 2>/dev/null && echo "✓ 创建: $target" || echo "⚠️ 无法创建: $target"
            else
                ln -sf "$source_file" "$target" && echo "✓ 创建: $target"
            fi
        else
            echo "✓ 已存在: $target"
        fi
    done
}

# 主逻辑
if find_existing_qgc; then
    # 找到现有文件，创建符号链接
    existing_qgc=$(find "$HOME" /usr/local/bin /opt -name "QGroundControl.AppImage" -type f -executable 2>/dev/null | head -1)
    if [ -n "$existing_qgc" ]; then
        echo "使用现有文件: $existing_qgc"
        create_standard_links "$existing_qgc"
        
        echo "========================================="
        echo "✅ QGroundControl路径问题已修复！"
        echo "========================================="
        echo "现在可以启动FlightControls启动器:"
        echo "flight_controls_launcher"
    fi
else
    echo "❌ 未找到QGroundControl.AppImage文件"
    echo
    echo "解决方案:"
    echo "1. 自动下载安装（推荐）:"
    echo "   ./scripts/install_qgroundcontrol.sh"
    echo
    echo "2. 手动下载:"
    echo "   - 访问: https://github.com/mavlink/qgroundcontrol/releases"
    echo "   - 下载QGroundControl.AppImage"
    echo "   - 保存到: $HOME/QGroundControl.AppImage"
    echo "   - 添加执行权限: chmod +x $HOME/QGroundControl.AppImage"
    echo
    
    read -p "是否现在自动下载安装? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if [ -f "scripts/install_qgroundcontrol.sh" ]; then
            chmod +x scripts/install_qgroundcontrol.sh
            ./scripts/install_qgroundcontrol.sh
        else
            echo "❌ 安装脚本不存在"
            exit 1
        fi
    fi
fi