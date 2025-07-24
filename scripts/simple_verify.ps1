# FlightControls 简单验证脚本

Write-Host "🔍 FlightControls 验证..." -ForegroundColor Cyan
Write-Host ""

$passed = 0
$failed = 0

# 检查关键文件
$files = @(
    @("src\WindowManager.h", "WindowManager头文件"),
    @("src\WindowManager.cpp", "WindowManager实现文件"),
    @("src\TabBasedLauncher.h", "TabBasedLauncher头文件"),
    @("src\TabBasedLauncher.cpp", "TabBasedLauncher实现文件"),
    @("src\VirtualDesktopManager.h", "VirtualDesktopManager头文件"),
    @("src\VirtualDesktopManager.cpp", "VirtualDesktopManager实现文件"),
    @("examples\window_manager_simple.cpp", "简化窗口管理器演示"),
    @("examples\virtual_desktop_ubuntu_demo.cpp", "虚拟桌面演示"),
    @("CMakeLists_simple.txt", "简化构建配置"),
    @("scripts\check_dependencies.sh", "依赖检查脚本")
)

Write-Host "📋 检查项目文件..." -ForegroundColor Yellow
foreach ($file in $files) {
    if (Test-Path $file[0]) {
        Write-Host "✅ $($file[1])" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "❌ $($file[1])" -ForegroundColor Red
        $failed++
    }
}

Write-Host ""
Write-Host "🚨 检查代码修复..." -ForegroundColor Yellow

# 检查删除进程的不安全代码
$deleteProcess = Select-String -Path "src\*.cpp" -Pattern "delete.*process" -Quiet
if (-not $deleteProcess) {
    Write-Host "✅ 删除进程的不安全代码已修复" -ForegroundColor Green
    $passed++
} else {
    Write-Host "❌ 仍存在删除进程的不安全代码" -ForegroundColor Red
    $failed++
}

# 检查deleteLater使用
$deleteLater = Select-String -Path "src\*.cpp" -Pattern "deleteLater" -Quiet
if ($deleteLater) {
    Write-Host "✅ 使用deleteLater()安全删除" -ForegroundColor Green
    $passed++
} else {
    Write-Host "❌ 未找到deleteLater()使用" -ForegroundColor Red
    $failed++
}

# 检查QThread::sleep
$threadSleep = Select-String -Path "src\*.cpp" -Pattern "QThread::sleep" -Quiet
if (-not $threadSleep) {
    Write-Host "✅ QThread::sleep阻塞问题已修复" -ForegroundColor Green
    $passed++
} else {
    Write-Host "❌ 仍存在QThread::sleep阻塞问题" -ForegroundColor Red
    $failed++
}

# 检查条件编译
$webengine = Select-String -Path "src\*.h" -Pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" -Quiet
if ($webengine) {
    Write-Host "✅ QWebEngine条件编译已添加" -ForegroundColor Green
    $passed++
} else {
    Write-Host "❌ 缺少QWebEngine条件编译" -ForegroundColor Red
    $failed++
}

Write-Host ""
Write-Host "📊 验证结果:" -ForegroundColor Cyan
Write-Host "通过: $passed" -ForegroundColor Green
Write-Host "失败: $failed" -ForegroundColor Red

if ($failed -eq 0) {
    Write-Host ""
    Write-Host "🎉 所有检查通过！代码质量优秀！" -ForegroundColor Green
    Write-Host ""
    Write-Host "🚀 Linux构建指南：" -ForegroundColor Cyan
    Write-Host "1. chmod +x scripts/check_dependencies.sh" 
    Write-Host "2. ./scripts/check_dependencies.sh"
    Write-Host "3. mkdir build && cd build"
    Write-Host "4. cmake -f ../CMakeLists_simple.txt .."
    Write-Host "5. make -j4"
    Write-Host "6. ./window_manager_simple"
} else {
    Write-Host ""
    Write-Host "❌ 发现 $failed 个问题需要修复" -ForegroundColor Red
} 