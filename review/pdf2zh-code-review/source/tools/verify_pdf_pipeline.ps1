# One-shot check: Python, pdf2zh patch, probe, optional translate.
param(
    [switch]$Translate,
    [string]$AppDir = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $AppDir) { $AppDir = Join-Path $root "build\Release" }
$py = Join-Path $root ".venv\Scripts\python.exe"
if (-not (Test-Path $py)) { throw "venv missing: $py" }

$env:ETE_APP_DIR = (Resolve-Path $AppDir).Path
$env:HF_HUB_DISABLE_SYMLINKS_WARNING = "1"
$tools = Join-Path $root "tools"

Write-Host "1/4 compat patch..."
& $py (Join-Path $tools "ensure_pdf2zh_compat.py") | Out-Host
if ($LASTEXITCODE -ne 0) { throw "compat failed" }

Write-Host "2/4 runner --version..."
& $py (Join-Path $AppDir "tools\pdf2zh_runner.py") --version 2>&1 | Out-Host
if ($LASTEXITCODE -ne 0) { throw "runner failed" }

Write-Host "3/4 probe..."
& $py (Join-Path $AppDir "tools\pdf_engines.py") probe --engine pdfmathtranslate 2>&1 | Out-Host
if ($LASTEXITCODE -ne 0) { throw "probe failed" }

if ($Translate) {
    Write-Host "4/4 translate (needs Ollama + translategemma:4b)..."
    $sample = Join-Path $root "test_sample.pdf"
    if (-not (Test-Path $sample)) {
        & $py -c "import fitz; d=fitz.open(); p=d.new_page(); p.insert_text((72,72),'Hello PDF test.',fontsize=14); d.save(r'$sample'); d.close()"
    }
    $out = Join-Path $env:TEMP "etemenanki_verify_out.pdf"
    & $py (Join-Path $AppDir "tools\pdf_engines.py") translate `
        --engine pdfmathtranslate --input $sample --output $out `
        --src-lang en --dst-lang ru --model translategemma:4b 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "translate failed" }
    if (-not (Test-Path $out)) { throw "no output: $out" }
    Write-Host "OK translate -> $out"
} else {
    Write-Host "4/4 skipped (use -Translate for full test)"
}

Write-Host "All checks passed."
