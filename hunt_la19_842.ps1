# hunt_la19_842_v2.ps1
# Purpose: brute-force run xsim.exe on la19 with different seeds/tabu/tsiters/tsmove
# and keep the best (lowest) Cmax + save best log & best gantt csv.
$ErrorActionPreference = "Stop"

# ========== Base config ==========
$exe      = ".\xsim.exe"
$instance = "la19"

# PSO / run controls (your known-good baseline that reached 850)
$iters    = 2000
$swarm    = 100
$eps0     = 0
$epsmin   = 0
$evalk    = 5
$finalk   = 200000
$sgs      = "gt"

# NEW: tabu neighborhood mode (requires your Xsimv36.cpp build)
#   swap   = old behavior
#   insert = insertion moves only
#   mixed  = swap + insertion (recommended)
$tsmoveL  = @("mixed","insert","swap")

# ========== Search space ==========
# Seeds: include the ones you've already used + a few extra "spread" values
$seeds    = @(11,22,33,44,55,66,77,88,99,101,111,123,222,333,444,555,666,777,888,999,2026,31415,424242)
# Tenure: keep around the sweet spot you found (15) but try neighbors
$tabus    = @(10,12,14,15,16,18,20,22)
# TS iterations: keep your known best (120k) + a couple larger budgets
$tsitersL = @(120000, 250000, 400000, 800000, 1200000)

# ========== Output ==========
$outdir = "hunt_la19"
New-Item -ItemType Directory -Force -Path $outdir | Out-Null

if (-not (Test-Path $exe)) {
  throw "Cannot find $exe in current folder. Run this script from the folder that contains xsim.exe."
}

# Global best
$best      = [int]::MaxValue
$bestSeed  = $null
$bestTabu  = $null
$bestTs    = $null
$bestMove  = $null
$bestLog   = $null
$bestGantt = $null

# Convenience: writes current best summary
function Show-Best {
  Write-Host ("  >>> BEST SO FAR: Cmax={0} | seed={1} tabu={2} tsiters={3} tsmove={4}" -f $best,$bestSeed,$bestTabu,$bestTs,$bestMove)
  if ($bestLog)   { Write-Host ("      log  : {0}" -f $bestLog) }
  if ($bestGantt) { Write-Host ("      gantt: {0}" -f $bestGantt) }
}

Write-Host "=========================================="
Write-Host "842 HUNT for $instance"
Write-Host "Base: iters=$iters swarm=$swarm evalk=$evalk finalk=$finalk sgs=$sgs"
Write-Host "Search:"
Write-Host "  tsmove = ($($tsmoveL  -join ' '))"
Write-Host "  seeds  = ($($seeds    -join ' '))"
Write-Host "  tabu   = ($($tabus    -join ' '))"
Write-Host "  tsiters= ($($tsitersL -join ' '))"
Write-Host "Logs: $outdir"
Write-Host "=========================================="

# ========== Run grid ==========
foreach ($tsmove in $tsmoveL) {
  foreach ($tsiters in $tsitersL) {
    foreach ($tabu in $tabus) {
      foreach ($seed in $seeds) {

        $tag = "{0}_seed_{1}_tabu_{2}_ts_{3}_mv_{4}" -f $instance,$seed,$tabu,$tsiters,$tsmove
        $logfile = Join-Path $outdir ($tag + ".log")

        Write-Host ""
        Write-Host ("[seed={0} tabu={1} tsiters={2} tsmove={3}] running..." -f $seed,$tabu,$tsiters,$tsmove)

        $args = @(
          $instance,
          "--sgs", $sgs,
          "--iters", $iters,
          "--swarm", $swarm,
          "--eps0", $eps0,
          "--epsmin", $epsmin,
          "--seed", $seed,
          "--evalk", $evalk,
          "--finalk", $finalk,
          "--tsiters", $tsiters,
          "--tabu", $tabu,
          "--tsmove", $tsmove
        )

        # Run and capture output to logfile (also keep console output if you want by removing Out-File)
        & $exe @args | Tee-Object -FilePath $logfile | Out-Null

        # Parse: Done. Best Cmax=...
        $done = Select-String -Path $logfile -Pattern "Done\. Best Cmax=" | Select-Object -First 1
        if (-not $done) {
          Write-Host "  WARNING: Could not parse Best Cmax. Check log:"
          Write-Host "    $logfile"
          continue
        }

        $m = [regex]::Match($done.Line, "Done\. Best Cmax=(\d+)")
        if (-not $m.Success) {
          Write-Host "  WARNING: Regex parse failed. Check log:"
          Write-Host "    $logfile"
          continue
        }

        $cmax = [int]$m.Groups[1].Value
        Write-Host ("  BestCmax={0}" -f $cmax)

        if ($cmax -lt $best) {
          $best      = $cmax
          $bestSeed  = $seed
          $bestTabu  = $tabu
          $bestTs    = $tsiters
          $bestMove  = $tsmove
          $bestLog   = $logfile

          # Save gantt (program writes gantt_<instance>.csv in current dir)
          $ganttSrc = "gantt_{0}.csv" -f $instance
          if (Test-Path $ganttSrc) {
            $bestGantt = Join-Path $outdir ("best_{0}_cmax_{1}.csv" -f $tag,$best)
            Copy-Item -Force $ganttSrc $bestGantt
          } else {
            $bestGantt = $null
          }

          Show-Best
          if ($best -le 842) {
            Write-Host ""
            Write-Host "🎯 TARGET HIT (<=842). Stopping early."
            break 4
          }
        }
      }
    }
  }
}

Write-Host ""
Write-Host "=========================================="
Write-Host "HUNT DONE."
Write-Host ("Best Cmax    = {0}" -f $best)
Write-Host ("Best seed    = {0}" -f $bestSeed)
Write-Host ("Best tabu    = {0}" -f $bestTabu)
Write-Host ("Best tsiters = {0}" -f $bestTs)
Write-Host ("Best tsmove  = {0}" -f $bestMove)
Write-Host ("Best log     = {0}" -f $bestLog)
if ($bestGantt) { Write-Host ("Best gantt   = {0}" -f $bestGantt) }
Write-Host "=========================================="
