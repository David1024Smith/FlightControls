# Ubuntu 18.04 虚拟桌面方案设置指南

## 系统要求

- Ubuntu 18.04 LTS
- GNOME桌面环境 (默认)
- Qt 5.12.8开发环境
- X11显示服务器

## 1. 环境检测和准备

### 检查桌面环境
```bash
# 检查当前桌面环境
echo $XDG_CURRENT_DESKTOP
# 应该显示: ubuntu:GNOME 或 GNOME

echo $DESKTOP_SESSION  
# 应该显示: ubuntu 或 gnome

echo $GDMSESSION
# 应该显示: ubuntu 或 gnome
```

### 检查工作区支持
```bash
# 检查当前工作区数量
gsettings get org.gnome.desktop.wm.preferences num-workspaces

# 检查工作区是否启用
gsettings get org.gnome.mutter dynamic-workspaces
# 如果为true，需要设置为false以使用固定工作区数量
```

## 2. 配置虚拟桌面

### 设置工作区数量
```bash
# 设置为4个工作区（推荐配置）
gsettings set org.gnome.desktop.wm.preferences num-workspaces 4

# 禁用动态工作区（重要！）
gsettings set org.gnome.mutter dynamic-workspaces false

# 验证设置
gsettings get org.gnome.desktop.wm.preferences num-workspaces
```

### 配置工作区快捷键
```bash
# 设置Super+数字键切换工作区
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-1 "['<Super>1']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-2 "['<Super>2']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-3 "['<Super>3']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-4 "['<Super>4']"

# 设置移动窗口到指定工作区的快捷键
gsettings set org.gnome.desktop.wm.keybindings move-to-workspace-1 "['<Super><Shift>1']"
gsettings set org.gnome.desktop.wm.keybindings move-to-workspace-2 "['<Super><Shift>2']"
gsettings set org.gnome.desktop.wm.keybindings move-to-workspace-3 "['<Super><Shift>3']"
gsettings set org.gnome.desktop.wm.keybindings move-to-workspace-4 "['<Super><Shift>4']"
```

### 启用工作区指示器
```bash
# 在顶部面板显示工作区信息
gsettings set org.gnome.shell.extensions.dash-to-dock show-apps-at-top true

# 如果安装了Workspace Indicator扩展
# 可通过GNOME扩展管理器启用工作区切换器
```

## 3. 应用程序配置

### QGroundControl设置
```bash
# 下载QGroundControl
cd ~/Downloads
wget https://d176tv9ibo4jno.cloudfront.net/latest/QGroundControl.AppImage
chmod +x QGroundControl.AppImage

# 创建桌面图标（可选）
cat > ~/.local/share/applications/qgroundcontrol.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=QGroundControl
Comment=Ground Control Station for Drones
Exec=$HOME/Downloads/QGroundControl.AppImage
Icon=$HOME/Downloads/qgc-icon.png
Terminal=false
Categories=Engineering;Science;
EOF
```

### ROS和RVIZ设置
```bash
# 安装ROS Melodic（Ubuntu 18.04推荐版本）
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

# 验证RVIZ安装
which rviz
rosrun rviz rviz --help
```

## 4. 程序化桌面切换

### 使用gdbus切换工作区
```bash
# 切换到工作区1
gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "global.workspace_manager.get_workspace_by_index(0).activate(global.get_current_time())"

# 切换到工作区2
gdbus call --session --dest org.gnome.Shell --object-path /org/gnome/Shell --method org.gnome.Shell.Eval "global.workspace_manager.get_workspace_by_index(1).activate(global.get_current_time())"
```

### 使用wmctrl切换（备选方案）
```bash
# 安装wmctrl
sudo apt install wmctrl

# 切换到工作区
wmctrl -s 0  # 切换到工作区1
wmctrl -s 1  # 切换到工作区2
wmctrl -s 2  # 切换到工作区3
```

### 使用xdotool切换（另一备选方案）
```bash
# 安装xdotool
sudo apt install xdotool

# 切换工作区
xdotool set_desktop 0  # 工作区1
xdotool set_desktop 1  # 工作区2
xdotool set_desktop 2  # 工作区3
```

