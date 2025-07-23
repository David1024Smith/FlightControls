# FlightControls Launcher - 使用说明

## 项目结构

```
FlightControlsLauncher/
├── CMakeLists.txt          # CMake构建配置
├── build.sh               # Linux构建脚本
├── build.bat              # Windows构建脚本
├── src/                   # 源代码目录
│   ├── main.cpp           # 程序入口
│   ├── MainWindow.h/cpp   # 主窗口类
│   ├── ProcessManager.h/cpp    # 进程管理类
│   ├── WindowEmbedder.h/cpp    # 窗口嵌入类
│   └── ApplicationSwitcher.h/cpp # 应用切换类
└── README.md              # 项目文档
```

## 编译要求

### Linux (Ubuntu 18.04 LTS)
```bash
# 安装Qt 5.12.8开发库
sudo apt-get install qt5-default qtbase5-dev

# 安装X11开发库
sudo apt-get install libx11-dev libxext-dev

# 安装CMake
sudo apt-get install cmake build-essential

# 安装ROS (如果需要RVIZ功能)
# 参考: http://wiki.ros.org/melodic/Installation/Ubuntu
```

### Windows
- Qt 5.12.8 或更高版本
- Visual Studio 2017 或更高版本
- CMake 3.10 或更高版本
- X11库 (通过WSL或虚拟机)

## 编译步骤

### Linux
```bash
# 克隆或下载项目
cd FlightControlsLauncher

# 运行构建脚本
./build.sh

# 或手动构建
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows
```cmd
REM 在项目目录中运行
build.bat

REM 或手动构建
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## 运行前准备

### 1. QGroundControl设置
- 下载QGroundControl.AppImage文件
- 将其放置在项目根目录或以下位置之一：
  - 当前目录
  - 用户主目录
  - ~/Downloads
  - /opt
  - /usr/local/bin
- 确保文件具有执行权限：
  ```bash
  chmod +x QGroundControl.AppImage
  ```

### 2. ROS/RVIZ设置
```bash
# 安装ROS Melodic (Ubuntu 18.04)
sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
sudo apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654
sudo apt update
sudo apt install ros-melodic-desktop-full

# 初始化rosdep
sudo rosdep init
rosdep update

# 设置环境变量
echo "source /opt/ros/melodic/setup.bash" >> ~/.bashrc
source ~/.bashrc

# 验证安装
roscore &
rosrun rviz rviz
```

## 运行应用程序

```bash
# 进入构建目录
cd build

# 运行程序
./FlightControlsLauncher
```

## 功能说明

### 主要功能
1. **应用程序预加载**: 启动时自动加载QGroundControl和RVIZ
2. **无缝切换**: 通过按钮或快捷键在应用程序间切换
3. **窗口嵌入**: 使用X11技术将外部应用嵌入到主界面
4. **进程管理**: 自动监控和重启崩溃的应用程序
5. **状态监控**: 实时显示应用程序运行状态

### 操作方式
- **按钮切换**: 点击工具栏的"QGroundControl"或"RVIZ"按钮
- **快捷键**: 
  - `Ctrl+1`: 切换到QGroundControl
  - `Ctrl+2`: 切换到RVIZ
- **菜单**: 使用菜单栏的应用程序选项

### 状态指示
- 工具栏右侧显示当前状态
- 状态栏显示详细信息
- 预加载进度实时更新

## 故障排除

### 常见问题

1. **QGroundControl无法启动**
   ```bash
   # 检查AppImage文件
   ls -la *.AppImage
   
   # 手动测试启动
   ./QGroundControl.AppImage
   
   # 检查权限
   chmod +x QGroundControl.AppImage
   ```

2. **RVIZ无法启动**
   ```bash
   # 检查ROS环境
   echo $ROS_DISTRO
   
   # 手动测试
   roscore &
   rosrun rviz rviz
   
   # 检查ROS安装
   rosversion -d
   ```

3. **窗口嵌入失败**
   ```bash
   # 检查X11环境
   echo $DISPLAY
   xdpyinfo
   
   # 检查X11库
   ldconfig -p | grep X11
   ```

4. **编译错误**
   ```bash
   # 检查Qt版本
   qmake --version
   
   # 检查CMake版本
   cmake --version
   
   # 检查依赖库
   pkg-config --list-all | grep qt5
   ```

### 日志调试
程序运行时会在控制台输出详细的调试信息，包括：
- 进程启动/停止状态
- 窗口查找和嵌入过程
- 错误信息和异常处理

## 系统要求

### 最低要求
- Ubuntu 18.04 LTS 或更高版本
- Qt 5.12.8 或更高版本
- X11窗口系统
- 4GB RAM
- 1GB可用磁盘空间

### 推荐配置
- Ubuntu 20.04 LTS
- Qt 5.15 或更高版本
- 8GB RAM
- 独立显卡支持
- SSD存储

## 许可证
本项目遵循MIT许可证。详见LICENSE文件。