@echo off
setlocal
cd /d "%~dp0"

if not exist "djssp_pso_hh.exe" (
    echo Hata: djssp_pso_hh.exe bulunamadi.
    pause
    exit /b 1
)

if not exist "data\jobshop1.txt" (
    echo Hata: data\jobshop1.txt bulunamadi.
    echo Bu script exe klasoru icindeki veri kopyasi ile calisir.
    pause
    exit /b 1
)

set "SEED1=%RANDOM%"
set "SEED2=%RANDOM%"

echo ========================================
echo XSIM ft06 mini demo
echo ========================================
echo.

echo [1/3] Hizli rastgele deneme
echo Seed: %SEED1%
djssp_pso_hh.exe ft06 --iters 1 --swarm 1 --evalk 1 --finalk 1 --seed %SEED1%

echo.
echo [2/3] Hizli rastgele deneme
echo Seed: %SEED2%
djssp_pso_hh.exe ft06 --iters 1 --swarm 1 --evalk 1 --finalk 1 --seed %SEED2%

echo.
echo [3/3] Hafif optimize deneme
echo Ayarlar: --iters 8 --swarm 6 --evalk 2 --finalk 20 --seed 55 --sgs gt --eps0 0 --epsmin 0 --traindet --tsiters 30 --tabu 5 --tsmove mixed
djssp_pso_hh.exe ft06 --iters 8 --swarm 6 --evalk 2 --finalk 20 --seed 55 --sgs gt --eps0 0 --epsmin 0 --traindet --tsiters 30 --tabu 5 --tsmove mixed

echo.
echo Demo tamamlandi.
echo Bu klasorde olusan dosyalar:
echo - gantt_ft06.csv
pause
