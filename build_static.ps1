$ErrorActionPreference = "Stop"

$compiler = "C:\msys64\ucrt64\bin\g++.exe"
if (-not (Test-Path $compiler)) {
    throw "Compiler not found: $compiler"
}

& $compiler -std=c++17 -O2 -static -static-libgcc -static-libstdc++ -I DJSSP_PSO_HH `
  DJSSP_PSO_HH/main.cpp `
  DJSSP_PSO_HH/InstanceGenerator.cpp `
  DJSSP_PSO_HH/Simulation.cpp `
  DJSSP_PSO_HH/Pso.cpp `
  -o djssp_pso_hh.exe
