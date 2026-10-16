param([Parameter(Mandatory=$true)][string]$UserSid)
$ErrorActionPreference = 'Stop'
$logDirectory = Join-Path ([Environment]::GetFolderPath('CommonApplicationData')) 'GlassStatsQt'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
if ((Get-Item -LiteralPath $logDirectory -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Unexpected helper log directory.' }
$logAcl = New-Object System.Security.AccessControl.DirectorySecurity
$logAcl.SetAccessRuleProtection($true, $false)
$logAcl.SetOwner((New-Object System.Security.Principal.SecurityIdentifier('S-1-5-32-544')))
foreach ($principal in @('S-1-5-18','S-1-5-32-544')) {
    $identity = New-Object System.Security.Principal.SecurityIdentifier($principal)
    $logAcl.AddAccessRule((New-Object System.Security.AccessControl.FileSystemAccessRule($identity, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')))
}
$logAcl.AddAccessRule((New-Object System.Security.AccessControl.FileSystemAccessRule((New-Object System.Security.Principal.SecurityIdentifier('S-1-5-32-545')), 'ReadAndExecute', 'ContainerInherit,ObjectInherit', 'None', 'Allow')))
Set-Acl -LiteralPath $logDirectory -AclObject $logAcl
function Write-SetupLog([string]$Message) {
    $temp = Join-Path $logDirectory ([Guid]::NewGuid().ToString('N') + '.tmp')
    [IO.File]::WriteAllText($temp, $Message)
    $target = Join-Path $logDirectory 'helper-setup.log'
    # Unlink an old log before replacing it; never truncate a preexisting hard-linked file as admin.
    [IO.File]::Delete($target)
    [IO.File]::Move($temp, $target)
}
try {
    $sid = New-Object System.Security.Principal.SecurityIdentifier($UserSid)
    if (-not $sid.IsAccountSid()) { throw 'An account SID is required.' }
    $packageRoot = Split-Path -Parent $PSScriptRoot
    $source = Join-Path $packageRoot 'sensor-bridge'
    $destination = Join-Path ([Environment]::GetFolderPath('ProgramFiles')) 'GlassStatsQtHelper'
    if (-not (Test-Path -LiteralPath (Join-Path $source 'GlassStats.SensorsBridge.exe'))) { throw 'The helper payload is missing.' }
    $driver = Join-Path $PSScriptRoot 'PawnIO_setup.exe'
    $signature = Get-AuthenticodeSignature -LiteralPath $driver
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'namazso') { throw 'The sensor driver signature is invalid.' }
    $driverVersion = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PawnIO' -ErrorAction SilentlyContinue
    $currentVersion = [Version]'0.0'
    if ($driverVersion.DisplayVersion) { [Version]::TryParse($driverVersion.DisplayVersion, [ref]$currentVersion) | Out-Null }
    if (-not (Test-Path 'HKLM:\SYSTEM\CurrentControlSet\Services\PawnIO') -or $currentVersion -lt [Version]'2.0') {
        $driverProcess = Start-Process -FilePath $driver -ArgumentList '-install' -Wait -PassThru -WindowStyle Hidden
        if ($driverProcess.ExitCode -ne 0) { throw "Sensor driver setup returned $($driverProcess.ExitCode)." }
    }
    if (-not (Test-Path 'HKLM:\SYSTEM\CurrentControlSet\Services\PawnIO')) { throw 'Sensor driver installation was not completed.' }
    $existingService = Get-Service -Name GlassStatsQtSensors -ErrorAction SilentlyContinue
    if ($existingService -and $existingService.Status -ne 'Stopped') { Stop-Service -Name GlassStatsQtSensors; $existingService.WaitForStatus('Stopped', [TimeSpan]::FromSeconds(15)) }
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    if ((Get-Item -LiteralPath $destination -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Unexpected helper installation directory.' }
    # Keep all privileged binaries/configuration writable only by administrators and SYSTEM.
    $acl = New-Object System.Security.AccessControl.DirectorySecurity
    $acl.SetAccessRuleProtection($true, $false)
    $acl.SetOwner((New-Object System.Security.Principal.SecurityIdentifier('S-1-5-32-544')))
    foreach ($principal in @('S-1-5-18','S-1-5-32-544')) {
        $identity = New-Object System.Security.Principal.SecurityIdentifier($principal)
        $rule = New-Object System.Security.AccessControl.FileSystemAccessRule($identity, 'FullControl', 'ContainerInherit,ObjectInherit', 'None', 'Allow')
        $acl.AddAccessRule($rule)
    }
    $users = New-Object System.Security.Principal.SecurityIdentifier('S-1-5-32-545')
    $acl.AddAccessRule((New-Object System.Security.AccessControl.FileSystemAccessRule($users, 'ReadAndExecute', 'ContainerInherit,ObjectInherit', 'None', 'Allow')))
    Set-Acl -LiteralPath $destination -AclObject $acl
    Get-ChildItem -LiteralPath $source | Copy-Item -Destination $destination -Recurse -Force
    $helperProcess = Start-Process -FilePath (Join-Path $destination 'GlassStats.SensorsBridge.exe') -ArgumentList @('--install',$sid.Value) -Wait -PassThru -WindowStyle Hidden
    if ($helperProcess.ExitCode -ne 0) { throw "Helper registration returned $($helperProcess.ExitCode)." }
    Write-SetupLog 'Temperature helper installed successfully.'
    exit 0
} catch {
    Write-SetupLog ($_ | Out-String)
    exit 1
}
