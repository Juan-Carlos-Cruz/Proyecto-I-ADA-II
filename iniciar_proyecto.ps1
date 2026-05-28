$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$noOpen = $false
foreach ($argument in $args) {
  if ($argument -eq "--no-open" -or $argument -eq "-NoOpen") {
    $noOpen = $true
  }
}

$rootDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pidFile = Join-Path $rootDir ".backend.pid"
$childPidFile = Join-Path $rootDir ".backend.child.pid"
$frontendFile = Join-Path $rootDir "riego_interfaz_frontend.html"
$buildDir = Join-Path $rootDir "build"
$logDir = Join-Path $rootDir "backend\logs"
$stdoutLog = Join-Path $logDir "backend.out.log"
$stderrLog = Join-Path $logDir "backend.err.log"
$backendBinary = Join-Path $rootDir "backend\bin\riego_backend.exe"
$port = if ($env:RIEGO_BACKEND_PORT) { [int]$env:RIEGO_BACKEND_PORT } else { 8080 }

function Test-Health {
  param([int]$Port)

  try {
    $response = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/health" -UseBasicParsing -TimeoutSec 2
    return $response.StatusCode -eq 200
  } catch {
    return $false
  }
}

function Get-ListeningPid {
  param([int]$Port)

  $netCommand = Get-Command Get-NetTCPConnection -ErrorAction SilentlyContinue
  if ($null -eq $netCommand) {
    return $null
  }

  $connection = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue |
    Select-Object -First 1

  if ($null -eq $connection) {
    return $null
  }

  return $connection.OwningProcess
}

if (Test-Health -Port $port) {
  $existingPid = Get-ListeningPid -Port $port
  if ($null -ne $existingPid) {
    Set-Content -Path $pidFile -Value $existingPid
    Set-Content -Path $childPidFile -Value $existingPid
  }

  Write-Host "Backend ya estaba activo en http://127.0.0.1:$port"
  Write-Host "Frontend listo en:"
  Write-Host "  $frontendFile"

  if (-not $noOpen) {
    Start-Process $frontendFile | Out-Null
  }

  exit 0
}

if ($null -eq (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw "No se encontro 'cmake' en PATH. Instala CMake para ejecutar el proyecto en Windows."
}

New-Item -ItemType Directory -Force -Path $logDir | Out-Null

Write-Host "Configurando compilacion nativa para Windows..."
& cmake -S $rootDir -B $buildDir

Write-Host "Compilando backend..."
& cmake --build $buildDir --config Release

if (-not (Test-Path $backendBinary)) {
  throw "No se encontro el backend compilado en $backendBinary"
}

Write-Host "Iniciando backend..."
$process = Start-Process -FilePath $backendBinary `
  -ArgumentList @("--port", "$port") `
  -WorkingDirectory $rootDir `
  -WindowStyle Hidden `
  -PassThru `
  -RedirectStandardOutput $stdoutLog `
  -RedirectStandardError $stderrLog

Set-Content -Path $pidFile -Value $process.Id
Set-Content -Path $childPidFile -Value $process.Id

$ready = $false
for ($attempt = 0; $attempt -lt 40; $attempt++) {
  Start-Sleep -Milliseconds 250

  if (Test-Health -Port $port) {
    $ready = $true
    break
  }

  if ($process.HasExited) {
    break
  }
}

if (-not $ready) {
  if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
  }

  Remove-Item -Force -ErrorAction SilentlyContinue $pidFile, $childPidFile
  throw "No se pudo iniciar el backend. Revisa:`n  $stdoutLog`n  $stderrLog"
}

Write-Host "Backend activo en http://127.0.0.1:$port"
Write-Host "Frontend listo en:"
Write-Host "  $frontendFile"
Write-Host "Logs del backend:"
Write-Host "  $stdoutLog"
Write-Host "  $stderrLog"

if (-not $noOpen) {
  Start-Process $frontendFile | Out-Null
}
