#!/bin/bash

# 安装后设置脚本 - 配置QGroundControl和RVIZ

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls 安装后设置${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查FlightControls启动器是否已安装
check_launcher_installed() {
    echo -e "${YELLOW}检查FlightControls启动器...${NC}"
    
    if command -v flight_controls_launcher >/dev/null 2>&1; then
        echo -e "${GREEN}✓ FlightControls启动器已安装${NC}"
        
        # 显示安装信息
        LAUNCHER_PATH=$(which flight_controls_launcher)
        echo -e "${BLUE}位置: $LAUNCHER_PATH${NC}"
        
        # 检查桌面快捷方式
        if [ -f "/usr/share/applications/flight-controls-launcher.desktop" ]; then
            echo -e "${GREEN}✓ 桌面快捷方式已创建${NC}"
        fi
    else
        echo -e "${RED}✗ FlightControls启动器未安装${NC}"
        echo "请先安装DEB包:"
        echo "sudo dpkg -i package/flight-controls-launcher_*.deb"
        exit 1
    fi
}

# 设置QGroundControl
setup_qgroundcontrol() {
    echo -e "${YELLOW}设置QGroundControl...${NC}"
    
    # 检查是否已存在
    local qgc_locations=(
        "$HOME/QGroundControl.AppImage"
        "/usr/local/bin/QGroundControl.AppImage"
        "/opt/QGroundControl/QGroundControl.AppImage"
    )
    
    local found_qgc=""
    for location in "${qgc_locations[@]}"; do
        if [ -f "$location" ] && [ -x "$location" ]; then
            found_qgc="$location"
            break
        fi
    done
    
    if [ -n "$found_qgc" ]; then
        echo -e "${GREEN}✓ QGroundControl已安装: $found_qgc${NC}"
    else
        echo -e "${YELLOW}⚠️ 未找到QGroundControl${NC}"
        echo "选择安装方式:"
        echo "1) 自动下载安装（推荐）"
        echo "2) 手动安装说明"
        echo "3) 跳过QGroundControl设置"
        
        read -p "请选择 (1-3): " choice
        
        case $choice in
            1)
                if [ -f "scripts/install_qgroundcontrol.sh" ]; then
                    chmod +x scripts/install_qgroundcontrol.sh
                    ./scripts/install_qgroundcontrol.sh
                else
                    echo -e "${RED}安装脚本不存在${NC}"
                fi
                ;;
            2)
                show_manual_qgc_install
                ;;
            3)
                echo -e "${YELLOW}跳过QGroundControl设置${NC}"
                ;;
            *)
                echo -e "${RED}无效选择${NC}"
                ;;
        esac
    fi
}

# 显示手动安装说明
show_manual_qgc_install() {
    echo -e "${BLUE}QGroundControl手动安装说明:${NC}"
    echo "1. 访问: https://github.com/mavlink/qgroundcontrol/releases"
    echo "2. 下载最新的QGroundControl.AppImage"
    echo "3. 将文件保存到: $HOME/QGroundControl.AppImage"
    echo "4. 添加执行权限: chmod +x $HOME/QGroundControl.AppImage"
    echo "5. 运行修复脚本: ./scripts/fix_qgc_path.sh"
}

# 设置ROS和RVIZ
setup_rviz() {
    echo -e "${YELLOW}设置ROS和RVIZ...${NC}"
    
    # 检查ROS是否已安装
    if command -v roscore >/dev/null 2>&1; then
        local ros_version=$(rosversion -d 2>/dev/null || echo "未知")
        echo -e "${GREEN}✓ ROS已安装: $ros_version${NC}"
        
        # 检查RVIZ
        if command -v rviz >/dev/null 2>&1; then
            echo -e "${GREEN}✓ RVIZ已安装${NC}"
        else
            echo -e "${YELLOW}⚠️ RVIZ未安装${NC}"
            echo "安装RVIZ:"
            echo "sudo apt-get install ros-$ros_version-rviz"
        fi
    else
        echo -e "${YELLOW}⚠️ 未检测到ROS环境${NC}"
        echo "选择ROS安装方式:"
        echo "1) 安装ROS Melodic（Ubuntu 18.04推荐）"
        echo "2) 显示手动安装说明"
        echo "3) 跳过ROS设置"
        
        read -p "请选择 (1-3): " choice
        
        case $choice in
            1)
                install_ros_melodic
                ;;
            2)
                show_manual_ros_install
                ;;
            3)
                echo -e "${YELLOW}跳过ROS设置${NC}"
                ;;
            *)
                echo -e "${RED}无效选择${NC}"
                ;;
        esac
    fi
}

