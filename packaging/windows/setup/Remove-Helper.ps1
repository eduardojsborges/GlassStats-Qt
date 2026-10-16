param([string]$UserSid)
$ErrorActionPreference = 'Stop'
try {
    $programFilesRoot = [IO.Path]::GetFullPath([Environment]::GetFolderPath('ProgramFiles'))
    $destination = [IO.Path]::GetFullPath((Join-Path $programFilesRoot 'GlassStatsQtHelper'))
    if ($destination -ne (Join-Path $programFilesRoot 'GlassStatsQtHelper')) { throw 'Unexpected helper path.' }
    if (Test-Path -LiteralPath $destination) {
        $pending = New-Object 'System.Collections.Generic.Queue[string]'
        $pending.Enqueue($destination)
        while ($pending.Count -gt 0) {
            $directory = $pending.Dequeue()
            if ((Get-Item -LiteralPath $directory -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Helper removal refuses linked directories.' }
            foreach ($entry in (Get-ChildItem -LiteralPath $directory -Force)) {
                if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Helper removal refuses linked files or directories.' }
                if ($entry.PSIsContainer) { $pending.Enqueue($entry.FullName) }
            }
        }
    }
    $service = Get-Service -Name GlassStatsQtSensors -ErrorAction SilentlyContinue
    if ($service -and $service.Status -ne 'Stopped') { Stop-Service -Name GlassStatsQtSensors; $service.WaitForStatus('Stopped', [TimeSpan]::FromSeconds(15)) }
    $helper = Join-Path $destination 'GlassStats.SensorsBridge.exe'
    if (Test-Path -LiteralPath $helper) {
        $process = Start-Process -FilePath $helper -ArgumentList '--uninstall' -Wait -PassThru -WindowStyle Hidden
        if ($process.ExitCode -ne 0) { throw 'Helper removal failed.' }
    } elseif ($service) {
        & "$env:SystemRoot\System32\sc.exe" delete GlassStatsQtSensors | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'Orphaned helper registration could not be removed.' }
    }
    if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Recurse -Force }
    # PawnIO is shared by other monitoring apps; leave its driver installed.
    exit 0
} catch { exit 1 }
