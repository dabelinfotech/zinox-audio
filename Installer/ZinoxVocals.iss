; Zinox Vocals - Windows installer
; Packages the VST3 plugin and Standalone application produced by the CMake
; build (see ..\build.ps1) into a single distributable setup executable.

#define MyAppName "Zinox Vocals"

; Overridable from the command line with /DMyAppVersion=1.2.3 (CI does this
; from the git tag). The #ifndef guard matters: an unconditional #define
; here would silently clobber a value already passed in via /D.
#ifndef MyAppVersion
  #define MyAppVersion "1.1.0"
#endif

#define MyAppPublisher "Zinox Audio"
#define MyAppURL "https://zinoxaudio.com"
#define MyAppExeName "Zinox Vocals.exe"
#define BuildDir "..\build\ZinoxVocals_artefacts\Release"

[Setup]
AppId={{6C9E6C1A-6E6D-4B62-9F7B-6B2E7B2F0A11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=Output
OutputBaseFilename=ZinoxVocals-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut for the Standalone app"; GroupDescription: "Additional icons:"

[Files]
; VST3 plugin - installed as a bundle folder under the shared VST3 directory
; so every host on the machine can see it.
Source: "{#BuildDir}\VST3\Zinox Vocals.vst3\*"; DestDir: "{commoncf64}\VST3\Zinox Vocals.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; Standalone application, for auditioning the plugin without a DAW.
Source: "{#BuildDir}\Standalone\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Zinox Vocals.vst3"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName} now"; Flags: nowait postinstall skipifsilent
