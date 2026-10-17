#define AppVersion "0.2.0-alpha.1"
#ifndef PayloadDir
#define PayloadDir "..\..\artifacts\GlassStats"
#endif
[Setup]
#ifdef QaInstall
AppId={{7E440641-F1E3-41BB-A788-7AABF512A6D8}
AppName=GlassStats Qt QA
OutputBaseFilename=GlassStats-Qt-QA-Setup
PrivilegesRequired=lowest
#else
AppId={{A2B5AC4F-3E70-49C2-87E6-3BAE90594026}
AppName=GlassStats Qt
OutputBaseFilename=GlassStats-Qt-0.2.0-alpha.1-windows-x64-setup
PrivilegesRequired=admin
#endif
AppVersion={#AppVersion}
AppVerName=GlassStats Qt {#AppVersion}
VersionInfoVersion=0.2.0.0
DefaultDirName={autopf}\GlassStatsQt
DefaultGroupName=GlassStats Qt
OutputDir=..\..\artifacts
SetupIconFile=..\..\assets\GlassStats.ico
UninstallDisplayIcon={app}\GlassStats.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.19045
#ifdef QaInstall
Compression=none
SolidCompression=no
#else
Compression=lzma2
SolidCompression=yes
#endif
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

[Files]
Source: "{#PayloadDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\GlassStats Qt"; Filename: "{app}\GlassStats.exe"
Name: "{group}\Uninstall GlassStats Qt"; Filename: "{uninstallexe}"
[Run]
#ifndef QaInstall
Filename: "{app}\setup\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Preparing the Microsoft native runtime…"; Flags: waituntilterminated; Check: NeedsNativeRuntime
#endif
Filename: "{app}\GlassStats.exe"; Description: "Launch GlassStats Qt"; Flags: nowait postinstall skipifsilent runasoriginaluser
[UninstallRun]
#ifndef QaInstall
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\setup\Remove-Helper.ps1"""; Flags: runhidden waituntilterminated; RunOnceId: "RemoveGlassStatsQtSensors"
#endif
[Code]
function NeedsNativeRuntime: Boolean;
var Installed, Major, Minor: Cardinal;
begin
  Result := True;
  if RegQueryDWordValue(HKLM64, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Installed) and
     RegQueryDWordValue(HKLM64, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Major', Major) and
     RegQueryDWordValue(HKLM64, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Minor', Minor) then
    Result := (Installed <> 1) or (Major < 14) or ((Major = 14) and (Minor < 44));
end;
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var UserKeys: TArrayOfString; Index: Integer; KeyPath, Value, ExpectedPrefix: String;
begin
  if CurUninstallStep = usUninstall then begin
    ExpectedPrefix := '"' + Lowercase(ExpandConstant('{app}')) + '\glassstats.exe"';
    if RegGetSubkeyNames(HKU, '', UserKeys) then
      for Index := 0 to GetArrayLength(UserKeys) - 1 do begin
        KeyPath := UserKeys[Index] + '\Software\Microsoft\Windows\CurrentVersion\Run';
        if RegQueryStringValue(HKU, KeyPath, 'GlassStatsQt', Value) then
          if Pos(ExpectedPrefix, Lowercase(Value)) = 1 then RegDeleteValue(HKU, KeyPath, 'GlassStatsQt');
      end;
  end;
end;
