param(
    [switch]$InstallPython,
    [switch]$InstallPipDeps,
    [switch]$InstallPdf2zh,
    [switch]$InstallOllama,
    [switch]$InstallEmbeddedModel,
    [string[]]$OllamaModels = @(),
    [string]$AppRoot = ""
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$LogFile = Join-Path $env:TEMP "Etemenanki-install-deps.log"
if (Test-Path $LogFile) { Remove-Item $LogFile -Force -ErrorAction SilentlyContinue }

$form = New-Object System.Windows.Forms.Form
$form.Text = "Etemenanki — установка компонентов"
$form.Size = New-Object System.Drawing.Size(520, 220)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.TopMost = $true

$title = New-Object System.Windows.Forms.Label
$title.Location = New-Object System.Drawing.Point(16, 16)
$title.Size = New-Object System.Drawing.Size(480, 24)
$title.Text = "Подождите, идёт загрузка и установка…"
$title.Font = New-Object System.Drawing.Font("Segoe UI", 10, [System.Drawing.FontStyle]::Bold)
$form.Controls.Add($title)

$status = New-Object System.Windows.Forms.Label
$status.Location = New-Object System.Drawing.Point(16, 44)
$status.Size = New-Object System.Drawing.Size(480, 40)
$status.Text = "Подготовка…"
$form.Controls.Add($status)

$bar = New-Object System.Windows.Forms.ProgressBar
$bar.Location = New-Object System.Drawing.Point(16, 92)
$bar.Size = New-Object System.Drawing.Size(480, 24)
$bar.Style = "Continuous"
$bar.Minimum = 0
$bar.Maximum = 100
$bar.Value = 0
$form.Controls.Add($bar)

$hint = New-Object System.Windows.Forms.Label
$hint.Location = New-Object System.Drawing.Point(16, 128)
$hint.Size = New-Object System.Drawing.Size(480, 48)
$hint.ForeColor = [System.Drawing.Color]::DimGray
$hint.Text = "Не закрывайте это окно. Скачивание модели ~1,7 ГБ может занять 10–30 минут."
$form.Controls.Add($hint)

$script:LastPct = 0
$script:PhaseBase = 0

function Set-UiProgress([int]$pct, [string]$msg) {
    $p = [Math]::Max(0, [Math]::Min(100, $pct))
    if ($p -ge $script:LastPct) {
        $script:LastPct = $p
        $bar.Value = $p
    }
    if ($msg) { $status.Text = $msg }
    $form.Refresh()
    [System.Windows.Forms.Application]::DoEvents()
}

function Apply-LogLine([string]$line) {
    if (-not $line) { return }

    if ($line -match 'ETEMENANKI_PROGRESS\s+(\d+)') {
        $raw = [int]$matches[1]
        $pct = [Math]::Max($script:LastPct, $raw)
        Set-UiProgress $pct ("Скачивание встроенной модели: $raw%")
        return
    }
    if ($line -match 'ETEMENANKI_PHASE\s+(\S+)') {
        switch ($matches[1]) {
            "pip"   { Set-UiProgress 8 "Установка пакетов Python…" }
            "download" { Set-UiProgress 12 "Скачивание модели (~1,7 ГБ)…" }
            default { }
        }
        return
    }

    if ($line -match 'Downloading Python|Downloading get-pip') {
        Set-UiProgress 3 "Скачивание Python 3.12…"
    }
    elseif ($line -match 'Extracting Python') {
        Set-UiProgress 6 "Распаковка Python…"
    }
    elseif ($line -match 'Installing Python dependencies|Installing pip') {
        Set-UiProgress 10 "Установка библиотек для документов…"
    }
    elseif ($line -match 'Installing huggingface_hub') {
        Set-UiProgress 14 "Подготовка загрузки модели…"
    }
    elseif ($line -match 'Downloading built-in GGUF|Downloading built-in') {
        Set-UiProgress 18 "Скачивание встроенной модели (~1,7 ГБ)…"
    }
    elseif ($line -match 'Downloading pdf2zh') {
        Set-UiProgress 5 "Скачивание pdf2zh…"
    }
    elseif ($line -match 'Pulling model:') {
        Set-UiProgress 20 ("Ollama: " + ($line -replace '.*Pulling model:\s*', ''))
    }
    elseif ($line -match '=== Done \(OK\)') {
        Set-UiProgress 100 "Готово!"
    }
}

$depsScript = Join-Path $PSScriptRoot "install_deps.ps1"
$argList = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $depsScript)
if ($AppRoot) { $argList += @("-AppRoot", $AppRoot) }
if ($InstallPython) { $argList += "-InstallPython" }
if ($InstallPipDeps) { $argList += "-InstallPipDeps" }
if ($InstallPdf2zh) { $argList += "-InstallPdf2zh" }
if ($InstallOllama) { $argList += "-InstallOllama" }
if ($InstallEmbeddedModel) { $argList += "-InstallEmbeddedModel" }
foreach ($m in $OllamaModels) {
    if ($m) { $argList += @("-OllamaModels", $m) }
}

$proc = Start-Process -FilePath "powershell.exe" `
    -ArgumentList $argList `
    -PassThru `
    -WindowStyle Hidden `
    -WorkingDirectory $(if ($AppRoot) { $AppRoot } else { (Split-Path -Parent $PSScriptRoot) })

$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 400
$script:LogPos = 0

$timer.Add_Tick({
    if (Test-Path $LogFile) {
        $content = Get-Content -Path $LogFile -Encoding UTF8 -ErrorAction SilentlyContinue
        if ($content -and $content.Count -gt $script:LogPos) {
            for ($i = $script:LogPos; $i -lt $content.Count; $i++) {
                Apply-LogLine $content[$i]
            }
            $script:LogPos = $content.Count
        }
    }
    if ($proc.HasExited) {
        $timer.Stop()
        if (Test-Path $LogFile) {
            $content = Get-Content -Path $LogFile -Encoding UTF8 -ErrorAction SilentlyContinue
            if ($content) {
                foreach ($ln in $content) { Apply-LogLine $ln }
            }
        }
        if ($proc.ExitCode -eq 0) {
            Set-UiProgress 100 "Установка завершена успешно."
            Start-Sleep -Milliseconds 800
            $form.DialogResult = [System.Windows.Forms.DialogResult]::OK
            $form.Close()
        } else {
            $status.Text = "Ошибка установки (код $($proc.ExitCode)). Откройте Настройки → журнал установки."
            $status.ForeColor = [System.Drawing.Color]::DarkRed
            $hint.Text = "Журнал: $(if ($AppRoot) { Join-Path $AppRoot 'logs\install-deps.log' } else { $LogFile })"
            $bar.Style = "Continuous"
        }
    }
})

$timer.Start()
[void]$form.ShowDialog()
exit $(if ($proc.ExitCode -ne $null) { $proc.ExitCode } else { 1 })
