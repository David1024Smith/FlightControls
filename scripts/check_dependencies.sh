#!/bin/bash

# FlightControls 依赖检查脚本
# 用于Ubuntu 18.04 LTS

set -e

echo "🔍 检查FlightControls项目依赖..."

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 检查结果统计
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0

check_command() {
    local cmd=$1
    local package=$2
    local install_cmd=$3
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    if command -v "$cmd" >/dev/null 2>&1; then
        echo -e "${GREEN}✅ $cmd 已安装${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        return 0
    else
        echo -e "${RED}❌ $cmd 未找到${NC}"
        if [ -n "$package" ]; then
            echo -e "${YELLOW}   建议安装: $install_cmd${NC}"
        fi
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        return 1
    fi
}

check_library() {
    local lib=$1
    local package=$2
    local install_cmd=$3
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    if pkg-config --exists "$lib" 2>/dev/null; then
        echo -e "${GREEN}✅ $lib 库已安装${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        return 0
    else
        echo -e "${RED}❌ $lib 库未找到${NC}"
        if [ -n "$package" ]; then
            echo -e "${YELLOW}   建议安装: $install_cmd${NC}"
        fi
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        return 1
    fi
}

check_qt_component() {
    local component=$1
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    if pkg-config --exists "$component" 2>/dev/null; then
        echo -e "${GREEN}✅ $component 已安装${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
        return 0
    else
        echo -e "${RED}❌ $component 未找到${NC}"
        echo -e "${YELLOW}   建议安装: sudo apt install qt5-default libqt5webengine5-dev${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
        return 1
    fi
}

echo "📋 检查系统信息..."
echo "操作系统: $(lsb_release -d | cut -f2)"
echo "内核版本: $(uname -r)"
echo "桌面环境: $XDG_CURRENT_DESKTOP"
echo ""

echo "🛠️ 检查构建工具..."
check_command "gcc" "build-essential" "sudo apt install build-essential"
check_command "g++" "build-essential" "sudo apt install build-essential"
check_command "cmake" "cmake" "sudo apt install cmake"
check_command "make" "build-essential" "sudo apt install build-essential"
check_command "pkg-config" "pkg-config" "sudo apt install pkg-config"
echo ""

echo "📦 检查Qt开发环境..."
check_qt_component "Qt5Core"
check_qt_component "Qt5Widgets"
check_qt_component "Qt5Gui"

# 检查Qt WebEngine（可选）
if pkg-config --exists "Qt5WebEngineWidgets" 2>/dev/null; then
    echo -e "${GREEN}✅ Qt5WebEngineWidgets 已安装${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${YELLOW}⚠️ Qt5WebEngineWidgets 未找到（TabBasedLauncher需要）${NC}"
    echo -e "${YELLOW}   建议安装: sudo apt install libqt5webengine5-dev${NC}"
fi
TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
echo ""

echo "🖥️ 检查X11开发库..."
check_library "x11" "libx11-dev" "sudo apt install libx11-dev"
check_library "xext" "libxext-dev" "sudo apt install libxext-dev"
echo ""

echo "🤖 检查ROS环境（RVIZ支持）..."
if [ -n "$ROS_DISTRO" ]; then
    echo -e "${GREEN}✅ ROS环境已设置: $ROS_DISTRO${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
    
    check_command "roscore" "ros-$ROS_DISTRO-core" "sudo apt install ros-$ROS_DISTRO-core"
    check_command "rviz" "ros-$ROS_DISTRO-rviz" "sudo apt install ros-$ROS_DISTRO-rviz"
else
    echo -e "${YELLOW}⚠️ ROS环境未设置${NC}"
    echo -e "${YELLOW}   对于Ubuntu 18.04建议安装ROS Melodic:${NC}"
    echo -e "${YELLOW}   wget -O - https://raw.githubusercontent.com/ros/rosdistro/master/ros.key | sudo apt-key add -${NC}"
    echo -e "${YELLOW}   sudo sh -c 'echo \"deb http://packages.ros.org/ros/ubuntu bionic main\" > /etc/apt/sources.list.d/ros-latest.list'${NC}"
    echo -e "${YELLOW}   sudo apt update && sudo apt install ros-melodic-desktop-full${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 2))
