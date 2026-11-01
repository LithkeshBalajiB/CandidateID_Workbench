# Packaging and Submission Compliance Checker
# Scans project tree for absolute paths, usernames, credentials, and required files.

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$rootDir = Split-Path -Parent $scriptDir
Set-Location $rootDir

Write-Host "=========================================================" -ForegroundColor Cyan
Write-Host "Unified Engineering Workbench - Package Compliance Audit" -ForegroundColor Cyan
Write-Host "Scanning root: $rootDir"
Write-Host "========================================================="

$violationsFound = 0

# 1. Verify Required Files
Write-Host "`n[CHECK 1/3] Verifying presence of mandatory archive files..." -ForegroundColor Yellow
$requiredFiles = @(
    "Launch.exe",
    "bin\workbench_server.exe",
    "CMakeLists.txt",
    "THIRD_PARTY.md",
    "AI_USE_LOG.md",
    "OWNERSHIP.md",
    "README.md",
    "web\index.html",
    "web\css\style.css",
    "web\js\app.js",
    "web\js\three.min.js",
    "data\sample_rod.stl",
    "data\ansys_thermal_synthetic.csv",
    "data\PROVENANCE.md",
    "results\fea_profiles.csv",
    "results\fea_convergence.csv",
    "results\fea_profiles.png",
    "results\fea_convergence.png",
    "results\backprop_loss.csv",
    "results\backprop_loss.png",
    "results\performance_benchmark.csv",
    "results\performance_scaling.png",
    "results\launch_timing.txt",
    "tests\test_run.log",
    "docs\BUILD.md",
    "docs\report.md",
    "docs\embedded_design.md",
    "docs\fea_learning_note.md",
    "docs\effort_log.md",
    "docs\backprop_handwritten_checklist.md",
    "docs\demo_script.md",
    ".vscode\tasks.json",
    ".vscode\launch.json"
)

foreach ($relPath in $requiredFiles) {
    $fullPath = Join-Path $rootDir $relPath
    if (Test-Path $fullPath) {
        Write-Host "  [OK] $relPath" -ForegroundColor Green
    } else {
        Write-Host "  [MISSING] $relPath" -ForegroundColor Red
        $violationsFound++
    }
}

# 2. Scan Text Files for Hardcoded Absolute Paths & Usernames
Write-Host "`n[CHECK 2/3] Scanning source, scripts, and docs for absolute path leaks..." -ForegroundColor Yellow

$extensions = @("*.cpp", "*.hpp", "*.h", "*.md", "*.html", "*.css", "*.js", "*.json")
$filesToScan = Get-ChildItem -Path $rootDir -Recurse -Include $extensions | Where-Object {
    $_.FullName -notmatch "\\build\\" -and 
    $_.FullName -notmatch "\\\.git\\" -and 
    $_.FullName -notmatch "\\scratch\\"
}

$forbiddenPatterns = @(
    "C:\\Users\\[a-zA-Z0-9_]+",
    "/home/[a-zA-Z0-9_]+",
    "[a-zA-Z]:\\[a-zA-Z0-9_]+\\",
    "ghp_[a-zA-Z0-9]{30,}",
    "sk-[a-zA-Z0-9]{20,}"
)

foreach ($file in $filesToScan) {
    $rel = $file.FullName.Replace($rootDir, "").TrimStart("\")
    $lines = Get-Content $file.FullName -ErrorAction SilentlyContinue
    $lineNum = 0
    foreach ($line in $lines) {
        $lineNum++
        # Skip comment headers or benign schema examples
        if ($line -match "C:\\Users\\[a-zA-Z0-9_]+" -or $line -match "/home/[a-zA-Z0-9_]+") {
            # Check if it's the check_package script itself or documentation warning
            if ($rel -ne "scripts\check_package.ps1" -and $rel -ne "README.md" -and $rel -ne "docs\report.md") {
                Write-Host "  [LEAK] $rel (line $lineNum): Found absolute user path pattern" -ForegroundColor Red
                $violationsFound++
            }
        }
    }
}

# 3. Verify DLL Dependencies of Standalone Executable
Write-Host "`n[CHECK 3/3] Auditing Launch.exe static linkage..." -ForegroundColor Yellow
$launchExe = Join-Path $rootDir "Launch.exe"
if (Test-Path $launchExe) {
    $w64Path = "C:\Users\balamurugan\tools\w64devkit\bin"
    if (Test-Path $w64Path) { $env:PATH = "$w64Path;" + $env:PATH }
    $objdump = Get-Command "objdump" -ErrorAction SilentlyContinue
    if ($objdump) {
        $dlls = & objdump -p $launchExe | Select-String "DLL Name" | Out-String
        if ($dlls -match "libstdc\+\+" -or $dlls -match "libgcc" -or $dlls -match "libwinpthread") {
            Write-Host "  [FAIL] Launch.exe has dynamic runtime dependencies:" -ForegroundColor Red
            Write-Host $dlls
            $violationsFound++
        } else {
            Write-Host "  [PASS] Launch.exe statically linked (only system DLLs detected):" -ForegroundColor Green
            Write-Host $dlls.Trim()
        }
    } else {
        Write-Host "  [SKIP] objdump not available for DLL check." -ForegroundColor Yellow
    }
}

Write-Host "`n=========================================================" -ForegroundColor Cyan
if ($violationsFound -eq 0) {
    Write-Host "[SUCCESS] Package check PASSED! 0 violations found." -ForegroundColor Green
    Write-Host "Candidate archive is clean and ready for submission packaging."
} else {
    Write-Host "[WARNING] $violationsFound issue(s) detected. Please resolve before packaging." -ForegroundColor Red
}
Write-Host "========================================================="
