---
date: 2026-05-22
type: plan
tags: [q-array-g4, geant4, namespace, dspx, cryostat, implementation]
---

# Implementation Plans — Completed

Archived implementation plans for completed namespace and geometry milestones.

---

## Plan A: Rename `QR` Namespace to `QArray`

Status: **✅ Completed**

[Detailed design and task breakdown → suppress for brevity in merged note]

### Summary

Renamed the project C++ namespace from `QR` to `QArray` across all headers and
implementation files while keeping the Geant4 UI command prefix `/QR/...` stable
for compatibility with existing macros and user scripts.

### Decisions

- C++ namespace: `namespace QArray` (was `namespace QR`)
- Geant4 UI commands: retained `/QR/...` (stable prefix)
- Architecture: mechanical rename only; no file renames and no command path
  changes unless explicitly requested later.

### Files Touched

- `main.cc`: `using namespace QArray;`
- `include/*.hh`: headers updated
- `src/*.cc`: implementations updated
- `AGENTS.md` and `CHANGELOG.md`: updated guidance and changelog entry

### Verification

```sh
# Confirmed no residual QR namespace references in C++ scope:
rg -n 'namespace QR|QR::|using namespace QR' main.cc include src tests
# (returns no matches)

# New namespace present:
rg -n 'namespace QArray|QArray::' main.cc include src
# (shows expected declarations)

# Smoke test passed:
cmake --build build && cd build && ./main run_test.mac
```

---

## Plan B: DSPX Cryostat Merge

Status: **✅ Done** (branch: `feature/dspx-cryostat-geometry`)  
Branch: `feature/dspx-cryostat-geometry`  
Build: `cmake -S . -B build-dspx -DQARRAY_DETECTOR_GEOMETRY=DSPX`

### Architecture

The DSPX geometry is a configure-time variant selected via
`-DQARRAY_DETECTOR_GEOMETRY=DSPX`. `LEIDEN_II` remains the default and is
never touched by DSPX development.

```
QARRAY_DETECTOR_GEOMETRY=DSPX compiles:
  src/DetectorConstruction_DSPX.cc   -- world/lab/fridge placement
  src/Geometry/CryostatBuilder.cc    -- cryostat internals
  src/Geometry/Shapes.cc             -- FilledTube, Bucket, HexBride helpers
```

`CryostatBuilder::Build()` returns `CryostatVolumes` with logical volume
pointers (`fridgeLogical`, `ovcLogical`, `ovcVacuumLogical`,
`mixingChamberLogical`, `plate10mKCenter`) for downstream detector placement.

### Geometry Decisions

**Plates:** CryoConcept2020 dimensions. See [[02_DSPX_Cryostat_Dimensions]]
for all confirmed values (canonical source).

**Screens:** `Bucket` (G4Polycone cup, flangeless). Screen1K, Screen4K,
Screen50K, OVC. No Screen10mK. No floating plate.

**Bride:** `HexBride` (G4Polyhedra, 6-sided). ri=187mm, flat-to-flat=416mm,
circumR=240.2mm, h=97mm, top disk e=14mm.

**Gap cascade:** CryoConcept/CROSS-style chain. Inter-screen gaps use CROSS
defaults (19.5mm, 19.8mm, 20.0mm) until DSPX drawings confirm.

**CADMesh:** submodule at `submodules/cadmesh` (v2.0.3), header resolved via
CMake. STL inventory and origin convention in `src/Geometry/obj/README.md`.

### Phase Status

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Source audit, lock geometry inputs | ✅ done |
| 1 | CMake geometry selector (`QARRAY_DETECTOR_GEOMETRY`) | ✅ done |
| 2 | Shape helpers (`FilledTube`, `Bucket`, `HexBride`) | ✅ done |
| 3 | `CryostatBuilder` -- plates, screens, OVC, bride | ✅ done |
| 4 | `DetectorConstruction_DSPX.cc` integration | ✅ done |
| 5 | Batch validation, overlap checks | ✅ done |
| 6 | CADMesh submodule + `src/Geometry/obj/` setup | ✅ done |
| 7 | `BuildMeshComponents()` -- STL loading + placement | ✅ done |

### Remaining Work

- [ ] Run `geometry_lab_proton.mac` (interactive vis) to visually verify the
  full DSPX geometry including the Experimental_Paddle mesh placement.
- [ ] Verify Experimental_Paddle overlap checks pass with `checkOverlaps=true`.
- [ ] Update `CHANGELOG.md` when the feature branch lands on main.
- [ ] Confirm inter-screen gap distances against actual DSPX drawings (currently
  using CROSS defaults: 19.5mm / 19.8mm / 20.0mm).

### Key Implementation Files

| File | Purpose |
|------|---------|
| `include/Geometry/CryostatBuilder.hh` | Builder API + `CryostatVolumes` struct |
| `src/Geometry/CryostatBuilder.cc` | CSG cryostat + `BuildMeshComponents()` |
| `include/Geometry/Shapes.hh` | `FilledTube`, `Bucket`, `HexBride` |
| `src/Geometry/Shapes.cc` | Shape implementations |
| `src/DetectorConstruction_DSPX.cc` | World/lab/fridge + delegates to builder |

### Adding a New STL Component

1. Export from FreeCAD: top face center at `(0,0,0)`, Z-up, mm, deviation
   <= 0.1mm. See [[05_STL_Placement_Workflow]].
2. Copy `.stl` to `src/Geometry/obj/`.
3. Add row to `src/Geometry/obj/README.md`.
4. Add `MeshSpec` entry in `CryostatBuilder::BuildMeshComponents()`.
5. Commit the `.stl` file (STL files are tracked in git).
6. `cmake --build build-dspx` — CMake copies the file automatically.
7. Verify with `builder.SetVerbose(1)` extent print + visualization.