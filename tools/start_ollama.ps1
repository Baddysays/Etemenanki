param(
    [int]$GpuIndex = -1,
    [string]$OllamaUrl = "http://127.0.0.1:11434"
)

$ErrorActionPreference = "Continue"

function Test-OllamaUp {
    try {
        $r = Invoke-WebRequest -Uri "$OllamaUrl/api/tags" -TimeoutSec 2 -UseBasicParsing
        return $r.StatusCode -eq 200
    } catch { return $false }
}

if (Test-OllamaUp) { exit 0 }

$ollamaExe = $null
if (Get-Command ollama -ErrorAction SilentlyContinue) {
    $ollamaExe = (Get-Command ollama).Source
}
if (-not $ollamaExe) {
    $cand = Join-Path $env:LOCALAPPDATA "Programs\Ollama\ollama.exe"
    if (Test-Path $cand) { $ollamaExe = $cand }
}
if (-not $ollamaExe) {
    Write-Host "Ollama not installed. Install from https://ollama.com/download/windows"
    exit 1
}

if ($GpuIndex -ge 0) {
    $env:CUDA_VISIBLE_DEVICES = "$GpuIndex"
    Write-Host "Using GPU index $GpuIndex for Ollama (NVIDIA)"
}

$existing = Get-Process -Name "ollama" -ErrorAction SilentlyContinue
if (-not $existing) {
    Start-Process -FilePath $ollamaExe -ArgumentList "serve" -WindowStyle Hidden
}

for ($i = 0; $i -lt 45; $i++) {
    Start-Sleep -Seconds 2
    if (Test-OllamaUp) { exit 0 }
}

Write-Host "Ollama did not respond on $OllamaUrl — in Windows Settings set ollama.exe to High performance (discrete GPU)"
exit 2
