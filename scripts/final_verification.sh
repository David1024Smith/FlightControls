#!/bin/bash

# FlightControls 最终验证脚本
# 检查所有已知问题是否已修复

set -e

echo "🔍 FlightControls 最终代码验证..."

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 统计变量
TOTAL_CHECKS=0
PASSED_CHECKS=0
FAILED_CHECKS=0
WARNING_CHECKS=0

check_pattern() {
    local pattern=$1
    local file_pattern=$2
    local description=$3
    local should_exist=$4  # true if pattern should exist, false if it shouldn't
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    local count=$(grep -r "$pattern" --include="$file_pattern" src/ 2>/dev/null | wc -l)
    
    if [ "$should_exist" = "true" ]; then
        if [ $count -gt 0 ]; then
            echo -e "${GREEN}✅ $description (发现 $count 处)${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        else
            echo -e "${RED}❌ $description (未发现)${NC}"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
        fi
    else
        if [ $count -eq 0 ]; then
            echo -e "${GREEN}✅ $description (已修复)${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
        else
            echo -e "${RED}❌ $description (仍存在 $count 处)${NC}"
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
            # 显示具体位置
            grep -n "$pattern" --include="$file_pattern" src/* 2>/dev/null | head -3
        fi
    fi
}

check_file_exists() {
    local file=$1
    local description=$2
    
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
    
    if [ -f "$file" ]; then
        echo -e "${GREEN}✅ $description${NC}"
        PASSED_CHECKS=$((PASSED_CHECKS + 1))
    else
        echo -e "${RED}❌ $description${NC}"
        FAILED_CHECKS=$((FAILED_CHECKS + 1))
    fi
}

echo "📋 检查项目结构..."
check_file_exists "src/WindowManager.h" "WindowManager头文件存在"
check_file_exists "src/WindowManager.cpp" "WindowManager实现文件存在"
check_file_exists "src/TabBasedLauncher.h" "TabBasedLauncher头文件存在"
check_file_exists "src/TabBasedLauncher.cpp" "TabBasedLauncher实现文件存在"
check_file_exists "src/VirtualDesktopManager.h" "VirtualDesktopManager头文件存在"
check_file_exists "src/VirtualDesktopManager.cpp" "VirtualDesktopManager实现文件存在"
check_file_exists "examples/window_manager_simple.cpp" "简化窗口管理器演示存在"
check_file_exists "examples/virtual_desktop_ubuntu_demo.cpp" "虚拟桌面演示存在"
check_file_exists "CMakeLists_simple.txt" "简化构建配置存在"
check_file_exists "scripts/check_dependencies.sh" "依赖检查脚本存在"
echo ""

echo "🚨 检查已修复的问题..."

# 1. 检查内存管理问题
echo "🔹 内存管理问题："
check_pattern "delete.*process" "*.cpp" "直接删除进程（不安全）" false
check_pattern "deleteLater()" "*.cpp" "使用deleteLater()（安全）" true

# 2. 检查线程安全问题
echo "🔹 线程安全问题："
check_pattern "QTimer::singleShot.*this.*\[" "*.cpp" "QTimer使用this指针（安全）" true
check_pattern "QThread::sleep" "*.cpp" "QThread::sleep使用（会阻塞UI）" false

# 3. 检查X11错误处理
echo "🔹 X11错误处理："
check_pattern "XSetErrorHandler" "*.cpp" "X11错误处理器设置" true

# 4. 检查头文件包含
echo "🔹 头文件包含："
check_pattern "#include <QDateTime>" "*.cpp" "QDateTime头文件包含" true
check_pattern "#include <QThread>" "*.cpp" "QThread头文件包含" true
check_pattern "#include <QTextCursor>" "*.cpp" "QTextCursor头文件包含" true

# 5. 检查条件编译
echo "🔹 条件编译支持："
check_pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" "*.h" "QWebEngine条件编译" true
check_pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" "*.cpp" "QWebEngine条件编译实现" true

# 6. 检查函数调用约定
echo "🔹 函数调用约定："
check_pattern "this->checkRVIZAvailable()" "*.cpp" "显式使用this指针" true
echo ""

echo "⚙️ 编译测试..."

# 检查简化构建
if [ -f "CMakeLists_simple.txt" ]; then
    echo "🔹 测试简化构建配置..."
    if mkdir -p build-test 2>/dev/null && cd build-test 2>/dev/null; then
        if cmake -f ../CMakeLists_simple.txt .. > cmake_output.log 2>&1; then
            echo -e "${GREEN}✅ CMake配置成功${NC}"
            PASSED_CHECKS=$((PASSED_CHECKS + 1))
            
            # 检查是否能找到必要的依赖
            if grep -q "Qt5_VERSION" cmake_output.log; then
                echo -e "${GREEN}✅ Qt5依赖检测成功${NC}"
                PASSED_CHECKS=$((PASSED_CHECKS + 1))
            else
                echo -e "${YELLOW}⚠️ Qt5依赖检测警告${NC}"
                WARNING_CHECKS=$((WARNING_CHECKS + 1))
            fi
            
            if grep -q "X11_LIBRARIES" cmake_output.log; then
                echo -e "${GREEN}✅ X11依赖检测成功${NC}"
                PASSED_CHECKS=$((PASSED_CHECKS + 1))
            else
                echo -e "${YELLOW}⚠️ X11依赖检测警告${NC}"
                WARNING_CHECKS=$((WARNING_CHECKS + 1))
            fi
        else
            echo -e "${RED}❌ CMake配置失败${NC}"
            echo "错误详情:"
            cat cmake_output.log | tail -10
            FAILED_CHECKS=$((FAILED_CHECKS + 1))
        fi
        cd .. && rm -rf build-test
    else
        echo -e "${YELLOW}⚠️ 无法创建测试构建目录${NC}"
        WARNING_CHECKS=$((WARNING_CHECKS + 1))
    fi
    TOTAL_CHECKS=$((TOTAL_CHECKS + 3))
else
    echo -e "${RED}❌ CMakeLists_simple.txt 不存在${NC}"
    FAILED_CHECKS=$((FAILED_CHECKS + 1))
    TOTAL_CHECKS=$((TOTAL_CHECKS + 1))
fi
echo ""

echo "🔧 代码质量检查..."

# 检查代码风格和潜在问题
echo "🔹 潜在问题检查："

# 检查可能的内存泄漏
leak_count=$(grep -r "new " --include="*.cpp" src/ | grep -v "new.*this)" | wc -l)
if [ $leak_count -lt 10 ]; then  # 阈值调整为合理范围
    echo -e "${GREEN}✅ 内存分配使用合理 ($leak_count 处new)${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${YELLOW}⚠️ 发现较多内存分配 ($leak_count 处new)${NC}"
    WARNING_CHECKS=$((WARNING_CHECKS + 1))
fi

# 检查空指针检查
null_check_count=$(grep -r "if.*!.*)" --include="*.cpp" src/ | wc -l)
if [ $null_check_count -gt 5 ]; then
    echo -e "${GREEN}✅ 空指针检查充分 ($null_check_count 处检查)${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${YELLOW}⚠️ 空指针检查较少 ($null_check_count 处检查)${NC}"
    WARNING_CHECKS=$((WARNING_CHECKS + 1))
fi

# 检查信号槽连接
signal_count=$(grep -r "connect(" --include="*.cpp" src/ | wc -l)
if [ $signal_count -gt 10 ]; then
    echo -e "${GREEN}✅ 信号槽使用活跃 ($signal_count 个连接)${NC}"
    PASSED_CHECKS=$((PASSED_CHECKS + 1))
else
    echo -e "${BLUE}ℹ️ 信号槽使用适中 ($signal_count 个连接)${NC}"
fi

TOTAL_CHECKS=$((TOTAL_CHECKS + 3))
echo ""

echo "📊 验证结果汇总:"
echo -e "总计检查项目: $TOTAL_CHECKS"
echo -e "${GREEN}通过: $PASSED_CHECKS${NC}"
echo -e "${YELLOW}警告: $WARNING_CHECKS${NC}"
echo -e "${RED}失败: $FAILED_CHECKS${NC}"

# 计算成功率
if [ $TOTAL_CHECKS -gt 0 ]; then
    success_rate=$(( (PASSED_CHECKS * 100) / TOTAL_CHECKS ))
    echo -e "成功率: ${success_rate}%"
fi
echo ""

if [ $FAILED_CHECKS -eq 0 ]; then
    if [ $WARNING_CHECKS -eq 0 ]; then
        echo -e "${GREEN}🎉 完美！所有检查通过，代码质量优秀！${NC}"
        echo ""
        echo "🚀 可以开始构建和使用："
        echo "1. 运行依赖检查: ./scripts/check_dependencies.sh"
        echo "2. 构建项目: mkdir build && cd build && cmake -f ../CMakeLists_simple.txt .. && make"
        echo "3. 运行演示: ./window_manager_simple 或 ./virtual_desktop_ubuntu_demo"
        exit 0
    else
        echo -e "${YELLOW}⚠️ 检查基本通过，但有 $WARNING_CHECKS 个警告需要注意${NC}"
        echo "建议在生产环境使用前解决警告问题"
        exit 1
    fi
else
    echo -e "${RED}❌ 发现 $FAILED_CHECKS 个严重问题，需要修复后才能使用${NC}"
    echo ""
    echo "建议修复措施："
    echo "1. 检查缺失的头文件包含"
    echo "2. 修复内存管理问题"
    echo "3. 解决编译配置问题"
    echo "4. 重新运行此验证脚本"
    exit 2
fi 