# Build Etemenanki installer (Release + windeployqt + Inno Setup 6)
$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $Root "build"
$ReleaseDir = Join-Path $BuildDir "Release"
$Iss = Join-Path $Root "installer\EtemenankiSetup.iss"
$DistDir = Join-Path $Root "installer\dist"

Write-Host "=== Etemenanki installer build ===" -ForegroundColor Cyan

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { throw "cmake not in PATH" }

Push-Location $Root
if (-not (Test-Path $BuildDir)) {
    cmake -B build -DCMAKE_BUILD_TYPE=Release
}
cmake --build build --config Release
Pop-Location

$exe = Join-Path $ReleaseDir "Etemenanki.exe"
if (-not (Test-Path $exe)) { throw "Missing $exe" }

$windeployqt = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($windeployqt) {
    Write-Host "windeployqt..." -ForegroundColor Yellow
    $qmlDir = Join-Path $Root "qml_cpp"
    $prevEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & windeployqt $exe --qmldir $qmlDir 2>&1 | ForEach-Object { Write-Host $_ }
    $ErrorActionPreference = $prevEap
} else {
    Write-Warning "windeployqt not in PATH"
}

$iscc = @(
    "C:\InnoSetup6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $iscc) {
    Write-Host "Installing Inno Setup 6 via winget..." -ForegroundColor Yellow
    winget install --id JRSoftware.InnoSetup -e --accept-package-agreements --accept-source-agreements
    $iscc = @(
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
}

if (-not $iscc) { throw "Inno Setup ISCC.exe not found" }

Write-Host "Compiling ISS..." -ForegroundColor Yellow
& $iscc $Iss
if ($LASTEXITCODE -ne 0) { throw "ISCC failed with code $LASTEXITCODE" }

New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
$built = Get-ChildItem $DistDir -Filter "EtemenankiSetup-*.exe" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $built) { throw "No EtemenankiSetup-*.exe in $DistDir" }

$out = Join-Path $DistDir "etemenanki-setup.exe"
Copy-Item $built.FullName $out -Force
Write-Host "OK: $out" -ForegroundColor Green
