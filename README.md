# FlightControls 浮动启动器

## 项目概述
基于Qt的飞行控制应用程序浮动启动器，悬浮显示在顶部居中置顶，集成QGroundControl和RVIZ的启动按钮，点击按钮就可以启动程序，提供统一的按钮界面。

## ✨ 核心特性

- **🚁 QGC启动**: 一键启动QGroundControl地面控制站
- **🤖 RVIZ启动**: 一键启动ROS可视化工具RVIZ
- **🔄 智能切换**: 在QGC和RVIZ之间智能切换窗口焦点
- **🔍 增强识别**: 多方法窗口识别（标题、进程、类名、PID匹配）
- **📍 悬浮置顶**: 始终显示在屏幕顶部居中位置
- **🎯 极简设计**: 紧凑的UI设计，不占用太多屏幕空间
- **🔄 状态监控**: 实时显示应用程序运行状态
- **🖱️ 可拖拽**: 支持鼠标拖拽移动位置
- **🎨 现代UI**: 半透明背景，圆角设计，阴影效果

## 🛠️ 技术栈

- **编程语言**: C++ 17
- **GUI框架**: Qt 5.12+
- **构建系统**: CMake
- **跨平台支持**: Windows, Linux, macOS

## 🏗️ 项目结构

```
FlightControls/
├── src/
│   ├── main.cpp                      # 程序入口
│   ├── FlightControlsLauncher.h      # 启动器头文件
│   └── FlightControlsLauncher.cpp    # 启动器实现文件
├── scripts/                          # 脚本文件
├── CMakeLists.txt                    # CMake构建文件
└── README.md                         # 项目说明
```

## 📋 系统要求

- **Linux系统**: Ubuntu 18.04+ (推荐)
- **Qt开发环境**: Qt 5.12+
- **构建工具**: CMake 3.10+
- **编译器**: GCC/G++支持C++17
- **X11开发包**: libx11-dev (用于窗口管理)
- **窗口管理工具**: wmctrl, xdotool (可选，提升兼容性)

## 🔧 编译错误修复

如果遇到编译错误，请使用自动修复脚本：

```bash
# 自动检测和修复编译问题
chmod +x scripts/fix_compilation_errors.sh
./scripts/fix_compilation_errors.sh

# 或测试特定的编译修复
chmod +x scripts/test_compilation_fix.sh
./scripts/test_compilation_fix.sh
```

**常见问题解决**：
```bash
# 安装Qt开发包
sudo apt-get install qt5-default qtbase5-dev

# 安装X11开发包
sudo apt-get install libx11-dev

# 安装窗口管理工具
sudo apt-get install wmctrl xdotool x11-utils
```

**已知修复的编译问题**：
- ✅ 成员变量声明缺失错误
- ✅ 方法声明缺失错误  
- ✅ 成员变量初始化顺序错误 (`[-Werror=reorder]`)
- ✅ X11/Qt头文件冲突
- ✅ 跨平台兼容性问题

详细的编译错误修复信息请参考：
- [COMPILATION_ERRORS_FIXED.md](COMPILATION_ERRORS_FIXED.md) - 主要编译错误修复
- [MEMBER_INITIALIZATION_ORDER_FIX.md](MEMBER_INITIALIZATION_ORDER_FIX.md) - 成员变量初始化顺序修复

## 🚀 快速开始

### 方式一：使用安装包（推荐）

#### Ubuntu 18.04 专用快速设置
```bash
# Ubuntu 18.04用户推荐使用专用设置脚本
chmod +x scripts/*.sh

# 方法1: 自动环境设置（推荐）
./scripts/ubuntu18_setup.sh --no-ros  # 跳过ROS安装
./scripts/quick_test.sh               # 快速编译测试

# 方法2: 直接测试编译
./scripts/quick_test.sh

# 编译成功后创建DEB包
./scripts/create_deb_package.sh
```

**Ubuntu 18.04特别说明**：
- 默认Qt版本5.9.5完全支持
- 已针对Qt 5.9进行兼容性优化
- 如遇到编译问题，使用`quick_test.sh`进行诊断

