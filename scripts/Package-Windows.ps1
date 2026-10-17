param([string]$InnoCompiler='',[string]$VcRedistributable='',[string]$PawnIoSetup='',[switch]$SkipInstaller)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
$qt=Join-Path $root '.tools\qt'
$env:PATH=(Join-Path $qt 'bin')+';'+$env:PATH
$payload=Join-Path $root 'artifacts\windows\GlassStats'
New-Item -ItemType Directory -Path artifacts -Force|Out-Null
& cmake --install build --prefix $payload --component Unspecified *> artifacts\deploy.log
if($LASTEXITCODE -ne 0){throw 'Qt deployment failed.'}
& "$qt\bin\windeployqt.exe" --release --verbose 0 --qmldir "$root\qml" --no-translations --compiler-runtime "$payload\GlassStats.exe" >> artifacts\deploy.log 2>&1
if($LASTEXITCODE -ne 0){throw 'QML deployment failed.'}
& "$PSScriptRoot\Copy-CompilerRuntime.ps1" -Destination $payload
& "$root\.tools\python\Scripts\python.exe" scripts\prepare_licenses.py --output "$payload\licenses\Qt" --sources-output "$root\artifacts\corresponding-sources"
if($LASTEXITCODE -ne 0){throw 'Qt source/license preparation failed.'}
& "$PSScriptRoot\Build-SensorBridge.ps1" -OutputDirectory "$payload\sensor-bridge"
New-Item -ItemType Directory -Path "$payload\setup" -Force|Out-Null
Copy-Item -Path packaging\windows\setup\*.ps1 -Destination "$payload\setup" -Force
Copy-Item -LiteralPath README.md,DEVELOPMENT.md,VALIDATION.md,CHANGELOG.md,LICENSE -Destination $payload -Force
Copy-Item -LiteralPath assets\GlassStats.ico -Destination $payload -Force
New-Item -ItemType Directory -Path "$payload\assets" -Force|Out-Null
Copy-Item -LiteralPath assets\glassstats-overview.png -Destination "$payload\assets\glassstats-overview.png" -Force
Copy-Item -Path docs -Destination $payload -Recurse -Force
if(-not $VcRedistributable){$VcRedistributable=Join-Path $root '..\GlassStats\.tools\vc_redist.x64.exe'}
if(-not $PawnIoSetup){$PawnIoSetup=Join-Path $root '..\GlassStats\.tools\reference\LibreHardwareMonitor-0.9.6\LibreHardwareMonitor\Resources\PawnIO_setup.exe'}
foreach($inputFile in @(@($VcRedistributable,'Microsoft','vc_redist.x64.exe'),@($PawnIoSetup,'namazso','PawnIO_setup.exe'))){$signature=Get-AuthenticodeSignature -LiteralPath $inputFile[0];if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch $inputFile[1]){throw ('Invalid prerequisite signature: '+$inputFile[2])};Copy-Item -LiteralPath $inputFile[0] -Destination (Join-Path "$payload\setup" $inputFile[2]) -Force}
$name='GlassStats-Qt-0.2.0-alpha.1-windows-x64'
$zip=Join-Path $root "artifacts\$name-portable.zip"
& tar.exe -a -cf $zip -C (Split-Path -Parent $payload) GlassStats
if($LASTEXITCODE -ne 0){throw 'Portable ZIP creation failed.'}
if(-not $SkipInstaller){if(-not $InnoCompiler){$InnoCompiler=Join-Path $root '..\GlassStats\.tools\inno\ISCC.exe'};& $InnoCompiler /Qp "/DPayloadDir=$payload" packaging\windows\GlassStats.iss;if($LASTEXITCODE -ne 0){throw 'Installer compilation failed.'}}
Get-ChildItem artifacts -File | Where-Object Name -Match ('^'+[regex]::Escape($name)+'.*\.(exe|zip)$') | ForEach-Object { $hash=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(); ($hash+'  '+$_.Name)|Set-Content -LiteralPath ($_.FullName+'.sha256') -Encoding ascii }
