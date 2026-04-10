$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$instance = "ft06"

$exeCandidates = @(
  (Join-Path $root "djssp_pso_hh.exe"),
  (Join-Path $root "xsim.exe")
)

$exe = $null
foreach ($candidate in $exeCandidates) {
  if (Test-Path $candidate) {
    $exe = $candidate
    break
  }
}

if (-not $exe) {
  throw "Cannot find djssp_pso_hh.exe or xsim.exe in: $root"
}

$dataSrc = Join-Path $root "data\jobshop1.txt"
if (-not (Test-Path $dataSrc)) {
  throw "Cannot find data file: $dataSrc"
}

$outdir = Join-Path $root "hunt_ft06"
$workdir = Join-Path $outdir "_work"
$workDataDir = Join-Path $workdir "data"

New-Item -ItemType Directory -Force -Path $outdir | Out-Null
New-Item -ItemType Directory -Force -Path $workDataDir | Out-Null
Copy-Item -Force $dataSrc (Join-Path $workDataDir "jobshop1.txt")

# Lightweight base config for the smallest instance.
$iters   = 8
$swarm   = 6
$eps0    = 0
$epsmin  = 0
$evalk   = 2
$finalk  = 20
$sgs     = "gt"
$tsmove  = "mixed"

# Small search grid so weak machines can still finish quickly.
$seeds   = @(55, 11, 22)
$tabus   = @(5, 3, 7)
$tsiters = @(30, 10, 50)

$target = 55

function Parse-BestCmaxFromLog($logPath) {
  $done = Select-String -Path $logPath -Pattern "Done\. Best Cmax=" | Select-Object -First 1
  if (-not $done) { return $null }

  $m = [regex]::Match($done.Line, "Done\. Best Cmax=(\d+)")
  if (-not $m.Success) { return $null }

  return [int]$m.Groups[1].Value
}

Write-Host "=========================================="
Write-Host "MINI HUNT for $instance"
Write-Host ("Exe    : {0}" -f $exe)
Write-Host ("Base   : iters={0} swarm={1} evalk={2} finalk={3} sgs={4} traindet tsmove={5}" -f $iters,$swarm,$evalk,$finalk,$sgs,$tsmove)
Write-Host ("Search : seeds=({0}) tabus=({1}) tsiters=({2})" -f ($seeds -join " "), ($tabus -join " "), ($tsiters -join " "))
Write-Host ("Target : Cmax <= {0}" -f $target)
Write-Host ("Output : {0}" -f $outdir)
Write-Host "=========================================="

$best      = [int]::MaxValue
$bestSeed  = $null
$bestTabu  = $null
$bestTs    = $null
$bestLog   = $null
$bestGantt = $null

:SearchGrid foreach ($seed in $seeds) {
  foreach ($tabu in $tabus) {
    foreach ($ts in $tsiters) {
      $tag = "{0}_seed_{1}_tabu_{2}_ts_{3}_mv_{4}" -f $instance,$seed,$tabu,$ts,$tsmove
      $logfile = Join-Path $outdir ($tag + ".log")

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
        "--traindet",
        "--tsiters", $ts,
        "--tabu", $tabu,
        "--tsmove", $tsmove
      )

      Write-Host ""
      Write-Host ("[seed={0} tabu={1} tsiters={2}] running..." -f $seed,$tabu,$ts)

      Push-Location $workdir
      try {
        & $exe @args | Tee-Object -FilePath $logfile | Out-Null
      } finally {
        Pop-Location
      }

      $cmax = Parse-BestCmaxFromLog $logfile
      if ($null -eq $cmax) {
        Write-Host "  WARNING: Could not parse Best Cmax from log."
        Write-Host ("  Log: {0}" -f $logfile)
        continue
      }

      Write-Host ("  BestCmax={0}" -f $cmax)

      if ($cmax -lt $best) {
        $best     = $cmax
        $bestSeed = $seed
        $bestTabu = $tabu
        $bestTs   = $ts
        $bestLog  = $logfile

        $ganttSrc = Join-Path $workdir ("gantt_{0}.csv" -f $instance)
        if (Test-Path $ganttSrc) {
          $bestGantt = Join-Path $outdir ("best_{0}_cmax_{1}.csv" -f $tag,$best)
          Copy-Item -Force $ganttSrc $bestGantt
        } else {
          $bestGantt = $null
        }

        Write-Host "  >>> NEW BEST FOUND"
        Write-Host ("      Cmax   = {0}" -f $best)
        Write-Host ("      seed   = {0}" -f $bestSeed)
        Write-Host ("      tabu   = {0}" -f $bestTabu)
        Write-Host ("      tsiters= {0}" -f $bestTs)
        if ($bestLog)   { Write-Host ("      log    = {0}" -f $bestLog) }
        if ($bestGantt) { Write-Host ("      gantt  = {0}" -f $bestGantt) }
      }

      if ($best -le $target) {
        Write-Host ""
        Write-Host ("Target hit (Cmax <= {0}). Stopping early." -f $target)
        break SearchGrid
      }
    }
  }
}

Write-Host ""
Write-Host "=========================================="
Write-Host "MINI HUNT DONE."
Write-Host ("Best Cmax    = {0}" -f $best)
Write-Host ("Best seed    = {0}" -f $bestSeed)
Write-Host ("Best tabu    = {0}" -f $bestTabu)
Write-Host ("Best tsiters = {0}" -f $bestTs)
Write-Host ("Best tsmove  = {0}" -f $tsmove)
Write-Host ("Best log     = {0}" -f $bestLog)
if ($bestGantt) { Write-Host ("Best gantt   = {0}" -f $bestGantt) }
Write-Host "=========================================="
