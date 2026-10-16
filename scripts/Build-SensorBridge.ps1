param([string]$OutputDirectory='artifacts\GlassStats\sensor-bridge')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
$dotnet=if($env:DOTNET_EXE){$env:DOTNET_EXE}elseif(Test-Path -LiteralPath "$root\.tools\dotnet\dotnet.exe"){"$root\.tools\dotnet\dotnet.exe"}elseif(Test-Path -LiteralPath "$root\..\GlassStats\.tools\dotnet\dotnet.exe"){"$root\..\GlassStats\.tools\dotnet\dotnet.exe"}else{'dotnet'}
$env:DOTNET_CLI_HOME="$root\.tools\cli-home"
$env:DOTNET_CLI_TELEMETRY_OPTOUT='1'
$env:DOTNET_GENERATE_ASPNET_CERTIFICATE='false'
$env:NUGET_PACKAGES=if(Test-Path -LiteralPath "$root\..\GlassStats\.tools\nuget"){"$root\..\GlassStats\.tools\nuget"}else{"$root\.tools\nuget"}
& $dotnet publish sensor-bridge\GlassStats.SensorsBridge.csproj -c Release -r win-x64 --self-contained true -p:PublishTrimmed=false -o $OutputDirectory
if($LASTEXITCODE -ne 0){throw 'Sensor bridge build failed.'}
