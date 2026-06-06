# Build (unless -SkipBuild) and flash center cluster (ESP32-P4).
# Bootloader offset 0x2000 is in sdkconfig — idf.py flash uses build/flash_args.
#   .\scripts\flash_cluster.ps1
#   .\scripts\flash_cluster.ps1 -Port COM7 -Monitor
param(
    [string]$Port = "COM7",
    [switch]$SkipBuild,
    [switch]$Monitor
)
$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

. (Join-Path $PSScriptRoot "idf_env_workspace.ps1")

$logDir = Join-Path $ProjectRoot "build\log"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$py = Join-Path $env:IDF_PYTHON_ENV_PATH "Scripts\python.exe"
$idfPy = Join-Path $env:IDF_PATH "tools\idf.py"

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "prepare_flash.ps1")
}

Write-Host "Flashing center_cluster to $Port (P4 bootloader @ 0x2000) ..."
& $py $idfPy "-p$Port" flash 2>&1 | Tee-Object -FilePath (Join-Path $logDir "flash_$Port.log")
if ($LASTEXITCODE -ne 0) { throw "idf.py flash failed: $LASTEXITCODE" }

Write-Host "PASS - center cluster flashed to $Port"
if ($Monitor) {
    & $py $idfPy "-p$Port" monitor
}
