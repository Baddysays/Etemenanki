; Inno Setup script for Etemenanki (Windows x64)
; Build Release first, then compile this script with Inno Setup 6.

#define MyAppName "Etemenanki"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "baddysays"
#define MyAppURL "https://github.com/Baddysays/Etemenanki"
#define MyAppExeName "Etemenanki.exe"
#define BuildDir "..\build\Release"
#define SetupOut "etemenanki-setup.exe"

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
SetupIconFile={#BuildDir}\assets\branding\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
WizardStyle=modern
WizardImageFile=
WizardSmallImageFile=
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#BuildDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb,*.lib,*.exp"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\tools\setup_pdf2zh.ps1"""; WorkingDir: "{app}"; StatusMsg: "Downloading PDF engine (pdf2zh)..."; Flags: runhidden waituntilterminated; Check: FileExists(ExpandConstant('{app}\tools\setup_pdf2zh.ps1'))

[Code]
function InitializeSetup(): Boolean;
begin
  if not FileExists(ExpandConstant('{#BuildDir}\{#MyAppExeName}')) then
  begin
    MsgBox('Build not found. Run: cmake --build build --config Release', mbError, MB_OK);
    Result := False;
  end
  else
    Result := True;
end;
