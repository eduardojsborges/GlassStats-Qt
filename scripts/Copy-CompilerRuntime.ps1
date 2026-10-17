param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference='Stop'
$redist=$env:VCToolsRedistDir
if(-not $redist){
    $vswhere="${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vs=& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $redist=(Get-ChildItem -LiteralPath "$vs\VC\Redist\MSVC" -Directory | Where-Object Name -Match '^\d+\.\d+\.\d+$' | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1).FullName
}
$crt=Join-Path $redist 'x64\Microsoft.VC143.CRT'
if(-not(Test-Path -LiteralPath "$crt\msvcp140.dll")){throw 'The MSVC x64 redistributable DLL directory is unavailable.'}
New-Item -ItemType Directory -Path $Destination -Force|Out-Null
foreach($dll in Get-ChildItem -LiteralPath $crt -Filter '*.dll'){
    $signature=Get-AuthenticodeSignature -LiteralPath $dll.FullName
    if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft'){throw ('Unverified MSVC runtime: '+$dll.Name)}
    Copy-Item -LiteralPath $dll.FullName -Destination $Destination -Force
}
