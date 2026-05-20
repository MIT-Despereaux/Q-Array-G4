# Simulation-develop Geometry Structure

Source archive: `Simulation-develop.zip`
Extracted location: `cache/Simulation-develop/`
Geometry module location: `cache/Simulation-develop/Geometry/`

## High-level layout

`Geometry/` is a self-contained Geant4 C++17 library-style module from the
extracted simulation. It is not just geometry source; the archive also includes
stale generated CMake build products.

- `CMakeLists.txt`: builds shared library target `Geometry` with output name
  `RicochetGeometry` and alias `Ricochet::Geometry`.
- `include/Geometry/`: public headers, 67 files.
- `src/`: implementations, 37 files.
- `OBJFiles/`: CAD mesh assets used by geometry builders:
  `couche2.obj`, `couche3.obj`, `couche4.obj`; also contains `.DS_Store`.
- `CMakeFiles/`, `Makefile`, `cmake_install.cmake`,
  `RicochetGeometryConfig.cmake`: generated/configured build files from the
  source project. Treat these as reference only, not integration source.

## Build dependencies

The module CMake expects:

- Geant4 `CONFIG` package, minimum 10.2.
- ROOT 6.16, specifically links `ROOT::Tree`.
- `Ricochet::G4Helpers`.
- `Ricochet::SimuOutput`.

Headers and source also reference helpers from `Utility`, ROOT `TDirectory` /
`TTree` / `TString`, and output metadata helpers such as
`SimuOutput::InfoHeaderWriter`, `VolumeHasher`, `VolumeID`, and `HitEvent`.

The current code base uses namespace `QR`, while this extracted module uses
namespace `Geometry` and Ricochet CMake target names. Integration will need a
namespace and dependency strategy before copying code directly.

## Core architecture

The module is organized around builder classes that construct Geant4 logical
volumes into a mother `G4LogicalVolume`.

- `VolumePack.hh`: small holder for logical/physical volume pairs.
- `WorldMaker.hh`: helper to create a world volume pack.
- `Shapes.hh` / `Shapes.cc`: reusable shape wrappers over Geant4 solids:
  `FilledCube`, `FilledBox`, `Wall`, `Box`, `SameThicknessBox`,
  `SameThicknessHollowBox`, `Tube`, `FilledTube`, `Bucket`,
  `ReversedBucket`, `Cylinder`, and `SameThicknessCylinder`.
- `BasicVolumeAdder.hh`, `VolumeAdder.hh`, `IdenticalVolumeAdder.hh`,
  `ColouredVolumeAdder.hh`: helper layers for constructing, coloring, and
  placing volumes.
- `VolumeBuilder.hh`: queues templated `VolumeAdder::Add<Solid>` calls and
  later builds them into a mother volume.
- `VolumeParser.hh`, `SpecificShapeArgs.hh`,
  `VolumeBuilderMessengerHelper.hh`: parse UI command arguments into typed
  shape builder calls.

## Builder wrappers

The module uses template wrappers to add behavior to plain builders:

- `ActiveBuilder.hh`: marks a builder active once `Build(...)` is called.
  Used so only activated builders can be serialized later.
- `SensitiveBuilder.hh`: wraps a builder and applies
  `Geometry::SensitiveDetector` to returned logical volumes.
- `TransformingBuilder.hh`: adds transform-aware construction.
- `DefaultBuilderMessenger.hh`, `OwningBuilderMessenger.hh`,
  `SimpleBuilderMessenger.hh`, `BuilderAware*`: select and own UI messengers
  for builders.

These wrappers are exposed through macros such as
`DEFINE_ACTIVE_BUILDER_FROM(...)` and `DEFINE_SENSITIVE_BUILDER_FROM(...)`.

## Major geometry builders

Detector and cryostat builders:

- `CryoCubeBuilder`
- `MiniCryoCubeBuilder`
- `GermaniumBolometerBuilder`
- `SimpleGermaniumBolometerBuilder`
- `GenericBolometerBuilder`
- `GenericCryostatBuilder`
- `Cryostat`
- `CryoConceptBuilder`
- `DubnaSpectrometerBuilder`

