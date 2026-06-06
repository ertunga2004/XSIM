param(
    [string]$Plan = ".\search_plan_la19.json"
)

$ErrorActionPreference = "Stop"

function To-Bool([object]$Value) {
    if ($null -eq $Value) { return $false }
    if ($Value -is [bool]) { return [bool]$Value }
    $s = $Value.ToString().Trim().ToLowerInvariant()
    return ($s -eq 'true' -or $s -eq '1' -or $s -eq 'yes')
}

function Ensure-Array([object]$Value) {
    if ($null -eq $Value) { return @() }
    if ($Value -is [System.Array]) { return @($Value) }
    return @($Value)
}

function Copy-IfExists([string]$Source, [string]$Target) {
    if (Test-Path $Source) {
        Copy-Item -Path $Source -Destination $Target -Recurse -Force
    }
}

if (-not (Test-Path $Plan)) {
    throw "Plan dosyasi bulunamadi: $Plan"
}

$planObj = Get-Content -Raw -Path $Plan | ConvertFrom-Json
$planDir = Split-Path -Parent (Resolve-Path $Plan)

$exe = if ($planObj.exe) { $planObj.exe } else { '.\\djssp_pso_hh.exe' }
if (-not [System.IO.Path]::IsPathRooted($exe)) {
    $exe = Join-Path $planDir $exe
}
if (-not (Test-Path $exe)) {
    throw "Calistirilabilir dosya bulunamadi: $exe"
}

$instance = $planObj.instance
if ([string]::IsNullOrWhiteSpace($instance)) {
    throw "Plan icinde 'instance' zorunludur."
}

$runsRoot = if ($planObj.runs_root) { $planObj.runs_root } else { '.\\runs' }
if (-not [System.IO.Path]::IsPathRooted($runsRoot)) {
    $runsRoot = Join-Path $planDir $runsRoot
}

$searchRoot = if ($planObj.search_root) { $planObj.search_root } else { '.\\search_results' }
if (-not [System.IO.Path]::IsPathRooted($searchRoot)) {
    $searchRoot = Join-Path $planDir $searchRoot
}
New-Item -ItemType Directory -Force -Path $searchRoot | Out-Null

$searchId = (Get-Date).ToString('yyyyMMdd_HHmmss') + '_' + $instance + '_iterative_search'
$searchDir = Join-Path $searchRoot $searchId
New-Item -ItemType Directory -Force -Path $searchDir | Out-Null

$allCsv = Join-Path $searchDir 'all_results.csv'
$allJson = Join-Path $searchDir 'all_results.json'
$summaryJson = Join-Path $searchDir 'summary.json'
$bestRunSnapshot = Join-Path $searchDir 'best_run_snapshot'
$logsDir = Join-Path $searchDir 'logs'
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null

$csvHeader = 'trial_id,instance,seed,sgs,iters,swarm,evalk,finalk,eps0,epsmin,fitavg,traindet,tsiters,tabu,tsmove,status,feasible,cmax,runtime_sec,run_id,run_dir,parse_source,log_file'
Set-Content -Path $allCsv -Value $csvHeader -Encoding UTF8

$seeds = Ensure-Array $planObj.seeds
if ($seeds.Count -eq 0) { $seeds = @(1) }

$grid = $planObj.grid
if ($null -eq $grid) { throw "Plan icinde 'grid' alani zorunludur." }

$sgsList = Ensure-Array $grid.sgs; if ($sgsList.Count -eq 0) { $sgsList = @('gt') }
$itersList = Ensure-Array $grid.iters; if ($itersList.Count -eq 0) { $itersList = @(100) }
$swarmList = Ensure-Array $grid.swarm; if ($swarmList.Count -eq 0) { $swarmList = @(30) }
$evalkList = Ensure-Array $grid.evalk; if ($evalkList.Count -eq 0) { $evalkList = @(3) }
$finalkList = Ensure-Array $grid.finalk; if ($finalkList.Count -eq 0) { $finalkList = @(1000) }
$eps0List = Ensure-Array $grid.eps0; if ($eps0List.Count -eq 0) { $eps0List = @(0.25) }
$epsminList = Ensure-Array $grid.epsmin; if ($epsminList.Count -eq 0) { $epsminList = @(0.05) }
$fitavgList = Ensure-Array $grid.fitavg; if ($fitavgList.Count -eq 0) { $fitavgList = @($false) }
$traindetList = Ensure-Array $grid.traindet; if ($traindetList.Count -eq 0) { $traindetList = @($false) }
$tsitersList = Ensure-Array $grid.tsiters; if ($tsitersList.Count -eq 0) { $tsitersList = @(0) }
$tabuList = Ensure-Array $grid.tabu; if ($tabuList.Count -eq 0) { $tabuList = @(10) }
$tsmoveList = Ensure-Array $grid.tsmove; if ($tsmoveList.Count -eq 0) { $tsmoveList = @('mixed') }

