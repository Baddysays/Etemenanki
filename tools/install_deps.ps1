param(
    [switch]$InstallPython,
    [switch]$InstallPipDeps,
    [switch]$InstallPdf2zh,
    [switch]$InstallOllama,
    [switch]$InstallEmbeddedModel,
    [string[]]$OllamaModels = @(),
    [string]$AppRoot = "",
    [switch]$Silent
)

$ErrorActionPreference = "Continue"

if (-not $AppRoot) {
    $AppRoot = Split-Path -Parent $PSScriptRoot
}

$EnginesDir = Join-Path $AppRoot "engines"
$PythonDir = Join-Path $EnginesDir "python"
$Pdf2zhDir = Join-Path $EnginesDir "pdf2zh"
$LogFile = Join-Path $env:TEMP "Etemenanki-install-deps.log"

function Log($msg) {
    $ts = Get-Date -Format "HH:mm:ss"
    $line = "[$ts] $msg"
    Write-Host $line
    Add-Content -Path $LogFile -Value $line -Encoding UTF8
}

function Test-PythonAvailable {
    $embedded = Join-Path $PythonDir "python.exe"
    if (Test-Path $embedded) { return $embedded }

    $venvPy = Join-Path $AppRoot ".venv\Scripts\python.exe"
    if (Test-Path $venvPy) { return $venvPy }

    $sysPy = Get-Command python -ErrorAction SilentlyContinue
    if ($sysPy) {
        try {
            $ver = & python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2>$null
            if ($ver -and [double]$ver -ge 3.10) { return $sysPy.Source }
        } catch {}
    }

    $sysPy3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($sysPy3) { return $sysPy3.Source }

    $pyLauncher = Get-Command py -ErrorAction SilentlyContinue
    if ($pyLauncher) { return $pyLauncher.Source }

    return $null
}

function Install-EmbeddedPython {
    Log "Downloading Python 3.12 embeddable..."
    $pyVersion = "3.12.8"
    $pyUrl = "https://www.python.org/ftp/python/$pyVersion/python-$pyVersion-embed-amd64.zip"
    $zipPath = Join-Path $env:TEMP "python-embed.zip"

    New-Item -ItemType Directory -Force -Path $PythonDir | Out-Null

    try {
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $pyUrl -OutFile $zipPath -UseBasicParsing
    } catch {
        Log "ERROR: Failed to download Python: $_"
        return $null
    }

    Log "Extracting Python to $PythonDir ..."
    Expand-Archive -Path $zipPath -DestinationPath $PythonDir -Force
    Remove-Item $zipPath -ErrorAction SilentlyContinue

    $pthFile = Get-ChildItem -Path $PythonDir -Filter "python*._pth" | Select-Object -First 1
    if ($pthFile) {
        $content = Get-Content $pthFile.FullName
        $content = $content -replace '#import site', 'import site'
        Set-Content -Path $pthFile.FullName -Value $content
    }

    $getPipUrl = "https://bootstrap.pypa.io/get-pip.py"
    $getPipPath = Join-Path $PythonDir "get-pip.py"
    Log "Downloading get-pip.py..."
    try {
        Invoke-WebRequest -Uri $getPipUrl -OutFile $getPipPath -UseBasicParsing
    } catch {
        Log "ERROR: Failed to download get-pip.py: $_"
        return $null
    }

    $pyExe = Join-Path $PythonDir "python.exe"
    Log "Installing pip..."
    & $pyExe $getPipPath --no-warn-script-location 2>&1 | ForEach-Object { Log "  pip: $_" }

    $pythonPathTxt = Join-Path $AppRoot "tools\python_path.txt"
    Set-Content -Path $pythonPathTxt -Value $pyExe -Encoding UTF8
    Log "Python installed: $pyExe"
    return $pyExe
}

