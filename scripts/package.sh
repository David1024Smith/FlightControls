#!/bin/bash

# FlightControls Launcher 统一打包脚本
# 自动检测系统并选择合适的打包方式

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls Launcher 统一打包工具${NC}"
echo -e "${BLUE}========================================${NC}"

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

# 显示菜单
show_menu() {
    echo -e "${YELLOW}请选择打包方式:${NC}"
    echo "1) DEB包 (Ubuntu/Debian)"
    echo "2) RPM包 (CentOS/RHEL/Fedora)"
    echo "3) 直接安装 (通用)"
    echo "4) 自动检测系统并打包"
    echo "5) 退出"
    echo
}

# 自动选择打包方式
auto_select() {
    case "$OS" in
        *Ubuntu*|*Debian*)
            echo -e "${GREEN}自动选择: DEB包${NC}"
            create_deb_package
            ;;
        *CentOS*|*Red\ Hat*|*Fedora*)
            echo -e "${GREEN}自动选择: RPM包${NC}"
            create_rpm_package
            ;;
        *)
            echo -e "${YELLOW}未知系统，使用直接安装方式${NC}"
            direct_install
            ;;
    esac
}

# 创建DEB包
create_deb_package() {
    echo -e "${YELLOW}创建DEB包...${NC}"
    if [ -f "$SCRIPT_DIR/create_deb_package.sh" ]; then
        bash "$SCRIPT_DIR/create_deb_package.sh"
    else
        echo -e "${RED}错误: create_deb_package.sh 不存在${NC}"
        exit 1
    fi
}

# 创建RPM包
create_rpm_package() {
    echo -e "${YELLOW}创建RPM包...${NC}"
    if [ -f "$SCRIPT_DIR/create_rpm_package.sh" ]; then
        bash "$SCRIPT_DIR/create_rpm_package.sh"
    else
        echo -e "${RED}错误: create_rpm_package.sh 不存在${NC}"
        exit 1
    fi
}

# 直接安装
direct_install() {
    echo -e "${YELLOW}直接安装...${NC}"
    if [ -f "$SCRIPT_DIR/install.sh" ]; then
        bash "$SCRIPT_DIR/install.sh"
    else
        echo -e "${RED}错误: install.sh 不存在${NC}"
        exit 1
    fi
}

# 主函数
main() {
    detect_system
    
    # 如果有命令行参数，直接执行
    case "$1" in
        deb|--deb)
            create_deb_package
            exit 0
            ;;
        rpm|--rpm)
            create_rpm_package
            exit 0
            ;;
        install|--install)
            direct_install
            exit 0
            ;;
        auto|--auto)
            auto_select
            exit 0
            ;;
    esac
    
    # 交互式菜单
    while true; do
        show_menu
        read -p "请输入选择 (1-5): " choice
        
        case $choice in
            1)
                create_deb_package
                break
                ;;
            2)
                create_rpm_package
                break
                ;;
            3)
                direct_install
                break
                ;;
            4)
                auto_select
                break
                ;;
            5)
                echo -e "${BLUE}退出${NC}"
                exit 0
                ;;
            *)
                echo -e "${RED}无效选择，请重新输入${NC}"
                ;;
        esac
    done
}

# 显示帮助信息
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "FlightControls Launcher 打包工具"
    echo
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  deb, --deb        创建DEB包"
    echo "  rpm, --rpm        创建RPM包"
    echo "  install, --install 直接安装"
    echo "  auto, --auto      自动检测系统并打包"
    echo "  -h, --help        显示此帮助信息"
    echo
    echo "示例:"
    echo "  $0 deb           # 创建DEB包"
    echo "  $0 auto          # 自动检测并打包"
    echo "  $0               # 显示交互式菜单"
    exit 0
fi

main "$@"