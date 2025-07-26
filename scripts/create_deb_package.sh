#!/bin/bash

# FlightControls Launcher DEB包创建脚本
# 适用于Ubuntu 18.04及更高版本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 项目信息
PROJECT_NAME="flight-controls-launcher"
VERSION="5.0.0"
MAINTAINER="FlightControls Team <support@flightcontrols.org>"
DESCRIPTION="基于Qt的飞行控制应用程序浮动启动器"
HOMEPAGE="https://github.com/flightcontrols/launcher"

# 目录设置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$PROJECT_ROOT/package"
DEB_DIR="$PACKAGE_DIR/deb"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls Launcher DEB包创建工具${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查依赖
check_dependencies() {
    echo -e "${YELLOW}检查构建依赖...${NC}"
    
    local missing_deps=()
    
    # 检查基本工具
    for cmd in cmake make g++ dpkg-deb fakeroot; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done
    
    # 检查Qt5
    if ! pkg-config --exists Qt5Core Qt5Widgets Qt5Gui; then
        missing_deps+=("qt5-default" "qtbase5-dev")
    fi
    
    # 检查Qt版本（Ubuntu 18.04默认是5.9.5）
    if pkg-config --exists Qt5Core; then
        QT_VERSION=$(pkg-config --modversion Qt5Core)
        echo -e "${BLUE}检测到Qt版本: $QT_VERSION${NC}"
        if [[ "$QT_VERSION" < "5.9.0" ]]; then
            echo -e "${RED}Qt版本过低，需要5.9.0或更高版本${NC}"
            missing_deps+=("qt5-default" "qtbase5-dev")
        fi
    fi
    
    # 检查X11
    if ! pkg-config --exists x11; then
        missing_deps+=("libx11-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${RED}缺少以下依赖:${NC}"
        printf '%s\n' "${missing_deps[@]}"
        echo -e "${YELLOW}请运行以下命令安装:${NC}"
        echo "sudo apt-get update"
        echo "sudo apt-get install ${missing_deps[*]}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ 所有依赖已满足${NC}"
}

# 清理旧的构建文件
clean_build() {
    echo -e "${YELLOW}清理旧的构建文件...${NC}"
    rm -rf "$BUILD_DIR"
    rm -rf "$PACKAGE_DIR"
    echo -e "${GREEN}✓ 清理完成${NC}"
}

# 编译项目
build_project() {
    echo -e "${YELLOW}编译项目...${NC}"
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # 配置CMake - 为Qt 5.9添加兼容性选项
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr \
          -DCMAKE_CXX_FLAGS="-Wno-reorder -Wno-unused-parameter" \
          "$PROJECT_ROOT"
    
    # 编译
    make -j$(nproc)
    
    echo -e "${GREEN}✓ 编译完成${NC}"
}

# 创建DEB包目录结构
create_deb_structure() {
    echo -e "${YELLOW}创建DEB包目录结构...${NC}"
    
    mkdir -p "$DEB_DIR"
    mkdir -p "$DEB_DIR/DEBIAN"
    mkdir -p "$DEB_DIR/usr/bin"
    mkdir -p "$DEB_DIR/usr/share/applications"
    mkdir -p "$DEB_DIR/usr/share/pixmaps"
    mkdir -p "$DEB_DIR/usr/share/doc/$PROJECT_NAME"
    mkdir -p "$DEB_DIR/usr/share/man/man1"
    
    echo -e "${GREEN}✓ 目录结构创建完成${NC}"
}

# 安装文件到DEB目录
install_files() {
    echo -e "${YELLOW}安装文件到DEB目录...${NC}"
    
    # 复制可执行文件（检查多个可能的位置）
    EXECUTABLE_PATHS=(
        "$BUILD_DIR/bin/flight_controls_launcher"
        "$BUILD_DIR/flight_controls_launcher"
        "$BUILD_DIR/src/flight_controls_launcher"
    )
    
    FOUND_EXECUTABLE=""
    for path in "${EXECUTABLE_PATHS[@]}"; do
        if [ -f "$path" ]; then
            FOUND_EXECUTABLE="$path"
            break
        fi
    done
    
    if [ -z "$FOUND_EXECUTABLE" ]; then
        echo -e "${RED}错误: 找不到可执行文件${NC}"
        echo "查找路径:"
        printf '%s\n' "${EXECUTABLE_PATHS[@]}"
        echo "构建目录内容:"
        find "$BUILD_DIR" -name "flight_controls_launcher" -type f 2>/dev/null || echo "未找到"
        exit 1
    fi
    
    echo -e "${BLUE}找到可执行文件: $FOUND_EXECUTABLE${NC}"
    cp "$FOUND_EXECUTABLE" "$DEB_DIR/usr/bin/"
    chmod 755 "$DEB_DIR/usr/bin/flight_controls_launcher"
    
    # 复制文档
    cp "$PROJECT_ROOT/README.md" "$DEB_DIR/usr/share/doc/$PROJECT_NAME/"
    cp "$PROJECT_ROOT/LICENSE" "$DEB_DIR/usr/share/doc/$PROJECT_NAME/"
    
    echo -e "${GREEN}✓ 文件安装完成${NC}"
}

# 创建桌面文件
create_desktop_file() {
    echo -e "${YELLOW}创建桌面文件...${NC}"
    
    cat > "$DEB_DIR/usr/share/applications/$PROJECT_NAME.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=FlightControls Launcher
Name[zh_CN]=飞行控制启动器
Comment=Flight Controls Floating Launcher
Comment[zh_CN]=基于Qt的飞行控制应用程序浮动启动器
Exec=/usr/bin/flight_controls_launcher
Icon=flight-controls-launcher
Terminal=false
Categories=Development;Engineering;
Keywords=flight;controls;launcher;qgroundcontrol;rviz;ros;
StartupNotify=true
StartupWMClass=flight_controls_launcher
EOF
    
    echo -e "${GREEN}✓ 桌面文件创建完成${NC}"
}

# 创建图标文件
create_icon() {
    echo -e "${YELLOW}创建应用程序图标...${NC}"
    
    # 创建简单的SVG图标
    cat > "$DEB_DIR/usr/share/pixmaps/flight-controls-launcher.svg" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="64" height="64" viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="grad1" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#4CAF50;stop-opacity:1" />
      <stop offset="100%" style="stop-color:#2196F3;stop-opacity:1" />
    </linearGradient>
  </defs>
  <circle cx="32" cy="32" r="30" fill="url(#grad1)" stroke="#333" stroke-width="2"/>
  <path d="M20 32 L32 20 L44 32 L32 44 Z" fill="white" opacity="0.9"/>
  <circle cx="32" cy="32" r="4" fill="#333"/>
</svg>
EOF
    
    echo -e "${GREEN}✓ 图标创建完成${NC}"
}

# 创建man页面
create_man_page() {
    echo -e "${YELLOW}创建man页面...${NC}"
    
    cat > "$DEB_DIR/usr/share/man/man1/flight_controls_launcher.1" << EOF
.TH FLIGHT_CONTROLS_LAUNCHER 1 "$(date +'%B %Y')" "version $VERSION" "User Commands"
.SH NAME
flight_controls_launcher \- Flight Controls Floating Launcher
.SH SYNOPSIS
.B flight_controls_launcher
.SH DESCRIPTION
基于Qt的飞行控制应用程序浮动启动器，悬浮显示在顶部居中置顶，集成QGroundControl和RVIZ的启动按钮。
.PP
主要功能包括：
.IP \[bu] 2
一键启动QGroundControl地面控制站
.IP \[bu] 2
一键启动ROS可视化工具RVIZ
.IP \[bu] 2
智能窗口切换功能
.IP \[bu] 2
悬浮置顶显示
.IP \[bu] 2
现代化UI设计
.SH OPTIONS
该程序不接受命令行参数。
.SH FILES
.I ~/.config/FlightControls/
.RS
用户配置目录
.RE
.SH AUTHOR
FlightControls Team <support@flightcontrols.org>
.SH SEE ALSO
.BR qgroundcontrol (1),
.BR rviz (1)
EOF
    
    # 压缩man页面
    gzip -9 "$DEB_DIR/usr/share/man/man1/flight_controls_launcher.1"
    
    echo -e "${GREEN}✓ man页面创建完成${NC}"
}

# 创建控制文件
create_control_file() {
    echo -e "${YELLOW}创建DEBIAN控制文件...${NC}"
    
    # 计算安装大小
    INSTALLED_SIZE=$(du -sk "$DEB_DIR" | cut -f1)
    
    cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: $PROJECT_NAME
Version: $VERSION
Section: devel
Priority: optional
Architecture: amd64
Depends: libqt5core5a (>= 5.9.0), libqt5gui5 (>= 5.9.0), libqt5widgets5 (>= 5.9.0), libx11-6, libc6 (>= 2.27)
Recommends: wmctrl, xdotool, qgroundcontrol
Suggests: ros-melodic-rviz | ros-noetic-rviz
Installed-Size: $INSTALLED_SIZE
Maintainer: $MAINTAINER
Description: $DESCRIPTION
 基于Qt的飞行控制应用程序浮动启动器，提供统一的按钮界面来启动
 QGroundControl和RVIZ应用程序。
 .
 主要特性：
  * 悬浮置顶显示
  * 一键启动QGC和RVIZ
  * 智能窗口切换
  * 现代化UI设计
  * 跨平台支持
Homepage: $HOMEPAGE
EOF
    
    echo -e "${GREEN}✓ 控制文件创建完成${NC}"
}

# 创建安装后脚本
create_postinst_script() {
    echo -e "${YELLOW}创建安装后脚本...${NC}"
    
    cat > "$DEB_DIR/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

case "$1" in
    configure)
        # 更新桌面数据库
        if command -v update-desktop-database >/dev/null 2>&1; then
            update-desktop-database -q /usr/share/applications || true
        fi
        
        # 更新图标缓存
        if command -v gtk-update-icon-cache >/dev/null 2>&1; then
            gtk-update-icon-cache -q /usr/share/pixmaps || true
        fi
        
        # 更新man数据库
        if command -v mandb >/dev/null 2>&1; then
            mandb -q || true
        fi
        
        echo "FlightControls Launcher 安装完成！"
        echo "您可以从应用程序菜单启动，或运行: flight_controls_launcher"
        ;;
esac

exit 0
EOF
    
    chmod 755 "$DEB_DIR/DEBIAN/postinst"
    
    echo -e "${GREEN}✓ 安装后脚本创建完成${NC}"
}

# 创建卸载前脚本
create_prerm_script() {
    echo -e "${YELLOW}创建卸载前脚本...${NC}"
    
    cat > "$DEB_DIR/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e

case "$1" in
    remove|upgrade|deconfigure)
        # 停止可能运行的实例
        pkill -f flight_controls_launcher || true
        ;;
esac

exit 0
EOF
    
    chmod 755 "$DEB_DIR/DEBIAN/prerm"
    
    echo -e "${GREEN}✓ 卸载前脚本创建完成${NC}"
}

