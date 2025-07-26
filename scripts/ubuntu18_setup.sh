#!/bin/bash

# Ubuntu 18.04 专用设置脚本
# 解决Qt版本兼容性和依赖问题

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Ubuntu 18.04 FlightControls 环境设置${NC}"
echo -e "${BLUE}========================================${NC}"

# 检测系统版本
check_ubuntu_version() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        if [[ "$NAME" == *"Ubuntu"* && "$VERSION_ID" == "18.04" ]]; then
            echo -e "${GREEN}✓ 检测到Ubuntu 18.04${NC}"
            return 0
        else
            echo -e "${YELLOW}⚠️ 检测到系统: $NAME $VERSION_ID${NC}"
            echo -e "${YELLOW}此脚本专为Ubuntu 18.04优化，但也可能适用于其他版本${NC}"
            read -p "是否继续? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
        fi
    else
        echo -e "${RED}无法检测系统版本${NC}"
        exit 1
    fi
}

# 更新包管理器
update_package_manager() {
    echo -e "${YELLOW}更新包管理器...${NC}"
    sudo apt-get update
    echo -e "${GREEN}✓ 包管理器更新完成${NC}"
}

# 安装基础构建工具
install_build_tools() {
    echo -e "${YELLOW}安装基础构建工具...${NC}"
    
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        dpkg-dev \
        fakeroot
    
    echo -e "${GREEN}✓ 基础构建工具安装完成${NC}"
}

# 安装Qt5开发包
install_qt5() {
    echo -e "${YELLOW}安装Qt5开发包...${NC}"
    
    # Ubuntu 18.04默认Qt版本是5.9.5，完全满足要求
    sudo apt-get install -y \
        qt5-default \
        qtbase5-dev \
        qtbase5-dev-tools \
        libqt5core5a \
        libqt5gui5 \
        libqt5widgets5
    
    # 检查Qt版本
    QT_VERSION=$(pkg-config --modversion Qt5Core 2>/dev/null || echo "未知")
    echo -e "${BLUE}安装的Qt版本: $QT_VERSION${NC}"
    
    if [[ "$QT_VERSION" < "5.9.0" ]]; then
        echo -e "${RED}✗ Qt版本过低，需要5.9.0或更高版本${NC}"
        exit 1
    else
        echo -e "${GREEN}✓ Qt版本满足要求${NC}"
    fi
}

# 安装X11开发库
install_x11() {
    echo -e "${YELLOW}安装X11开发库...${NC}"
    
    sudo apt-get install -y \
        libx11-dev \
        libx11-6 \
        x11-utils \
        xorg-dev
    
    echo -e "${GREEN}✓ X11开发库安装完成${NC}"
}

# 安装窗口管理工具
install_window_tools() {
    echo -e "${YELLOW}安装窗口管理工具...${NC}"
    
    sudo apt-get install -y \
        wmctrl \
        xdotool \
        xwininfo
    
    echo -e "${GREEN}✓ 窗口管理工具安装完成${NC}"
}

# 安装可选的ROS支持
install_ros_support() {
    echo -e "${YELLOW}检查ROS支持...${NC}"
    
    # 检查是否已安装ROS
    if command -v roscore >/dev/null 2>&1; then
        echo -e "${GREEN}✓ 检测到ROS环境${NC}"
        ROS_VERSION=$(rosversion -d 2>/dev/null || echo "未知")
        echo -e "${BLUE}ROS版本: $ROS_VERSION${NC}"
    else
        echo -e "${YELLOW}未检测到ROS环境${NC}"
        echo -e "${YELLOW}如需使用RVIZ功能，请安装ROS:${NC}"
        echo "  # 安装ROS Melodic (Ubuntu 18.04推荐)"
        echo "  sudo sh -c 'echo \"deb http://packages.ros.org/ros/ubuntu \$(lsb_release -sc) main\" > /etc/apt/sources.list.d/ros-latest.list'"
        echo "  sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654"
        echo "  sudo apt-get update"
        echo "  sudo apt-get install ros-melodic-desktop-full"
        echo "  echo \"source /opt/ros/melodic/setup.bash\" >> ~/.bashrc"
        echo
        
        read -p "是否现在安装ROS Melodic? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_ros_melodic
        fi
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
    
    # 初始化rosdep
    if ! command -v rosdep >/dev/null 2>&1; then
        sudo apt-get install -y python-rosdep
        sudo rosdep init || true
    fi
    rosdep update || true
    
    # 设置环境变量
    if ! grep -q "source /opt/ros/melodic/setup.bash" ~/.bashrc; then
        echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc
    fi
    
    echo -e "${GREEN}✓ ROS Melodic安装完成${NC}"
    echo -e "${YELLOW}请重新打开终端或运行: source ~/.bashrc${NC}"
}

