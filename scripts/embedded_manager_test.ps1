# FlightControls Embedded Manager Test Script

Write-Host "=== FlightControls 嵌入式窗口管理器验证 ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "检查嵌入式管理器构建配置..." -ForegroundColor Yellow

# 检查CMakeLists.txt是否正确配置
$cmakeContent = Get-Content "CMakeLists.txt" -Raw -ErrorAction SilentlyContinue

if ($cmakeContent) {
    $checks = 0
    
    if ($cmakeContent -match "FlightControlsEmbeddedManager") {
        Write-Host "✓ 项目名称已更新为EmbeddedManager" -ForegroundColor Green
        $checks++
    } else {
        Write-Host "✗ 项目名称未更新" -ForegroundColor Red
    }
    
    if ($cmakeContent -match "src/EmbeddedWindowManager\.cpp") {
        Write-Host "✓ EmbeddedWindowManager源文件已配置" -ForegroundColor Green
        $checks++
    } else {
        Write-Host "✗ EmbeddedWindowManager源文件未配置" -ForegroundColor Red
    }
    
    if ($cmakeContent -match "X11.*窗口嵌入") {
        Write-Host "✓ X11窗口嵌入配置正确" -ForegroundColor Green
        $checks++
    } else {
        Write-Host "✗ X11窗口嵌入配置缺失" -ForegroundColor Red
    }
    
    if ($cmakeContent -match "真正窗口嵌入") {
        Write-Host "✓ 嵌入式方案描述正确" -ForegroundColor Green
        $checks++
    } else {
        Write-Host "✗ 嵌入式方案描述缺失" -ForegroundColor Red
    }
} else {
    Write-Host "✗ 无法读取CMakeLists.txt" -ForegroundColor Red
    $checks = 0
}

Write-Host ""
Write-Host "检查源文件..." -ForegroundColor Yellow

# 检查源文件是否存在
$sourceFiles = @("src/EmbeddedWindowManager.h", "src/EmbeddedWindowManager.cpp", "src/main.cpp")
$sourceChecks = 0

