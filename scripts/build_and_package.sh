#!/bin/bash

# 一键编译和打包脚本 - Ubuntu 18.04优化版

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls 一键编译和打包${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查系统信息
echo -e "${YELLOW}系统信息:${NC}"
echo "操作系统: $(lsb_release -d | cut -f2)"
echo "Qt版本: $(pkg-config --modversion Qt5Core 2>/dev/null || echo '未安装')"
echo "CMake版本: $(cmake --version | head -1 | cut -d' ' -f3)"
echo "GCC版本: $(gcc --version | head -1 | cut -d' ' -f4)"
echo

# 步骤1: 清理和准备
echo -e "${YELLOW}步骤1: 清理旧的构建文件...${NC}"
rm -rf build package
echo -e "${GREEN}✓ 清理完成${NC}"

# 步骤2: 编译项目
echo -e "${YELLOW}步骤2: 编译项目...${NC}"
mkdir build
cd build

# 配置CMake
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_CXX_FLAGS="-Wno-reorder -Wno-unused-parameter" \
      ..

# 编译
make -j$(nproc)

# 检查编译结果
EXECUTABLE=$(find . -name "flight_controls_launcher" -type f | head -1)
if [ -n "$EXECUTABLE" ]; then
    echo -e "${GREEN}✓ 编译成功${NC}"
    echo -e "${BLUE}可执行文件: $EXECUTABLE${NC}"
    
    # 显示文件信息
    ls -la "$EXECUTABLE"
    echo -e "${BLUE}依赖库:${NC}"
    ldd "$EXECUTABLE" | grep -E "(Qt|X11)" | head -5
else
    echo -e "${RED}❌ 编译失败，未找到可执行文件${NC}"
    exit 1
fi

cd ..

# 步骤3: 创建DEB包
echo -e "${YELLOW}步骤3: 创建DEB安装包...${NC}"
./scripts/create_deb_package.sh

# 步骤4: 验证DEB包
echo -e "${YELLOW}步骤4: 验证DEB包...${NC}"
if [ -f "package/flight-controls-launcher_5.0.0_amd64.deb" ]; then
    DEB_FILE="package/flight-controls-launcher_5.0.0_amd64.deb"
    echo -e "${GREEN}✓ DEB包创建成功${NC}"
    echo -e "${BLUE}包文件: $DEB_FILE${NC}"
    
    # 显示包信息
    echo -e "${BLUE}包大小:${NC}"
    ls -lh "$DEB_FILE" | awk '{print $5}'
    
    echo -e "${BLUE}包内容预览:${NC}"
    dpkg-deb --contents "$DEB_FILE" | head -10
    
    echo -e "${BLUE}包信息:${NC}"
    dpkg-deb --info "$DEB_FILE" | grep -E "(Package|Version|Description)"
else
    echo -e "${RED}❌ DEB包创建失败${NC}"
    exit 1
fi

# 步骤5: 显示安装说明
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}🎉 构建和打包完成！${NC}"
echo -e "${BLUE}========================================${NC}"

echo -e "${YELLOW}安装命令:${NC}"
echo "sudo dpkg -i $DEB_FILE"
echo "sudo apt-get install -f  # 如果有依赖问题"
echo

echo -e "${YELLOW}卸载命令:${NC}"
echo "sudo apt-get remove flight-controls-launcher"
echo

echo -e "${YELLOW}直接运行（无需安装）:${NC}"
echo "cd build && ./$EXECUTABLE"
echo

echo -e "${YELLOW}启动已安装的程序:${NC}"
echo "flight_controls_launcher"
echo "或从应用程序菜单中找到 'FlightControls Launcher'"
echo

echo -e "${GREEN}✅ 所有步骤完成！${NC}"