function Install-PipDependencies {
    param([string]$PyExe)

    if (-not $PyExe -or -not (Test-Path $PyExe)) {
        Log "ERROR: Python not found for pip install"
        return $false
    }

    $reqFile = Join-Path $AppRoot "tools\requirements-pdf.txt"
    if (-not (Test-Path $reqFile)) {
        Log "ERROR: requirements-pdf.txt not found"
        return $false
    }

    Log "Installing Python dependencies..."
    & $PyExe -m pip install --no-warn-script-location -q -r $reqFile 2>&1 | ForEach-Object { Log "  pip: $_" }
    if ($LASTEXITCODE -ne 0) {
        Log "WARNING: pip install exited with code $LASTEXITCODE"
        Log "Retrying without pdf2zh..."
        & $PyExe -m pip install --no-warn-script-location -q pymupdf pymupdf-fonts PyPDF2 pypdf python-docx openpyxl numpy ollama 2>&1 | ForEach-Object { Log "  pip: $_" }
    }

    $compatScript = Join-Path $AppRoot "tools\ensure_pdf2zh_compat.py"
    if (Test-Path $compatScript) {
        Log "Running NumPy compat patch..."
        & $PyExe $compatScript 2>&1 | ForEach-Object { Log "  compat: $_" }
    }

    Log "Python dependencies installed"
    return $true
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

function Install-OllamaApp {
    if (Test-OllamaInstalled) {
        Log "Ollama already available"
        return $true
    }
    Log "Installing Ollama via winget..."
    $winget = Get-Command winget -ErrorAction SilentlyContinue
    if (-not $winget) {
        Log "winget not found — open https://ollama.com/download/windows"
        Start-Process "https://ollama.com/download/windows"
        return $false
    }
    & winget install --id Ollama.Ollama -e --accept-package-agreements --accept-source-agreements 2>&1 |
        ForEach-Object { Log "  winget: $_" }
    Start-Sleep -Seconds 5
    return (Test-OllamaInstalled)
}

function Test-OllamaInstalled {
    if (Get-Command ollama -ErrorAction SilentlyContinue) { return $true }
    if (Test-Path "C:\Users\$env:USERNAME\AppData\Local\Programs\Ollama\ollama.exe") { return $true }
    try {
        $resp = Invoke-WebRequest -Uri "http://127.0.0.1:11434/api/tags" -TimeoutSec 2 -UseBasicParsing -ErrorAction SilentlyContinue
        if ($resp.StatusCode -eq 200) { return $true }
    } catch {}
    return $false
}

function Install-EmbeddedLlmModel {
    param([string]$PyExe)

    if (-not $PyExe -or -not (Test-Path $PyExe)) {
        Log "ERROR: Python not found for embedded model"
        return $false
    }

    $reqDl = Join-Path $AppRoot "tools\requirements-embedded-download.txt"
    $script = Join-Path $AppRoot "tools\embedded_llm.py"
    if (-not (Test-Path $script)) {
        Log "ERROR: embedded_llm.py not found"
        return $false
    }

    $env:ETEMENANKI_ROOT = $AppRoot
    $hfArgs = @("-m", "pip", "install", "--no-warn-script-location", "--prefer-binary", "-q", "huggingface_hub")
    if (Test-Path $reqDl) {
        $hfArgs = @("-m", "pip", "install", "--no-warn-script-location", "--prefer-binary", "-q", "-r", $reqDl)
    }
    Log "Installing huggingface_hub (for model download)..."
    & $PyExe @hfArgs 2>&1 | ForEach-Object { Log "  pip: $_" }
    if ($LASTEXITCODE -ne 0) {
        Log "ERROR: huggingface_hub install failed (code $LASTEXITCODE)"
        return $false
    }

    Log "Downloading built-in GGUF model (~1.7 GB) — do not close this window..."
    & $PyExe $script download 2>&1 | ForEach-Object { Log "  embedded: $_" }
    if ($LASTEXITCODE -eq 0) {
        Log "Built-in model ready"
        $flagFile = Join-Path $AppRoot "engines\llm\.install_embedded_mode"
        Set-Content -Path $flagFile -Value "1" -Encoding UTF8
        return $true
    }
    Log "ERROR: embedded model download failed (code $LASTEXITCODE)"
    return $false
}

function Install-OllamaModels {
    param([string[]]$Models)

    if (-not (Get-Command ollama -ErrorAction SilentlyContinue)) {
        Log "WARNING: ollama CLI not found. Install from https://ollama.com/download"
        return $false
    }

    foreach ($model in $Models) {
        Log "Pulling model: $model ..."
        & ollama pull $model 2>&1 | ForEach-Object { Log "  ollama: $_" }
        if ($LASTEXITCODE -eq 0) {
            Log "Model $model installed"
        } else {
            Log "WARNING: Failed to pull $model (code $LASTEXITCODE)"
        }
    }
    return $true
}

Log "=== Etemenanki dependency installer ==="
Log "App root: $AppRoot"
Log "Log file: $LogFile"

$script:InstallOk = $true

if ($InstallEmbeddedModel -and -not $InstallPython) {
    Log "Embedded model: enabling Python install"
    $InstallPython = $true
}
if ($InstallEmbeddedModel -and -not $InstallPipDeps) {
    Log "Embedded model: enabling document libraries"
    $InstallPipDeps = $true
}

$pyExe = $null

if ($InstallPython) {
    $existing = Test-PythonAvailable
    if ($existing) {
        Log "Python found: $existing"
        $pyExe = $existing
    } else {
        $pyExe = Install-EmbeddedPython
    }
} else {
    $pyExe = Test-PythonAvailable
}

if ($InstallPipDeps -and $pyExe) {
    Install-PipDependencies -PyExe $pyExe
}

if ($InstallPdf2zh) {
    Install-Pdf2zhPortable
}

if ($InstallOllama) {
    Install-OllamaApp | Out-Null
    if ($OllamaModels.Count -gt 0) {
        Install-OllamaModels -Models $OllamaModels
    }
}

if ($InstallEmbeddedModel) {
    if (-not $pyExe) { $pyExe = Test-PythonAvailable }
    if (-not (Install-EmbeddedLlmModel -PyExe $pyExe)) {
        $script:InstallOk = $false
    }
}

if ($script:InstallOk) {
    Log "=== Done (OK) ==="
    exit 0
}
Log "=== Done (errors — retry from Etemenanki Settings) ==="
exit 1
