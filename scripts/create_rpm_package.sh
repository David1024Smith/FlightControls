#!/bin/bash

# FlightControls Launcher RPM包创建脚本
# 适用于CentOS/RHEL/Fedora系统

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 项目信息
PROJECT_NAME="flight-controls-launcher"
VERSION="5.0.0"
RELEASE="1"
SUMMARY="基于Qt的飞行控制应用程序浮动启动器"
LICENSE="MIT"
URL="https://github.com/flightcontrols/launcher"

# 目录设置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
RPM_BUILD_DIR="$PROJECT_ROOT/rpmbuild"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}FlightControls Launcher RPM包创建工具${NC}"
echo -e "${BLUE}========================================${NC}"

# 检查依赖
check_dependencies() {
    echo -e "${YELLOW}检查构建依赖...${NC}"
    
    local missing_deps=()
    
    # 检查基本工具
    for cmd in cmake make g++ rpmbuild rpmdev-setuptree; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done
    
    # 检查Qt5
    if ! pkg-config --exists Qt5Core Qt5Widgets Qt5Gui; then
        missing_deps+=("qt5-qtbase-devel")
    fi
    
    # 检查X11
    if ! pkg-config --exists x11; then
        missing_deps+=("libX11-devel")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${RED}缺少以下依赖:${NC}"
        printf '%s\n' "${missing_deps[@]}"
        echo -e "${YELLOW}请运行以下命令安装:${NC}"
        echo "sudo yum groupinstall 'Development Tools'"
        echo "sudo yum install ${missing_deps[*]}"
        exit 1
    fi
    
    echo -e "${GREEN}✓ 所有依赖已满足${NC}"
}

# 设置RPM构建环境
setup_rpm_environment() {
    echo -e "${YELLOW}设置RPM构建环境...${NC}"
    
    # 创建RPM构建目录结构
    mkdir -p "$RPM_BUILD_DIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
    
    echo -e "${GREEN}✓ RPM构建环境设置完成${NC}"
}

# 创建源码包
create_source_tarball() {
    echo -e "${YELLOW}创建源码包...${NC}"
    
    cd "$PROJECT_ROOT"
    
    # 创建源码目录
    SOURCE_DIR="${PROJECT_NAME}-${VERSION}"
    mkdir -p "/tmp/$SOURCE_DIR"
    
    # 复制源码文件
    cp -r src CMakeLists.txt README.md LICENSE "/tmp/$SOURCE_DIR/"
    
    # 创建tar包
    cd /tmp
    tar -czf "$RPM_BUILD_DIR/SOURCES/${PROJECT_NAME}-${VERSION}.tar.gz" "$SOURCE_DIR"
    
    # 清理临时目录
    rm -rf "/tmp/$SOURCE_DIR"
    
    echo -e "${GREEN}✓ 源码包创建完成${NC}"
}

