# FlightControls 浮动启动器

## 项目概述
基于Qt的飞行控制应用程序浮动启动器，悬浮显示在顶部居中置顶，集成QGroundControl和RVIZ的启动按钮，点击按钮就可以启动程序，提供统一的按钮界面。

## ✨ 核心特性

- **🚁 QGC启动**: 一键启动QGroundControl地面控制站
- **🤖 RVIZ启动**: 一键启动ROS可视化工具RVIZ
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

## 🚀 快速开始

### 1. 编译项目
```bash
mkdir build && cd build
cmake ..
make
```

### 2. 运行启动器
```bash
./flight_controls_launcher
```

### 3. 使用方法
- 启动器会自动显示在屏幕顶部居中位置
- 点击"🚁 启动 QGC"按钮启动QGroundControl
- 点击"🤖 启动 RVIZ"按钮启动RVIZ
- 可以拖拽启动器到任意位置
- 按钮会根据程序运行状态变化（启动/停止）

## 🎯 功能特点

### 悬浮设计
- 始终置顶显示，不会被其他窗口遮挡
- 紧凑的界面设计（360x100像素）
- 半透明背景，现代化视觉效果

### 智能状态管理
- 实时监控QGC和RVIZ运行状态
- 按钮文本动态变化（启动→停止）
- 状态指示器显示当前运行的程序

### 简单易用
- 无需复杂配置，开箱即用
- 一键启动/停止功能
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

## 📸 界面预览

启动器采用现代化的UI设计：
- 深色半透明背景
- 绿色QGC按钮，蓝色RVIZ按钮
- 状态指示器实时显示运行状态
- 右上角关闭按钮

## 🔄 版本历史

- **v5.0**: 简化的浮动启动器设计
- **v4.0**: 之前的嵌入式窗口管理器（已重构）

## 🤝 贡献指南

欢迎提交Issues和Pull Requests来改进这个项目！

## 📄 许可证

本项目采用MIT许可证 - 查看[LICENSE](LICENSE)文件了解详情。
