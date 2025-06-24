#define MyAppVersion GetEnv('RELEASE_VERSION')

[Setup]
AppName=Immersion
AppVersion={#MyAppVersion}
DefaultDirName={pf}\Immersion
DefaultGroupName=Immersion
OutputDir=build\installer
OutputBaseFilename=ImmersionSetup
Compression=lzma
SolidCompression=yes
SetupIconFile=resources\icon.ico

[Files]
Source: "build\Release\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Immersion"; Filename: "{app}\immersion.exe"
