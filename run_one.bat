@echo off
set INSTANCE=la19
set SEED=777

.\xsim.exe %INSTANCE% --sgs gt --iters 2000 --swarm 100 --eps0 0 --epsmin 0 --seed %SEED% --evalk 5 --finalk 2000 --tsiters 120000 --tabu 12 > run_%INSTANCE%_%SEED%.log

echo Done. Log saved to run_%INSTANCE%_%SEED%.log
pause
