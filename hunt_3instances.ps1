# hunt_3instances.ps1
# Runs xsim.exe for multiple instances and hunts best (lowest) Cmax.
# IMPORTANT: xsim expects instance name as FIRST positional argument (no -inst / --inst).

$ErrorActionPreference = "Stop"

# ---- PATHS ----
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$xsim = Join-Path $root "xsim.exe"
if (-not (Test-Path $xsim)) { throw "xsim.exe not found at: $xsim" }

# ---- INSTANCES ----
$instances = @("swv11")

# ---- BASE PARAMS ----
$finalk = 200000
$evalk  = 5
$iters  = 2000
$swarm  = 100
$tsmove = "mixed"   # maps to --tsmove mixed

# Search grid (edit freely)
$seeds   = @(55,66,77,88,99)
$tabus   = @(10,12,15,16,18,20)
$tsiters = @(60000,80000,100000,120000)

function Show-Header($instance) {
  Write-Host "=========================================="
  Write-Host "HUNT for $instance"
  Write-Host ("Base: finalk={0} evalk={1} iters={2} swarm={3} tsmove={4}" -f $finalk,$evalk,$iters,$swarm,$tsmove)
  Write-Host ("Search: seeds=({0}) tabus=({1}) tsiters=({2})" -f `
    ($seeds -join " "), ($tabus -join " "), ($tsiters -join " "))
  Write-Host "=========================================="
}

function Parse-BestCmaxFromLog($logPath) {
  # xsim prints: "Done. Best Cmax=###"
  $done = Select-String -Path $logPath -Pattern "Done\. Best Cmax=" | Select-Object -First 1
  if (-not $done) { return $null }

  $m = [regex]::Match($done.Line, "Done\. Best Cmax=(\d+)")
  if (-not $m.Success) { return $null }

  return [int]$m.Groups[1].Value
}

foreach ($instance in $instances) {

  Show-Header $instance

  $outdir = Join-Path $root ("hunt_{0}" -f $instance)
  New-Item -ItemType Directory -Force -Path $outdir | Out-Null

  $best      = [int]::MaxValue
  $bestSeed  = $null
  $bestTabu  = $null
  $bestTs    = $null
  $bestLog   = $null
  $bestGantt = $null

  foreach ($seed in $seeds) {
    foreach ($tabu in $tabus) {
      foreach ($ts in $tsiters) {

        $tag = "{0}_seed_{1}_tabu_{2}_ts_{3}_mv_{4}" -f $instance,$seed,$tabu,$ts,$tsmove
        $logfile = Join-Path $outdir ("{0}.log" -f $tag)

        # ---- IMPORTANT: instance is positional first arg ----
        $args = @(
          $instance,
          "--seed",   $seed,
          "--tabu",   $tabu,
          "--tsiters",$ts,
          "--finalk", $finalk,
          "--evalk",  $evalk,
          "--iters",  $iters,
          "--swarm",  $swarm,
          "--tsmove", $tsmove
        )

        Write-Host ("RUN  {0} {1}" -f $xsim, ($args -join " "))

        # Run and capture
        $output = & $xsim @args 2>&1 | Out-String
        $output | Set-Content -Encoding UTF8 $logfile

        $cmax = Parse-BestCmaxFromLog $logfile
        if ($null -eq $cmax) {
          Write-Host "  WARNING: Could not parse Best Cmax from log:"
          Write-Host "    $logfile"
          continue
        }

        Write-Host ("  BestCmax={0}" -f $cmax)

        if ($cmax -lt $best) {
          $best     = $cmax
          $bestSeed = $seed
          $bestTabu = $tabu
          $bestTs   = $ts
          $bestLog  = $logfile

          # Program writes gantt_{instance}.csv in current directory
          $ganttSrc = Join-Path $root ("gantt_{0}.csv" -f $instance)
          if (Test-Path $ganttSrc) {
            $bestGantt = Join-Path $outdir ("best_{0}_cmax_{1}.csv" -f $tag,$best)
            Copy-Item -Force $ganttSrc $bestGantt
          } else {
            $bestGantt = $null
          }

          Write-Host ""
          Write-Host ">>> NEW BEST FOUND!"
          Write-Host ("Best Cmax    = {0}" -f $best)
          Write-Host ("Best seed    = {0}" -f $bestSeed)
          Write-Host ("Best tabu    = {0}" -f $bestTabu)
          Write-Host ("Best tsiters = {0}" -f $bestTs)
          Write-Host ("Best log     = {0}" -f $bestLog)
          if ($bestGantt) { Write-Host ("Best gantt   = {0}" -f $bestGantt) }
          Write-Host "------------------------------------------"
          Write-Host ""
        }
      }
    }
  }

  Write-Host ""
  Write-Host "=========================================="
  Write-Host ("HUNT DONE for {0}." -f $instance)
  Write-Host ("Best Cmax    = {0}" -f $best)
  Write-Host ("Best seed    = {0}" -f $bestSeed)
  Write-Host ("Best tabu    = {0}" -f $bestTabu)
  Write-Host ("Best tsiters = {0}" -f $bestTs)
  Write-Host ("Best tsmove  = {0}" -f $tsmove)
  Write-Host ("Best log     = {0}" -f $bestLog)
  if ($bestGantt) { Write-Host ("Best gantt   = {0}" -f $bestGantt) }
  Write-Host "=========================================="
  Write-Host ""
}

Write-Host "ALL INSTANCES DONE."
