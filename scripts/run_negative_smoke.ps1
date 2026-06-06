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

function Invoke-ExpectedFailure {
    param(
        [string] $Name,
        [string[]] $Arguments,
        [string] $ExpectedPattern
    )

    $oldErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $ExePath @Arguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $oldErrorActionPreference
    }
    $text = $output -join "`n"

    if ($exitCode -eq 0) {
        throw "$Name unexpectedly succeeded. Output: $text"
    }
    if ([string]::IsNullOrWhiteSpace($text)) {
        throw "$Name failed silently with no visible error output."
    }
    if ($text -notmatch $ExpectedPattern) {
        throw "$Name failed with unexpected output. Expected pattern '$ExpectedPattern'. Output: $text"
    }

    Write-Host "PASS ${Name}: exit=$exitCode matched '$ExpectedPattern'"
}

Invoke-ExpectedFailure `
    -Name "unknown rule" `
    -Arguments @("--config", "configs/smoke/ft06_bad_rule.json") `
    -ExpectedPattern "Unknown rule name"

Invoke-ExpectedFailure `
    -Name "unknown feature" `
    -Arguments @("--config", "configs/smoke/ft06_bad_feature.json") `
    -ExpectedPattern "Unknown feature name"

Invoke-ExpectedFailure `
    -Name "bad instance path" `
    -Arguments @("--config", "configs/smoke/ft06_bad_path.json") `
    -ExpectedPattern "Cannot open file|Make sure instance"

Invoke-ExpectedFailure `
    -Name "missing batch config" `
    -Arguments @("--batch", "configs/no_such_suite.json") `
    -ExpectedPattern "Could not open batch config file"

Write-Host "Negative smoke tests passed."