# 创建RPM spec文件
create_spec_file() {
    echo -e "${YELLOW}创建RPM spec文件...${NC}"
    
    cat > "$RPM_BUILD_DIR/SPECS/${PROJECT_NAME}.spec" << EOF
Name:           $PROJECT_NAME
Version:        $VERSION
Release:        $RELEASE%{?dist}
Summary:        $SUMMARY

License:        $LICENSE
URL:            $URL
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.10
BuildRequires:  gcc-c++
BuildRequires:  qt5-qtbase-devel
BuildRequires:  libX11-devel
BuildRequires:  desktop-file-utils

Requires:       qt5-qtbase >= 5.9
Requires:       libX11
Requires:       wmctrl
Requires:       xdotool

%description
基于Qt的飞行控制应用程序浮动启动器，提供统一的按钮界面来启动
QGroundControl和RVIZ应用程序。

主要特性：
* 悬浮置顶显示
* 一键启动QGC和RVIZ
* 智能窗口切换
* 现代化UI设计
* 跨平台支持

%prep
%setup -q

%build
mkdir -p build
cd build
%cmake -DCMAKE_BUILD_TYPE=Release ..
%make_build

%install
cd build
%make_install

# 创建桌面文件
mkdir -p %{buildroot}%{_datadir}/applications
cat > %{buildroot}%{_datadir}/applications/%{name}.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=FlightControls Launcher
Name[zh_CN]=飞行控制启动器
Comment=Flight Controls Floating Launcher
Comment[zh_CN]=基于Qt的飞行控制应用程序浮动启动器
Exec=%{_bindir}/flight_controls_launcher
Terminal=false
Categories=Development;Engineering;
Keywords=flight;controls;launcher;qgroundcontrol;rviz;ros;
StartupNotify=true
DESKTOP_EOF

# 创建图标
mkdir -p %{buildroot}%{_datadir}/pixmaps
cat > %{buildroot}%{_datadir}/pixmaps/%{name}.svg << 'ICON_EOF'
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
ICON_EOF

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%license LICENSE
%doc README.md
%{_bindir}/flight_controls_launcher
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.svg

%post
/bin/touch --no-create %{_datadir}/applications &>/dev/null || :
if [ \$1 -eq 1 ] ; then
    /usr/bin/update-desktop-database %{_datadir}/applications &> /dev/null || :
fi

%postun
if [ \$1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/applications &>/dev/null
    /usr/bin/update-desktop-database %{_datadir}/applications &> /dev/null || :
fi

%posttrans
/usr/bin/update-desktop-database %{_datadir}/applications &> /dev/null || :

%changelog
* $(date +'%a %b %d %Y') FlightControls Team <support@flightcontrols.org> - $VERSION-$RELEASE
- Initial RPM package
- 基于Qt的浮动启动器
- 支持QGroundControl和RVIZ启动
- 智能窗口管理功能
EOF
    
    echo -e "${GREEN}✓ RPM spec文件创建完成${NC}"
}

# 构建RPM包
build_rpm_package() {
    echo -e "${YELLOW}构建RPM包...${NC}"
    
    cd "$RPM_BUILD_DIR"
    
    # 构建源码RPM
    rpmbuild --define "_topdir $(pwd)" -bs "SPECS/${PROJECT_NAME}.spec"
    
    # 构建二进制RPM
    rpmbuild --define "_topdir $(pwd)" -bb "SPECS/${PROJECT_NAME}.spec"
    
    echo -e "${GREEN}✓ RPM包构建完成${NC}"
}

# 验证RPM包
verify_package() {
    echo -e "${YELLOW}验证RPM包...${NC}"
    
    cd "$RPM_BUILD_DIR"
    
    # 查找生成的RPM文件
    RPM_FILE=$(find RPMS -name "*.rpm" -type f | head -1)
    
    if [ -z "$RPM_FILE" ]; then
        echo -e "${RED}✗ 未找到生成的RPM文件${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}包信息:${NC}"
    rpm -qip "$RPM_FILE"
    
    echo -e "${BLUE}包内容:${NC}"
    rpm -qlp "$RPM_FILE"
    
    echo -e "${GREEN}✓ 包验证完成${NC}"
}

# 显示安装说明
show_install_instructions() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${GREEN}RPM包创建成功！${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    cd "$RPM_BUILD_DIR"
    RPM_FILE=$(find RPMS -name "*.rpm" -type f | head -1)
    SRPM_FILE=$(find SRPMS -name "*.rpm" -type f | head -1)
    
    echo -e "${YELLOW}二进制RPM包:${NC}"
    echo "$(pwd)/$RPM_FILE"
    
    echo -e "${YELLOW}源码RPM包:${NC}"
    echo "$(pwd)/$SRPM_FILE"
    
    echo -e "${YELLOW}安装命令:${NC}"
    echo "sudo yum localinstall $RPM_FILE"
    echo "# 或者"
    echo "sudo rpm -ivh $RPM_FILE"
    
    echo -e "${YELLOW}卸载命令:${NC}"
    echo "sudo yum remove $PROJECT_NAME"
    echo "# 或者"
    echo "sudo rpm -e $PROJECT_NAME"
    
    echo -e "${YELLOW}启动命令:${NC}"
    echo "flight_controls_launcher"
    echo "或从应用程序菜单中找到 'FlightControls Launcher'"
    
    echo -e "${YELLOW}包大小:${NC}"
    ls -lh "$RPM_FILE" | awk '{print $5}'
}

# 主函数
main() {
    check_dependencies
    setup_rpm_environment
    create_source_tarball
    create_spec_file
    build_rpm_package
    verify_package
    show_install_instructions
}

# 运行主函数
main "$@"