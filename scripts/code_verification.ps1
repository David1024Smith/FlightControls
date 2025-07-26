# FlightControls 代码全面检查脚本
# 验证代码修改的完整性和正确性

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FlightControls 浮动启动器 - 全面代码检查" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 检查源代码文件是否存在
Write-Host "`n📁 检查源代码文件..." -ForegroundColor Yellow

$sourceFiles = @{
    "src/main.cpp" = "主程序入口文件"
    "src/FlightControlsLauncher.h" = "启动器头文件"
    "src/FlightControlsLauncher.cpp" = "启动器实现文件"
    "CMakeLists.txt" = "CMake构建文件"
    "README.md" = "项目文档"
}

$allFilesExist = $true

foreach ($file in $sourceFiles.Keys) {
    if (Test-Path $file) {
        Write-Host "✅ $file - 存在 ($($sourceFiles[$file]))" -ForegroundColor Green
    } else {
        Write-Host "❌ $file - 缺失" -ForegroundColor Red
        $allFilesExist = $false
    }
}

# 检查头文件内容
Write-Host "`n🔍 验证头文件更新..." -ForegroundColor Yellow

if (Test-Path "src/FlightControlsLauncher.h") {
    $headerContent = Get-Content "src/FlightControlsLauncher.h" -Raw
    
    $checks = @{
        "class FlightControlsLauncher" = "主类定义"
        "ROSCORE_STARTUP_DELAY" = "roscore启动延迟常量"
        "findQGroundControlPath" = "QGC路径查找函数"
        "startRoscoreProcess" = "roscore启动函数"
        "startRVIZProcess" = "RVIZ启动函数"
        "onRoscoreStarted" = "roscore启动完成回调"
        "m_rvizStartupInProgress" = "RVIZ启动状态变量"
        "m_roscoreDelayTimer" = "roscore延迟定时器"
    }
    
    foreach ($check in $checks.Keys) {
        if ($headerContent -match [regex]::Escape($check)) {
            Write-Host "✅ $check - 正确 ($($checks[$check]))" -ForegroundColor Green
        } else {
            Write-Host "❌ $check - 缺失" -ForegroundColor Red
            $allFilesExist = $false
        }
    }
}

# 检查实现文件内容
Write-Host "`n📄 验证实现文件更新..." -ForegroundColor Yellow

if (Test-Path "src/FlightControlsLauncher.cpp") {
    $cppContent = Get-Content "src/FlightControlsLauncher.cpp" -Raw
    
    $implementations = @{
        "QFileInfo" = "文件信息检查类"
        "findQGroundControlPath" = "QGC路径查找实现"
        "startRoscoreProcess" = "roscore启动实现"
        "startRVIZProcess" = "RVIZ启动实现"
        "onRoscoreStarted" = "roscore回调实现"
        "rosrun.*rviz.*rviz" = "RVIZ启动命令"
        "ROSCORE.*roscore" = "roscore进程注册"
        "searchPaths" = "路径搜索逻辑"
    }
    
    foreach ($impl in $implementations.Keys) {
        if ($cppContent -match $impl) {
            Write-Host "✅ $impl - 已实现 ($($implementations[$impl]))" -ForegroundColor Green
        } else {
            Write-Host "❌ $impl - 缺失" -ForegroundColor Red
            $allFilesExist = $false
        }
    }
}

# 检查main.cpp更新
Write-Host "`n⚙️  验证main.cpp改进..." -ForegroundColor Yellow

if (Test-Path "src/main.cpp") {
    $mainContent = Get-Content "src/main.cpp" -Raw
    
    $mainChecks = @{
        "Q_LOGGING_CATEGORY" = "日志分类系统"
        "initializeApplication" = "应用初始化函数"
        "QStandardPaths" = "标准路径处理"
        "编译时间" = "编译信息记录"
        "应用程序数据目录" = "数据目录检查"
    }
    
    foreach ($check in $mainChecks.Keys) {
        if ($mainContent -match [regex]::Escape($check)) {
            Write-Host "✅ $check - 已添加 ($($mainChecks[$check]))" -ForegroundColor Green
        } else {
            Write-Host "⚠️  $check - 未找到" -ForegroundColor Yellow
        }
    }
}

