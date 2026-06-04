# Etemenanki dependency installer (optional components only)
# Model + Python are bundled with the installer — this script is for extras.
param(
    [switch]$InstallPdf2zh,
    [string]$AppRoot = ""
)

$ErrorActionPreference = "Continue"

if (-not $AppRoot) {
    $AppRoot = Split-Path -Parent $PSScriptRoot
}

$EnginesDir = Join-Path $AppRoot "engines"
$Pdf2zhDir = Join-Path $EnginesDir "pdf2zh"
$LogFile = Join-Path $env:TEMP "Etemenanki-install-deps.log"

function Log($msg) {
    $ts = Get-Date -Format "HH:mm:ss"
    $line = "[$ts] $msg"
    Write-Host $line
    Add-Content -Path $LogFile -Value $line -Encoding UTF8
}

function Install-Pdf2zhPortable {
    Log "Downloading pdf2zh portable..."
    $script = Join-Path $AppRoot "tools\setup_pdf2zh.ps1"
    if (Test-Path $script) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $script 2>&1 | ForEach-Object { Log "  pdf2zh: $_" }
        $found = Get-ChildItem -Path $Pdf2zhDir -Filter "pdf2zh.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            Log "pdf2zh installed: $($found.FullName)"
            return $true
        }
    }
    Log "WARNING: pdf2zh installation failed"
    return $false
}

Log "=== Etemenanki optional components installer ==="
Log "App root: $AppRoot"

$script:InstallOk = $true

if ($InstallPdf2zh) {
    if (-not (Install-Pdf2zhPortable)) {
        $script:InstallOk = $false
    }
}

$appLogDir = Join-Path $AppRoot "logs"
if (Test-Path $AppRoot) {
    New-Item -ItemType Directory -Force -Path $appLogDir | Out-Null
    Copy-Item -Path $LogFile -Destination (Join-Path $appLogDir "install-deps.log") -Force -ErrorAction SilentlyContinue
}

if ($script:InstallOk) {
    Log "=== Done (OK) ==="
    exit 0
}
Log "=== Done (errors) ==="
exit 1
