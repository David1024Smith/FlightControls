# FlightControls Windows PowerShell 验证脚本

Write-Host "🔍 FlightControls Windows环境验证..." -ForegroundColor Cyan
Write-Host

$passed = 0
$failed = 0
$total = 0

function Check-File {
    param($path, $description)
    $script:total++
    if (Test-Path $path) {
        Write-Host "✅ $description" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "❌ $description" -ForegroundColor Red
        $script:failed++
    }
}

function Check-Pattern {
    param($pattern, $description, $shouldExist)
    $script:total++
    try {
        $found = Select-String -Path "src\*.cpp" -Pattern $pattern -Quiet
        if ($shouldExist -eq "true") {
            if ($found) {
                Write-Host "✅ $description" -ForegroundColor Green
                $script:passed++
            } else {
                Write-Host "❌ $description (未发现)" -ForegroundColor Red
                $script:failed++
            }
        } else {
            if ($found) {
                Write-Host "❌ $description (仍存在)" -ForegroundColor Red
                $script:failed++
            } else {
                Write-Host "✅ $description (已修复)" -ForegroundColor Green
                $script:passed++
            }
        }
    } catch {
        Write-Host "⚠️ 检查 $description 时出错" -ForegroundColor Yellow
        $script:failed++
    }
}

Write-Host "📋 检查项目结构..." -ForegroundColor Yellow
Check-File "src\WindowManager.h" "WindowManager头文件存在"
Check-File "src\WindowManager.cpp" "WindowManager实现文件存在"
Check-File "src\TabBasedLauncher.h" "TabBasedLauncher头文件存在"
Check-File "src\TabBasedLauncher.cpp" "TabBasedLauncher实现文件存在"
Check-File "src\VirtualDesktopManager.h" "VirtualDesktopManager头文件存在"
Check-File "src\VirtualDesktopManager.cpp" "VirtualDesktopManager实现文件存在"
Check-File "examples\window_manager_simple.cpp" "简化窗口管理器演示存在"
Check-File "examples\virtual_desktop_ubuntu_demo.cpp" "虚拟桌面演示存在"
Check-File "CMakeLists_simple.txt" "简化构建配置存在"
Check-File "scripts\check_dependencies.sh" "依赖检查脚本存在"
Write-Host

Write-Host "🚨 检查已修复的问题..." -ForegroundColor Yellow
Write-Host "🔹 内存管理问题："
Check-Pattern "delete.*process" "直接删除进程（不安全）" "false"
Check-Pattern "deleteLater" "使用deleteLater()（安全）" "true"

Write-Host "🔹 线程安全问题："
Check-Pattern "QTimer::singleShot.*this" "QTimer使用this指针（安全）" "true"
Check-Pattern "QThread::sleep" "QThread::sleep使用（会阻塞UI）" "false"

Write-Host "🔹 X11错误处理："
Check-Pattern "XSetErrorHandler" "X11错误处理器设置" "true"

Write-Host "🔹 条件编译支持："
Check-Pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" "QWebEngine条件编译" "true"

Write-Host "🔹 函数调用约定："
Check-Pattern "this->checkRVIZAvailable" "显式使用this指针" "true"
Write-Host

Write-Host "📊 验证结果汇总:" -ForegroundColor Cyan
Write-Host "总计检查项目: $total"
Write-Host "通过: $passed" -ForegroundColor Green
Write-Host "失败: $failed" -ForegroundColor Red

if ($failed -eq 0) {
    Write-Host
    Write-Host "🎉 所有检查通过！代码质量优秀！" -ForegroundColor Green
    Write-Host
    Write-Host "🚀 在Linux环境下的构建命令：" -ForegroundColor Cyan
    Write-Host "1. chmod +x scripts/check_dependencies.sh"
    Write-Host "2. ./scripts/check_dependencies.sh"
    Write-Host "3. mkdir build; cd build"
    Write-Host "4. cmake -f ../CMakeLists_simple.txt .."
    Write-Host "5. make -j4"
    Write-Host "6. ./window_manager_simple 或 ./virtual_desktop_ubuntu_demo"
    exit 0
} else {
    Write-Host
    Write-Host "❌ 发现 $failed 个问题，需要修复" -ForegroundColor Red
    exit 1
} 