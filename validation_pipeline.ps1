param(
    [string]$Matrix = ".\validation_run_matrix.csv",
    [string]$PresetRegistry = ".\preset_registry.json",
    [string]$Exe = ".\djssp_pso_hh.exe",
    [string]$RunsRoot = ".\runs",
    [string]$SearchRoot = ".\search_results",
    [string]$ValidationResults = ".\validation_results.csv",
    [string]$AggregatedResults = ".\aggregated_validation_results.csv"
)

$ErrorActionPreference = "Stop"

function Ensure-Directory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Force -Path $Path | Out-Null
    }
}

function Add-ContentSafe([string]$Path, [string]$Value) {
    for ($i = 0; $i -lt 20; $i++) {
        try {
            [System.IO.File]::AppendAllText(
                $Path,
                $Value + [System.Environment]::NewLine,
                [System.Text.Encoding]::UTF8
            )
            return
        }
        catch {
            Start-Sleep -Milliseconds 250
        }
    }
    throw "Dosyaya yazilamadi: $Path"
}

function Get-BeforeRuns([string]$Root) {
    if (-not (Test-Path $Root)) { return @() }
    return @(Get-ChildItem -Path $Root -Directory | Select-Object -ExpandProperty FullName)
}

function Get-NewRunDir([string]$Root, [array]$BeforeRuns) {
    if (-not (Test-Path $Root)) { return $null }
    $afterRuns = @(Get-ChildItem -Path $Root -Directory | Select-Object -ExpandProperty FullName)
    $newRuns = @($afterRuns | Where-Object { $BeforeRuns -notcontains $_ })
    if ($newRuns.Count -gt 0) {
        $dir = Get-ChildItem -Path $Root -Directory |
            Where-Object { $newRuns -contains $_.FullName } |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1
        if ($dir) { return $dir.FullName }
    }
    return $null
}

function Get-LatestMatchingRunDir([string]$Root, [string]$Instance, [string]$Seed) {
    if (-not (Test-Path $Root)) { return $null }
    $dir = Get-ChildItem -Path $Root -Directory |
        Where-Object { $_.Name -like "*_${Instance}_seed${Seed}_*" } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($dir) { return $dir.FullName }
    return $null
}

function To-DoubleOrNull([object]$v) {
    if ($null -eq $v -or $v -eq "") { return $null }
    try { return [double]$v } catch { return $null }
}

function To-IntOrNull([object]$v) {
    if ($null -eq $v -or $v -eq "") { return $null }
    try { return [int]$v } catch { return $null }
}

function ConvertTo-CsvLine([string[]]$values) {
    $escaped = @()
    foreach ($v in $values) {
        if ($null -eq $v) { $v = "" }
        $escaped += '"' + ($v.ToString() -replace '"', '""') + '"'
    }
    return ($escaped -join ',')
}

function Get-Median([double[]]$Values) {
    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }
    $sorted = $Values | Sort-Object
    $n = $sorted.Count
    if ($n % 2 -eq 1) {
        return [double]$sorted[[int][math]::Floor($n / 2)]
    }
    else {
        $a = [double]$sorted[($n / 2) - 1]
        $b = [double]$sorted[$n / 2]
        return (($a + $b) / 2.0)
    }
}

function Get-Std([double[]]$Values) {
    if ($null -eq $Values -or $Values.Count -le 1) { return 0.0 }
    $mean = ($Values | Measure-Object -Average).Average
    $sum = 0.0
    foreach ($x in $Values) { $sum += [math]::Pow(($x - $mean), 2) }
    return [math]::Sqrt($sum / ($Values.Count - 1))
}

if (-not (Test-Path $Matrix)) { throw "Validation matrix bulunamadi: $Matrix" }
if (-not (Test-Path $PresetRegistry)) { throw "Preset registry bulunamadi: $PresetRegistry" }
if (-not (Test-Path $Exe)) { throw "Executable bulunamadi: $Exe" }
Ensure-Directory $RunsRoot
Ensure-Directory $SearchRoot

$registryObj = Get-Content -Raw -Path $PresetRegistry | ConvertFrom-Json
$presets = @{}
foreach ($p in $registryObj.presets) {
    $presets[$p.preset_id] = $p
}

$matrixRows = @(Import-Csv -Path $Matrix)
if ($matrixRows.Count -eq 0) { throw "Validation matrix bos gorunuyor: $Matrix" }

