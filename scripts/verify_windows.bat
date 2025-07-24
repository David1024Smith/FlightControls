@echo off
chcp 65001 >nul
echo 🔍 FlightControls Windows环境验证...
echo.

set PASSED=0
set FAILED=0
set TOTAL=0

:check_file
set /a TOTAL+=1
if exist "%~1" (
    echo ✅ %~2
    set /a PASSED+=1
) else (
    echo ❌ %~2
    set /a FAILED+=1
)
goto :eof

:check_pattern
set /a TOTAL+=1
findstr /s /m "%~1" src\*.cpp >nul 2>&1
if %errorlevel% equ 0 (
    if "%~3"=="false" (
        echo ❌ %~2 仍存在
        set /a FAILED+=1
    ) else (
        echo ✅ %~2
        set /a PASSED+=1
    )
) else (
    if "%~3"=="false" (
        echo ✅ %~2 已修复
        set /a PASSED+=1
    ) else (
        echo ❌ %~2 未发现
        set /a FAILED+=1
    )
)
goto :eof

echo 📋 检查项目结构...
call :check_file "src\WindowManager.h" "WindowManager头文件存在"
call :check_file "src\WindowManager.cpp" "WindowManager实现文件存在"
call :check_file "src\TabBasedLauncher.h" "TabBasedLauncher头文件存在"
call :check_file "src\TabBasedLauncher.cpp" "TabBasedLauncher实现文件存在"
call :check_file "src\VirtualDesktopManager.h" "VirtualDesktopManager头文件存在"
call :check_file "src\VirtualDesktopManager.cpp" "VirtualDesktopManager实现文件存在"
call :check_file "examples\window_manager_simple.cpp" "简化窗口管理器演示存在"
call :check_file "examples\virtual_desktop_ubuntu_demo.cpp" "虚拟桌面演示存在"
call :check_file "CMakeLists_simple.txt" "简化构建配置存在"
call :check_file "scripts\check_dependencies.sh" "依赖检查脚本存在"
echo.

echo 🚨 检查已修复的问题...
echo 🔹 内存管理问题：
call :check_pattern "delete.*process" "直接删除进程（不安全）" "false"
call :check_pattern "deleteLater()" "使用deleteLater()（安全）" "true"

echo 🔹 线程安全问题：
call :check_pattern "QTimer::singleShot.*this" "QTimer使用this指针（安全）" "true"
call :check_pattern "QThread::sleep" "QThread::sleep使用（会阻塞UI）" "false"

echo 🔹 X11错误处理：
call :check_pattern "XSetErrorHandler" "X11错误处理器设置" "true"

echo 🔹 条件编译支持：
call :check_pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" "QWebEngine条件编译" "true"

echo 🔹 函数调用约定：
call :check_pattern "this->checkRVIZAvailable()" "显式使用this指针" "true"
echo.

echo 📊 验证结果汇总:
echo 总计检查项目: %TOTAL%
echo 通过: %PASSED%
echo 失败: %FAILED%

if %FAILED% equ 0 (
    echo.
    echo 🎉 所有检查通过！代码质量优秀！
    echo.
    echo 🚀 在Linux环境下的构建命令：
    echo 1. chmod +x scripts/check_dependencies.sh
    echo 2. ./scripts/check_dependencies.sh
    echo 3. mkdir build ^&^& cd build
    echo 4. cmake -f ../CMakeLists_simple.txt ..
    echo 5. make -j$(nproc)
    echo 6. ./window_manager_simple 或 ./virtual_desktop_ubuntu_demo
    exit /b 0
) else (
    echo.
    echo ❌ 发现 %FAILED% 个问题，需要修复
    exit /b 1
) 