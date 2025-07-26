#!/bin/bash

# FlightControls Launcher 快速安装脚本
# 支持Ubuntu/Debian和CentOS/RHEL系统

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 检测系统类型
detect_system() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
    elif type lsb_release >/dev/null 2>&1; then
        OS=$(lsb_release -si)
        VER=$(lsb_release -sr)
    else
        OS=$(uname -s)
        VER=$(uname -r)
    fi
    
    echo -e "${BLUE}检测到系统: $OS $VER${NC}"
}

# 安装依赖 - Ubuntu/Debian
install_deps_debian() {
    echo -e "${YELLOW}安装Ubuntu/Debian依赖...${NC}"
    
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        qt5-default \
        qtbase5-dev \
        libx11-dev \
        wmctrl \
        xdotool \
        x11-utils \
        dpkg-dev \
        fakeroot
    
    echo -e "${GREEN}✓ 依赖安装完成${NC}"
}

# 安装依赖 - CentOS/RHEL
install_deps_centos() {
    echo -e "${YELLOW}安装CentOS/RHEL依赖...${NC}"
    
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y \
        cmake3 \
        qt5-qtbase-devel \
        libX11-devel \
        wmctrl \
        xdotool \
        rpm-build \
        rpmdevtools
    
    # 创建cmake符号链接
    if [ ! -f /usr/bin/cmake ] && [ -f /usr/bin/cmake3 ]; then
        sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake
    fi
    
    echo -e "${GREEN}✓ 依赖安装完成${NC}"
}

# 编译和安装
build_and_install() {
    echo -e "${YELLOW}编译和安装FlightControls Launcher...${NC}"
    
    # 创建构建目录
    mkdir -p build
    cd build
    
    # 配置
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          ..
    
    # 编译
    make -j$(nproc)
    
    # 安装
    sudo make install
    
    echo -e "${GREEN}✓ 编译和安装完成${NC}"
}

# 创建桌面文件
create_desktop_entry() {
    echo -e "${YELLOW}创建桌面快捷方式...${NC}"
    
    sudo mkdir -p /usr/share/applications
    
    sudo tee /usr/share/applications/flight-controls-launcher.desktop > /dev/null << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=FlightControls Launcher
Name[zh_CN]=飞行控制启动器
Comment=Flight Controls Floating Launcher
Comment[zh_CN]=基于Qt的飞行控制应用程序浮动启动器
Exec=/usr/local/bin/flight_controls_launcher
Terminal=false
Categories=Development;Engineering;
Keywords=flight;controls;launcher;qgroundcontrol;rviz;ros;
StartupNotify=true
EOF
    
    # 更新桌面数据库
    if command -v update-desktop-database >/dev/null 2>&1; then
        sudo update-desktop-database /usr/share/applications
    fi
    
    echo -e "${GREEN}✓ 桌面快捷方式创建完成${NC}"
}

# 主安装流程
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}FlightControls Launcher 安装程序${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    detect_system
    
    case "$OS" in
        *Ubuntu*|*Debian*)
            install_deps_debian
            ;;
        *CentOS*|*Red\ Hat*)
            install_deps_centos
            ;;
        *)
            echo -e "${RED}不支持的系统: $OS${NC}"
            echo -e "${YELLOW}请手动安装以下依赖:${NC}"
            echo "- CMake 3.10+"
            echo "- Qt5 开发包"
            echo "- X11 开发包"
            echo "- C++ 编译器"
            read -p "是否继续安装? (y/N): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Yy]$ ]]; then
                exit 1
            fi
            ;;
    esac
    
    build_and_install
    create_desktop_entry
    
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}安装完成！${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "${YELLOW}启动命令:${NC}"
    echo "/usr/local/bin/flight_controls_launcher"
    echo -e "${YELLOW}或从应用程序菜单中找到 'FlightControls Launcher'${NC}"
}

main "$@"