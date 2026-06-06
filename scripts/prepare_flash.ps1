# Build center firmware for morning flash (no USB).
#   .\scripts\prepare_flash.ps1
#   .\scripts\prepare_flash.ps1 -FullClean
param([switch]$FullClean)
$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location $ProjectRoot

. (Join-Path $PSScriptRoot "idf_env_workspace.ps1")

$logDir = Join-Path $ProjectRoot "build\log"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$py = Join-Path $env:IDF_PYTHON_ENV_PATH "Scripts\python.exe"
$idfPy = Join-Path $env:IDF_PATH "tools\idf.py"

if ($FullClean) {
    & $py $idfPy fullclean
    if ($LASTEXITCODE -ne 0) { throw "idf.py fullclean failed: $LASTEXITCODE" }
}

$logFile = Join-Path $logDir "prepare_flash.log"
& $py $idfPy build *> $logFile
if ($LASTEXITCODE -ne 0) {
    Get-Content $logFile -Tail 30
    throw "idf.py build failed: $LASTEXITCODE"
}

$bin = Join-Path $ProjectRoot "build\center_cluster.bin"
if (-not (Test-Path $bin)) { throw "Missing $bin" }

$flashArgs = Join-Path $ProjectRoot "build\flash_args"
if (Test-Path $flashArgs) {
    $argsText = Get-Content -Raw $flashArgs
    if ($argsText -notmatch "0x2000") {
        Write-Warning "flash_args missing 0x2000 bootloader offset - P4 may not boot. Check sdkconfig BOOTLOADER_OFFSET."
    } else {
        Write-Host 'OK  build/flash_args uses bootloader @ 0x2000'
    }
}

$head = git -C $ProjectRoot rev-parse --short HEAD 2>$null
Write-Host ""
Write-Host "READY  center_cluster commit $head"
Write-Host "       $bin size $((Get-Item $bin).Length) bytes"
Write-Host "Flash: .\scripts\flash_cluster.ps1 -Port COM7"
