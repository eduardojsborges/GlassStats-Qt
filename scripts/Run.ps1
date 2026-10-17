param([string]$Profile='',[switch]$Build)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
if($Build){& "$PSScriptRoot\Build.ps1"}
$executable=Join-Path $root 'build\GlassStats.exe'
if(-not(Test-Path -LiteralPath $executable)){throw 'Build the app with scripts\Build.ps1 first.'}
$env:PATH=(Join-Path $root '.tools\qt\bin')+';'+$env:PATH
$env:QT_PLUGIN_PATH=Join-Path $root '.tools\qt\plugins'
$env:QML_IMPORT_PATH=Join-Path $root '.tools\qt\qml'
$arguments=@()
if($Profile){$arguments+=@('--profile',('"'+[IO.Path]::GetFullPath($Profile)+'"'))}
# The native window is visible; the helper/launcher should not create a console.
if($arguments.Count){Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden}
else {Start-Process -FilePath $executable -WindowStyle Hidden}