# 验证安装
verify_installation() {
    echo -e "${YELLOW}验证安装...${NC}"
    
    local all_good=true
    
    # 检查CMake
    if command -v cmake >/dev/null 2>&1; then
        CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
        echo -e "${GREEN}✓ CMake: $CMAKE_VERSION${NC}"
    else
        echo -e "${RED}✗ CMake未安装${NC}"
        all_good=false
    fi
    
    # 检查编译器
    if command -v g++ >/dev/null 2>&1; then
        GCC_VERSION=$(g++ --version | head -n1 | cut -d' ' -f4)
        echo -e "${GREEN}✓ G++: $GCC_VERSION${NC}"
    else
        echo -e "${RED}✗ G++未安装${NC}"
        all_good=false
    fi
    
    # 检查Qt5
    if pkg-config --exists Qt5Core; then
        QT_VERSION=$(pkg-config --modversion Qt5Core)
        echo -e "${GREEN}✓ Qt5: $QT_VERSION${NC}"
    else
        echo -e "${RED}✗ Qt5开发包未正确安装${NC}"
        all_good=false
    fi
    
    # 检查X11
    if pkg-config --exists x11; then
        X11_VERSION=$(pkg-config --modversion x11)
        echo -e "${GREEN}✓ X11: $X11_VERSION${NC}"
    else
        echo -e "${RED}✗ X11开发库未正确安装${NC}"
        all_good=false
    fi
    
    # 检查窗口管理工具
    for tool in wmctrl xdotool; do
        if command -v $tool >/dev/null 2>&1; then
            echo -e "${GREEN}✓ $tool: 已安装${NC}"
        else
            echo -e "${YELLOW}⚠️ $tool: 未安装（可选）${NC}"
        fi
    done
    
    if [ "$all_good" = true ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}✅ 所有必需组件安装成功！${NC}"
        echo -e "${GREEN}========================================${NC}"
        return 0
    else
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}❌ 部分组件安装失败${NC}"
        echo -e "${RED}========================================${NC}"
        return 1
    fi
}

# 显示下一步说明
show_next_steps() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}下一步操作${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo
    echo -e "${YELLOW}1. 编译项目:${NC}"
    echo "   mkdir build && cd build"
    echo "   cmake .."
    echo "   make"
    echo
    echo -e "${YELLOW}2. 运行程序:${NC}"
    echo "   ./flight_controls_launcher"
    echo
    echo -e "${YELLOW}3. 创建安装包:${NC}"
    echo "   chmod +x scripts/*.sh"
    echo "   ./scripts/create_deb_package.sh"
    echo
    echo -e "${YELLOW}4. 安装DEB包:${NC}"
    echo "   sudo dpkg -i package/flight-controls-launcher_5.0.0_amd64.deb"
    echo
    echo -e "${GREEN}环境设置完成！${NC}"
}

# 主函数
main() {
    check_ubuntu_version
    update_package_manager
    install_build_tools
    install_qt5
    install_x11
    install_window_tools
    install_ros_support
    
    if verify_installation; then
        show_next_steps
    else
        echo -e "${RED}请检查错误信息并重新运行脚本${NC}"
        exit 1
    fi
}

# 显示帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Ubuntu 18.04 FlightControls 环境设置脚本"
    echo
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  -h, --help    显示此帮助信息"
    echo "  --no-ros      跳过ROS安装"
    echo
    echo "此脚本将自动安装以下组件:"
    echo "  - 基础构建工具 (build-essential, cmake)"
    echo "  - Qt5开发包 (qt5-default, qtbase5-dev)"
    echo "  - X11开发库 (libx11-dev)"
    echo "  - 窗口管理工具 (wmctrl, xdotool)"
    echo "  - ROS Melodic (可选)"
    exit 0
fi

# 检查--no-ros参数
if [ "$1" = "--no-ros" ]; then
    install_ros_support() {
        echo -e "${YELLOW}跳过ROS安装${NC}"
    }
fi

main "$@"