; Etemenanki Inno Setup Installer
; Build Release first:  cmake --build build --config Release
; Run windeployqt:      windeployqt build\Release\Etemenanki.exe
; Then compile this ISS with Inno Setup 6.

#define MyAppName "Etemenanki"
#define MyAppVersion "1.0.3"
#define MyAppPublisher "baddysays"
#define MyAppURL "https://github.com/Baddysays/Etemenanki"
#define MyAppExeName "Etemenanki.exe"
#define BuildDir "..\build\Release"

[Setup]
AppId={{A8F3E2B1-4C5D-6E7F-8A9B-0C1D2E3F4A5B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=dist
OutputBaseFilename=EtemenankiSetup-{#MyAppVersion}
SetupIconFile=..\assets\branding\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
DisableProgramGroupPage=yes
LicenseFile=
InfoBeforeFile=

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "install_python"; Description: "Установить Python 3.12 (встроенный)"; GroupDescription: "Зависимости:"; Flags: checkedonce
Name: "install_pip_deps"; Description: "Установить Python-библиотеки (PyMuPDF, docx, openpyxl)"; GroupDescription: "Зависимости:"; Flags: checkedonce
Name: "install_pdf2zh"; Description: "Скачать pdf2zh (PDF с вёрсткой, ~2 ГБ)"; GroupDescription: "Зависимости:"; Flags: unchecked
Name: "install_ollama_model"; Description: "Установить Ollama и модель translategemma:4b"; GroupDescription: "Зависимости:"; Flags: unchecked
Name: "install_embedded_model"; Description: "Скачать встроенную модель ИИ (~1,7 ГБ, без Ollama)"; GroupDescription: "Зависимости:"; Flags: checkedonce

[Files]
Source: "{#BuildDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\assets\*"; DestDir: "{app}\assets"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\tools\*"; DestDir: "{app}\tools"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\engines\*"; DestDir: "{app}\engines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\releases\*"; DestDir: "{app}\releases"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\qt*"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Etemenanki\*"; DestDir: "{app}\Etemenanki"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist; Excludes: "*.pdb"
Source: "{#BuildDir}\qml\*"; DestDir: "{app}\qml"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#BuildDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#BuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist
Source: "{#BuildDir}\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Удалить {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Запустить {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]

function VCRedistInstalled(): Boolean;
var
  installed: String;
begin
  Result := RegQueryStringValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\X64', 'Installed', installed) and (installed = '1');
end;

function PythonAvailable(): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec('python', '--version', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  if not Result then
    Result := Exec('py', '-3 --version', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
  if not Result then
    Result := FileExists(ExpandConstant('{app}\engines\python\python.exe'));
  if not Result then
    Result := FileExists(ExpandConstant('{app}\.venv\Scripts\python.exe'));
end;

function OllamaInstalled(): Boolean;
var
  ResultCode: Integer;
begin
  Result := Exec('ollama', '--version', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

function BuildDepsArgs(): String;
var
  args: String;
begin
  args := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{app}\tools\install_deps.ps1') + '"';
  args := args + ' -AppRoot "' + ExpandConstant('{app}') + '"';

  if WizardIsTaskSelected('install_python') then
    args := args + ' -InstallPython';
  if WizardIsTaskSelected('install_pip_deps') then
    args := args + ' -InstallPipDeps';
  if WizardIsTaskSelected('install_pdf2zh') then
    args := args + ' -InstallPdf2zh';
  if WizardIsTaskSelected('install_ollama_model') then
    args := args + ' -InstallOllama -OllamaModels translategemma:4b';
  if WizardIsTaskSelected('install_embedded_model') then
    args := args + ' -InstallEmbeddedModel';

  Result := args;
end;

function DepsInstallRequested(): Boolean;
begin
  Result := WizardIsTaskSelected('install_python') or
            WizardIsTaskSelected('install_pip_deps') or
            WizardIsTaskSelected('install_pdf2zh') or
            WizardIsTaskSelected('install_ollama_model') or
            WizardIsTaskSelected('install_embedded_model');
end;

procedure InstallVCRedistIfNeeded();
var
  ResultCode: Integer;
  vcUrl: String;
  vcPath: String;
begin
  if VCRedistInstalled() then
  begin
    WizardForm.StatusLabel.Caption := 'Microsoft Visual C++: уже установлен';
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Microsoft Visual C++: загрузка...';
  WizardForm.Update;
  vcUrl := 'https://aka.ms/vs/17/release/vc_redist.x64.exe';
  vcPath := ExpandConstant('{tmp}\vc_redist.x64.exe');
  if DownloadTemporaryFile(vcUrl, 'vc_redist.x64.exe', '', nil) <= 0 then
  begin
    MsgBox('Не удалось скачать Visual C++.' + #13#10 +
           'Установите вручную: ' + vcUrl, mbError, MB_OK);
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Microsoft Visual C++: установка (1–2 мин, окно может мигнуть)...';
  WizardForm.Update;
  Exec(vcPath, '/quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  if VCRedistInstalled() then
    MsgBox('Visual C++ установлен успешно.', mbInformation, MB_OK)
  else
    MsgBox('Visual C++ не подтверждён. Если программа не запускается, установите вручную:' + #13#10 +
           vcUrl, mbInformation, MB_OK);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  depsArgs: String;
begin
  if CurStep = ssPostInstall then
  begin
    InstallVCRedistIfNeeded();

    depsArgs := BuildDepsArgs();
    if DepsInstallRequested() then
    begin
      WizardForm.StatusLabel.Caption := 'Установка Python и модели (~1,7 ГБ). Не закрывайте окно PowerShell!';
      WizardForm.Update;
      Exec('powershell.exe', depsArgs, ExpandConstant('{app}'), SW_SHOW, ewWaitUntilTerminated, ResultCode);
      if ResultCode <> 0 then
      begin
        MsgBox('Часть компонентов не установилась (часто — не докачалась модель).' + #13#10 +
               'Запустите Etemenanki → Настройки → «Скачать встроенную модель».' + #13#10 +
               'Журнал: ' + ExpandConstant('{app}') + '\logs\install-deps.log', mbInformation, MB_OK);
      end
      else if WizardIsTaskSelected('install_embedded_model') then
        MsgBox('Установка завершена. При первом переводе может скачаться движок llama (ещё несколько минут).',
               mbInformation, MB_OK);
    end;
  end;
end;

procedure InitializeWizard();
begin
  WizardSelectTasks('install_python install_pip_deps install_embedded_model');
end;
