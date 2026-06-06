$ErrorActionPreference = "Stop"

<<<<<<< HEAD
g++ -std=c++17 -O2 -I DJSSP_PSO_HH `
  DJSSP_PSO_HH/main.cpp `
  DJSSP_PSO_HH/InstanceGenerator.cpp `
  DJSSP_PSO_HH/Simulation.cpp `
  DJSSP_PSO_HH/Pso.cpp `
  -o djssp_pso_hh.exe
=======
function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $candidates = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "C:\msys64\ucrt64\bin\cmake.exe",
        "C:\msys64\mingw64\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    return $null
}

function Copy-BuiltBinary {
    param([string] $BuildDir)

    $candidates = @(
        (Join-Path $BuildDir "bin\djssp_pso_hh.exe"),
        (Join-Path $BuildDir "bin\Release\djssp_pso_hh.exe"),
        (Join-Path $BuildDir "Release\djssp_pso_hh.exe"),
        (Join-Path $BuildDir "djssp_pso_hh.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            Copy-Item -LiteralPath $candidate -Destination ".\djssp_pso_hh.exe" -Force
            Write-Host "Copied binary to .\djssp_pso_hh.exe"
            return
        }
    }

    throw "Built binary not found under $BuildDir"
}

function Invoke-LegacyGccBuild {
    Write-Warning "CMake was not found. Falling back to the legacy g++ build so the local Windows workflow keeps working."

    $compilerCommand = Get-Command g++ -ErrorAction SilentlyContinue
    if (-not $compilerCommand) {
        throw "Neither cmake nor g++ was found. Install CMake or make g++ available on PATH."
    }

    $sources = @(
        "DJSSP_PSO_HH/main.cpp",
        "DJSSP_PSO_HH/BatchConfigLoader.cpp",
        "DJSSP_PSO_HH/BatchRunner.cpp",
        "DJSSP_PSO_HH/ConfigLoader.cpp",
        "DJSSP_PSO_HH/InstanceGenerator.cpp",
        "DJSSP_PSO_HH/InstanceCatalog.cpp",
        "DJSSP_PSO_HH/InstanceLoader.cpp",
        "DJSSP_PSO_HH/FeatureVectorBuilder.cpp",
        "DJSSP_PSO_HH/RuleRegistry.cpp",
        "DJSSP_PSO_HH/Simulation.cpp",
        "DJSSP_PSO_HH/GtScheduleGenerator.cpp",
        "DJSSP_PSO_HH/EventScheduleGenerator.cpp",
        "DJSSP_PSO_HH/FeasibilityChecker.cpp",
        "DJSSP_PSO_HH/Pso.cpp",
        "DJSSP_PSO_HH/ResultWriter.cpp"
    )

    & $compilerCommand.Source -std=c++17 -O2 -I DJSSP_PSO_HH @sources -o djssp_pso_hh.exe
}

$cmake = Find-CMake
if ($cmake) {
    & $cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    & $cmake --build build --config Release
    Copy-BuiltBinary -BuildDir "build"
} else {
    Invoke-LegacyGccBuild
}
>>>>>>> e2a2af7 (Checkpoint: P0-P3 refactor completed)