# 安装ROS Melodic
install_ros_melodic() {
    echo -e "${YELLOW}安装ROS Melodic...${NC}"
    
    # 添加ROS软件源
    sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
    
    # 添加密钥
    sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
    
    # 更新包列表
    sudo apt-get update
    
    # 安装ROS
    sudo apt-get install -y ros-melodic-desktop-full
    
    # 设置环境变量
    if ! grep -q "source /opt/ros/melodic/setup.bash" ~/.bashrc; then
        echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc
    fi
    
    echo -e "${GREEN}✓ ROS Melodic安装完成${NC}"
    echo -e "${YELLOW}请重新打开终端或运行: source ~/.bashrc${NC}"
}

# 显示ROS手动安装说明
show_manual_ros_install() {
    echo -e "${BLUE}ROS手动安装说明:${NC}"
    echo "Ubuntu 18.04 - ROS Melodic:"
    echo "1. sudo sh -c 'echo \"deb http://packages.ros.org/ros/ubuntu \$(lsb_release -sc) main\" > /etc/apt/sources.list.d/ros-latest.list'"
    echo "2. sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654"
    echo "3. sudo apt-get update"
    echo "4. sudo apt-get install ros-melodic-desktop-full"
    echo "5. echo \"source /opt/ros/melodic/setup.bash\" >> ~/.bashrc"
    echo "6. source ~/.bashrc"
}

# 测试启动器
test_launcher() {
    echo -e "${YELLOW}测试FlightControls启动器...${NC}"
    
    echo -e "${BLUE}启动启动器进行测试（5秒后自动退出）...${NC}"
    timeout 5s flight_controls_launcher >/dev/null 2>&1 || true
    
    echo -e "${GREEN}✓ 启动器测试完成${NC}"
}

# 显示完成信息
show_completion_info() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}🎉 安装后设置完成！${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    echo -e "${YELLOW}启动方法:${NC}"
    echo "1. 命令行: flight_controls_launcher"
    echo "2. 桌面快捷方式: 在应用程序菜单中找到 'FlightControls Launcher'"
    echo "3. Alt+F2 然后输入: flight_controls_launcher"
    echo
    
    echo -e "${YELLOW}功能说明:${NC}"
    echo "• 🚁 启动 QGC: 启动QGroundControl地面控制站"
    echo "• 🤖 启动 RVIZ: 启动ROS可视化工具"
    echo "• 悬浮置顶: 启动器始终显示在屏幕顶部"
    echo "• 智能管理: 自动检测和管理应用程序状态"
    echo
    
    echo -e "${YELLOW}故障排除:${NC}"
    echo "• 如果QGC按钮无效: ./scripts/fix_qgc_path.sh"
    echo "• 如果RVIZ按钮无效: 检查ROS环境是否正确配置"
    echo "• 查看日志: 启动器会在终端显示详细信息"
    echo
    
    echo -e "${GREEN}现在可以开始使用FlightControls启动器了！${NC}"
}

# 主函数
main() {
    check_launcher_installed
    setup_qgroundcontrol
    setup_rviz
    test_launcher
    show_completion_info
}

# 显示帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "FlightControls安装后设置脚本"
    echo
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  --qgc-only     仅设置QGroundControl"
    echo "  --ros-only     仅设置ROS/RVIZ"
    echo "  --skip-test    跳过测试"
    echo
    echo "此脚本将配置QGroundControl和ROS/RVIZ以供FlightControls启动器使用"
    exit 0
fi

# 处理命令行参数
case "$1" in
    --qgc-only)
        check_launcher_installed
        setup_qgroundcontrol
        show_completion_info
        ;;
    --ros-only)
        check_launcher_installed
        setup_rviz
        show_completion_info
        ;;
    --skip-test)
        check_launcher_installed
        setup_qgroundcontrol
        setup_rviz
        show_completion_info
        ;;
    *)
        main
        ;;
esac