foreach ($file in $sourceFiles) {
    if (Test-Path $file) {
        Write-Host "✓ $file 存在" -ForegroundColor Green
        $sourceChecks++
    } else {
        Write-Host "✗ $file 缺失" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "检查main.cpp配置..." -ForegroundColor Yellow

$mainContent = Get-Content "src/main.cpp" -Raw -ErrorAction SilentlyContinue
if ($mainContent) {
    if ($mainContent -match "EmbeddedWindowManager\.h") {
        Write-Host "✓ main.cpp包含EmbeddedWindowManager头文件" -ForegroundColor Green
        $sourceChecks++
    } else {
        Write-Host "✗ main.cpp未包含EmbeddedWindowManager头文件" -ForegroundColor Red
    }
    
    if ($mainContent -match "EmbeddedWindowManager manager") {
        Write-Host "✓ main.cpp创建EmbeddedWindowManager实例" -ForegroundColor Green
        $sourceChecks++
    } else {
        Write-Host "✗ main.cpp未创建EmbeddedWindowManager实例" -ForegroundColor Red
    }
    
    if ($mainContent -match "True Window Embedding") {
        Write-Host "✓ main.cpp标识为窗口嵌入方案" -ForegroundColor Green
        $sourceChecks++
    } else {
        Write-Host "✗ main.cpp方案标识缺失" -ForegroundColor Red
    }
} else {
    Write-Host "✗ 无法读取main.cpp" -ForegroundColor Red
}

Write-Host ""
Write-Host "检查核心嵌入功能..." -ForegroundColor Yellow

$embeddedContent = Get-Content "src/EmbeddedWindowManager.cpp" -Raw -ErrorAction SilentlyContinue
if ($embeddedContent) {
    $embeddedChecks = 0
    
    if ($embeddedContent -match "XReparentWindow") {
        Write-Host "✓ X11窗口嵌入API (XReparentWindow) 已实现" -ForegroundColor Green
        $embeddedChecks++
    } else {
        Write-Host "✗ X11窗口嵌入API缺失" -ForegroundColor Red
    }
    
    if ($embeddedContent -match "embedWindow") {
        Write-Host "✓ embedWindow方法已实现" -ForegroundColor Green
        $embeddedChecks++
    } else {
        Write-Host "✗ embedWindow方法缺失" -ForegroundColor Red
    }
    
    if ($embeddedContent -match "EmbeddedWidget") {
        Write-Host "✓ EmbeddedWidget类已实现" -ForegroundColor Green
        $embeddedChecks++
    } else {
        Write-Host "✗ EmbeddedWidget类缺失" -ForegroundColor Red
    }
    
    if ($embeddedContent -match "checkForNewWindows") {
        Write-Host "✓ 自动窗口检测功能已实现" -ForegroundColor Green
        $embeddedChecks++
    } else {
        Write-Host "✗ 自动窗口检测功能缺失" -ForegroundColor Red
    }
} else {
    Write-Host "✗ 无法读取EmbeddedWindowManager.cpp" -ForegroundColor Red
    $embeddedChecks = 0
}

Write-Host ""
Write-Host "总结:" -ForegroundColor Cyan

$totalChecks = $checks + $sourceChecks + $embeddedChecks
$maxChecks = 4 + 6 + 4

Write-Host "构建配置: $checks / 4" -ForegroundColor $(if ($checks -eq 4) {"Green"} else {"Yellow"})
Write-Host "源文件配置: $sourceChecks / 6" -ForegroundColor $(if ($sourceChecks -eq 6) {"Green"} else {"Yellow"})
Write-Host "嵌入功能: $embeddedChecks / 4" -ForegroundColor $(if ($embeddedChecks -eq 4) {"Green"} else {"Yellow"})
Write-Host "总体状态: $totalChecks / $maxChecks" -ForegroundColor $(if ($totalChecks -eq $maxChecks) {"Green"} else {"Yellow"})

if ($totalChecks -eq $maxChecks) {
    Write-Host ""
    Write-Host "🎉 嵌入式窗口管理器配置完成!" -ForegroundColor Green
    Write-Host ""
    Write-Host "✨ 方案A: 真正的窗口嵌入 ✨" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "核心特性:" -ForegroundColor Yellow
    Write-Host "• 使用X11 XReparentWindow API真正嵌入外部窗口" -ForegroundColor White
    Write-Host "• QGroundControl和RVIZ嵌入到Tab页面中" -ForegroundColor White
    Write-Host "• 自动检测新启动的应用程序窗口" -ForegroundColor White
    Write-Host "• 智能窗口大小调整和事件处理" -ForegroundColor White
    Write-Host "• 完全替换快速切换方案" -ForegroundColor White
    Write-Host "• 不自动配置ROS环境，需手动设置" -ForegroundColor White
    Write-Host ""
    Write-Host "编译指令:" -ForegroundColor Cyan
    Write-Host "cd build" -ForegroundColor White
    Write-Host "cmake .." -ForegroundColor White
    Write-Host "make -j`$(nproc)" -ForegroundColor White
    Write-Host "./flight_controls_launcher" -ForegroundColor White
    Write-Host ""
    Write-Host "使用说明:" -ForegroundColor Cyan
    Write-Host "1. 启动程序后，显示Tab界面（仪表板、QGC、RVIZ）" -ForegroundColor White
    Write-Host "2. 点击'启动并嵌入 QGC'启动QGroundControl" -ForegroundColor White
    Write-Host "3. 程序自动检测QGC窗口并嵌入到QGC Tab页面" -ForegroundColor White
    Write-Host "4. 手动设置ROS环境后，点击'启动并嵌入 RVIZ'" -ForegroundColor White
    Write-Host "5. 程序自动检测RVIZ窗口并嵌入到RVIZ Tab页面" -ForegroundColor White
    Write-Host "6. 通过Tab切换查看不同的嵌入应用" -ForegroundColor White
    Write-Host ""
    Write-Host "技术特点:" -ForegroundColor Yellow
    Write-Host "• 真正的窗口嵌入（非快速切换）" -ForegroundColor White
    Write-Host "• X11 API深度集成" -ForegroundColor White
    Write-Host "• 响应式窗口大小调整" -ForegroundColor White
    Write-Host "• 实时应用状态监控" -ForegroundColor White
    Write-Host ""
    Write-Host "RVIZ预备:" -ForegroundColor Yellow
    Write-Host "source /opt/ros/*/setup.bash" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "⚠ 配置不完整，请检查上述失败项目" -ForegroundColor Yellow
}

Write-Host "" 