# scripts/create_archive.ps1
# Creates the final submission zip archive: CandidateID_Workbench.zip
# Enforces single top-level directory: CandidateID_Workbench/
# Excludes intermediate build folders (.git, build/) to maintain optimal archive size.

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$StagingDir = Join-Path (Split-Path -Parent $Root) "staging_workbench\CandidateID_Workbench"
$ZipTarget = Join-Path $Root "CandidateID_Workbench.zip"

Write-Host "========================================================="
Write-Host "Packaging Submission Archive: CandidateID_Workbench.zip"
Write-Host "Source Root: $Root"
Write-Host "Staging Dir: $StagingDir"
Write-Host "========================================================="

# 1. Clean previous staging & zip
if (Test-Path $StagingDir) {
    Remove-Item -Recurse -Force (Split-Path -Parent $StagingDir)
}
if (Test-Path $ZipTarget) {
    Remove-Item -Force $ZipTarget
}
New-Item -ItemType Directory -Path $StagingDir -Force | Out-Null

# 2. Files & Directories to include
$ItemsToCopy = @(
    "Launch.exe",
    "CMakeLists.txt",
    "README.md",
    "THIRD_PARTY.md",
    "AI_USE_LOG.md",
    "OWNERSHIP.md",
    "CandidateID_Workbench.bundle",
    ".vscode",
    "bin",
    "data",
    "docs",
    "results",
    "scripts",
    "src",
    "tests",
    "third_party",
    "tools",
    "web"
)

Write-Host "Copying distribution files into staging area..."
foreach ($item in $ItemsToCopy) {
    $srcPath = Join-Path $Root $item
    if (Test-Path $srcPath) {
        Copy-Item -Path $srcPath -Destination $StagingDir -Recurse -Force
        Write-Host "  [COPIED] $item"
    } else {
        Write-Warning "Missing item: $item"
    }
}

# 3. Compress staging area into CandidateID_Workbench.zip
Write-Host "Compressing to: $ZipTarget ..."
$ParentStaging = Split-Path -Parent $StagingDir
Compress-Archive -Path $StagingDir -DestinationPath $ZipTarget -Force

# 4. Clean up staging
Remove-Item -Recurse -Force $ParentStaging

# 5. Verify ZIP
$zipSizeMB = [math]::Round((Get-Item $ZipTarget).Length / 1MB, 2)
Write-Host "Archive created successfully!"
Write-Host "Archive File: $ZipTarget"
Write-Host "Archive Size: $zipSizeMB MB"
Write-Host "========================================================="
