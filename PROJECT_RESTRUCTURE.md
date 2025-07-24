# FlightControls 项目重构说明

## 🔄 项目重构完成

### 重构目标
将混合的原始文件和替代方案文件分离，移除原始X11窗口嵌入方案，专注于新的替代方案。

### 文件结构变更

#### ✅ 保留文件（新替代方案）
```
src/
├── WindowManager.h           # 方案1: 窗口管理
├── WindowManager.cpp
├── TabBasedLauncher.h        # 方案2: Tab界面组织
├── TabBasedLauncher.cpp
├── VirtualDesktopManager.h   # 方案3: 虚拟桌面
└── VirtualDesktopManager.cpp

examples/
├── window_manager_simple.cpp     # 窗口管理简单演示
├── virtual_desktop_ubuntu_demo.cpp # 虚拟桌面演示
└── alternative_main.cpp           # 综合演示程序

CMakeLists.txt                     # 主构建配置
CMakeLists_simple.txt              # 备用简化配置
```

#### 📁 移动文件（原始实现）
```
original_files/
├── main.cpp                  # 原始主程序
├── MainWindow.h/cpp          # 原始主窗口
├── ProcessManager.h/cpp      # 原始进程管理
├── WindowEmbedder.h/cpp      # 原始窗口嵌入
├── ApplicationSwitcher.h/cpp # 原始应用切换器
└── X11Helper.h/cpp           # 原始X11辅助函数

CMakeLists_original.txt       # 原始构建配置
```

#### 🗑️ 删除文件
```
CMakeLists_alternatives.txt   # 已删除，功能合并到主配置
```

### 构建配置变更

#### 新的主要构建目标
1. **flight_controls_launcher** - 主要启动器
   - 包含所有三种方案的选择界面
   - 自动处理Qt WebEngine依赖
   - 在缺少WebEngine时禁用Tab方案

2. **flight_controls_window_manager** - 窗口管理演示
   - 独立的窗口管理方案演示
   - 最小依赖，兼容性最好

3. **flight_controls_virtual_desktop** - 虚拟桌面演示
   - Ubuntu 18.04 GNOME专用演示
   - 包含桌面环境检测和配置

#### 条件编译支持
- 自动检测Qt WebEngine可用性
- 在缺少WebEngine时构建简化版本
- 提供用户友好的错误提示

### 构建命令

#### Linux环境（推荐）
```bash
# 1. 检查依赖
chmod +x scripts/check_dependencies.sh
./scripts/check_dependencies.sh

# 2. 构建项目
mkdir build && cd build
cmake ..
make -j$(nproc)

# 3. 运行程序
./flight_controls_launcher              # 主启动器
./flight_controls_window_manager        # 窗口管理演示
./flight_controls_virtual_desktop       # 虚拟桌面演示
```

#### Windows环境（验证）
```powershell
# 验证代码质量
powershell -ExecutionPolicy Bypass -File scripts\simple_verify.ps1
```

### 依赖要求

#### 必需依赖
- Qt5 Core, Widgets, Gui (>= 5.12)
- X11 development libraries
- CMake (>= 3.10)
- C++17 编译器

#### 可选依赖
- Qt5 WebEngineWidgets (用于Tab界面方案)
- ROS Melodic (用于RVIZ支持)
- wmctrl, xdotool (用于增强窗口管理)

### 使用建议

#### 生产环境
- 推荐使用 **flight_controls_window_manager** (方案1)
- 最佳兼容性和稳定性
- 资源占用最低

#### 开发环境
- 推荐使用 **flight_controls_launcher** (综合)
- 可以测试所有方案
- 便于功能对比

#### 演示环境
- Ubuntu系统推荐 **flight_controls_virtual_desktop** (方案3)
- 最佳视觉效果
- 充分利用桌面环境特性

### 兼容性矩阵

| 方案 | Ubuntu 18.04 | Ubuntu 20.04+ | 其他Linux | Windows | macOS |
|------|--------------|---------------|-----------|---------|-------|
| 窗口管理 | ✅ | ✅ | ✅ | ❌ | ❌ |
| Tab界面 | ✅ | ✅ | ✅ | ❌ | ❌ |
| 虚拟桌面 | ✅ | ✅ | ⚠️ | ❌ | ❌ |

**说明**: 
- ✅ 完全支持
- ⚠️ 部分支持（取决于桌面环境）
- ❌ 不支持

### 后续改进计划

#### 短期目标
- [ ] 添加自动化测试套件
- [ ] 改进错误处理和用户反馈
- [ ] 优化内存使用和性能

#### 中期目标
- [ ] 支持更多Linux发行版
- [ ] 添加配置文件支持
- [ ] 实现插件架构

#### 长期目标
- [ ] Wayland支持
- [ ] 跨平台兼容性
- [ ] 远程控制功能

### 故障排除
方案1: 基于窗口管理的快速切换 (推荐⭐⭐⭐⭐⭐)
最佳的生产环境解决方案
原理: 通过智能窗口管理实现快速切换，不嵌入窗口
优势: 兼容性极好、稳定性高、切换速度快
适用: 稳定性要求高的生产环境
方案2: 基于Tab Widget的界面组织 (推荐⭐⭐⭐⭐)
最佳的用户体验解决方案
原理: 使用Qt Tab Widget组织界面，提供完整的管理功能
优势: 界面美观、功能丰富、易于使用
适用: 注重用户体验和功能丰富性的场景
方案3: 基于虚拟桌面的解决方案 (推荐⭐⭐⭐)
最佳的性能解决方案
原理: 利用Linux虚拟桌面功能，每个应用独占一个桌面
优势: 零性能开销、原生体验、支持快捷键
适用: 支持虚拟桌面的环境下提供最佳体验
#### 编译错误
1. **找不到Qt5**: 安装 `qt5-default libqt5widgets5-dev`
2. **找不到X11**: 安装 `libx11-dev libxext-dev`
3. **WebEngine错误**: 安装 `libqt5webengine5-dev` 或使用简化版本

#### 运行时错误
1. **DISPLAY错误**: 检查X11环境变量
2. **权限错误**: 确保有X11窗口管理权限
3. **应用启动失败**: 检查QGroundControl和ROS环境

### 联系信息
如有问题，请参考：
- `CODE_FIXES_SUMMARY.md` - 详细修复记录
- `scripts/check_dependencies.sh` - 依赖检查
- `scripts/simple_verify.ps1` - 代码验证

---
**重构完成时间**: 2024年12月  
**项目状态**: 生产就绪  
**推荐方案**: 方案1 (窗口管理) - 最佳兼容性 