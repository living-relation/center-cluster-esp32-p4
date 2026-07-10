# Launch CAN Live Sender.
#
# The desktop shortcut normally starts the app silently via pythonw.exe (no window).
# Run THIS script directly from PowerShell if the app won't start -- it shows setup
# output and any Python tracebacks, and rebuilds the virtual environment if needed.

$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot

$venvPy  = Join-Path $PSScriptRoot '.venv\Scripts\python.exe'
$venvPyw = Join-Path $PSScriptRoot '.venv\Scripts\pythonw.exe'

# First run (or repaired venv): create the environment and install dependencies.
if (-not (Test-Path $venvPy)) {
    Write-Host 'First run: creating virtual environment...'
    python -m venv .venv
    & $venvPy -m pip install --upgrade pip
    & $venvPy -m pip install -r requirements.txt
    Write-Host 'Setup complete.'
}

Write-Host 'Launching CAN Live Sender...'
# Use python.exe (not pythonw) here so tracebacks are visible when run for diagnostics.
& $venvPy app.py