$resultsHeader = @(
    'phase','instance','preset_id','seed','runtime_budget_sec','termination_mode',
    'reference_value','reference_type','priority_group','run_id','run_dir','status',
    'feasible','cmax','gap_percent','runtime_sec','result_json','metadata_json',
    'convergence_csv','schedule_csv','notes'
) -join ','
Set-Content -Path $ValidationResults -Value $resultsHeader -Encoding UTF8

Write-Host "=========================================="
Write-Host "XSIM VALIDATION PIPELINE BASLADI"
Write-Host ("Matrix       : {0}" -f (Resolve-Path $Matrix))
Write-Host ("PresetReg    : {0}" -f (Resolve-Path $PresetRegistry))
Write-Host ("Executable   : {0}" -f (Resolve-Path $Exe))
Write-Host ("ValResults   : {0}" -f (Resolve-Path (Split-Path -Parent $ValidationResults) -ErrorAction SilentlyContinue))
Write-Host "=========================================="

foreach ($row in $matrixRows) {
    $instance = $row.instance
    $presetId = $row.preset_id
    $seed = $row.seed
    $runtimeBudget = $row.runtime_budget_sec
    $terminationMode = $row.termination_mode
    $referenceValue = $row.reference_value
    $referenceType = $row.reference_type
    $priorityGroup = $row.priority_group
    $notes = $row.notes

    if (-not $presets.ContainsKey($presetId)) {
        throw "Preset registry icinde bulunamayan preset_id: $presetId"
    }
    $preset = $presets[$presetId]
    $a = $preset.solver_args

    $beforeRuns = Get-BeforeRuns -Root $RunsRoot

    $argsList = @(
        $instance,
        '--sgs', [string]$a.sgs,
        '--iters', [string]$a.iters,
        '--swarm', [string]$a.swarm,
        '--evalk', [string]$a.evalk,
        '--finalk', [string]$a.finalk,
        '--seed', [string]$seed,
        '--eps0', [string]$a.eps0,
        '--epsmin', [string]$a.epsmin
    )

    if ([bool]$a.fitavg)   { $argsList += '--fitavg' }
    if ([bool]$a.traindet) { $argsList += '--traindet' }
    if ([int]$a.tsiters -gt 0) {
        $argsList += @('--tsiters', [string]$a.tsiters, '--tabu', [string]$a.tabu, '--tsmove', [string]$a.tsmove)
    }

    Write-Host ""
    Write-Host "------------------------------------------"
    Write-Host ("Instance : {0}" -f $instance)
    Write-Host ("Preset   : {0}" -f $presetId)
    Write-Host ("Seed     : {0}" -f $seed)
    Write-Host ("Args     : {0}" -f ($argsList -join ' '))
    Write-Host "------------------------------------------"

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $output = & $Exe @argsList 2>&1 | Out-String
    $stopwatch.Stop()
    $exitCode = $LASTEXITCODE

    $runDir = Get-NewRunDir -Root $RunsRoot -BeforeRuns $beforeRuns
    if (-not $runDir) {
        $runDir = Get-LatestMatchingRunDir -Root $RunsRoot -Instance $instance -Seed $seed
    }

    $runId = $null
    $status = 'unknown'
    $feasible = $false
    $cmax = $null
    $runtimeSec = [math]::Round($stopwatch.Elapsed.TotalSeconds, 6)
    $resultJson = ''
    $metadataJson = ''
    $convergenceCsv = ''
    $scheduleCsv = ''

    if ($runDir) {
        $resultPath = Join-Path $runDir 'result.json'
        $metadataPath = Join-Path $runDir 'metadata.json'
        $convPath = Join-Path $runDir 'convergence.csv'
        $schedPath = Join-Path $runDir 'schedule.csv'

        if (Test-Path $resultPath) {
            try {
                $resultObj = Get-Content -Raw -Path $resultPath | ConvertFrom-Json
                $runId = $resultObj.run_id
                $status = if ($resultObj.status) { [string]$resultObj.status } else { 'unknown' }
                if ($resultObj.feasibility -and $null -ne $resultObj.feasibility.valid) {
                    $feasible = [bool]$resultObj.feasibility.valid
                }
                if ($resultObj.metrics -and $null -ne $resultObj.metrics.cmax) {
                    $cmax = [int]$resultObj.metrics.cmax
                }
                if ($null -ne $resultObj.runtime_sec) {
                    $runtimeSec = [double]$resultObj.runtime_sec
                }
                $resultJson = $resultPath
            }
            catch {
                $status = 'result_json_error'
            }
        }
        if (Test-Path $metadataPath) { $metadataJson = $metadataPath }
        if (Test-Path $convPath) { $convergenceCsv = $convPath }
        if (Test-Path $schedPath) { $scheduleCsv = $schedPath }
    }

    if ($null -eq $cmax) {
        $m = [regex]::Match($output, 'Done\. Best Cmax=\s*(\d+)')
        if ($m.Success) {
            $cmax = [int]$m.Groups[1].Value
            if ($status -eq 'unknown') { $status = 'success' }
            $feasible = $true
        }
    }

    if ($exitCode -ne 0 -and $status -eq 'unknown') {
        $status = 'failed'
    }

    $gapPercent = ''
    try {
        if ($null -ne $cmax -and $referenceValue -ne '') {
            $ref = [double]$referenceValue
            $gap = 100.0 * (([double]$cmax - $ref) / $ref)
            $gapPercent = ([math]::Round($gap, 6)).ToString([System.Globalization.CultureInfo]::InvariantCulture)
        }
    }
    catch {
    }

    $csvLine = ConvertTo-CsvLine @(
        [string]$row.phase,
        [string]$instance,
        [string]$presetId,
        [string]$seed,
        [string]$runtimeBudget,
        [string]$terminationMode,
        [string]$referenceValue,
        [string]$referenceType,
        [string]$priorityGroup,
        [string]$runId,
        [string]$runDir,
        [string]$status,
        [string]$feasible,
        [string]$cmax,
        [string]$gapPercent,
        ([double]$runtimeSec).ToString([System.Globalization.CultureInfo]::InvariantCulture),
        [string]$resultJson,
        [string]$metadataJson,
        [string]$convergenceCsv,
        [string]$scheduleCsv,
        [string]$notes
    )
    Add-ContentSafe -Path $ValidationResults -Value $csvLine
}

