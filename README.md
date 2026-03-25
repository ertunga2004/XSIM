# XSIM

This repository contains the modularized `DJSSP_PSO_HH` source tree extracted from the original XSIM workspace.

## Layout

- `DJSSP_PSO_HH/`: modular C++ source tree
- `data/jobshop1.txt`: OR-Library job shop benchmark data used by the executable
- `build.ps1`: local build helper for MinGW g++

## Build

```powershell
.\build.ps1
```

This produces `djssp_pso_hh.exe`.

## Run

```powershell
.\djssp_pso_hh.exe ft06 --iters 1 --swarm 2 --evalk 1 --finalk 1 --seed 1
```
