$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$instance = Split-Path -Leaf $root
$exe = Join-Path $root "djssp_pso_hh.exe"
$data = Join-Path $root "data\jobshop1.txt"
$logs = Join-Path $root "logs"
$results = Join-Path $root "results"

if (-not (Test-Path $exe)) {
    throw "djssp_pso_hh.exe not found: $exe"
}

if (-not (Test-Path $data)) {
    throw "data\jobshop1.txt not found: $data"
}

New-Item -ItemType Directory -Force -Path $logs | Out-Null
New-Item -ItemType Directory -Force -Path $results | Out-Null

function New-Seed {
    return Get-Random -Minimum 1 -Maximum 9999999
}

function Parse-Cmax($text) {
    $m = [regex]::Match($text, "Done\. Best Cmax=(\d+)")
    if ($m.Success) {
        return [int]$m.Groups[1].Value
    }
    return $null
}

function Run-One($label, [string[]]$argsList) {
    $log = Join-Path $logs ($label + ".log")

    Write-Host ""
    Write-Host "=========================================="
    Write-Host ("RUN {0}: {1}" -f $label, ($argsList -join " "))
    Write-Host "=========================================="

    $output = & $exe @argsList 2>&1 | Tee-Object -FilePath $log | Out-String
    $cmax = Parse-Cmax $output

    $gantt = Join-Path $root ("gantt_{0}.csv" -f $instance)
    if (Test-Path $gantt) {
        $suffix = if ($null -eq $cmax) { "na" } else { $cmax.ToString() }
        $target = Join-Path $results ("{0}_gantt_cmax_{1}.csv" -f $label, $suffix)
        Copy-Item -Force $gantt $target
    }

    return [pscustomobject]@{
        Label = $label
        Cmax = $cmax
        Log = $log
    }
}

$seed1 = New-Seed
$seed2 = New-Seed

$runs = @(
    @{
        Label = "01_quick_random"
        Args = @(
            $instance,
            "--iters", "1",
            "--swarm", "1",
            "--evalk", "1",
            "--finalk", "1",
            "--seed", "$seed1"
        )
    },
    @{
        Label = "02_quick_random"
        Args = @(
            $instance,
            "--iters", "1",
            "--swarm", "2",
            "--evalk", "1",
            "--finalk", "1",
            "--seed", "$seed2"
        )
    },
    @{
        Label = "03_light_guided"
        Args = @(
            $instance,
            "--sgs", "gt",
            "--iters", "4",
            "--swarm", "4",
            "--evalk", "1",
            "--finalk", "5",
            "--seed", "55",
            "--eps0", "0",
            "--epsmin", "0",
            "--traindet",
            "--tsiters", "10",
            "--tabu", "5",
            "--tsmove", "mixed"
        )
    }
)

Write-Host "=========================================="
Write-Host ("XSIM MINI HUNT: {0}" -f $instance)
Write-Host "3 hafif deneme calistirilacak."
Write-Host ("Exe    : {0}" -f $exe)
Write-Host ("Logs   : {0}" -f $logs)
Write-Host ("Result : {0}" -f $results)
Write-Host "=========================================="

$summary = @()
foreach ($run in $runs) {
    $summary += Run-One $run.Label $run.Args
}

$valid = $summary | Where-Object { $null -ne $_.Cmax } | Sort-Object Cmax

Write-Host ""
Write-Host "=========================================="
Write-Host "OZET"
foreach ($item in $summary) {
    $shown = if ($null -eq $item.Cmax) { "NA" } else { $item.Cmax }
    Write-Host ("{0}: Cmax={1}" -f $item.Label, $shown)
}

if ($valid.Count -gt 0) {
    $best = $valid[0]
    Write-Host ""
    Write-Host ("En iyi deneme: {0}" -f $best.Label)
    Write-Host ("En iyi Cmax  : {0}" -f $best.Cmax)
}
Write-Host "=========================================="
