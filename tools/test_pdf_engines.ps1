# Smoke test for PDF engines (Etemenanki dev/CI)
param(
    [string]$Python = "",
    [string]$AppDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $Python) { $Python = Join-Path $root ".venv\Scripts\python.exe" }
if (-not (Test-Path $Python)) { throw "Python not found: $Python" }

if (-not $AppDir) { $AppDir = Join-Path $root "build\Release" }
$env:ETE_APP_DIR = $AppDir
$env:HF_HUB_DISABLE_SYMLINKS_WARNING = "1"

Write-Host "=== probe ==="
& $Python (Join-Path $root "tools\pdf_engines.py") probe --engine all
if ($LASTEXITCODE -ne 0) { throw "probe failed" }

$sample = Join-Path $root "test_sample.pdf"
if (-not (Test-Path $sample)) {
    $mk = @"
import fitz
d = fitz.open()
p = d.new_page()
p.insert_text((72, 72), 'Hello world. This is a test PDF for translation.', fontsize=14)
d.save(r'$sample')
d.close()
"@
    & $Python -c $mk
}

$outDir = Join-Path $env:TEMP "Etemenanki_pdf_test"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$outPdf = Join-Path $outDir "result.pdf"

Write-Host "=== translate (pdfmathtranslate) ==="
& $Python (Join-Path $root "tools\pdf_engines.py") translate `
    --engine pdfmathtranslate `
    --input $sample `
    --output $outPdf `
    --src-lang en --dst-lang ru `
    --runtime local `
    --model translategemma:4b `
    --ollama-url http://127.0.0.1:11434

if ($LASTEXITCODE -ne 0) { throw "translate failed with exit $LASTEXITCODE" }
if (-not (Test-Path $outPdf)) { throw "output PDF missing" }
$size = (Get-Item $outPdf).Length
if ($size -lt 500) { throw "output PDF too small: $size bytes" }

Write-Host "OK: $outPdf ($size bytes)"
