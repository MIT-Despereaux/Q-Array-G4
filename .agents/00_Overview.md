---
date: 2026-05-22
type: overview
tags: [q-array-g4, geant4, dspx, cryostat, simulation]
---

# Q-Array G4 — Project Vault

Organized technical documentation for the q-array-g4 Geant4 simulation.

## Reference Notes

- [[02_DSPX_Cryostat_Dimensions]] — Canonical DSPX cryostat dimensions and geometry decisions
- [[03_Geant4_Tutorial_Notes]] — Structured technical summary from Geant4 tutorial PDFs
- [[04_Geometry_Module_Reference]] — Analysis of the Simulation-develop geometry module
- [[05_STL_Placement_Workflow]] — FreeCAD → CADMesh → Geant4 STL placement guide

## Completed Plans

- [[01_Implementation_Plans]] — Completed implementation plans (namespace rename, DSPX cryostat merge)

## Active Plans

- [[06_Detector_Box_CAD_Import_Plan]] — Mixed CAD+CSG detector-box assembly plan (draft)

## Quick Links

- Repo: `~/GitProjects/q-array-g4`
- Build (DSPX): `cmake -S . -B build-dspx -DQARRAY_DETECTOR_GEOMETRY=DSPX && cmake --build build-dspx`
- Smoke test: `cd build && ./main run_test.mac`
- Namespace: `QArray` (C++), `/QR/` (Geant4 UI command prefix, stable for compatibility)