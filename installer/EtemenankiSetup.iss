; Etemenanki Inno Setup Installer — Self-Contained
; 
; Build steps:
;   1. cmake --build build --config Release
;   2. scripts\prepare_release.ps1          (downloads model + Python into build\Release)
;   3. windeployqt build\Release\Etemenanki.exe
;   4. Compile this ISS with Inno Setup 6
;
; Result: single .exe installer (~2.0–2.1 GB) with TranslateGemma 4B included.
; User installs in 2 clicks — no post-install model download needed.

#define MyAppName "Etemenanki"
#define MyAppVersion "1.0.5"
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
SetupLogging=yes

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "install_pdf2zh"; Description: "Скачать pdf2zh (PDF с вёрсткой, ~200 МБ, опционально)"; GroupDescription: "Дополнительно:"; Flags: unchecked

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

procedure InstallVCRedistIfNeeded();
var
  ResultCode: Integer;
  vcUrl: String;
  vcPath: String;
begin
  if VCRedistInstalled() then
  begin
    WizardForm.StatusLabel.Caption := 'Visual C++: уже установлен';
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Visual C++: загрузка и установка...';
  WizardForm.Update;
  vcUrl := 'https://aka.ms/vs/17/release/vc_redist.x64.exe';
  vcPath := ExpandConstant('{tmp}\vc_redist.x64.exe');
  if DownloadTemporaryFile(vcUrl, 'vc_redist.x64.exe', '', nil) <= 0 then
  begin
    MsgBox('Не удалось скачать Visual C++ Redistributable.' + #13#10 +
           'Скачайте вручную: ' + vcUrl, mbError, MB_OK);
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Visual C++: установка (окно может мигнуть)...';
  WizardForm.Update;
  Exec(vcPath, '/quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

procedure InstallPdf2zh();
var
  ResultCode: Integer;
  scriptPath: String;
begin
  scriptPath := ExpandConstant('{app}\tools\setup_pdf2zh.ps1');
  if not FileExists(scriptPath) then
    Exit;

  WizardForm.StatusLabel.Caption := 'Скачивание pdf2zh (~200 МБ)...';
  WizardForm.Update;
  Exec('powershell.exe', '-NoProfile -ExecutionPolicy Bypass -File "' + scriptPath + '"',
       ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    InstallVCRedistIfNeeded();

    if WizardIsTaskSelected('install_pdf2zh') then
      InstallPdf2zh();

    WizardForm.StatusLabel.Caption := 'Готово! Нажмите «Завершить».';
    WizardForm.Update;
  end;
end;
