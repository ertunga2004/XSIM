@echo off
cd /d "%~dp0"

echo XSIM mini hunt baslatiliyor...
echo Klasor: %CD%
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_hunt.ps1"

echo.
echo Islem bitti.
pause
