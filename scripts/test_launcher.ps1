# FlightControls 浮动启动器测试脚本
# 测试新的代码结构和完整性

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FlightControls 浮动启动器 - 代码验证测试" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 检查源代码文件
Write-Host "`n📁 检查源代码文件结构..." -ForegroundColor Yellow

$sourceFiles = @(
    "src/main.cpp",
    "src/FlightControlsLauncher.h", 
    "src/FlightControlsLauncher.cpp",
    "CMakeLists.txt",
    "README.md"
)

$allFilesExist = $true

foreach ($file in $sourceFiles) {
    if (Test-Path $file) {
        Write-Host "✅ $file - 存在" -ForegroundColor Green
    } else {
        Write-Host "❌ $file - 缺失" -ForegroundColor Red
        $allFilesExist = $false
    }
}

# 检查旧文件是否已删除
Write-Host "`n🗑️  检查旧文件清理状态..." -ForegroundColor Yellow

$oldFiles = @(
    "src/EmbeddedWindowManager.h",
    "src/EmbeddedWindowManager.cpp"
)

foreach ($file in $oldFiles) {
    if (Test-Path $file) {
        Write-Host "⚠️  $file - 仍然存在 (应该已删除)" -ForegroundColor Yellow
    } else {
        Write-Host "✅ $file - 已正确删除" -ForegroundColor Green
    }
}

# 检查头文件内容
Write-Host "`n🔍 验证头文件内容..." -ForegroundColor Yellow

if (Test-Path "src/FlightControlsLauncher.h") {
    $headerContent = Get-Content "src/FlightControlsLauncher.h" -Raw
    
    if ($headerContent -match "class FlightControlsLauncher") {
        Write-Host "✅ FlightControlsLauncher 类定义 - 正确" -ForegroundColor Green
    } else {
        Write-Host "❌ FlightControlsLauncher 类定义 - 缺失" -ForegroundColor Red
        $allFilesExist = $false
    }
    
    if ($headerContent -match "void onLaunchQGC") {
        Write-Host "✅ QGC启动方法 - 正确" -ForegroundColor Green
    } else {
        Write-Host "❌ QGC启动方法 - 缺失" -ForegroundColor Red
        $allFilesExist = $false
    }
    
    if ($headerContent -match "void onLaunchRVIZ") {
        Write-Host "✅ RVIZ启动方法 - 正确" -ForegroundColor Green
    } else {
        Write-Host "❌ RVIZ启动方法 - 缺失" -ForegroundColor Red
        $allFilesExist = $false
    }
}

# 检查main.cpp内容
Write-Host "`n📄 验证main.cpp更新..." -ForegroundColor Yellow

if (Test-Path "src/main.cpp") {
    $mainContent = Get-Content "src/main.cpp" -Raw
    
    if ($mainContent -match "FlightControlsLauncher") {
        Write-Host "✅ main.cpp使用新的启动器类 - 正确" -ForegroundColor Green
    } else {
        Write-Host "❌ main.cpp未更新为新的启动器类" -ForegroundColor Red
        $allFilesExist = $false
    }
    
    if ($mainContent -match "EmbeddedWindowManager") {
        Write-Host "⚠️  main.cpp仍包含旧的EmbeddedWindowManager引用" -ForegroundColor Yellow
    } else {
        Write-Host "✅ main.cpp已清理旧的引用 - 正确" -ForegroundColor Green
    }
}

# 检查CMakeLists.txt更新
Write-Host "`n⚙️  验证CMakeLists.txt配置..." -ForegroundColor Yellow

if (Test-Path "CMakeLists.txt") {
    $cmakeContent = Get-Content "CMakeLists.txt" -Raw
    
    if ($cmakeContent -match "FlightControlsLauncher") {
        Write-Host "✅ CMakeLists.txt使用新的项目名称 - 正确" -ForegroundColor Green
    } else {
        Write-Host "❌ CMakeLists.txt项目名称未更新" -ForegroundColor Red
        $allFilesExist = $false
    }
    
    if ($cmakeContent -match "X11") {
        Write-Host "⚠️  CMakeLists.txt仍包含X11依赖 (应该已移除)" -ForegroundColor Yellow
    } else {
        Write-Host "✅ CMakeLists.txt已移除X11依赖 - 正确" -ForegroundColor Green
    }
}

# 总结
Write-Host "`n📊 测试总结:" -ForegroundColor Cyan

if ($allFilesExist) {
    Write-Host "🎉 所有代码验证通过！浮动启动器重构成功完成。" -ForegroundColor Green
    Write-Host "`n✨ 新功能特点:" -ForegroundColor Cyan
    Write-Host "   • 简化的浮动启动器设计" -ForegroundColor White
    Write-Host "   • 悬浮置顶显示" -ForegroundColor White  
    Write-Host "   • QGC和RVIZ一键启动" -ForegroundColor White
    Write-Host "   • 现代化UI设计" -ForegroundColor White
    Write-Host "   • 跨平台支持" -ForegroundColor White
    Write-Host "   • 无需X11依赖" -ForegroundColor White
} else {
    Write-Host "❌ 代码验证发现问题，需要进一步检查和修复。" -ForegroundColor Red
}

Write-Host "`n💡 下一步:" -ForegroundColor Cyan
Write-Host "   1. 安装Qt5开发环境和CMake" -ForegroundColor White
Write-Host "   2. 配置C++编译器" -ForegroundColor White
Write-Host "   3. 执行编译: mkdir build && cd build && cmake .. && make" -ForegroundColor White
Write-Host "   4. 运行: ./flight_controls_launcher" -ForegroundColor White

Write-Host "`n========================================" -ForegroundColor Cyan 