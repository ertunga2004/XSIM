$ErrorActionPreference = "Stop"

$cmake = Get-Command cmake -ErrorAction SilentlyContinue

if ($cmake) {
    Write-Host "CMake bulundu. CMake build akisi kullaniliyor..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

    $builtExe = Join-Path "build" "bin\djssp_pso_hh.exe"
    if (-not (Test-Path $builtExe)) {
        $builtExe = Join-Path "build" "djssp_pso_hh.exe"
    }

    if (-not (Test-Path $builtExe)) {
        throw "CMake build tamamlandi ama djssp_pso_hh.exe bulunamadi."
    }

    Copy-Item -Force $builtExe ".\djssp_pso_hh.exe"
    Write-Host "Build tamamlandi: .\djssp_pso_hh.exe"
    exit 0
}

Write-Warning "CMake bulunamadi. Legacy g++ fallback kullanilacak."

g++ -std=c++17 -O2 -I DJSSP_PSO_HH `
  DJSSP_PSO_HH/main.cpp `
  DJSSP_PSO_HH/InstanceGenerator.cpp `
  DJSSP_PSO_HH/Simulation.cpp `
  DJSSP_PSO_HH/Pso.cpp `
  DJSSP_PSO_HH/ResultWriter.cpp `
  DJSSP_PSO_HH/FeasibilityChecker.cpp `
  DJSSP_PSO_HH/RuleRegistry.cpp `
  DJSSP_PSO_HH/FeatureVectorBuilder.cpp `
  DJSSP_PSO_HH/BatchConfigLoader.cpp `
  DJSSP_PSO_HH/BatchRunner.cpp `
  -o djssp_pso_hh.exe

Write-Host "Legacy build tamamlandi: .\djssp_pso_hh.exe"
