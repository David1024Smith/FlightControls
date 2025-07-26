#!/bin/bash

# 简单的编译测试脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls 编译测试${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查Qt版本
echo -e "${YELLOW}检查Qt版本...${NC}"
if pkg-config --exists Qt5Core; then
    QT_VERSION=$(pkg-config --modversion Qt5Core)
    echo -e "${GREEN}Qt版本: $QT_VERSION${NC}"
else
    echo -e "${RED}Qt5未安装${NC}"
    exit 1
fi

# 清理旧的构建
echo -e "${YELLOW}清理旧的构建文件...${NC}"
rm -rf build
mkdir build
cd build

# 配置CMake
echo -e "${YELLOW}配置CMake...${NC}"
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-Wno-error" \
      ..

# 编译
echo -e "${YELLOW}开始编译...${NC}"
make VERBOSE=1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}✅ 编译成功！${NC}"
    echo -e "${GREEN}========================================${NC}"
    
    echo -e "${YELLOW}可执行文件位置:${NC}"
    ls -la flight_controls_launcher
    
    echo -e "${YELLOW}测试运行（5秒后自动退出）:${NC}"
    timeout 5s ./flight_controls_launcher || echo -e "${BLUE}程序已退出${NC}"
else
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}❌ 编译失败${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi