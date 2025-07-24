# FlightControls Restructure Verification Script
# Check if project restructure was successful

Write-Host "Checking FlightControls project restructure..." -ForegroundColor Cyan
Write-Host ""

$passed = 0
$failed = 0

# Check essential files exist
$essential_files = @(
    "src\WindowManager.h",
    "src\WindowManager.cpp", 
    "src\TabBasedLauncher.h",
    "src\TabBasedLauncher.cpp",
    "src\VirtualDesktopManager.h",
    "src\VirtualDesktopManager.cpp",
    "examples\window_manager_simple.cpp",
    "examples\virtual_desktop_ubuntu_demo.cpp",
    "examples\alternative_main.cpp",
    "CMakeLists.txt",
    "CMakeLists_simple.txt",
    "CMakeLists_original.txt"
)

Write-Host "Checking Essential Files:" -ForegroundColor Yellow
foreach ($file in $essential_files) {
    if (Test-Path $file) {
        Write-Host "  OK: $file" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "  MISSING: $file" -ForegroundColor Red
        $failed++
    }
}

# Check original files moved
Write-Host ""
Write-Host "Checking Original Files Moved:" -ForegroundColor Yellow
$original_files = @(
    "original_files\main.cpp",
    "original_files\MainWindow.cpp",
    "original_files\ProcessManager.cpp",
    "original_files\ApplicationSwitcher.cpp"
)

foreach ($file in $original_files) {
    if (Test-Path $file) {
        Write-Host "  OK: $file moved to original_files/" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "  MISSING: $file not found in original_files/" -ForegroundColor Red
        $failed++
    }
}

# Check src directory is clean
Write-Host ""
Write-Host "Checking src Directory Clean:" -ForegroundColor Yellow
$unwanted_files = @(
    "src\main.cpp",
    "src\MainWindow.cpp", 
    "src\ProcessManager.cpp",
    "src\ApplicationSwitcher.cpp"
)

$clean = $true
foreach ($file in $unwanted_files) {
    if (Test-Path $file) {
        Write-Host "  ERROR: $file still in src/ (should be moved)" -ForegroundColor Red
        $failed++
        $clean = $false
    }
}

if ($clean) {
    Write-Host "  OK: src/ directory contains only new alternative files" -ForegroundColor Green
    $passed++
}

# Check removed files
Write-Host ""
Write-Host "Checking Removed Files:" -ForegroundColor Yellow
if (-not (Test-Path "CMakeLists_alternatives.txt")) {
    Write-Host "  OK: CMakeLists_alternatives.txt removed" -ForegroundColor Green
    $passed++
} else {
    Write-Host "  ERROR: CMakeLists_alternatives.txt still exists" -ForegroundColor Red
    $failed++
}

# Check code quality fixes
Write-Host ""
Write-Host "Checking Code Quality:" -ForegroundColor Yellow

try {
    $unsafe_delete = Select-String -Path "src\*.cpp" -Pattern "delete.*process" -Quiet
    if (-not $unsafe_delete) {
        Write-Host "  OK: No unsafe process deletion found" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "  WARNING: Unsafe process deletion still exists" -ForegroundColor Red
        $failed++
    }

    $safe_delete = Select-String -Path "src\*.cpp" -Pattern "deleteLater" -Quiet
    if ($safe_delete) {
        Write-Host "  OK: Safe deleteLater() usage found" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "  WARNING: No deleteLater() usage found" -ForegroundColor Yellow
    }

    $webengine_conditional = Select-String -Path "src\*.h" -Pattern "#ifdef QT_WEBENGINEWIDGETS_LIB" -Quiet
    if ($webengine_conditional) {
        Write-Host "  OK: Conditional WebEngine compilation found" -ForegroundColor Green
        $passed++
    } else {
        Write-Host "  WARNING: No conditional WebEngine compilation" -ForegroundColor Yellow
    }
} catch {
    Write-Host "  WARNING: Could not check code patterns" -ForegroundColor Yellow
}

# Summary
Write-Host ""
Write-Host "Restructure Summary:" -ForegroundColor Cyan
Write-Host "  Passed: $passed" -ForegroundColor Green
Write-Host "  Failed: $failed" -ForegroundColor Red

if ($failed -eq 0) {
    Write-Host ""
    Write-Host "SUCCESS: Project restructure completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next Steps (on Linux):" -ForegroundColor Cyan
    Write-Host "  1. chmod +x scripts/check_dependencies.sh"
    Write-Host "  2. ./scripts/check_dependencies.sh"
    Write-Host "  3. mkdir build && cd build"
    Write-Host "  4. cmake .."
    Write-Host "  5. make -j4"
    Write-Host ""
    Write-Host "Available executables:" -ForegroundColor Cyan
    Write-Host "  - flight_controls_launcher (main launcher)"
    Write-Host "  - flight_controls_window_manager (window management demo)"
    Write-Host "  - flight_controls_virtual_desktop (virtual desktop demo)"
    exit 0
} else {
    Write-Host ""
    Write-Host "FAILED: $failed issues found. Please check the errors above." -ForegroundColor Red
    exit 1
} 