## 5. 验证配置

### 检查工作区配置
```bash
# 查看当前工作区
gsettings get org.gnome.desktop.wm.preferences num-workspaces

# 查看快捷键配置
gsettings get org.gnome.desktop.wm.keybindings switch-to-workspace-1
gsettings get org.gnome.desktop.wm.keybindings switch-to-workspace-2
gsettings get org.gnome.desktop.wm.keybindings switch-to-workspace-3
gsettings get org.gnome.desktop.wm.keybindings switch-to-workspace-4
```

### 测试工作区切换
```bash
# 手动测试快捷键
# 按 Super+1, Super+2, Super+3, Super+4
# 应该能看到工作区切换动画

# 程序化测试
./virtual_desktop_ubuntu_demo
```

## 6. 故障排除

### 常见问题

#### 工作区数量无法固定
```bash
# 确保禁用动态工作区
gsettings set org.gnome.mutter dynamic-workspaces false

# 重启GNOME Shell（Alt+F2，输入r，回车）
# 或注销重新登录
```

#### 快捷键不生效
```bash
# 检查是否有快捷键冲突
gsettings list-recursively org.gnome.desktop.wm.keybindings | grep workspace

# 重置快捷键配置
gsettings reset org.gnome.desktop.wm.keybindings switch-to-workspace-1
gsettings reset org.gnome.desktop.wm.keybindings switch-to-workspace-2
# 然后重新设置
```

#### gdbus命令失败
```bash
# 检查GNOME Shell是否运行
ps aux | grep gnome-shell

# 检查D-Bus连接
gdbus introspect --session --dest org.gnome.Shell --object-path /org/gnome/Shell
```

#### 应用程序不在正确工作区
```bash
# 检查窗口管理器规则
gsettings list-recursively org.gnome.desktop.wm.preferences

# 手动移动窗口到指定工作区
wmctrl -r "QGroundControl" -t 1  # 移动到工作区2
wmctrl -r "RViz" -t 2           # 移动到工作区3
```

## 7. 性能优化

### 禁用不必要的动画（提高切换速度）
```bash
# 减少动画时间
gsettings set org.gnome.desktop.interface enable-animations false

# 或者只减少动画时间而不完全禁用
gsettings set org.gnome.desktop.interface enable-animations true
# 通过调整动画速度（需要安装GNOME Tweaks）
```

### 优化内存使用
```bash
# 限制后台应用程序资源使用
# 在应用程序启动脚本中设置
export QT_QPA_PLATFORM=xcb
export QT_AUTO_SCREEN_SCALE_FACTOR=0
```

## 8. 自动化脚本

### 一键配置脚本
```bash
#!/bin/bash
# setup_virtual_desktop.sh

echo "配置Ubuntu 18.04虚拟桌面环境..."

# 设置工作区
gsettings set org.gnome.desktop.wm.preferences num-workspaces 4
gsettings set org.gnome.mutter dynamic-workspaces false

# 设置快捷键
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-1 "['<Super>1']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-2 "['<Super>2']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-3 "['<Super>3']"
gsettings set org.gnome.desktop.wm.keybindings switch-to-workspace-4 "['<Super>4']"

echo "虚拟桌面配置完成！"
echo "请注销并重新登录以使所有设置生效。"
```

### 应用程序启动脚本
```bash
#!/bin/bash
# launch_apps.sh

echo "启动飞行控制应用程序..."

# 启动QGroundControl到工作区2
wmctrl -s 1
./QGroundControl.AppImage &
sleep 3

# 启动RVIZ到工作区3
wmctrl -s 2
roscore &
sleep 2
rosrun rviz rviz &
sleep 3

# 返回主控制台
wmctrl -s 0

echo "所有应用程序已启动完成！"
```

## 9. 集成测试

运行我们的演示程序：
```bash
cd build-alternatives
./virtual_desktop_ubuntu_demo
```

预期结果：
- ✅ 自动检测Ubuntu GNOME环境
- ✅ 配置4个工作区
- ✅ 设置快捷键绑定
- ✅ 注册QGC和RVIZ应用
- ✅ 实现一键切换功能 