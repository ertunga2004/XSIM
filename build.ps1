$ErrorActionPreference = "Stop"

g++ -std=c++17 -O2 -I DJSSP_PSO_HH `
  DJSSP_PSO_HH/main.cpp `
  DJSSP_PSO_HH/InstanceGenerator.cpp `
  DJSSP_PSO_HH/Simulation.cpp `
  DJSSP_PSO_HH/Pso.cpp `
  -o djssp_pso_hh.exe