**完整的Ubuntu 18.04使用流程**：
```bash
# 1. 检查项目状态
./scripts/check_status.sh

# 2. 一键编译和打包（推荐）
./scripts/build_and_package.sh

# 3. 或者分步执行
./scripts/quick_test.sh          # 测试编译
./scripts/create_deb_package.sh  # 创建DEB包

# 4. 安装DEB包
sudo dpkg -i package/flight-controls-launcher_5.0.0_amd64.deb
sudo apt-get install -f

# 5. 安装QGroundControl（必需）
./scripts/install_qgroundcontrol.sh

# 6. 运行程序
flight_controls_launcher
```

### QGroundControl安装说明

FlightControls启动器需要QGroundControl才能正常工作。如果启动时提示"未找到QGroundControl.AppImage文件"，请按以下步骤安装：

#### 自动安装（推荐）
```bash
# 自动下载并安装QGroundControl
chmod +x scripts/install_qgroundcontrol.sh
./scripts/install_qgroundcontrol.sh

# 或者使用简化脚本
./scripts/setup_qgc_simple.sh
```

#### 手动安装
```bash
# 1. 下载QGroundControl
wget https://github.com/mavlink/qgroundcontrol/releases/download/v4.3.0/QGroundControl.AppImage

# 2. 移动到标准位置
mv QGroundControl.AppImage ~/QGroundControl.AppImage

# 3. 添加执行权限
chmod +x ~/QGroundControl.AppImage

# 4. 修复路径问题（如果需要）
./scripts/fix_qgc_path.sh
```

#### Ubuntu/Debian系统 - DEB包
```bash
# 创建DEB安装包
chmod +x scripts/create_deb_package.sh
./scripts/create_deb_package.sh

# 安装DEB包
sudo dpkg -i package/flight-controls-launcher_5.0.0_amd64.deb
sudo apt-get install -f  # 修复依赖问题（如果有）
```

#### CentOS/RHEL/Fedora系统 - RPM包
```bash
# 创建RPM安装包
chmod +x scripts/create_rpm_package.sh
./scripts/create_rpm_package.sh

# 安装RPM包
sudo yum localinstall rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm
# 或者
sudo rpm -ivh rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm
```

#### 统一打包脚本（自动检测系统）
```bash
# 使用统一打包脚本
chmod +x scripts/package.sh
./scripts/package.sh auto  # 自动检测系统并创建对应包

# 或者使用交互式菜单
./scripts/package.sh
```

### 方式二：直接安装
```bash
# 一键安装脚本（自动处理依赖）
chmod +x scripts/install.sh
./scripts/install.sh
```

### 方式三：手动编译
```bash
# 1. 安装依赖
# Ubuntu/Debian:
sudo apt-get install build-essential cmake qt5-default qtbase5-dev libx11-dev wmctrl xdotool

# CentOS/RHEL:
sudo yum groupinstall "Development Tools"
sudo yum install cmake3 qt5-qtbase-devel libX11-devel wmctrl xdotool

# 2. 编译项目
mkdir build && cd build
cmake ..
make

# 3. 运行启动器
./flight_controls_launcher
```

### 3. 使用方法
- 启动器会自动显示在屏幕顶部居中位置
- 点击"🚁 启动 QGC"按钮启动QGroundControl
- 点击"🤖 启动 RVIZ"按钮启动RVIZ
- 点击"🔄 切换"按钮在QGC和RVIZ之间智能切换焦点
- 可以拖拽启动器到任意位置
- 按钮会根据程序运行状态变化（启动/停止/切换/置前）

### 4. 窗口检测诊断
如果切换功能无法找到应用窗口，可以使用诊断工具：

```bash
# 运行全面的窗口检测诊断
chmod +x scripts/debug_window_detection.sh
./scripts/debug_window_detection.sh

# 或运行终极测试脚本
chmod +x scripts/test_ultimate_window_detection.sh
./scripts/test_ultimate_window_detection.sh
```

## 🎯 功能特点

### 悬浮设计
- 始终置顶显示，不会被其他窗口遮挡
- 紧凑的界面设计（360x100像素）
- 半透明背景，现代化视觉效果

### 智能状态管理
- 实时监控QGC和RVIZ运行状态
- 按钮文本动态变化（启动→停止→切换→置前）
- 状态指示器显示当前运行的程序

