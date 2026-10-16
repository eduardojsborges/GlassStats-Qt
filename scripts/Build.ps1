param([switch]$Package,[string]$Configuration='Release')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
$qt=Join-Path $root '.tools\qt'
if(-not(Test-Path -LiteralPath (Join-Path $qt 'lib\cmake\Qt6\Qt6Config.cmake'))){throw 'Run scripts\Bootstrap.ps1 first.'}
$vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(-not $vs){throw 'Visual Studio 2022 Build Tools with C++ x64 tools is required.'}
$dev=Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1'
& $dev -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
$env:PATH=(Join-Path $qt 'bin')+';'+$env:PATH
& cmake -S . -B build -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration" "-DCMAKE_PREFIX_PATH=$qt"
if($LASTEXITCODE -ne 0){throw 'CMake configure failed.'}
& cmake --build build --parallel 4
if($LASTEXITCODE -ne 0){throw 'C++ build failed.'}
& ctest --test-dir build --output-on-failure
if($LASTEXITCODE -ne 0){throw 'Core tests failed.'}
if($Package){& (Join-Path $PSScriptRoot 'Package-Windows.ps1')}
