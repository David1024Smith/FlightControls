#!/bin/bash

# FlightControls Launcher 打包结构测试脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls Launcher 打包结构测试${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查必需文件
check_required_files() {
    echo -e "${YELLOW}检查必需文件...${NC}"
    
    local required_files=(
        "CMakeLists.txt"
        "README.md"
        "LICENSE"
        "src/main.cpp"
        "src/FlightControlsLauncher.h"
        "src/FlightControlsLauncher.cpp"
        "scripts/create_deb_package.sh"
        "scripts/create_rpm_package.sh"
        "scripts/install.sh"
        "scripts/package.sh"
        "PACKAGING_GUIDE.md"
    )
    
    local missing_files=()
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$PROJECT_ROOT/$file" ]; then
            missing_files+=("$file")
        fi
    done
    
    if [ ${#missing_files[@]} -eq 0 ]; then
        echo -e "${GREEN}✓ 所有必需文件都存在${NC}"
    else
        echo -e "${RED}✗ 缺少以下文件:${NC}"
        printf '%s\n' "${missing_files[@]}"
        return 1
    fi
}

# 检查脚本权限
check_script_permissions() {
    echo -e "${YELLOW}检查脚本权限...${NC}"
    
    local scripts=(
        "scripts/create_deb_package.sh"
        "scripts/create_rpm_package.sh"
        "scripts/install.sh"
        "scripts/package.sh"
    )
    
    local non_executable=()
    
    for script in "${scripts[@]}"; do
        if [ ! -x "$PROJECT_ROOT/$script" ]; then
            non_executable+=("$script")
        fi
    done
    
    if [ ${#non_executable[@]} -eq 0 ]; then
        echo -e "${GREEN}✓ 所有脚本都有执行权限${NC}"
    else
        echo -e "${YELLOW}! 以下脚本需要执行权限:${NC}"
        printf '%s\n' "${non_executable[@]}"
        echo -e "${YELLOW}运行: chmod +x scripts/*.sh${NC}"
    fi
}

# 检查脚本语法
check_script_syntax() {
    echo -e "${YELLOW}检查脚本语法...${NC}"
    
    local scripts=(
        "scripts/create_deb_package.sh"
        "scripts/create_rpm_package.sh"
        "scripts/install.sh"
        "scripts/package.sh"
    )
    
    local syntax_errors=()
    
    for script in "${scripts[@]}"; do
        if ! bash -n "$PROJECT_ROOT/$script" 2>/dev/null; then
            syntax_errors+=("$script")
        fi
    done
    
    if [ ${#syntax_errors[@]} -eq 0 ]; then
        echo -e "${GREEN}✓ 所有脚本语法正确${NC}"
    else
        echo -e "${RED}✗ 以下脚本有语法错误:${NC}"
        printf '%s\n' "${syntax_errors[@]}"
        return 1
    fi
}

# 检查CMake配置
check_cmake_config() {
    echo -e "${YELLOW}检查CMake配置...${NC}"
    
    if grep -q "cmake_minimum_required" "$PROJECT_ROOT/CMakeLists.txt" && \
       grep -q "find_package.*Qt5" "$PROJECT_ROOT/CMakeLists.txt" && \
       grep -q "add_executable" "$PROJECT_ROOT/CMakeLists.txt"; then
        echo -e "${GREEN}✓ CMake配置正确${NC}"
    else
        echo -e "${RED}✗ CMake配置可能有问题${NC}"
        return 1
    fi
}

# 检查源码文件
check_source_files() {
    echo -e "${YELLOW}检查源码文件...${NC}"
    
    if grep -q "#include.*FlightControlsLauncher.h" "$PROJECT_ROOT/src/main.cpp" && \
       grep -q "class.*FlightControlsLauncher" "$PROJECT_ROOT/src/FlightControlsLauncher.h" && \
       grep -q "#include.*FlightControlsLauncher.h" "$PROJECT_ROOT/src/FlightControlsLauncher.cpp"; then
        echo -e "${GREEN}✓ 源码文件结构正确${NC}"
    else
        echo -e "${RED}✗ 源码文件可能有问题${NC}"
        return 1
    fi
}

# 显示项目信息
show_project_info() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}项目信息${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    echo -e "${YELLOW}项目根目录:${NC} $PROJECT_ROOT"
    echo -e "${YELLOW}脚本目录:${NC} $SCRIPT_DIR"
    
    if [ -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
        local version=$(grep "CPACK_PACKAGE_VERSION" "$PROJECT_ROOT/CMakeLists.txt" | sed 's/.*"\(.*\)".*/\1/')
        echo -e "${YELLOW}项目版本:${NC} $version"
    fi
    
    echo -e "${YELLOW}支持的包格式:${NC} DEB, RPM"
    echo -e "${YELLOW}目标系统:${NC} Ubuntu/Debian, CentOS/RHEL/Fedora"
}

# 显示使用说明
show_usage() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}使用说明${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    echo -e "${YELLOW}1. 给脚本添加执行权限:${NC}"
    echo "   chmod +x scripts/*.sh"
    echo
    
    echo -e "${YELLOW}2. 自动打包:${NC}"
    echo "   ./scripts/package.sh auto"
    echo
    
    echo -e "${YELLOW}3. 创建DEB包:${NC}"
    echo "   ./scripts/create_deb_package.sh"
    echo
    
    echo -e "${YELLOW}4. 创建RPM包:${NC}"
    echo "   ./scripts/create_rpm_package.sh"
    echo
    
    echo -e "${YELLOW}5. 直接安装:${NC}"
    echo "   ./scripts/install.sh"
    echo
    
    echo -e "${YELLOW}6. 查看详细文档:${NC}"
    echo "   cat PACKAGING_GUIDE.md"
}

# 主函数
main() {
    cd "$PROJECT_ROOT"
    
    local all_passed=true
    
    check_required_files || all_passed=false
    echo
    
    check_script_permissions
    echo
    
    check_script_syntax || all_passed=false
    echo
    
    check_cmake_config || all_passed=false
    echo
    
    check_source_files || all_passed=false
    echo
    
    show_project_info
    echo
    
    show_usage
    echo
    
    if [ "$all_passed" = true ]; then
        echo -e "${GREEN}========================================${NC}"
        echo -e "${GREEN}✓ 所有检查通过，项目结构正确！${NC}"
        echo -e "${GREEN}========================================${NC}"
        return 0
    else
        echo -e "${RED}========================================${NC}"
        echo -e "${RED}✗ 发现问题，请修复后重试${NC}"
        echo -e "${RED}========================================${NC}"
        return 1
    fi
}

main "$@"