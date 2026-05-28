$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pidFile = Join-Path $rootDir ".backend.pid"
$childPidFile = Join-Path $rootDir ".backend.child.pid"
$port = if ($env:RIEGO_BACKEND_PORT) { [int]$env:RIEGO_BACKEND_PORT } else { 8080 }

function Stop-TrackedProcess {
  param(
    [string]$FilePath,
    [string]$Label
  )

  if (-not (Test-Path $FilePath)) {
    return $false
  }

  $rawPid = (Get-Content -Path $FilePath -ErrorAction SilentlyContinue | Select-Object -First 1)
  if ([string]::IsNullOrWhiteSpace($rawPid)) {
    return $false
  }

  $processId = [int]$rawPid.Trim()

  try {
    $process = Get-Process -Id $processId -ErrorAction Stop
    Stop-Process -Id $process.Id -Force
    Write-Host "$Label detenido (PID $processId)."
    return $true
  } catch {
    Write-Host "El proceso registrado en $FilePath ya no estaba activo."
    return $false
  }
}

$stopped = $false
$stopped = (Stop-TrackedProcess -FilePath $pidFile -Label "Proceso backend") -or $stopped
$stopped = (Stop-TrackedProcess -FilePath $childPidFile -Label "Proceso backend") -or $stopped

$netCommand = Get-Command Get-NetTCPConnection -ErrorAction SilentlyContinue
if ($null -ne $netCommand) {
  $owningProcesses = Get-NetTCPConnection -LocalPort $port -State Listen -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty OwningProcess -Unique

  foreach ($processId in $owningProcesses) {
    try {
      Stop-Process -Id $processId -Force
      Write-Host "Proceso relacionado detenido (PID $processId)."
      $stopped = $true
    } catch {
    }
  }
}

Remove-Item -Force -ErrorAction SilentlyContinue $pidFile, $childPidFile

if (-not $stopped) {
  Write-Host "No se encontro un backend activo en el puerto $port."
}