$allResults = New-Object System.Collections.Generic.List[object]
$bestResult = $null
$trialId = 0

Write-Host "=========================================="
Write-Host "XSIM ITERATIVE SEARCH BASLADI"
Write-Host "Instance : $instance"
Write-Host "Exe      : $exe"
Write-Host "RunsRoot : $runsRoot"
Write-Host "Output   : $searchDir"
Write-Host "=========================================="

foreach ($seed in $seeds) {
    foreach ($sgs in $sgsList) {
        foreach ($iters in $itersList) {
            foreach ($swarm in $swarmList) {
                foreach ($evalk in $evalkList) {
                    foreach ($finalk in $finalkList) {
                        foreach ($eps0 in $eps0List) {
                            foreach ($epsmin in $epsminList) {
                                foreach ($fitavg in $fitavgList) {
                                    foreach ($traindet in $traindetList) {
                                        foreach ($tsiters in $tsitersList) {
                                            foreach ($tabu in $tabuList) {
                                                foreach ($tsmove in $tsmoveList) {
                                                    $trialId++
                                                    $trialLabel = ('trial_{0:D4}' -f $trialId)
                                                    $beforeRuns = @()
                                                    if (Test-Path $runsRoot) {
                                                        $beforeRuns = Get-ChildItem -Path $runsRoot -Directory | Select-Object -ExpandProperty FullName
                                                    }
                                                    $argsList = @($instance, '--sgs', "$sgs", '--iters', "$iters", '--swarm', "$swarm", '--evalk', "$evalk", '--finalk', "$finalk", '--seed', "$seed", '--eps0', "$eps0", '--epsmin', "$epsmin")
                                                    if (To-Bool $fitavg) { $argsList += '--fitavg' }
                                                    if (To-Bool $traindet) { $argsList += '--traindet' }
                                                    if ([int]$tsiters -gt 0) {
                                                        $argsList += @('--tsiters', "$tsiters", '--tabu', "$tabu", '--tsmove', "$tsmove")
                                                    }

                                                    $logFile = Join-Path $logsDir ($trialLabel + '.log')
                                                    Write-Host ""
                                                    Write-Host "=========================================="
                                                    Write-Host ("{0}: {1}" -f $trialLabel, (($argsList -join ' ')))
                                                    Write-Host "=========================================="
                                                    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
                                                    $output = & $exe @argsList 2>&1 | Tee-Object -FilePath $logFile | Out-String
                                                    $stopwatch.Stop()
                                                    $exitCode = $LASTEXITCODE

                                                    $afterRuns = @()
                                                    if (Test-Path $runsRoot) {
                                                        $afterRuns = Get-ChildItem -Path $runsRoot -Directory | Select-Object -ExpandProperty FullName
                                                    }
                                                    $newRuns = @($afterRuns | Where-Object { $beforeRuns -notcontains $_ })
                                                    $runDir = $null
                                                    if ($newRuns.Count -gt 0) {
                                                        $runDir = (Get-ChildItem -Path ($newRuns) -Directory | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
                                                    } elseif (Test-Path $runsRoot) {
                                                        $runDir = (Get-ChildItem -Path $runsRoot -Directory | Where-Object { $_.Name -like "*_${instance}_seed${seed}_*" } | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
                                                    }

                                                    $status = 'unknown'
                                                    $feasible = $false
                                                    $cmax = $null
                                                    $runtimeSec = [math]::Round($stopwatch.Elapsed.TotalSeconds, 6)
                                                    $runId = $null
                                                    $parseSource = 'none'

                                                    if ($runDir) {
                                                        $resultPath = Join-Path $runDir 'result.json'
                                                        if (Test-Path $resultPath) {
                                                            try {
                                                                $resultObj = Get-Content -Raw -Path $resultPath | ConvertFrom-Json
                                                                $status = if ($resultObj.status) { $resultObj.status } else { 'unknown' }
                                                                if ($resultObj.metrics -and $null -ne $resultObj.metrics.cmax) { $cmax = [int]$resultObj.metrics.cmax }
                                                                if ($null -ne $resultObj.runtime_sec) { $runtimeSec = [double]$resultObj.runtime_sec }
                                                                if ($resultObj.run_id) { $runId = $resultObj.run_id }
                                                                if ($resultObj.feasibility -and $null -ne $resultObj.feasibility.valid) { $feasible = [bool]$resultObj.feasibility.valid }
                                                                $parseSource = 'result_json'
                                                            } catch {
                                                                $parseSource = 'result_json_error'
                                                            }
                                                        }
                                                    }

                                                    if ($null -eq $cmax) {
                                                        $m = [regex]::Match($output, 'Done\. Best Cmax=\s*(\d+)')
                                                        if ($m.Success) {
                                                            $cmax = [int]$m.Groups[1].Value
                                                            $status = 'success'
                                                            $feasible = $true
                                                            $parseSource = 'legacy_stdout'
                                                        }
                                                    }

                                                    if ($exitCode -ne 0 -and $status -eq 'unknown') {
                                                        $status = 'failed'
                                                    }

                                                    $record = [pscustomobject]@{
                                                        trial_id = $trialLabel
                                                        instance = $instance
                                                        seed = [int]$seed
                                                        sgs = "$sgs"
                                                        iters = [int]$iters
                                                        swarm = [int]$swarm
                                                        evalk = [int]$evalk
                                                        finalk = [int]$finalk
                                                        eps0 = [double]$eps0
                                                        epsmin = [double]$epsmin
                                                        fitavg = (To-Bool $fitavg)
                                                        traindet = (To-Bool $traindet)
                                                        tsiters = [int]$tsiters
                                                        tabu = [int]$tabu
                                                        tsmove = "$tsmove"
                                                        status = "$status"
                                                        feasible = [bool]$feasible
                                                        cmax = $cmax
                                                        runtime_sec = $runtimeSec
                                                        run_id = $runId
                                                        run_dir = $runDir
                                                        parse_source = $parseSource
                                                        log_file = $logFile
                                                    }
                                                    $allResults.Add($record) | Out-Null

                                                    $csvLine = @(
                                                        $record.trial_id,
                                                        $record.instance,
                                                        $record.seed,
                                                        $record.sgs,
                                                        $record.iters,
                                                        $record.swarm,
                                                        $record.evalk,
                                                        $record.finalk,
                                                        $record.eps0,
                                                        $record.epsmin,
                                                        $record.fitavg,
                                                        $record.traindet,
                                                        $record.tsiters,
                                                        $record.tabu,
                                                        $record.tsmove,
                                                        $record.status,
                                                        $record.feasible,
                                                        $record.cmax,
                                                        $record.runtime_sec,
                                                        $record.run_id,
                                                        ('"' + (($record.run_dir -replace '"','""')) + '"'),
                                                        $record.parse_source,
                                                        ('"' + (($record.log_file -replace '"','""')) + '"')
                                                    ) -join ','
                                                    Add-Content -Path $allCsv -Value $csvLine -Encoding UTF8

                                                    if ($record.feasible -and $null -ne $record.cmax) {
                                                        if ($null -eq $bestResult -or [int]$record.cmax -lt [int]$bestResult.cmax -or ([int]$record.cmax -eq [int]$bestResult.cmax -and [double]$record.runtime_sec -lt [double]$bestResult.runtime_sec)) {
                                                            $bestResult = $record
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

$allResults | ConvertTo-Json -Depth 6 | Set-Content -Path $allJson -Encoding UTF8

if ($bestResult -and $bestResult.run_dir) {
    Copy-IfExists $bestResult.run_dir $bestRunSnapshot
}

$summary = [pscustomobject]@{
    search_id = $searchId
    instance = $instance
    total_trials = $allResults.Count
    feasible_trials = @($allResults | Where-Object { $_.feasible -eq $true }).Count
    best = $bestResult
    created_at = (Get-Date).ToString('yyyy-MM-ddTHH:mm:ssK')
    plan_file = (Resolve-Path $Plan).Path
    all_results_csv = $allCsv
    all_results_json = $allJson
    best_run_snapshot = if (Test-Path $bestRunSnapshot) { $bestRunSnapshot } else { $null }
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -Path $summaryJson -Encoding UTF8

Write-Host ""
Write-Host "=========================================="
Write-Host "ARAMA TAMAMLANDI"
Write-Host ("Toplam deneme : {0}" -f $allResults.Count)
Write-Host ("Gecerli deneme: {0}" -f (@($allResults | Where-Object { $_.feasible -eq $true }).Count))
if ($bestResult) {
    Write-Host ("En iyi Cmax  : {0}" -f $bestResult.cmax)
    Write-Host ("Trial        : {0}" -f $bestResult.trial_id)
    Write-Host ("Run ID       : {0}" -f $bestResult.run_id)
    Write-Host ("Run klasoru  : {0}" -f $bestResult.run_dir)
} else {
    Write-Host "Gecerli sonuc bulunamadi."
}
Write-Host ("CSV          : {0}" -f $allCsv)
Write-Host ("JSON         : {0}" -f $summaryJson)
Write-Host "=========================================="