# Aggregate validation results
$valRows = @(Import-Csv -Path $ValidationResults)
$groups = $valRows | Group-Object instance, preset_id

$aggHeader = @(
    'phase','instance','preset_id','reference_value','reference_type','priority_group',
    'run_count','completed_run_count','feasible_run_count','failed_run_count',
    'best_cmax','mean_cmax','median_cmax','std_cmax',
    'best_gap_percent','mean_gap_percent','median_gap_percent','std_gap_percent',
    'runtime_min_sec','runtime_mean_sec','runtime_median_sec','runtime_max_sec',
    'success_rate','winning_flag','keep_for_final','decision_note'
) -join ','
Set-Content -Path $AggregatedResults -Value $aggHeader -Encoding UTF8

# First compute summaries per group
$summaries = @()
foreach ($g in $groups) {
    $rows = @($g.Group)
    $instance = $rows[0].instance
    $presetId = $rows[0].preset_id
    $referenceValue = $rows[0].reference_value
    $referenceType = $rows[0].reference_type
    $priorityGroup = $rows[0].priority_group

    $completed = @($rows | Where-Object { $_.status -ne 'failed' -and $_.status -ne 'unknown' })
    $feasible = @($rows | Where-Object { $_.feasible -eq 'True' })
    $failed = @($rows | Where-Object { $_.status -eq 'failed' })

    $cmaxVals = @($feasible | ForEach-Object { To-DoubleOrNull $_.cmax } | Where-Object { $null -ne $_ })
    $gapVals = @($feasible | ForEach-Object { To-DoubleOrNull $_.gap_percent } | Where-Object { $null -ne $_ })
    $runtimeVals = @($completed | ForEach-Object { To-DoubleOrNull $_.runtime_sec } | Where-Object { $null -ne $_ })

    $bestCmax = if ($cmaxVals.Count -gt 0) { ($cmaxVals | Measure-Object -Minimum).Minimum } else { $null }
    $meanCmax = if ($cmaxVals.Count -gt 0) { ($cmaxVals | Measure-Object -Average).Average } else { $null }
    $medianCmax = if ($cmaxVals.Count -gt 0) { Get-Median $cmaxVals } else { $null }
    $stdCmax = if ($cmaxVals.Count -gt 0) { Get-Std $cmaxVals } else { $null }

    $bestGap = if ($gapVals.Count -gt 0) { ($gapVals | Measure-Object -Minimum).Minimum } else { $null }
    $meanGap = if ($gapVals.Count -gt 0) { ($gapVals | Measure-Object -Average).Average } else { $null }
    $medianGap = if ($gapVals.Count -gt 0) { Get-Median $gapVals } else { $null }
    $stdGap = if ($gapVals.Count -gt 0) { Get-Std $gapVals } else { $null }

    $runtimeMin = if ($runtimeVals.Count -gt 0) { ($runtimeVals | Measure-Object -Minimum).Minimum } else { $null }
    $runtimeMean = if ($runtimeVals.Count -gt 0) { ($runtimeVals | Measure-Object -Average).Average } else { $null }
    $runtimeMedian = if ($runtimeVals.Count -gt 0) { Get-Median $runtimeVals } else { $null }
    $runtimeMax = if ($runtimeVals.Count -gt 0) { ($runtimeVals | Measure-Object -Maximum).Maximum } else { $null }

    $successRate = if ($rows.Count -gt 0) { [math]::Round((100.0 * $feasible.Count / $rows.Count), 4) } else { 0.0 }

    $summary = [pscustomobject]@{
        phase = 'validation'
        instance = $instance
        preset_id = $presetId
        reference_value = $referenceValue
        reference_type = $referenceType
        priority_group = $priorityGroup
        run_count = $rows.Count
        completed_run_count = $completed.Count
        feasible_run_count = $feasible.Count
        failed_run_count = $failed.Count
        best_cmax = $bestCmax
        mean_cmax = $meanCmax
        median_cmax = $medianCmax
        std_cmax = $stdCmax
        best_gap_percent = $bestGap
        mean_gap_percent = $meanGap
        median_gap_percent = $medianGap
        std_gap_percent = $stdGap
        runtime_min_sec = $runtimeMin
        runtime_mean_sec = $runtimeMean
        runtime_median_sec = $runtimeMedian
        runtime_max_sec = $runtimeMax
        success_rate = $successRate
        winning_flag = ''
        keep_for_final = ''
        decision_note = ''
    }
    $summaries += $summary
}

