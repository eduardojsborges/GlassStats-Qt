$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $root
$python=Join-Path $root '.tools\python\Scripts\python.exe'
if(-not(Test-Path -LiteralPath $python)){& python -m venv .tools\python}
& $python -m pip install --disable-pip-version-check py7zr==1.1.4
if($LASTEXITCODE -ne 0){throw 'Qt extraction tool setup failed.'}
& $python scripts\download_qt.py --host windows
if($LASTEXITCODE -ne 0){throw 'Qt setup failed.'}
