#!/bin/bash

# QGroundControl 自动下载和安装脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}QGroundControl 自动安装脚本${NC}"
echo -e "${BLUE}========================================${NC}"

# QGroundControl下载信息
QGC_VERSION="4.3.0"
QGC_URL="https://github.com/mavlink/qgroundcontrol/releases/download/v${QGC_VERSION}/QGroundControl.AppImage"
QGC_FILENAME="QGroundControl.AppImage"

# 安装位置选项
INSTALL_LOCATIONS=(
    "$HOME/Applications"
    "/opt/QGroundControl"
    "/usr/local/bin"
    "$(pwd)"
)

# 检查网络连接
check_network() {
    echo -e "${YELLOW}检查网络连接...${NC}"
    if ping -c 1 github.com >/dev/null 2>&1; then
        echo -e "${GREEN}✓ 网络连接正常${NC}"
    else
        echo -e "${RED}✗ 网络连接失败，请检查网络设置${NC}"
        exit 1
    fi
}

# 选择安装位置
choose_install_location() {
    echo -e "${YELLOW}选择安装位置:${NC}"
    for i in "${!INSTALL_LOCATIONS[@]}"; do
        echo "$((i+1))) ${INSTALL_LOCATIONS[$i]}"
    done
    echo "$((${#INSTALL_LOCATIONS[@]}+1))) 自定义路径"
    echo
    
    read -p "请选择 (1-$((${#INSTALL_LOCATIONS[@]}+1))): " choice
    
    if [[ "$choice" -ge 1 && "$choice" -le "${#INSTALL_LOCATIONS[@]}" ]]; then
        INSTALL_DIR="${INSTALL_LOCATIONS[$((choice-1))]}"
    elif [[ "$choice" -eq "$((${#INSTALL_LOCATIONS[@]}+1))" ]]; then
        read -p "请输入自定义路径: " INSTALL_DIR
    else
        echo -e "${RED}无效选择，使用默认位置: ${INSTALL_LOCATIONS[0]}${NC}"
        INSTALL_DIR="${INSTALL_LOCATIONS[0]}"
    fi
    
    echo -e "${BLUE}选择的安装位置: $INSTALL_DIR${NC}"
}