# 创建卸载后脚本
create_postrm_script() {
    echo -e "${YELLOW}创建卸载后脚本...${NC}"
    
    cat > "$DEB_DIR/DEBIAN/postrm" << 'EOF'
#!/bin/bash
set -e

case "$1" in
    remove)
        # 更新桌面数据库
        if command -v update-desktop-database >/dev/null 2>&1; then
            update-desktop-database -q /usr/share/applications || true
        fi
        
        # 更新图标缓存
        if command -v gtk-update-icon-cache >/dev/null 2>&1; then
            gtk-update-icon-cache -q /usr/share/pixmaps || true
        fi
        ;;
    purge)
        # 清理用户配置（可选）
        echo "如需清理用户配置，请手动删除 ~/.config/FlightControls/ 目录"
        ;;
esac

exit 0
EOF
    
    chmod 755 "$DEB_DIR/DEBIAN/postrm"
    
    echo -e "${GREEN}✓ 卸载后脚本创建完成${NC}"
}

# 构建DEB包
build_deb_package() {
    echo -e "${YELLOW}构建DEB包...${NC}"
    
    cd "$PACKAGE_DIR"
    
    # 设置正确的权限
    find deb -type d -exec chmod 755 {} \;
    find deb -type f -exec chmod 644 {} \;
    chmod 755 deb/usr/bin/flight_controls_launcher
    chmod 755 deb/DEBIAN/postinst deb/DEBIAN/prerm deb/DEBIAN/postrm
    
    # 构建包
    DEB_FILE="${PROJECT_NAME}_${VERSION}_amd64.deb"
    fakeroot dpkg-deb --build deb "$DEB_FILE"
    
    echo -e "${GREEN}✓ DEB包构建完成: $DEB_FILE${NC}"
}

