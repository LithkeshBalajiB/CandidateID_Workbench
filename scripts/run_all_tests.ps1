# Single unified test runner script for Unified Engineering Workbench
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = Split-Path -Parent $scriptDir
Set-Location $rootDir

# Ensure toolchain is in path if available
$w64Path = "C:\Users\balamurugan\tools\w64devkit\bin"
$cmakePath = "C:\Users\balamurugan\tools\cmake\bin"
if (Test-Path $w64Path) { $env:PATH = "$w64Path;$cmakePath;" + $env:PATH }

$logFile = Join-Path $rootDir "tests\test_run.log"
"" | Out-File -FilePath $logFile -Encoding utf8

function Write-TestLog {
    param([string]$msg)
    Write-Host $msg
    $msg | Out-File -FilePath $logFile -Append -Encoding utf8
}

Write-TestLog "========================================================="
Write-TestLog "Unified Engineering Workbench - Comprehensive Test Runner"
Write-TestLog "Executed at: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Write-TestLog "========================================================="

# 1. Run Unit Tests Runner (doctest)
Write-TestLog "`n[STEP 1/5] Executing Unit Test Suite (workbench_tests.exe)..."
$testExe = Join-Path $rootDir "build\workbench_tests.exe"
if (Test-Path $testExe) {
    & $testExe | ForEach-Object { Write-TestLog $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-TestLog "[FAILURE] Unit tests failed with exit code $LASTEXITCODE"
        exit $LASTEXITCODE
    }
    Write-TestLog "[SUCCESS] All unit test cases and assertions PASSED."
} else {
    Write-TestLog "[ERROR] workbench_tests.exe not found at $testExe. Run build first."
    exit 1
}

# 2. Run Task C Generator
Write-TestLog "`n[STEP 2/5] Executing Task C FEA Solver and Convergence Generator..."
$feaExe = Join-Path $rootDir "build\generate_fea_results.exe"
if (Test-Path $feaExe) {
    & $feaExe | ForEach-Object { Write-TestLog $_ }
}

# 3. Run Task F Backprop Generator
Write-TestLog "`n[STEP 3/5] Executing Task F Backprop Gradient Check and Loss Generator..."
$backpropExe = Join-Path $rootDir "build\generate_backprop_results.exe"
if (Test-Path $backpropExe) {
    & $backpropExe | ForEach-Object { Write-TestLog $_ }
}

# 4. Run Task D Multicore Benchmark
Write-TestLog "`n[STEP 4/5] Executing Task D Multicore Scaling Benchmark..."
$benchExe = Join-Path $rootDir "build\generate_multicore_benchmark.exe"
if (Test-Path $benchExe) {
    & $benchExe | ForEach-Object { Write-TestLog $_ }
}

# 5. Run Task B File Size Limits Stress Test
Write-TestLog "`n[STEP 5/5] Executing Task B File-Size Limits Stress Test..."
$stressExe = Join-Path $rootDir "build\test_file_size_limits.exe"
if (Test-Path $stressExe) {
    & $stressExe | ForEach-Object { Write-TestLog $_ }
}

Write-TestLog "`n========================================================="
Write-TestLog "[COMPLETE] All verification tests executed successfully."
Write-TestLog "Log saved to: tests/test_run.log"
Write-TestLog "========================================================="