# Mark best mean-gap per instance as winning_flag=yes
$instanceGroups = $summaries | Group-Object instance
foreach ($ig in $instanceGroups) {
    $valid = @($ig.Group | Where-Object { $null -ne $_.mean_gap_percent })
    if ($valid.Count -gt 0) {
        $bestMeanGap = ($valid | Measure-Object -Property mean_gap_percent -Minimum).Minimum
        foreach ($s in $ig.Group) {
            if ($null -ne $s.mean_gap_percent -and [double]$s.mean_gap_percent -eq [double]$bestMeanGap) {
                $s.winning_flag = 'yes'
            }
            else {
                $s.winning_flag = 'no'
            }
        }
    }
}

foreach ($s in $summaries) {
    $line = ConvertTo-CsvLine @(
        [string]$s.phase,
        [string]$s.instance,
        [string]$s.preset_id,
        [string]$s.reference_value,
        [string]$s.reference_type,
        [string]$s.priority_group,
        [string]$s.run_count,
        [string]$s.completed_run_count,
        [string]$s.feasible_run_count,
        [string]$s.failed_run_count,
        [string]$s.best_cmax,
        [string]$s.mean_cmax,
        [string]$s.median_cmax,
        [string]$s.std_cmax,
        [string]$s.best_gap_percent,
        [string]$s.mean_gap_percent,
        [string]$s.median_gap_percent,
        [string]$s.std_gap_percent,
        [string]$s.runtime_min_sec,
        [string]$s.runtime_mean_sec,
        [string]$s.runtime_median_sec,
        [string]$s.runtime_max_sec,
        [string]$s.success_rate,
        [string]$s.winning_flag,
        [string]$s.keep_for_final,
        [string]$s.decision_note
    )
    Add-ContentSafe -Path $AggregatedResults -Value $line
}

Write-Host ""
Write-Host "=========================================="
Write-Host "XSIM VALIDATION PIPELINE BITTI"
Write-Host ("Validation Results : {0}" -f (Resolve-Path $ValidationResults))
Write-Host ("Aggregated Results : {0}" -f (Resolve-Path $AggregatedResults))
Write-Host "=========================================="