# 验证DEB包
verify_package() {
    echo -e "${YELLOW}验证DEB包...${NC}"
    
    cd "$PACKAGE_DIR"
    DEB_FILE="${PROJECT_NAME}_${VERSION}_amd64.deb"
    
    # 检查包信息
    echo -e "${BLUE}包信息:${NC}"
    dpkg-deb --info "$DEB_FILE"
    
    echo -e "${BLUE}包内容:${NC}"
    dpkg-deb --contents "$DEB_FILE"
    
    # 检查包完整性
    if dpkg-deb --fsys-tarfile "$DEB_FILE" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ 包完整性验证通过${NC}"
    else
        echo -e "${RED}✗ 包完整性验证失败${NC}"
        exit 1
    fi
}

# 显示安装说明
show_install_instructions() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}DEB包创建成功！${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    cd "$PACKAGE_DIR"
    DEB_FILE="${PROJECT_NAME}_${VERSION}_amd64.deb"
    
    echo -e "${YELLOW}包文件位置:${NC}"
    echo "$(pwd)/$DEB_FILE"
    
    echo -e "${YELLOW}安装命令:${NC}"
    echo "sudo dpkg -i $DEB_FILE"
    echo "sudo apt-get install -f  # 如果有依赖问题"
    
    echo -e "${YELLOW}卸载命令:${NC}"
    echo "sudo apt-get remove $PROJECT_NAME"
    
    echo -e "${YELLOW}启动命令:${NC}"
    echo "flight_controls_launcher"
    echo "或从应用程序菜单中找到 'FlightControls Launcher'"
    
    echo -e "${YELLOW}包大小:${NC}"
    ls -lh "$DEB_FILE" | awk '{print $5}'
}

# 主函数
main() {
    check_dependencies
    clean_build
    build_project
    create_deb_structure
    install_files
    create_desktop_file
    create_icon
    create_man_page
    create_control_file
    create_postinst_script
    create_prerm_script
    create_postrm_script
    build_deb_package
    verify_package
    show_install_instructions
}

# 运行主函数
main "$@"