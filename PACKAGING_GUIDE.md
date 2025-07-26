# FlightControls Launcher 打包指南

## 概述

本项目提供了多种Linux安装包创建方式，支持主流的Linux发行版。所有脚本都经过测试，可以在Ubuntu 18.04及更高版本上正常工作。

## 快速开始

### 1. 自动打包（推荐）
```bash
# 给脚本添加执行权限
chmod +x scripts/*.sh

# 自动检测系统并创建对应的安装包
./scripts/package.sh auto
```

### 2. 手动选择打包方式
```bash
# 运行交互式菜单
./scripts/package.sh

# 或直接指定包类型
./scripts/package.sh deb     # DEB包
./scripts/package.sh rpm     # RPM包
./scripts/package.sh install # 直接安装
```

## 详细说明

### DEB包 (Ubuntu/Debian)

**创建命令**：
```bash
./scripts/create_deb_package.sh
```

**生成文件**：
- `package/flight-controls-launcher_5.0.0_amd64.deb`

**安装方法**：
```bash
sudo dpkg -i package/flight-controls-launcher_5.0.0_amd64.deb
sudo apt-get install -f  # 修复依赖问题
```

**卸载方法**：
```bash
sudo apt-get remove flight-controls-launcher
```

**包特性**：
- 自动处理依赖关系
- 包含安装/卸载脚本
- 创建桌面快捷方式
- 更新应用程序菜单
- 包含man页面

### RPM包 (CentOS/RHEL/Fedora)

**创建命令**：
```bash
./scripts/create_rpm_package.sh
```

**生成文件**：
- `rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm` (二进制包)
- `rpmbuild/SRPMS/flight-controls-launcher-5.0.0-1.*.src.rpm` (源码包)

**安装方法**：
```bash
sudo yum localinstall rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm
# 或者
sudo rpm -ivh rpmbuild/RPMS/x86_64/flight-controls-launcher-5.0.0-1.*.rpm
```

**卸载方法**：
```bash
sudo yum remove flight-controls-launcher
# 或者
sudo rpm -e flight-controls-launcher
```

### 直接安装 (通用)

**安装命令**：
```bash
./scripts/install.sh
```

**特点**：
- 自动检测系统类型
- 安装对应的依赖包
- 编译并安装到 `/usr/local/bin/`
- 创建桌面快捷方式
- 适用于所有Linux发行版

## 系统要求

### 构建依赖
- CMake 3.10+
- C++17兼容编译器 (GCC 7+)
- Qt5开发包 (5.12+)
- X11开发库

### Ubuntu/Debian安装依赖
```bash
sudo apt-get update
sudo apt-get install build-essential cmake qt5-default qtbase5-dev libx11-dev wmctrl xdotool dpkg-dev fakeroot
```

### CentOS/RHEL安装依赖
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake3 qt5-qtbase-devel libX11-devel wmctrl xdotool rpm-build rpmdevtools
```

## 包内容

所有安装包都包含：

### 可执行文件
- `/usr/bin/flight_controls_launcher` (DEB/RPM)
- `/usr/local/bin/flight_controls_launcher` (直接安装)

### 桌面集成
- `/usr/share/applications/flight-controls-launcher.desktop` - 应用程序菜单项
- `/usr/share/pixmaps/flight-controls-launcher.svg` - 应用程序图标

### 文档
- `/usr/share/doc/flight-controls-launcher/README.md`
- `/usr/share/doc/flight-controls-launcher/LICENSE`
- `/usr/share/man/man1/flight_controls_launcher.1.gz` - Man页面

## 运行时依赖

### 必需依赖
- Qt5 Core, Widgets, Gui (>= 5.12)
- X11库 (libX11)

### 推荐依赖
- `wmctrl` - 增强窗口管理功能
- `xdotool` - 窗口操作工具
- `x11-utils` - X11实用工具

### 可选依赖
- `qgroundcontrol` - QGroundControl地面控制站
- `ros-*-rviz` - ROS可视化工具

## 故障排除

### 常见问题

#### 1. 缺少Qt5开发包
**错误**: `Could not find Qt5Core`
**解决**: 
```bash
# Ubuntu/Debian
sudo apt-get install qt5-default qtbase5-dev

# CentOS/RHEL
sudo yum install qt5-qtbase-devel
```

#### 2. 缺少X11开发库
**错误**: `Could not find X11`
**解决**:
```bash
# Ubuntu/Debian
sudo apt-get install libx11-dev

# CentOS/RHEL
sudo yum install libX11-devel
```

#### 3. CMake版本过低
**错误**: `CMake 3.10 or higher is required`
**解决**:
```bash
# Ubuntu 18.04+
sudo apt-get install cmake

# CentOS 7
sudo yum install cmake3
sudo ln -sf /usr/bin/cmake3 /usr/bin/cmake
```

#### 4. 权限问题
**错误**: `Permission denied`
**解决**:
```bash
chmod +x scripts/*.sh
```

### 调试模式

如果遇到构建问题，可以启用详细输出：
```bash
# 设置详细模式
export VERBOSE=1

# 运行打包脚本
./scripts/create_deb_package.sh
```

## Windows用户

Windows用户可以通过以下方式使用：

### 1. WSL (Windows Subsystem for Linux)
```cmd
# 运行Windows批处理脚本
scripts\package.bat

# 或直接在WSL中运行
wsl bash -c "./scripts/package.sh auto"
```

### 2. 虚拟机
在Linux虚拟机中运行打包脚本。

### 3. Docker
```bash
# 使用Ubuntu容器
docker run -it --rm -v $(pwd):/workspace ubuntu:20.04
cd /workspace
apt-get update && apt-get install -y build-essential cmake qt5-default qtbase5-dev libx11-dev
./scripts/package.sh auto
```

## 自定义配置

### 修改版本号
编辑 `CMakeLists.txt` 中的版本信息：
```cmake
set(CPACK_PACKAGE_VERSION "5.0.0")
```

### 修改包信息
编辑对应的打包脚本：
- DEB包: `scripts/create_deb_package.sh`
- RPM包: `scripts/create_rpm_package.sh`

### 添加依赖
在打包脚本中修改依赖列表：
```bash
# DEB包依赖
Depends: libqt5core5a (>= 5.12.0), libqt5gui5, libqt5widgets5, libx11-6

# RPM包依赖
Requires: qt5-qtbase >= 5.12, libX11
```

## 贡献

欢迎提交问题报告和改进建议！

### 测试新系统
如果您在新的Linux发行版上测试了打包脚本，请告诉我们结果。

### 改进脚本
如果您有改进建议或发现了bug，请提交Pull Request。

## 许可证

本项目采用MIT许可证，详见LICENSE文件。