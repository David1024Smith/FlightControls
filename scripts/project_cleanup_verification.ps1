# FlightControls Project Cleanup Verification Script

Write-Host "=== FlightControls 项目清理验证 ===" -ForegroundColor Cyan
Write-Host ""

Write-Host "检查项目结构..." -ForegroundColor Yellow

# 验证当前项目结构
$expectedFiles = @(
    "src/main.cpp",
    "src/EmbeddedWindowManager.h", 
    "src/EmbeddedWindowManager.cpp",
    "CMakeLists.txt",
    "README.md",
    ".gitignore",
    "LICENSE"
)

$expectedDirs = @(
    "src",
    "scripts",
    "build",
    ".git"
)

Write-Host "检查核心文件..." -ForegroundColor Yellow
$fileChecks = 0
foreach ($file in $expectedFiles) {
    if (Test-Path $file) {
        Write-Host "✓ $file 存在" -ForegroundColor Green
        $fileChecks++
    } else {
        Write-Host "✗ $file 缺失" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "检查目录结构..." -ForegroundColor Yellow
$dirChecks = 0
foreach ($dir in $expectedDirs) {
    if (Test-Path $dir -PathType Container) {
        Write-Host "✓ $dir/ 存在" -ForegroundColor Green
        $dirChecks++
    } else {
        Write-Host "✗ $dir/ 缺失" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "验证原始项目文件已删除..." -ForegroundColor Yellow

$deletedFiles = @(
    "src/WindowManager.h",
    "src/WindowManager.cpp", 
    "src/TabBasedLauncher.h",
    "src/TabBasedLauncher.cpp",
    "USAGE.md",
    "ALTERNATIVES.md",
    "PROJECT_RESTRUCTURE.md",
    "CODE_FIXES_SUMMARY.md",
    "TAB_LAUNCHER_GUIDE.md",
    "README_CLEAN.md",
    "README_WINDOW_MANAGER.md"
)

$deletedDirs = @(
    "examples",
    "docs", 
    "original_files"
)

$deleteChecks = 0
foreach ($file in $deletedFiles) {
    if (-not (Test-Path $file)) {
        Write-Host "✓ $file 已删除" -ForegroundColor Green
        $deleteChecks++
    } else {
        Write-Host "✗ $file 仍然存在" -ForegroundColor Red
    }
}

foreach ($dir in $deletedDirs) {
    if (-not (Test-Path $dir)) {
        Write-Host "✓ $dir/ 已删除" -ForegroundColor Green
        $deleteChecks++
    } else {
        Write-Host "✗ $dir/ 仍然存在" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "检查代码内容..." -ForegroundColor Yellow

$codeChecks = 0

# 检查main.cpp是否使用EmbeddedWindowManager
$mainContent = Get-Content "src/main.cpp" -Raw -ErrorAction SilentlyContinue
if ($mainContent -and $mainContent -match "EmbeddedWindowManager") {
    Write-Host "✓ main.cpp 使用 EmbeddedWindowManager" -ForegroundColor Green
    $codeChecks++
} else {
    Write-Host "✗ main.cpp 未使用 EmbeddedWindowManager" -ForegroundColor Red
}

# 检查CMakeLists.txt是否配置正确
$cmakeContent = Get-Content "CMakeLists.txt" -Raw -ErrorAction SilentlyContinue
if ($cmakeContent -and $cmakeContent -match "EmbeddedWindowManager") {
    Write-Host "✓ CMakeLists.txt 配置 EmbeddedWindowManager" -ForegroundColor Green
    $codeChecks++
} else {
    Write-Host "✗ CMakeLists.txt 未配置 EmbeddedWindowManager" -ForegroundColor Red
}

# 检查README.md是否更新为嵌入式版本
$readmeContent = Get-Content "README.md" -Raw -ErrorAction SilentlyContinue
if ($readmeContent -and $readmeContent -match "嵌入式管理器") {
    Write-Host "✓ README.md 已更新为嵌入式版本" -ForegroundColor Green
    $codeChecks++
} else {
    Write-Host "✗ README.md 未更新" -ForegroundColor Red
}

Write-Host ""
Write-Host "总结:" -ForegroundColor Cyan

$totalChecks = $fileChecks + $dirChecks + $deleteChecks + $codeChecks
$maxChecks = $expectedFiles.Count + $expectedDirs.Count + $deletedFiles.Count + $deletedDirs.Count + 3

Write-Host "核心文件: $fileChecks / $($expectedFiles.Count)" -ForegroundColor $(if ($fileChecks -eq $expectedFiles.Count) {"Green"} else {"Yellow"})
Write-Host "目录结构: $dirChecks / $($expectedDirs.Count)" -ForegroundColor $(if ($dirChecks -eq $expectedDirs.Count) {"Green"} else {"Yellow"})
Write-Host "文件清理: $deleteChecks / $($deletedFiles.Count + $deletedDirs.Count)" -ForegroundColor $(if ($deleteChecks -eq ($deletedFiles.Count + $deletedDirs.Count)) {"Green"} else {"Yellow"})
Write-Host "代码更新: $codeChecks / 3" -ForegroundColor $(if ($codeChecks -eq 3) {"Green"} else {"Yellow"})
Write-Host "总体状态: $totalChecks / $maxChecks" -ForegroundColor $(if ($totalChecks -eq $maxChecks) {"Green"} else {"Yellow"})

if ($totalChecks -eq $maxChecks) {
    Write-Host ""
    Write-Host "🎉 项目清理完成!" -ForegroundColor Green
    Write-Host ""
    Write-Host "✨ 纯净的嵌入式窗口管理器项目 ✨" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "当前项目结构:" -ForegroundColor Yellow
    Write-Host "FlightControls/" -ForegroundColor White
    Write-Host "├── src/" -ForegroundColor White
    Write-Host "│   ├── main.cpp                      # 主程序入口" -ForegroundColor White
    Write-Host "│   ├── EmbeddedWindowManager.h       # 嵌入式管理器头文件" -ForegroundColor White
    Write-Host "│   └── EmbeddedWindowManager.cpp     # 嵌入式管理器实现" -ForegroundColor White
    Write-Host "├── scripts/" -ForegroundColor White
    Write-Host "│   ├── embedded_manager_test.ps1     # 功能验证脚本" -ForegroundColor White
    Write-Host "│   └── project_cleanup_verification.ps1  # 清理验证脚本" -ForegroundColor White
    Write-Host "├── CMakeLists.txt                    # 构建配置" -ForegroundColor White
    Write-Host "├── README.md                         # 项目说明" -ForegroundColor White
    Write-Host "├── .gitignore                        # Git忽略文件" -ForegroundColor White
    Write-Host "└── LICENSE                           # 许可证" -ForegroundColor White
    Write-Host ""
    Write-Host "✅ 所有原始项目代码已删除" -ForegroundColor Green
    Write-Host "✅ 只保留嵌入式窗口管理器核心功能" -ForegroundColor Green
    Write-Host "✅ 项目结构清洁简单" -ForegroundColor Green
    Write-Host "✅ 方案A完全实现" -ForegroundColor Green
    Write-Host ""
    Write-Host "准备编译:" -ForegroundColor Cyan
    Write-Host "cd build" -ForegroundColor White
    Write-Host "cmake .." -ForegroundColor White
    Write-Host "make -j`$(nproc)" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "⚠ 清理不完整，请检查上述失败项目" -ForegroundColor Yellow
}

Write-Host "" 