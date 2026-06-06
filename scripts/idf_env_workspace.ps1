# ESP-IDF environment for center (ESP32-P4). Dot-source before idf.py:
#   . .\scripts\idf_env_workspace.ps1
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$LeftRoot = Join-Path (Split-Path $ProjectRoot) "left-side-cluster-esp32s3"
$LeftTools = Join-Path $LeftRoot "tools\espidf"

if (-not $env:IDF_PATH) { $env:IDF_PATH = "C:\esp\v5.4.2\esp-idf" }
if (Test-Path $LeftTools) {
    $env:IDF_TOOLS_PATH = $LeftTools
} elseif (Test-Path "C:\Espressif\tools") {
    $env:IDF_TOOLS_PATH = "C:\Espressif"
}
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v5.4.2\venv"

$RiscvBin = "C:\Espressif\tools\tools\riscv32-esp-elf\esp-14.2.0_20241119\riscv32-esp-elf\bin"
$CmakeBin   = "C:\Espressif\tools\tools\cmake\3.30.2\bin"
$NinjaDir   = "C:\Espressif\tools\tools\ninja\1.12.1"
foreach ($dir in @($RiscvBin, $CmakeBin, $NinjaDir)) {
    if (Test-Path $dir) { $env:Path = "$dir;$env:Path" }
}
