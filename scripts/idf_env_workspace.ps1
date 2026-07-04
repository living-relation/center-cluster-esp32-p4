# ESP-IDF environment for center (ESP32-P4). Dot-source before idf.py:
#   . .\scripts\idf_env_workspace.ps1
#
# The exact ESP-IDF install location varies per PC, so this auto-detects a 5.4.x
# install rather than hard-coding one path. It supports:
#   - an already-activated shell (ESP-IDF terminal / prior export)  -> reused as-is
#   - the ESP-IDF Installation Manager (EIM) layout, recorded in eim_idf.json
#     (often nested, e.g. C:\esp\v5.4.2\v5.4.2\esp-idf)
#   - the classic offline-installer layout (C:\esp\v5.4.2\esp-idf + C:\Espressif)
# 5.4.x is required for the P4 v1.x silicon (not 5.3.x, not 5.5+). See
# SETUP_BEFORE_YOU_BUILD.txt.

function Test-IdfEnvReady {
    if (-not $env:IDF_PATH) { return $false }
    if (-not (Test-Path (Join-Path $env:IDF_PATH 'tools\idf.py'))) { return $false }
    if (-not $env:IDF_PYTHON_ENV_PATH) { return $false }
    if (-not (Test-Path (Join-Path $env:IDF_PYTHON_ENV_PATH 'Scripts\python.exe'))) { return $false }
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) { return $false }
    return $true
}

function Enable-IdfInstall {
    param([string]$IdfPath, [string]$ToolsPath, [string]$ActivationScript)

    if ($ActivationScript -and (Test-Path $ActivationScript)) {
        . $ActivationScript | Out-Null
        return
    }
    # Fall back to ESP-IDF's own export.ps1 (sets PATH, tools, python venv).
    $export = Join-Path $IdfPath 'export.ps1'
    if (Test-Path $export) {
        $env:IDF_PATH = $IdfPath
        if ($ToolsPath) { $env:IDF_TOOLS_PATH = $ToolsPath }
        . $export | Out-Null
    }
}

# 1) Respect an already-activated environment (e.g. an ESP-IDF terminal).
if (Test-IdfEnvReady) { return }

# 2) EIM manifest(s): prefer a 5.4.x install, else the selected one.
$eimManifests = @(
    'C:\Espressif\tools\eim_idf.json',
    (Join-Path $env:USERPROFILE '.espressif\tools\eim_idf.json'),
    'C:\esp\tools\eim_idf.json'
)
foreach ($manifest in $eimManifests) {
    if (Test-IdfEnvReady) { break }
    if (-not (Test-Path $manifest)) { continue }
    try {
        $eim = Get-Content -Raw $manifest | ConvertFrom-Json
    } catch { continue }
    $installs = @($eim.idfInstalled) | Where-Object { $_ -and $_.path -and (Test-Path $_.path) }
    if (-not $installs) { continue }
    $pick = $installs | Where-Object { $_.name -like '*5.4*' -or $_.path -like '*5.4*' } | Select-Object -First 1
    if (-not $pick) { $pick = $installs | Where-Object { $_.id -eq $eim.idfSelectedId } | Select-Object -First 1 }
    if (-not $pick) { continue }
    Enable-IdfInstall -IdfPath $pick.path -ToolsPath $pick.idfToolsPath -ActivationScript $pick.activationScript
}

# 3) Classic offline-installer layout fallback.
if (-not (Test-IdfEnvReady)) {
    $classicIdf = @(
        'C:\esp\v5.4.2\esp-idf',
        'C:\esp\v5.4.2\v5.4.2\esp-idf'
    ) | Where-Object { Test-Path (Join-Path $_ 'export.ps1') } | Select-Object -First 1
    if ($classicIdf) {
        $classicTools = @('C:\Espressif', 'C:\esp\v5.4.2\v5.4.2\tools') |
            Where-Object { Test-Path $_ } | Select-Object -First 1
        Enable-IdfInstall -IdfPath $classicIdf -ToolsPath $classicTools
    }
}

if (-not (Test-IdfEnvReady)) {
    throw "Could not locate an ESP-IDF 5.4.x install. Install v5.4.2 via the ESP-IDF VS Code extension / Installation Manager, open an ESP-IDF terminal, or set IDF_PATH + IDF_PYTHON_ENV_PATH manually. See SETUP_BEFORE_YOU_BUILD.txt."
}
