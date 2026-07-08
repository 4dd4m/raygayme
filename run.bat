@echo off

taskkill /IM game.exe /F >nul 2>&1

powershell -ExecutionPolicy Bypass -File .\build.ps1


if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)

game.exe 2>&1 | powershell -Command "$input | Tee-Object -FilePath log.txt"