# Downloads and extracts the portable pdf2zh Windows bundle into engines/pdf2zh/
param(
    [string]$Version = "1.9.11",
    [string]$Repo = "Byaidu/PDFMathTranslate"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$destRoot = Join-Path $root "engines\pdf2zh"
$zipName = "pdf2zh-v$Version-with-assets-win64.zip"
$url = "https://github.com/$Repo/releases/download/v$Version/$zipName"
$zipPath = Join-Path $env:TEMP $zipName

Write-Host "Etemenanki: downloading $zipName ..."
Write-Host "  $url"

New-Item -ItemType Directory -Force -Path $destRoot | Out-Null

if (-not (Test-Path $zipPath)) {
    Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing
}

Write-Host "Extracting to $destRoot ..."
Expand-Archive -Path $zipPath -DestinationPath $destRoot -Force

$exe = Get-ChildItem -Path $destRoot -Filter "pdf2zh.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) {
    Write-Error "pdf2zh.exe not found after extraction. Check the archive layout."
}

Write-Host "Done. pdf2zh.exe: $($exe.FullName)"
Write-Host "Rebuild Etemenanki or copy engines/ next to Etemenanki.exe."