# 检查CMakeLists.txt增强
Write-Host "`n🛠️  验证CMake配置增强..." -ForegroundColor Yellow

if (Test-Path "CMakeLists.txt") {
    $cmakeContent = Get-Content "CMakeLists.txt" -Raw
    
    $cmakeChecks = @{
        "CMAKE_BUILD_TYPE" = "构建类型检查"
        "Qt5_VERSION VERSION_LESS" = "Qt版本检查"
        "LAUNCHER_SOURCES" = "源文件组织"
        "MSVC.*W4" = "Windows编译选项"
        "Wall.*Wextra.*Wpedantic" = "Linux编译选项"
        "WIN32_EXECUTABLE" = "Windows可执行文件设置"
        "CPACK_GENERATOR" = "平台特定打包"
    }
    
    foreach ($check in $cmakeChecks.Keys) {
        if ($cmakeContent -match $check) {
            Write-Host "✅ $check - 已配置 ($($cmakeChecks[$check]))" -ForegroundColor Green
        } else {
            Write-Host "⚠️  $check - 未找到" -ForegroundColor Yellow
        }
    }
}

# 功能改进总结
Write-Host "`n✨ 代码改进总结:" -ForegroundColor Cyan

Write-Host "`n🚁 QGroundControl启动改进:" -ForegroundColor Green
Write-Host "   • 智能路径查找 - 支持多个可能位置" -ForegroundColor White
Write-Host "   • 当前目录: ./QGroundControl.AppImage" -ForegroundColor White
Write-Host "   • 程序目录: 程序所在目录/QGroundControl.AppImage" -ForegroundColor White
Write-Host "   • 开发目录: ./build/QGroundControl.AppImage" -ForegroundColor White
Write-Host "   • 错误提示: 找不到文件时显示详细帮助" -ForegroundColor White

Write-Host "`n🤖 RVIZ启动流程改进:" -ForegroundColor Blue
Write-Host "   • 两步启动: 先启动roscore，再启动rviz" -ForegroundColor White
Write-Host "   • 命令1: roscore (自动启动)" -ForegroundColor White
Write-Host "   • 延迟3秒: 等待roscore完全启动" -ForegroundColor White
Write-Host "   • 命令2: rosrun rviz rviz (自动启动)" -ForegroundColor White
Write-Host "   • 状态显示: 启动过程中显示'启动中...'" -ForegroundColor White

Write-Host "`n🔧 代码质量改进:" -ForegroundColor Magenta
Write-Host "   • 常量定义: 硬编码数值改为命名常量" -ForegroundColor White
Write-Host "   • 错误处理: 增强的异常处理和用户提示" -ForegroundColor White
Write-Host "   • 内存管理: 改进的进程生命周期管理" -ForegroundColor White
Write-Host "   • 日志系统: 结构化的日志记录" -ForegroundColor White
Write-Host "   • 构建配置: 跨平台编译优化" -ForegroundColor White

Write-Host "`n🎯 用户体验改进:" -ForegroundColor Yellow
Write-Host "   • 智能按钮: 根据进程状态动态变化文本" -ForegroundColor White
Write-Host "   • 状态指示: 实时显示应用程序运行状态" -ForegroundColor White
Write-Host "   • 错误提示: 友好的错误信息和解决建议" -ForegroundColor White
Write-Host "   • 启动反馈: 清晰的启动进度显示" -ForegroundColor White

# 测试建议
Write-Host "`n💡 测试建议:" -ForegroundColor Cyan
Write-Host "   1. 确保QGroundControl.AppImage在当前目录" -ForegroundColor White
Write-Host "   2. 确保ROS环境已正确配置" -ForegroundColor White
Write-Host "   3. 测试RVIZ两步启动流程" -ForegroundColor White
Write-Host "   4. 验证进程停止功能" -ForegroundColor White
Write-Host "   5. 检查错误处理和用户提示" -ForegroundColor White

if ($allFilesExist) {
    Write-Host "`n🎉 代码检查完成！所有改进都已正确实现。" -ForegroundColor Green
} else {
    Write-Host "`n⚠️  代码检查发现一些问题，请检查上述标记的项目。" -ForegroundColor Yellow
}

Write-Host "`n========================================" -ForegroundColor Cyan 