### 增强窗口识别
- **多重识别方法**: 标题匹配、进程名匹配、窗口类名匹配、PID关联
- **智能切换逻辑**: 自动检测当前活动窗口，切换到另一个应用
- **强化RVIZ支持**: 特别优化for RVIZ窗口识别（支持"default.rviz* - RViz"等格式）
- **容错机制**: 多种方法确保窗口识别的可靠性

### 简单易用
- 无需复杂配置，开箱即用
- 一键启动/停止功能
- 一键智能切换功能
- 自动进程管理

## 🔧 配置说明

### 应用程序路径配置
默认启动命令：
- **QGroundControl**: `QGroundControl.AppImage`
- **RVIZ**: `rviz`

如需修改启动命令，请编辑`FlightControlsLauncher.cpp`文件中的相关配置。

### RVIZ环境配置
RVIZ需要ROS环境，请确保已正确安装并配置：
```bash
source /opt/ros/your_ros_version/setup.bash
```

## ⚡ 高级功能

### 多重窗口识别机制
程序采用5种窗口识别方法确保可靠性：

1. **X11 API方法**：
   - 窗口标题匹配（支持"default.rviz* - RViz"等格式）
   - 进程名称匹配（通过PID关联）
   - 窗口类名匹配
   - 进程窗口遍历

2. **命令行工具方法**：
   - `wmctrl` 窗口查找和切换
   - `xdotool` 窗口查找和切换
   - `ps` + `xwininfo` 组合查找

### 智能切换逻辑
- 自动检测当前活动窗口
- 智能决策切换目标（QGC ↔ RVIZ）
- 多重备用方案确保成功率

### 自动诊断功能
- 启动时自动检测工具可用性
- 失败时自动列出所有窗口信息
- 详细的调试日志输出

## 🛠️ 故障排除

### 常见问题

#### 1. 切换按钮提示"找不到窗口"
**症状**：点击切换按钮时提示无法找到QGC或RVIZ窗口

**解决步骤**：
```bash
# 1. 运行诊断脚本
chmod +x scripts/debug_window_detection.sh
./scripts/debug_window_detection.sh

# 2. 检查进程是否存在
pgrep -f rviz
pgrep -f QGroundControl

# 3. 检查窗口是否存在
wmctrl -l | grep -i rviz
xdotool search --name rviz
```

**可能原因**：
- 应用程序未完全启动
- 窗口标题格式不匹配
- 窗口管理工具缺失
- X11权限问题

#### 2. 缺少窗口管理工具
**症状**：启动时提示工具不可用

**解决方案**：
```bash
# Ubuntu/Debian系统
sudo apt-get update
sudo apt-get install wmctrl xdotool x11-utils

# 检查安装结果
wmctrl --version
xdotool version
```

#### 3. RVIZ启动失败
**症状**：点击RVIZ按钮后无响应或报错

**解决步骤**：
```bash
# 1. 检查ROS环境
echo $ROS_PACKAGE_PATH
source /opt/ros/*/setup.bash

# 2. 手动测试RVIZ
roscore &
rosrun rviz rviz

# 3. 检查终端模拟器
gnome-terminal --version
```

#### 4. X11权限问题
**症状**：无法获取窗口信息或切换窗口

**解决方案**：
```bash
# 检查DISPLAY环境变量
echo $DISPLAY

# 测试X11连接
xwininfo -root

# 如果使用SSH，确保X11转发
ssh -X username@hostname
```

### 手动诊断方法

#### 获取RVIZ窗口信息
```bash
# 方法1: 使用xwininfo
xwininfo  # 然后点击RVIZ窗口

# 方法2: 使用wmctrl
wmctrl -l | grep -i rviz

# 方法3: 使用xdotool
xdotool search --name rviz
```

#### 测试窗口切换
```bash
# 测试wmctrl切换
wmctrl -a "default.rviz"
wmctrl -a rviz

# 测试xdotool切换
xdotool search --name rviz windowactivate
```

### 调试模式
启动器提供详细的调试输出，观察控制台信息：
- 🔧 工具可用性检测
- 🔍 窗口搜索过程
- 📋 找到的窗口详细信息
- ❌ 失败原因分析