fi
TOTAL_CHECKS=$((TOTAL_CHECKS + 3))
echo ""

echo "🎮 检查桌面环境工具..."
check_command "gsettings" "glib-2.0" ""
check_command "gdbus" "glib-2.0" ""

# 可选工具
if command -v "wmctrl" >/dev/null 2>&1; then
    echo -e "${GREEN}✅ wmctrl 已安装（窗口管理工具）${NC}"
else
    echo -e "${YELLOW}ℹ️ wmctrl 未安装（可选的窗口管理工具）${NC}"
    echo -e "${YELLOW}   安装命令: sudo apt install wmctrl${NC}"
fi

if command -v "xdotool" >/dev/null 2>&1; then
    echo -e "${GREEN}✅ xdotool 已安装（X11自动化工具）${NC}"
else
    echo -e "${YELLOW}ℹ️ xdotool 未安装（可选的X11自动化工具）${NC}"
    echo -e "${YELLOW}   安装命令: sudo apt install xdotool${NC}"
fi
echo ""

echo "📁 检查项目文件结构..."
if [ -f "CMakeLists_alternatives.txt" ]; then
    echo -e "${GREEN}✅ CMakeLists_alternatives.txt 存在${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${RED}❌ CMakeLists_alternatives.txt 不存在${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
fi

if [ -d "src" ]; then
    echo -e "${GREEN}✅ src目录存在${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
    
    # 检查关键源文件
    for file in "WindowManager.h" "WindowManager.cpp" "TabBasedLauncher.h" "TabBasedLauncher.cpp" "VirtualDesktopManager.h" "VirtualDesktopManager.cpp"; do
        if [ -f "src/$file" ]; then
            echo -e "${GREEN}  ✅ src/$file${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        else
            echo -e "${RED}  ❌ src/$file 缺失${NC}"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
        fi
        TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    done
else
    echo -e "${RED}❌ src目录不存在${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
fi

if [ -d "examples" ]; then
    echo -e "${GREEN}✅ examples目录存在${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
    
    # 检查示例文件
    for file in "alternative_main.cpp" "virtual_desktop_ubuntu_demo.cpp"; do
        if [ -f "examples/$file" ]; then
            echo -e "${GREEN}  ✅ examples/$file${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        else
            echo -e "${RED}  ❌ examples/$file 缺失${NC}"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
        fi
        TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    done
else
    echo -e "${RED}❌ examples目录不存在${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
fi
TOTAL_CHECKS=$((TOTAL_CHECKS + 3))
echo ""

echo "📊 检查结果汇总:"
echo -e "总计检查项目: $TOTAL_CHECKS"
echo -e "${GREEN}通过: $PASSED_CHECKS${NC}"
echo -e "${RED}失败: $FAILED_CHECKS${NC}"

if [ $FAILED_CHECKS -eq 0 ]; then
    echo -e "${GREEN}🎉 所有检查通过！您可以开始构建项目了。${NC}"
    echo ""
    echo "构建命令:"
    echo "mkdir -p build && cd build"
    echo "cmake -f ../CMakeLists_alternatives.txt .."
    echo "make -j\$(nproc)"
    exit 0
else
    echo -e "${YELLOW}⚠️ 发现 $FAILED_CHECKS 个问题，建议先解决这些依赖问题。${NC}"
    echo ""
    echo "快速安装所有依赖的命令（Ubuntu 18.04）:"
    echo "sudo apt update"
    echo "sudo apt install build-essential cmake qt5-default libqt5webengine5-dev libx11-dev libxext-dev wmctrl xdotool"
    exit 1
fi 