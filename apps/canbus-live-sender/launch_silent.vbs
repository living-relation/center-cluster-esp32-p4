' Silent entry point for the desktop shortcut -- double-clicking this (via
' wscript.exe, which the shortcut targets) never shows a console window.
'
' Fast path: venv already built -> launch pythonw.exe directly (instant, no
' window at all). First run / repaired venv: fall back to launch.ps1 hidden,
' which creates the venv, installs deps, then starts the app -- still no
' visible window, just a short delay before the app appears.
'
' If something goes wrong during that hidden setup, run launch.ps1 directly
' from PowerShell instead -- it shows the same steps with visible output.

Set fso = CreateObject("Scripting.FileSystemObject")
scriptDir = fso.GetParentFolderName(WScript.ScriptFullName)

Set shell = CreateObject("WScript.Shell")
shell.CurrentDirectory = scriptDir

pythonwPath = scriptDir & "\.venv\Scripts\pythonw.exe"

If fso.FileExists(pythonwPath) Then
    shell.Run """" & pythonwPath & """ app.py", 0, False
Else
    shell.Run "powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File """ & scriptDir & "\launch.ps1""", 0, False
End If