### 联系支持
如果问题仍然存在，请提供：
1. 系统信息：`uname -a`
2. Qt版本：启动器输出的Qt版本信息
3. 窗口管理器：`echo $DESKTOP_SESSION`
4. RVIZ窗口信息：`xwininfo`点击RVIZ窗口的输出
5. 调试日志：启动器控制台的完整输出

## 📸 界面预览

启动器采用现代化的UI设计：
- 深色半透明背景
- 绿色QGC按钮，蓝色RVIZ按钮
- 状态指示器实时显示运行状态
- 右上角关闭按钮

## 🔄 版本历史

- **v5.0**: 简化的浮动启动器设计
- **v4.0**: 之前的嵌入式窗口管理器（已重构）

## 📦 打包和分发

### 支持的包格式
- **DEB包**: 适用于Ubuntu、Debian及其衍生版本
- **RPM包**: 适用于CentOS、RHEL、Fedora及其衍生版本
- **直接安装**: 适用于所有Linux发行版

### 打包脚本说明

#### 1. 统一打包脚本 (`scripts/package.sh`)
自动检测系统类型并选择合适的打包方式：
```bash
chmod +x scripts/package.sh

# 自动检测并打包
./scripts/package.sh auto

# 交互式菜单
./scripts/package.sh

# 直接指定包类型
./scripts/package.sh deb    # 创建DEB包
./scripts/package.sh rpm    # 创建RPM包
./scripts/package.sh install # 直接安装
```

#### 2. DEB包创建 (`scripts/create_deb_package.sh`)
专门用于创建Debian/Ubuntu安装包：
```bash
chmod +x scripts/create_deb_package.sh
./scripts/create_deb_package.sh
```

**生成的文件**：
- `package/flight-controls-launcher_5.0.0_amd64.deb` - 主安装包
- 包含完整的依赖关系和安装/卸载脚本
- 自动创建桌面快捷方式和应用程序菜单项

**安装和卸载**：
```bash
# 安装
sudo dpkg -i package/flight-controls-launcher_5.0.0_amd64.deb
sudo apt-get install -f  # 修复依赖（如果需要）

# 卸载
sudo apt-get remove flight-controls-launcher
```

#### 3. RPM包创建 (`scripts/create_rpm_package.sh`)
专门用于创建RedHat/CentOS/Fedora安装包：
```bash
chmod +x scripts/create_rpm_package.sh
./scripts/create_rpm_package.sh
```

**生成的文件**：
- `rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm` - 二进制包
- `rpmbuild/SRPMS/flight-controls-launcher-5.0.0-1.*.src.rpm` - 源码包

**安装和卸载**：
```bash
# 安装
sudo yum localinstall rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm
# 或者
sudo rpm -ivh rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm

# 卸载
sudo yum remove flight-controls-launcher
# 或者
sudo rpm -e flight-controls-launcher
```

#### 4. 直接安装脚本 (`scripts/install.sh`)
适用于所有Linux发行版的通用安装方式：
```bash
chmod +x scripts/install.sh
./scripts/install.sh
```

**功能特点**：
- 自动检测系统类型并安装对应依赖
- 编译并安装到 `/usr/local/bin/`
- 创建桌面快捷方式
- 支持Ubuntu/Debian和CentOS/RHEL系统

### 包内容说明

所有安装包都包含以下内容：
- **可执行文件**: `flight_controls_launcher`
- **桌面文件**: 应用程序菜单快捷方式
- **图标文件**: SVG格式的应用程序图标
- **文档文件**: README.md 和 LICENSE
- **Man页面**: 命令行帮助文档

### 依赖关系

#### 运行时依赖
- Qt5 Core, Widgets, Gui (>= 5.12)
- X11库 (libX11)
- 推荐安装: wmctrl, xdotool (增强窗口管理)

#### 构建时依赖
- CMake (>= 3.10)
- C++17兼容编译器 (GCC/Clang)
- Qt5开发包
- X11开发包

### 系统兼容性

**测试通过的系统**：
- Ubuntu 18.04, 20.04, 22.04
- Debian 10, 11
- CentOS 7, 8
- RHEL 7, 8
- Fedora 30+

**理论支持的系统**：
- 所有基于X11的Linux发行版
- 安装了Qt5的类Unix系统

## 🤝 贡献指南
