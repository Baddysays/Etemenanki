# Prepare Etemenanki release: download model + Python embed into build/Release
# Run after: cmake --build build --config Release
# Then run: windeployqt build\Release\Etemenanki.exe
# Then compile: installer\EtemenankiSetup.iss

param(
    [string]$BuildDir = "build\Release",
    [switch]$SkipModel,
    [switch]$SkipPython
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$BuildPath = Join-Path $Root $BuildDir

if (-not (Test-Path (Join-Path $BuildPath "Etemenanki.exe"))) {
    Write-Error "Build not found: $BuildPath\Etemenanki.exe`nRun: cmake --build build --config Release"
    exit 1
}

Write-Host "=== Etemenanki Release Preparation ===" -ForegroundColor Cyan
Write-Host "Build: $BuildPath"

# --- Python embeddable ---
if (-not $SkipPython) {
    $PythonDir = Join-Path $BuildPath "engines\python"
    $PythonExe = Join-Path $PythonDir "python.exe"

    if (Test-Path $PythonExe) {
        Write-Host "[Python] Already exists: $PythonExe" -ForegroundColor Green
    } else {
        Write-Host "[Python] Downloading Python 3.12 embeddable..." -ForegroundColor Yellow
        $pyVersion = "3.12.8"
        $pyUrl = "https://www.python.org/ftp/python/$pyVersion/python-$pyVersion-embed-amd64.zip"
        $zipPath = Join-Path $env:TEMP "python-embed.zip"

        New-Item -ItemType Directory -Force -Path $PythonDir | Out-Null
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $pyUrl -OutFile $zipPath -UseBasicParsing

        Write-Host "[Python] Extracting..."
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
        Write-Host "[Python] Installing pip..."
        Invoke-WebRequest -Uri $getPipUrl -OutFile $getPipPath -UseBasicParsing
        & $PythonExe $getPipPath --no-warn-script-location 2>&1 | Out-Null
        Remove-Item $getPipPath -ErrorAction SilentlyContinue

        Write-Host "[Python] Installing document libraries..."
        $reqFile = Join-Path $Root "tools\requirements-pdf.txt"
        if (Test-Path $reqFile) {
            & $PythonExe -m pip install --no-warn-script-location -q -r $reqFile 2>&1 | Out-Null
        } else {
            & $PythonExe -m pip install --no-warn-script-location -q pymupdf pymupdf-fonts PyPDF2 pypdf python-docx openpyxl numpy ollama 2>&1 | Out-Null
        }

        $compatScript = Join-Path $Root "tools\ensure_pdf2zh_compat.py"
        if (Test-Path $compatScript) {
            & $PythonExe $compatScript 2>&1 | Out-Null
        }

        $pythonPathTxt = Join-Path $BuildPath "tools\python_path.txt"
        Set-Content -Path $pythonPathTxt -Value $PythonExe -Encoding UTF8
        Write-Host "[Python] Ready: $PythonExe" -ForegroundColor Green
    }
}

# --- Embedded LLM model ---
if (-not $SkipModel) {
    $ModelsDir = Join-Path $BuildPath "engines\llm\models"
    $ManifestFile = Join-Path $BuildPath "engines\llm\manifest.json"

    if (-not (Test-Path $ManifestFile)) {
        $ManifestFile = Join-Path $Root "engines\llm\manifest.json"
    }

    if (-not (Test-Path $ManifestFile)) {
        Write-Host "[Model] No manifest.json found — skipping" -ForegroundColor Yellow
    } else {
        $manifest = Get-Content $ManifestFile -Raw | ConvertFrom-Json
        $model = $manifest.models | Select-Object -First 1
        $filename = $model.filename
        $modelPath = Join-Path $ModelsDir $filename

        if (Test-Path $modelPath) {
            $size = (Get-Item $modelPath).Length / 1MB
            Write-Host "[Model] Already exists: $filename ($([int]$size) MB)" -ForegroundColor Green
        } else {
            $repoId = $model.repo_id
            $repoFile = $model.repo_file
            $sizeMb = $model.size_mb
            Write-Host "[Model] Downloading $repoId / $repoFile (~$sizeMb MB)..." -ForegroundColor Yellow
            Write-Host "This may take 10-30 minutes depending on your internet connection."

            New-Item -ItemType Directory -Force -Path $ModelsDir | Out-Null

            $hfUrl = "https://huggingface.co/$repoId/resolve/main/$repoFile"
            $tempPath = Join-Path $env:TEMP $filename

            $maxRetries = 3
            $retry = 0
            $success = $false

            while (-not $success -and $retry -lt $maxRetries) {
                $retry++
                Write-Host "[Model] Attempt $retry of $maxRetries..."

                try {
                    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
                    $ProgressPreference = 'SilentlyContinue'
                    Invoke-WebRequest -Uri $hfUrl -OutFile $tempPath -UseBasicParsing -TimeoutSec 3600
                    $ProgressPreference = 'Continue'

                    if (Test-Path $tempPath) {
                        $downloadedSize = (Get-Item $tempPath).Length / 1MB
                        $minSize = [int]($sizeMb * 0.85)
                        if ($downloadedSize -ge $minSize) {
                            Move-Item -Path $tempPath -Destination $modelPath -Force
                            Write-Host "[Model] Downloaded: $filename ($([int]$downloadedSize) MB)" -ForegroundColor Green
                            $success = $true
                        } else {
                            Write-Host "[Model] Incomplete: $([int]$downloadedSize) MB < $minSize MB" -ForegroundColor Red
                            Remove-Item $tempPath -ErrorAction SilentlyContinue
                        }
                    }
                } catch {
                    Write-Host "[Model] Download error: $_" -ForegroundColor Red
                    if (Test-Path $tempPath) { Remove-Item $tempPath -ErrorAction SilentlyContinue }
                }
            }

            if (-not $success) {
                Write-Host "[Model] FAILED after $maxRetries attempts. You can retry later from the app." -ForegroundColor Red
            }
        }

        $flagDir = Join-Path $BuildPath "engines\llm"
        $flagFile = Join-Path $flagDir ".install_embedded_mode"
        if (Test-Path $modelPath) {
            New-Item -ItemType Directory -Force -Path $flagDir | Out-Null
            Set-Content -Path $flagFile -Value "1" -Encoding UTF8
        }
    }
}

# --- Summary ---
Write-Host ""
Write-Host "=== Preparation Complete ===" -ForegroundColor Cyan
Write-Host "Build directory: $BuildPath"
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Run: windeployqt $BuildDir\Etemenanki.exe"
Write-Host "  2. Compile: installer\EtemenankiSetup.iss (Inno Setup 6)"
Write-Host "  3. Upload: installer\dist\EtemenankiSetup-*.exe to GitHub Releases"
Write-Host ""
