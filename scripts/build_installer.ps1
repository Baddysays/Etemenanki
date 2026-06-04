# Build Etemenanki installer (automates all steps)
param(
    [string]$QtPath = "C:\Qt\6.8.0\msvc2022_64",
    [switch]$SkipPrepare,
    [switch]$SkipWinDeployQt
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot

Write-Host "=== Etemenanki Installer Build ===" -ForegroundColor Cyan
Write-Host "Root: $Root"
Write-Host "Qt: $QtPath"

# Step 1: CMake build
Write-Host ""
Write-Host "[1/4] Building Release..." -ForegroundColor Yellow
if (-not (Test-Path (Join-Path $Root "build\CMakeCache.txt"))) {
    cmake -S $Root -B (Join-Path $Root "build") -DCMAKE_PREFIX_PATH="$QtPath"
}
cmake --build (Join-Path $Root "build") --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake build failed"
    exit 1
}

# Step 2: Prepare release (model + Python)
if (-not $SkipPrepare) {
    Write-Host ""
    Write-Host "[2/4] Preparing release (downloading model + Python)..." -ForegroundColor Yellow
    & (Join-Path $PSScriptRoot "prepare_release.ps1") -BuildDir "build\Release"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: prepare_release.ps1 failed — installer will be incomplete" -ForegroundColor Red
    }
}

# Step 3: windeployqt
if (-not $SkipWinDeployQt) {
    Write-Host ""
    Write-Host "[3/4] Deploying Qt dependencies..." -ForegroundColor Yellow
    $winDeployQt = Join-Path $QtPath "bin\windeployqt.exe"
    if (Test-Path $winDeployQt) {
        & $winDeployQt (Join-Path $Root "build\Release\Etemenanki.exe")
        if ($LASTEXITCODE -ne 0) {
            Write-Host "WARNING: windeployqt failed" -ForegroundColor Red
        }
    } else {
        Write-Host "WARNING: windeployqt not found at $winDeployQt" -ForegroundColor Red
    }
}

# Step 4: Inno Setup
Write-Host ""
Write-Host "[4/4] Compiling installer..." -ForegroundColor Yellow
$innoCompiler = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (Test-Path $innoCompiler) {
    & $innoCompiler (Join-Path $Root "installer\EtemenankiSetup.iss")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Inno Setup compilation failed"
        exit 1
    }
} else {
    Write-Host "Inno Setup 6 not found at: $innoCompiler" -ForegroundColor Red
    Write-Host "Download from: https://jrsoftware.org/isinfo.php" -ForegroundColor Yellow
    Write-Host "Then open installer\EtemenankiSetup.iss and compile manually." -ForegroundColor Yellow
}

# Summary
Write-Host ""
Write-Host "=== Build Complete ===" -ForegroundColor Cyan
$distDir = Join-Path $Root "installer\dist"
if (Test-Path $distDir) {
    $installers = Get-ChildItem -Path $distDir -Filter "*.exe" | Sort-Object LastWriteTime -Descending
    if ($installers) {
        Write-Host "Installer: $($installers[0].FullName)" -ForegroundColor Green
        Write-Host "Size: $([int]($installers[0].Length / 1MB)) MB" -ForegroundColor Green
    }
}
