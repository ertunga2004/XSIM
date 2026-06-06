@echo off
cd /d "%~dp0"

echo XSIM mini hunt baslatiliyor...
echo Klasor: %CD%
echo Ciktilar: logs\ ve results\
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_hunt.ps1"
if errorlevel 1 (
    echo.
    echo Hata olustu. Ayrinti icin ekrandaki mesaja ve logs klasorune bakin.
    pause
    exit /b 1
)

echo.
echo Islem bitti.
echo Ozet dosyalari: results\summary.csv ve results\summary.json
pause
