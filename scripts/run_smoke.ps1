param(
    [string] $ExePath = ".\djssp_pso_hh.exe"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Resolve-Path (Join-Path $ScriptDir "..")
Set-Location -LiteralPath $RepoRoot

if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "Executable not found: $ExePath"
}

function Invoke-XSim {
    param(
        [string[]] $Arguments,
        [string] $Name
    )

    $output = & $ExePath @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "$Name failed with exit code ${exitCode}: $($output -join "`n")"
    }

    return [pscustomobject]@{
        Name = $Name
        Output = @($output)
    }
}

function Get-RunDirFromOutput {
    param([string[]] $Output)

    $line = $Output | Select-String -Pattern "Run outputs written to" | Select-Object -Last 1
    if (-not $line) {
        throw "Run output directory line was not found."
    }

    return ($line.ToString() -replace "^.*Run outputs written to\s+", "").Trim()
}

function Get-BatchDirFromOutput {
    param([string[]] $Output)

    $line = $Output | Select-String -Pattern "Batch outputs written to" | Select-Object -Last 1
    if (-not $line) {
        throw "Batch output directory line was not found."
    }

    return ($line.ToString() -replace "^.*Batch outputs written to\s+", "").Trim()
}

function Assert-RunContract {
    param(
        [string] $RunDir,
        [string] $Name
    )

    $resultPath = Join-Path $RunDir "result.json"
    $schedulePath = Join-Path $RunDir "schedule.csv"
    $metadataPath = Join-Path $RunDir "metadata.json"

    if (-not (Test-Path -LiteralPath $RunDir)) {
        throw "$Name run directory was not created: $RunDir"
    }
    if (-not (Test-Path -LiteralPath $resultPath)) {
        throw "$Name missing result.json: $resultPath"
    }
    if (-not (Test-Path -LiteralPath $schedulePath)) {
        throw "$Name missing schedule.csv: $schedulePath"
    }
    if (-not (Test-Path -LiteralPath $metadataPath)) {
        throw "$Name missing metadata.json: $metadataPath"
    }

    $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    if ($result.feasibility.valid -ne $true) {
        throw "$Name feasibility.valid is not true."
    }
    if ([double]$result.metrics.cmax -ne [double]$result.feasibility.schedule_cmax) {
        throw "$Name result cmax does not match feasibility schedule_cmax."
    }

    $rows = @(Import-Csv -LiteralPath $schedulePath)
    if ($rows.Count -ne [int]$result.feasibility.expected_operation_count) {
        throw "$Name schedule row count $($rows.Count) does not match expected_operation_count $($result.feasibility.expected_operation_count)."
    }
    if ($rows.Count -ne [int]$result.feasibility.actual_operation_count) {
        throw "$Name schedule row count $($rows.Count) does not match actual_operation_count $($result.feasibility.actual_operation_count)."
    }
    if ($rows.Count -gt 0) {
        $scheduleCmax = [double](($rows | Measure-Object -Property end -Maximum).Maximum)
        if ($scheduleCmax -ne [double]$result.metrics.cmax) {
            throw "$Name schedule.csv max end $scheduleCmax does not match result cmax $($result.metrics.cmax)."
        }
    }

    return [pscustomobject]@{
        Name = $Name
        RunDir = $RunDir
        RunId = $result.run_id
        Cmax = $result.metrics.cmax
        Rows = $rows.Count
    }
}

function Assert-BatchContract {
    param([string] $BatchDir)

    $summaryPath = Join-Path $BatchDir "batch_summary.csv"
    if (-not (Test-Path -LiteralPath $BatchDir)) {
        throw "Batch directory was not created: $BatchDir"
    }
    if (-not (Test-Path -LiteralPath $summaryPath)) {
        throw "Batch summary CSV was not created: $summaryPath"
    }

    $summaryRows = @(Import-Csv -LiteralPath $summaryPath)
    if ($summaryRows.Count -lt 1) {
        throw "Batch summary CSV has no rows."
    }

    foreach ($row in $summaryRows) {
        if ($row.status -ne "success") {
            throw "Expected positive batch row to be success, got '$($row.status)' for $($row.config_path)."
        }
        if ($row.feasibility_valid -ne "true") {
            throw "Expected positive batch row feasibility_valid=true for $($row.config_path)."
        }
        if ([string]::IsNullOrWhiteSpace($row.run_id)) {
            throw "Positive batch row has empty run_id for $($row.config_path)."
        }

        $resultPath = Join-Path (Join-Path "runs" $row.run_id) "result.json"
        if (-not (Test-Path -LiteralPath $resultPath)) {
            throw "Batch result.json missing for run_id $($row.run_id): $resultPath"
        }
        $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
        if ([double]$result.metrics.cmax -ne [double]$row.cmax) {
            throw "Batch summary cmax $($row.cmax) does not match result.json cmax $($result.metrics.cmax)."
        }
    }

    return [pscustomobject]@{
        Name = "batch"
        BatchDir = $BatchDir
        Rows = $summaryRows.Count
    }
}

$checks = @()

$singleFull = Invoke-XSim -Name "single full config" -Arguments @("--config", "configs/smoke/ft06_smoke.json")
$checks += Assert-RunContract -Name "single full config" -RunDir (Get-RunDirFromOutput -Output $singleFull.Output)

$singleRule = Invoke-XSim -Name "single subset rule" -Arguments @("--config", "configs/smoke/ft06_spt.json")
$checks += Assert-RunContract -Name "single subset rule" -RunDir (Get-RunDirFromOutput -Output $singleRule.Output)

$singleFeature = Invoke-XSim -Name "single subset feature" -Arguments @("--config", "configs/smoke/ft06_feature_subset.json")
$checks += Assert-RunContract -Name "single subset feature" -RunDir (Get-RunDirFromOutput -Output $singleFeature.Output)

$batch = Invoke-XSim -Name "batch smoke" -Arguments @("--batch", "configs/benchmark_suite.json")
$checks += Assert-BatchContract -BatchDir (Get-BatchDirFromOutput -Output $batch.Output)

foreach ($check in $checks) {
    if ($check.BatchDir) {
        Write-Host "PASS $($check.Name): $($check.BatchDir) rows=$($check.Rows)"
    } else {
        Write-Host "PASS $($check.Name): $($check.RunDir) cmax=$($check.Cmax) rows=$($check.Rows)"
    }
}

Write-Host "Positive smoke tests passed."
