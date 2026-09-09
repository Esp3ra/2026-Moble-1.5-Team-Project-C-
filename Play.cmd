@echo off
setlocal
if not exist "%~dp0bin\Release\LevelDevil.exe" (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build.ps1" -Configuration Release
    if errorlevel 1 (
        pause
        exit /b 1
    )
)
start "" /D "%~dp0bin\Release" "%~dp0bin\Release\LevelDevil.exe"
