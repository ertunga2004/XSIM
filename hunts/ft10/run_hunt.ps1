$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$instance = Split-Path -Leaf $root
$exe = Join-Path $root "djssp_pso_hh.exe"
$data = Join-Path $root "data\jobshop1.txt"
$logs = Join-Path $root "logs"
$results = Join-Path $root "results"
$runsRoot = Join-Path $root "runs"

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

function Parse-LegacyCmax($text) {
    $m = [regex]::Match($text, "Done\. Best Cmax=(\d+)")
    if ($m.Success) {
        return [int]$m.Groups[1].Value
    }
    return $null
}

function Get-RunResultFromJson($resultPath) {
    try {
        $json = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    }
    catch {
        Write-Warning ("result.json okunamadi, legacy fallback kullanilacak: {0}" -f $resultPath)
        return $null
    }

    $cmax = $null
    if ($null -ne $json.metrics -and $null -ne $json.metrics.cmax) {
        $cmax = [double]$json.metrics.cmax
    }
    elseif ($null -ne $json.objective_value) {
        $cmax = [double]$json.objective_value
    }

    $feasible = $null
    if ($null -ne $json.feasibility -and $null -ne $json.feasibility.valid) {
        $feasible = [bool]$json.feasibility.valid
    }

    return [pscustomobject]@{
        RunId = [string]$json.run_id
        Status = [string]$json.status
        Cmax = $cmax
        RuntimeSec = if ($null -ne $json.runtime_sec) { [double]$json.runtime_sec } else { $null }
        FeasibilityValid = $feasible
        ResultPath = $resultPath
        RunDir = Split-Path -Parent $resultPath
        Source = "result.json"
    }
}

function Find-NewRunResult($startedAt) {
    if (-not (Test-Path $runsRoot)) {
        return $null
    }

    $result = Get-ChildItem -LiteralPath $runsRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object {
            $resultPath = Join-Path $_.FullName "result.json"
            if (Test-Path $resultPath) {
                Get-Item -LiteralPath $resultPath
            }
        } |
        Where-Object { $_.LastWriteTime -ge $startedAt.AddSeconds(-2) } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($null -eq $result) {
        return $null
    }
    return $result.FullName
}

function Copy-IfExists($source, $target) {
    if (Test-Path $source) {
        Copy-Item -Force -LiteralPath $source -Destination $target
        return $target
    }
    return $null
}

function Copy-RunArtifacts($label, $runInfo, $legacyCmax) {
    $copied = @()
    if ($null -ne $runInfo -and $runInfo.RunDir -and (Test-Path $runInfo.RunDir)) {
        $prefix = if ($runInfo.RunId) { "{0}_{1}" -f $label, $runInfo.RunId } else { $label }
        $artifacts = @(
            @{ Name = "result.json"; Target = "{0}_result.json" -f $prefix },
            @{ Name = "metadata.json"; Target = "{0}_metadata.json" -f $prefix },
            @{ Name = "schedule.csv"; Target = "{0}_schedule.csv" -f $prefix },
            @{ Name = "gantt.csv"; Target = "{0}_gantt.csv" -f $prefix },
            @{ Name = "gantt.html"; Target = "{0}_gantt.html" -f $prefix }
        )

        foreach ($artifact in $artifacts) {
            $source = Join-Path $runInfo.RunDir $artifact.Name
            $target = Join-Path $results $artifact.Target
            $copiedPath = Copy-IfExists $source $target
            if ($copiedPath) {
                $copied += $copiedPath
            }
        }
        return $copied
    }

    $legacyGantt = Join-Path $root ("gantt_{0}.csv" -f $instance)
    if (Test-Path $legacyGantt) {
        $suffix = if ($null -eq $legacyCmax) { "na" } else { $legacyCmax.ToString() }
        $target = Join-Path $results ("{0}_gantt_cmax_{1}.csv" -f $label, $suffix)
        Copy-Item -Force -LiteralPath $legacyGantt -Destination $target
        $copied += $target
    }
    return $copied
}

function Run-One($label, [string[]]$argsList) {
    $log = Join-Path $logs ($label + ".log")

    Write-Host ""
    Write-Host "=========================================="
    Write-Host ("RUN {0}: {1}" -f $label, ($argsList -join " "))
    Write-Host "=========================================="

    $startedAt = Get-Date
    $output = & $exe @argsList 2>&1 | Tee-Object -FilePath $log | Out-String
    $legacyCmax = Parse-LegacyCmax $output
    $resultPath = Find-NewRunResult $startedAt
    $runInfo = if ($resultPath) { Get-RunResultFromJson $resultPath } else { $null }

    if ($null -ne $runInfo -and $null -ne $runInfo.Cmax) {
        $cmax = $runInfo.Cmax
        $source = $runInfo.Source
    }
    else {
        $cmax = $legacyCmax
        $source = "legacy_stdout"
    }

    $copiedArtifacts = Copy-RunArtifacts $label $runInfo $legacyCmax

    return [pscustomobject]@{
        Label = $label
        Status = if ($null -ne $runInfo -and $runInfo.Status) { $runInfo.Status } else { "unknown" }
        Cmax = $cmax
        RuntimeSec = if ($null -ne $runInfo) { $runInfo.RuntimeSec } else { $null }
        FeasibilityValid = if ($null -ne $runInfo) { $runInfo.FeasibilityValid } else { $null }
        RunId = if ($null -ne $runInfo) { $runInfo.RunId } else { "" }
        RunDir = if ($null -ne $runInfo) { $runInfo.RunDir } else { "" }
        Log = $log
        ParseSource = $source
        CopiedArtifacts = ($copiedArtifacts -join ";")
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
Push-Location $root
try {
    foreach ($run in $runs) {
        $summary += Run-One $run.Label $run.Args
    }
}
finally {
    Pop-Location
}

$valid = $summary | Where-Object { $null -ne $_.Cmax } | Sort-Object Cmax
$summaryCsv = Join-Path $results "summary.csv"
$summaryJson = Join-Path $results "summary.json"
$summaryForFiles = $summary | Select-Object Label,Status,Cmax,RuntimeSec,FeasibilityValid,RunId,RunDir,Log,ParseSource,CopiedArtifacts
$summaryForFiles | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $summaryCsv
$summaryForFiles | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 -Path $summaryJson

Write-Host ""
Write-Host "=========================================="
Write-Host "OZET"
foreach ($item in $summary) {
    $shown = if ($null -eq $item.Cmax) { "NA" } else { $item.Cmax }
    $runShown = if ([string]::IsNullOrWhiteSpace($item.RunId)) { "-" } else { $item.RunId }
    Write-Host ("{0}: Cmax={1} | status={2} | run_id={3}" -f $item.Label, $shown, $item.Status, $runShown)
    Write-Host ("  log: {0}" -f $item.Log)
}

if ($valid.Count -gt 0) {
    $best = $valid[0]
    Write-Host ""
    Write-Host ("En iyi deneme: {0}" -f $best.Label)
    Write-Host ("En iyi Cmax  : {0}" -f $best.Cmax)
}
Write-Host ""
Write-Host ("Summary CSV : {0}" -f $summaryCsv)
Write-Host ("Summary JSON: {0}" -f $summaryJson)
Write-Host "=========================================="