Shielding and site builders:

- `RicochetBasicBuilder`: aggregate/top-level RICOCHET geometry builder.
- `ColdInnerShieldingBuilder`
- `RingInnerShieldingBuilder`
- `OuterShieldingBuilder`
- `OuterShieldingBuilderWithDT`
- `CylindricalShieldingBuilder`
- `MuonVetoBuilder`
- `ChoozSiteBuilder`

`RicochetBasicBuilder` composes cryocube, minicryocube, cryostat, inner
shielding, ring shielding, outer shielding, outer shielding with DT, and muon
veto builders. It has toggles for each subsystem and spacing/configuration
setters, then builds the selected pieces from one position.

## Geometry configurations

`GeometryConfiguration.hh` defines fixed configuration names:

- `CROSS`
- `Lyon`
- `CryoConcept2020`
- `RicochetFinalDesign`
- `RicochetRUN013`
- `RicochetRUN014`
- `RicochetRUN015`
- `RicochetRUN016`

The class is a small value wrapper with `ToStr()`, equality operators,
`ListOfConfig()`, and `FromStr(...)`.

## Command and messenger surface

Most user control is via Geant4 UI commands rooted under `/geometry/...`.
Important command families include:

- `/geometry/volumes/add`, `/clearQueue`, `/build`, `/buildNoClear`
- `/geometry/detector/CryoCube/build`
- `/geometry/detector/MiniCryoCube/build`
- `/geometry/detector/GermaniumBolometer/build`
- `/geometry/detector/SimpleGermaniumBolometer/build`
- `/geometry/detector/DubnaSpectrometer/build`
- `/geometry/detector/CryoCube/sensitive`
- `/geometry/site/Chooz/build`
- builder-specific commands for cryostats, shielding, muon veto, spacing,
  verbosity, sensitivity, and configuration selection.

The extracted parent `DetectorConstruction` owns a tuple of
`OwningBuilderMessenger<...>` instances, one per builder. The current code base
instead has `QR::DetectorConstruction` with direct construction methods and
`G4GenericMessenger` metadata commands, so integration is likely to be adapter
based rather than a drop-in replacement.

## Sensitive detector and output support

- `Hit.hh` / `Hit.cc`: Geant4 hit representation with SimuOutput volume IDs.
- `SensitiveDetector.hh` / `.cc`: records hits and uses
  `SimuOutput::VolumeHasher`.
- `ObjectSerialiser.hh` / `.cc`: writes activated builder metadata to ROOT.
- Several builders implement `Write(TDirectory&)` using
  `SimuOutput::InfoHeaderWriter`.

This is broader than geometry construction: bringing in sensitive detector
support also brings ROOT and SimuOutput contracts.

## Integration notes for current code base

- Do not import generated files from `Geometry/CMakeFiles`, `Makefile`, or
  `cmake_install.cmake`.
- Start with source files under `Geometry/include/Geometry` and `Geometry/src`,
  plus the OBJ assets if a builder needs CAD mesh layers.
- Resolve namespace mismatch: extracted module is `Geometry`; current project
  code is `QR`.
- Resolve target/dependency mismatch: current project does not currently expose
  Ricochet `G4Helpers`, `SimuOutput`, `Utility`, or ROOT helper targets in the
  same way as the extracted module.
- Consider integrating one builder family at a time. `Shapes`, `VolumePack`,
  `VolumeAdder`, `MaterialColour`, and `MaterialsBuilder` are foundational.
  Higher-level builders such as `RicochetBasicBuilder` depend on many of the
  lower-level pieces and on output serialization.
- The current `QR::DetectorConstruction` already constructs lab, table, fridge,
  and scintillator/chip sensitive detectors directly. The extracted simulation
  uses a builder/messenger registry in its own `Geometry::DetectorConstruction`;
  merging both models should be planned explicitly.
