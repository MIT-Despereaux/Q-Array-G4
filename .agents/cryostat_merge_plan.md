# Cryostat Merge Plan — COMPLETED

## Summary

Created `QArray::Geometry::CryostatBuilder` in this code base using geometry
logic mined from `cache/Simulation-develop/Geometry/`, without importing ROOT
serialization, messenger infrastructure, SimuOutput, or the full Ricochet
builder framework. Implemented on branch `feature/dspx-cryostat-geometry`.

The DSPX geometry is a configure-time variant selected via
`-DQARRAY_DETECTOR_GEOMETRY=DSPX`. `LEIDEN_II` remains the default.

## What Was Built

### Architecture
```
QARRAY_DETECTOR_GEOMETRY=DSPX compiles:
  src/DetectorConstruction_DSPX.cc   -- world/lab/fridge placement
  src/Geometry/CryostatBuilder.cc    -- cryostat internals
  src/Geometry/Shapes.cc             -- FilledTube, Bucket, HexBride helpers
```

`CryostatBuilder::Build()` returns `CryostatVolumes` with logical volume
pointers (`fridgeLogical`, `ovcLogical`, `ovcVacuumLogical`,
`mixingChamberLogical`, `plate10mKCenter`) for downstream detector placement.

### Key Design Decisions

**Namespace:** `QArray::Geometry` — not the extracted `Geometry` namespace.

**Shapes kept:** `FilledTube`, `Bucket`, `HexBride` (DSPX-specific hexagonal
prism bride via `G4Polyhedra`). `ReversedBucket` retained for reference but
NOT used for DSPX.

**What was stripped:** ROOT headers, `Write(TDirectory&)`, `ObjectSerialiser`,
`SimuOutput::*`, `G4Helpers::*`, `Utility::*`, `ActiveBuilder`,
`SensitiveBuilder`, `OwningBuilderMessenger`, `/geometry/...` UI commands,
`MiniCryoCubeBuilder` coupling, generated build files.

**Target configuration:** CryoConcept2020 plate dimensions + Bucket screen
construction. No Screen10mK, no floating plate. Bride = `HexBride` (not
`ReversedBucket`). Position cascade uses CryoConcept/CROSS-style gap chain.

**CMake:** Configure-time selection, not run-time switching. `LEIDEN_II` is
default until DSPX is validated.

### Geometry Decisions

**Plates:** CryoConcept2020 dimensions. See `02_DSPX_Cryostat_Dimensions.md`.

**Screens:** `Bucket` (G4Polycone cup, flangeless). Screen1K, Screen4K,
Screen50K, OVC. Inter-screen gaps use CROSS defaults (19.5mm, 19.8mm, 20.0mm)
until DSPX drawings confirm.

**Bride:** `HexBride` (G4Polyhedra, 6-sided). ri=187mm, flat-to-flat=416mm,
circumR=240.2mm, h=97mm, top disk e=14mm.

**CADMesh:** submodule at `submodules/cadmesh` (v2.0.3). STL inventory and
origin convention in `src/Geometry/obj/README.md`.

### Phase Status

| Phase | Description | Status |
|-------|-------------|--------|
| 0 | Source audit, lock geometry inputs | ✅ done |
| 1 | CMake geometry selector (`QARRAY_DETECTOR_GEOMETRY`) | ✅ done |
| 2 | Shape helpers (`FilledTube`, `Bucket`, `HexBride`) | ✅ done |
| 3 | `CryostatBuilder` — plates, screens, OVC, bride | ✅ done |
| 4 | `DetectorConstruction_DSPX.cc` integration | ✅ done |
| 5 | Batch validation, overlap checks | ✅ done |
| 6 | CADMesh submodule + `src/Geometry/obj/` setup | ✅ done |
| 7 | `BuildMeshComponents()` — STL loading + placement | ✅ done |

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
   <= 0.1mm. See `05_STL_Placement_Workflow.md`.
2. Copy `.stl` to `src/Geometry/obj/`.
3. Add row to `src/Geometry/obj/README.md`.
4. Add `MeshSpec` entry in `CryostatBuilder::BuildMeshComponents()`. The struct
   uses `G4ThreeVector position` (offset in parent LV frame) and `G4double
   rotationZ` (degrees around Z axis). See inline comments in
   `CryostatBuilder.cc` for the `ovcVacLocalCenterZ` constant.
5. Commit the `.stl` file (tracked in git).
6. `cmake --build build-dspx` — CMake copies the file automatically.
7. Verify with `builder.SetVerbose(1)` extent print + visualization.

---

## Remaining Work

- [ ] Run `geometry_lab_proton.mac` (interactive vis) to visually verify the
  full DSPX geometry including the detector box assembly.
- [ ] Verify overlap checks pass with `checkOverlaps=true` for all mesh components
  (Experimental_Paddle, Ricochet_Box_Platform, Detector_Base, Sn crystal).
- [ ] Complete detector-box CSG sub-parts (small copper/sapphire pieces, box cover
  STL) per `06_Detector_Box_CAD_Import_Plan.md`.
- [ ] Update `CHANGELOG.md` when the feature branch lands on main.
- [ ] Confirm inter-screen gap distances against actual DSPX drawings (currently
  using CROSS defaults: 19.5mm / 19.8mm / 20.0mm).

### Previous fixes
- [x] Fix Experimental_Paddle STL format (ASCII `.ast` → `.stl`), rename file (commits `987244c`, `988b7a1`)
- [x] Correct mesh placement z-position and G4Tubs center offset (commit `84b6a26`)
- [x] Adjust visualization macro for DSPX geometry (commit `07f114c`)
- [x] Add detector-box assembly mother LV + Sn crystal CSG (commit `82ecb21`)
