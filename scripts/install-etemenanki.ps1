# Download latest Etemenanki installer from GitHub Releases (like Saylat APK flow)
$ErrorActionPreference = "Stop"
$Repo = "Baddysays/Etemenanki"
$OutDir = Join-Path $env:LOCALAPPDATA "Etemenanki"
$OutFile = Join-Path $OutDir "etemenanki-setup.exe"

Write-Host "Etemenanki: fetching latest release from GitHub..."
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/$Repo/releases/latest" -Headers @{ "User-Agent" = "Etemenanki-Install-Script" }
$asset = $release.assets | Where-Object { $_.name -eq "etemenanki-setup.exe" } | Select-Object -First 1
if (-not $asset) {
    $asset = $release.assets | Where-Object { $_.name -like "*.exe" } | Select-Object -First 1
}
if (-not $asset) {
    Write-Error "No installer .exe in latest release. Open: https://github.com/$Repo/releases/latest"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Write-Host "Downloading $($asset.name) ..."
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $OutFile -UseBasicParsing
Write-Host "Saved: $OutFile"
Write-Host "Run installer, then install Ollama: https://ollama.com/download/windows"
Start-Process -FilePath $OutFile