# 创建安装目录
create_install_dir() {
    echo -e "${YELLOW}创建安装目录...${NC}"
    
    if [ ! -d "$INSTALL_DIR" ]; then
        if [[ "$INSTALL_DIR" == /opt/* ]] || [[ "$INSTALL_DIR" == /usr/* ]]; then
            sudo mkdir -p "$INSTALL_DIR"
        else
            mkdir -p "$INSTALL_DIR"
        fi
    fi
    
    echo -e "${GREEN}✓ 安装目录准备完成${NC}"
}

# 下载QGroundControl
download_qgroundcontrol() {
    echo -e "${YELLOW}下载QGroundControl v${QGC_VERSION}...${NC}"
    echo -e "${BLUE}下载地址: $QGC_URL${NC}"
    
    TEMP_FILE="/tmp/$QGC_FILENAME"
    
    # 使用wget或curl下载
    if command -v wget >/dev/null 2>&1; then
        wget -O "$TEMP_FILE" "$QGC_URL" --progress=bar:force 2>&1
    elif command -v curl >/dev/null 2>&1; then
        curl -L -o "$TEMP_FILE" "$QGC_URL" --progress-bar
    else
        echo -e "${RED}✗ 未找到wget或curl，无法下载${NC}"
        echo "请手动下载QGroundControl.AppImage并放置到以下位置之一:"
        printf '%s\n' "${INSTALL_LOCATIONS[@]}"
        exit 1
    fi
    
    # 检查下载是否成功
    if [ -f "$TEMP_FILE" ] && [ -s "$TEMP_FILE" ]; then
        echo -e "${GREEN}✓ 下载完成${NC}"
        
        # 显示文件信息
        echo -e "${BLUE}文件大小: $(ls -lh "$TEMP_FILE" | awk '{print $5}')${NC}"
    else
        echo -e "${RED}✗ 下载失败${NC}"
        exit 1
    fi
}

# 安装QGroundControl
install_qgroundcontrol() {
    echo -e "${YELLOW}安装QGroundControl...${NC}"
    
    FINAL_PATH="$INSTALL_DIR/$QGC_FILENAME"
    
    # 移动文件到安装位置
    if [[ "$INSTALL_DIR" == /opt/* ]] || [[ "$INSTALL_DIR" == /usr/* ]]; then
        sudo mv "/tmp/$QGC_FILENAME" "$FINAL_PATH"
        sudo chmod +x "$FINAL_PATH"
    else
        mv "/tmp/$QGC_FILENAME" "$FINAL_PATH"
        chmod +x "$FINAL_PATH"
    fi
    
    echo -e "${GREEN}✓ QGroundControl安装完成${NC}"
    echo -e "${BLUE}安装位置: $FINAL_PATH${NC}"
    
    # 验证安装
    if [ -x "$FINAL_PATH" ]; then
        echo -e "${GREEN}✓ 文件权限正确${NC}"
    else
        echo -e "${RED}✗ 文件权限错误${NC}"
        exit 1
    fi
}

# 创建符号链接
create_symlinks() {
    echo -e "${YELLOW}创建符号链接...${NC}"
    
    FINAL_PATH="$INSTALL_DIR/$QGC_FILENAME"
    
    # 创建到常用位置的符号链接
    SYMLINK_LOCATIONS=(
        "$HOME/QGroundControl.AppImage"
        "/usr/local/bin/QGroundControl.AppImage"
    )
    
    for link_path in "${SYMLINK_LOCATIONS[@]}"; do
        if [ ! -e "$link_path" ]; then
            if [[ "$link_path" == /usr/* ]]; then
                sudo ln -sf "$FINAL_PATH" "$link_path" 2>/dev/null || true
            else
                ln -sf "$FINAL_PATH" "$link_path" 2>/dev/null || true
            fi
            
            if [ -L "$link_path" ]; then
                echo -e "${GREEN}✓ 创建符号链接: $link_path${NC}"
            fi
        fi
    done
}

# 创建桌面快捷方式
create_desktop_entry() {
    echo -e "${YELLOW}创建桌面快捷方式...${NC}"
    
    FINAL_PATH="$INSTALL_DIR/$QGC_FILENAME"
    DESKTOP_FILE="$HOME/.local/share/applications/qgroundcontrol.desktop"
    
    mkdir -p "$(dirname "$DESKTOP_FILE")"
    
    cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=QGroundControl
Comment=Ground Control Station for Unmanned Vehicles
Exec=$FINAL_PATH
Icon=qgroundcontrol
Terminal=false
Categories=Development;Engineering;
Keywords=drone;uav;mavlink;px4;ardupilot;
StartupNotify=true
EOF
    
    chmod +x "$DESKTOP_FILE"
    
    # 更新桌面数据库
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
    fi
    
    echo -e "${GREEN}✓ 桌面快捷方式创建完成${NC}"
}

# 测试QGroundControl
test_qgroundcontrol() {
    echo -e "${YELLOW}测试QGroundControl...${NC}"
    
    FINAL_PATH="$INSTALL_DIR/$QGC_FILENAME"
    
    # 快速启动测试（3秒后自动退出）
    echo -e "${BLUE}启动QGroundControl进行测试（3秒后自动退出）...${NC}"
    timeout 3s "$FINAL_PATH" >/dev/null 2>&1 || true
    
    echo -e "${GREEN}✓ QGroundControl测试完成${NC}"
}

# 显示安装信息
show_installation_info() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}🎉 QGroundControl安装完成！${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    FINAL_PATH="$INSTALL_DIR/$QGC_FILENAME"
    
    echo -e "${YELLOW}安装信息:${NC}"
    echo "版本: v$QGC_VERSION"
    echo "位置: $FINAL_PATH"
    echo "大小: $(ls -lh "$FINAL_PATH" | awk '{print $5}')"
    echo
    
    echo -e "${YELLOW}启动方法:${NC}"
    echo "1. 命令行: $FINAL_PATH"
    echo "2. 桌面快捷方式: 在应用程序菜单中找到 'QGroundControl'"
    echo "3. FlightControls启动器: 现在可以正常使用QGC按钮"
    echo
    
    echo -e "${YELLOW}符号链接:${NC}"
    for link_path in "$HOME/QGroundControl.AppImage" "/usr/local/bin/QGroundControl.AppImage"; do
        if [ -L "$link_path" ]; then
            echo "✓ $link_path"
        fi
    done
    echo
    
    echo -e "${GREEN}现在可以启动FlightControls启动器了！${NC}"
    echo "运行: flight_controls_launcher"
}

# 主函数
main() {
    check_network
    choose_install_location
    create_install_dir
    download_qgroundcontrol
    install_qgroundcontrol
    create_symlinks
    create_desktop_entry
    test_qgroundcontrol
    show_installation_info
}

# 显示帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "QGroundControl自动安装脚本"
    echo
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  --version VER  指定版本号 (默认: $QGC_VERSION)"
    echo "  --dir PATH     指定安装目录"
    echo
    echo "此脚本将自动下载并安装QGroundControl AppImage"
    exit 0
fi

# 处理命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --version)
            QGC_VERSION="$2"
            QGC_URL="https://github.com/mavlink/qgroundcontrol/releases/download/v${QGC_VERSION}/QGroundControl.AppImage"
            shift 2
            ;;
        --dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

main "$@"