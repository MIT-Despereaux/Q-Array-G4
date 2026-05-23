---
date: 2026-05-20
type: reference
tags: [geant4, simulation-develop, geometry, architecture, integration]
---

# Geometry Module Reference — Simulation-develop

Source archive: `Simulation-develop.zip`
Extracted location: `cache/Simulation-develop/`
Geometry module: `cache/Simulation-develop/Geometry/`

---

## High-Level Layout

`Geometry/` is a self-contained Geant4 C++17 library-style module from the
extracted simulation. The archive also includes stale generated CMake build products.

- `CMakeLists.txt`: builds shared library target `Geometry` with output name
  `RicochetGeometry` and alias `Ricochet::Geometry`.
- `include/Geometry/`: public headers, 67 files.
- `src/`: implementations, 37 files.
- `OBJFiles/`: CAD mesh assets (`couche2.obj`, `couche3.obj`, `couche4.obj`).

## Build Dependencies

The module CMake expects:
- Geant4 `CONFIG` package, minimum 10.2
- ROOT 6.16 (specifically `ROOT::Tree`)
- `Ricochet::G4Helpers`
- `Ricochet::SimuOutput`

## Core Architecture

Organized around builder classes that construct Geant4 logical volumes into a
mother `G4LogicalVolume`.

- `VolumePack.hh`: small holder for logical/physical volume pairs
- `WorldMaker.hh`: helper to create a world volume pack
- `Shapes.hh` / `Shapes.cc`: reusable shape wrappers over Geant4 solids
- `BasicVolumeAdder.hh`: helper layers for constructing, coloring, and placing volumes
- `VolumeBuilder.hh`: queues templated `VolumeAdder::Add<Solid>` calls

## Builder Wrappers

The module uses template wrappers to add behaviour to plain builders:
- `ActiveBuilder.hh`: marks a builder active once `Build(...)` is called
- `SensitiveBuilder.hh`: wraps a builder and applies `Geometry::SensitiveDetector`
- `TransformingBuilder.hh`: transform-aware construction

## Major Geometry Builders

**Detector and cryostat:**
- `CryoCubeBuilder`
- `GermaniumBolometerBuilder`
- `GenericCryostatBuilder`
- `CryoConceptBuilder`
- `DubnaSpectrometerBuilder`

**Shielding and site:**
- `RicochetBasicBuilder`: aggregate/top-level builder
- `ColdInnerShieldingBuilder`, `RingInnerShieldingBuilder`
- `OuterShieldingBuilder`, `MuonVetoBuilder`, `ChoozSiteBuilder`

## Geometry Configurations

`GeometryConfiguration.hh` defines fixed configuration names:
`CROSS`, `Lyon`, `CryoConcept2020`, `RicochetFinalDesign`,
`RicochetRUN013`–`RUN016`.

## Integration Notes for Current Code Base

- Do not import generated files from `Geometry/CMakeFiles`, `Makefile`, or
  `cmake_install.cmake`.
- Source files under `Geometry/include/Geometry` and `Geometry/src` only.
- **Namespace mismatch:** extracted module uses `Geometry`; current project uses `QArray`.
- **Target mismatch:** current project does not expose Ricochet targets
  (`G4Helpers`, `SimuOutput`, `Utility`, `ROOT::Tree`).
- Start with foundation: `Shapes`, `VolumePack`, `VolumeAdder` before higher-level builders.
- Current `DetectorConstruction` builds lab/table/fridge/scintillator directly.
  Merging both models requires an explicit integration plan.