#define MyAppName "Xiaomi Light Bar"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "salem"
#define MyAppURL "https://github.com/salem-5"
#define MyAppExeName "XiaomiLightBar.exe"

#ifndef Arch
  #define Arch "x64"
#endif
#ifndef SourceDir
  #define SourceDir "build\Release"
#endif
#ifndef OutputDir
  #define OutputDir "installer"
#endif

[Setup]
AppId={{B7F3A2C1-9E4D-4A6B-8C2F-1D3E5A7B9C0D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=XiaomiLightBar-{#MyAppVersion}-{#Arch}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed={#Arch}
ArchitecturesInstallIn64BitMode={#Arch}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\resources.pri"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\Microsoft.WindowsAppRuntime.Bootstrap.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\app.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\app.png"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\WindowsAppRuntimeInstall.exe"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\WindowsAppRuntimeInstall.exe"; Parameters: "--quiet"; Flags: runhidden waituntilterminated skipifdoesntexist
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
