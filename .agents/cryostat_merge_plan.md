# Cryostat Merge Plan

## Decision

Use `QArray::Geometry` for imported cryostat geometry code.

The current repository uses `QArray` as its real namespace. The extracted
simulation in `cache/Simulation-develop/Geometry` uses `Geometry`, but we do
not want to import that whole framework. The cryostat code should become part
of this project, under a geometry sub-namespace owned by QArray:

```cpp
namespace QArray::Geometry
{
  class CryostatBuilder;
}
```

This keeps the application namespace coherent while leaving room for future
geometry builders if more extracted components are merged later.

## Goal

Create a stripped-down cryostat builder in this code base using useful geometry
logic from the cached simulation, without importing ROOT serialization,
messenger infrastructure, SimuOutput, or the full Ricochet builder framework.

The current `QArray::DetectorConstruction` should remain the owner of world,
lab, table, fridge placement, and run-time geometry options. The imported
cryostat code should only build cryostat geometry into a provided mother
logical volume.

## Current Code Base

Current implementation:

- Namespace: `QArray`
- Main geometry owner: `QArray::DetectorConstruction`
- Current cryostat entry point: `DetectorConstruction::ConstructFridge()`
- Style: direct Geant4 construction with `G4Box`, `G4Tubs`,
  `G4Polycone`, `G4SubtractionSolid`, `G4LogicalVolume`, and
  `G4PVPlacement`
- Configuration: current metadata / messenger style under QArray code
- Sensitive detector/output: current local QArray classes

This should remain the controlling framework.

## Cache Code To Mine

Relevant cached files:

- `cache/Simulation-develop/Geometry/include/Geometry/Cryostat.hh`
- `cache/Simulation-develop/Geometry/src/Cryostat.cc`
- `cache/Simulation-develop/Geometry/include/Geometry/GenericCryostatBuilder.hh`
- `cache/Simulation-develop/Geometry/src/GenericCryostatBuilder.cc`
- `cache/Simulation-develop/Geometry/include/Geometry/GeometryConfiguration.hh`

`Geometry::Cryostat` is simpler and more standalone, but appears older and
Lyon-style.

`Geometry::GenericCryostatBuilder` has the more relevant final/run geometry
configuration logic, but it depends on the extracted framework. Prefer mining
its geometry constants and construction logic, not copying its framework shape
as-is.

## Proposed Local API

Create:

- `include/CryostatBuilder.hh`
- `src/CryostatBuilder.cc`

Initial API:

```cpp
namespace QArray::Geometry
{
  struct CryostatVolumes
  {
    G4LogicalVolume* ovcLogical = nullptr;
    G4LogicalVolume* ovcVacuumLogical = nullptr;
    G4LogicalVolume* mixingChamberLogical = nullptr;
  };

  class CryostatBuilder
  {
  public:
    explicit CryostatBuilder(G4LogicalVolume& mother);

    void SetCheckOverlaps(G4bool value);
    void SetVerbose(G4int level);

    CryostatVolumes Build(const G4ThreeVector& position);
  };
}
```

Return key logical volumes only if current detector/chip placement needs them.
If no downstream code needs these pointers, `Build(...)` can return `void`.

## What To Strip

Do not import:

- ROOT headers such as `TDirectory.h`, `TTree.h`, `TString.h`
- `Write(TDirectory&)` methods
- `ObjectSerialiser`
- `SimuOutput::*`
- `G4Helpers::*`
- `Utility::*`
- `ActiveBuilder`
- `SensitiveBuilder`
- `OwningBuilderMessenger`
- `/geometry/...` Geant4 UI command framework
- `MiniCryoCubeBuilder` coupling unless explicitly needed later
- Generated build files from the cache tree

## Shape Helpers To Keep

Keep a small local version of the cache shape wrappers because they improve
readability and reduce repeated Geant4 boilerplate.

Recommended local files:

- `include/Geometry/Shapes.hh`
- `src/Geometry/Shapes.cc`
- optionally `include/Geometry/VolumePlacement.hh`
- optionally `src/Geometry/VolumePlacement.cc`

Use namespace `QArray::Geometry`, not the cache namespace:

```cpp
namespace QArray::Geometry
{
  struct FilledTube;
  struct Bucket;
  struct ReversedBucket;
}
```

For the cryostat merge, start with only the shapes needed by the cryostat:

- `FilledTube`
- `Bucket`
- `ReversedBucket`
- possibly `Tube` if useful internally

Do not import the whole volume-builder framework just to get these wrappers.
In particular, avoid bringing over `VolumeBuilder`, `VolumeParser`,
`VolumeBuilderMessenger`, and the `/geometry/volumes/...` command layer.

It is acceptable to add a small local placement helper so construction calls can
stay compact, for example:

```cpp
template <class Solid, class... Args>
G4LogicalVolume* PlaceVolume(
    G4LogicalVolume& mother,
    const G4String& name,
    G4Material& material,
    const G4ThreeVector& position,
    G4bool checkOverlaps,
    Args&&... args);
```

This gives us most of the readability benefit of cache calls such as:

```cpp
factory.Add<FilledTube>("Plate10mK", "CopperPlate", position, radius, height);
```

without adopting cache dependencies such as `MaterialColour`, `VolumePack`,
`ColouredVolumeAdder`, `G4Helpers`, or command parsing.

## Output Dependency Clarification

The output dependencies in the cached geometry module are not needed just to
build the cryostat geometry.

In the cache framework, `SimuOutput` and ROOT are used for extra bookkeeping:

- writing activated builder metadata into ROOT output files
- hashing physical volume names into stable volume IDs
- storing hit/event data with volume IDs
- serializing builder settings through `InfoHeaderWriter`

That is useful for the original Ricochet simulation workflow, but it is not a
geometry-construction requirement. For this merge, keep output ownership in the
current QArray data/output classes and avoid importing cached output contracts.

If later we need volume-name metadata, add a small QArray-native hook after the
geometry is constructed instead of adopting `SimuOutput`.

## Implementation Approach

1. Add `QArray::Geometry::CryostatBuilder`.
2. Move reusable cryostat helpers out of `DetectorConstruction::ConstructFridge`
   where appropriate, such as can/screen construction helpers.
3. Add a small `QArray::Geometry` shape-helper layer inspired by cache
   `Shapes.hh`, especially `FilledTube`, `Bucket`, and `ReversedBucket`.
4. Mine dimensions and final-design construction logic from
   `GenericCryostatBuilder`, translating framework calls like
   `factory.Add<FilledTube>(...)` into local helper calls using
   `QArray::Geometry::FilledTube` and direct `G4PVPlacement`.
5. Keep material setup local and minimal. Prefer NIST materials already used in
   this project. Define custom cryostat materials only when the geometry needs
   names not available through NIST.
6. Update `QArray::DetectorConstruction::ConstructFridge()` to delegate to
   `QArray::Geometry::CryostatBuilder`.
7. Preserve existing current-code behavior for lab/table/global offset/fridge
   placement unless deliberately changing it.
8. Build and run the project smoke macro after implementation.

## CMake Geometry Selection

Add a CMake cache option to choose the detector geometry implementation at
configure time:

```cmake
set(QARRAY_DETECTOR_GEOMETRY "LEIDEN_II" CACHE STRING "Detector geometry implementation: CURIE, LEIDEN_II, or DSPX")
set_property(CACHE QARRAY_DETECTOR_GEOMETRY PROPERTY STRINGS CURIE LEIDEN_II DSPX)
```

Intended meanings:

- `CURIE`: `src/DetectorConstruction_CURIE.cc`.
- `LEIDEN_II`: current `src/DetectorConstruction.cc` implementation with lab,
  table, and the existing non-DSPX fridge geometry.
- `DSPX`: future imported/translated cryostat geometry mined from
  `cache/Simulation-develop`.

Implementation notes:

- Avoid compiling multiple `DetectorConstruction` implementations that define
  the same class.
- Replace the current ad hoc source exclusion:
  `list(REMOVE_ITEM sources ${PROJECT_SOURCE_DIR}/src/DetectorConstruction_CURIE.cc)`
  with explicit selection.
- Keep common helpers such as `QArray::Geometry::Shapes` available to DSPX
  without requiring ROOT, SimuOutput, or cache libraries.
- Add a compile definition such as `QARRAY_DETECTOR_GEOMETRY_DSPX` only if
  needed by shared source files. Prefer source selection where practical.

Possible CMake shape:

```cmake
set(QARRAY_DETECTOR_GEOMETRY "LEIDEN_II" CACHE STRING "Detector geometry implementation: CURIE, LEIDEN_II, or DSPX")
set_property(CACHE QARRAY_DETECTOR_GEOMETRY PROPERTY STRINGS CURIE LEIDEN_II DSPX)

if(QARRAY_DETECTOR_GEOMETRY STREQUAL "CURIE")
  list(APPEND sources ${PROJECT_SOURCE_DIR}/src/DetectorConstruction_CURIE.cc)
elseif(QARRAY_DETECTOR_GEOMETRY STREQUAL "LEIDEN_II")
  list(APPEND sources ${PROJECT_SOURCE_DIR}/src/DetectorConstruction.cc)
elseif(QARRAY_DETECTOR_GEOMETRY STREQUAL "DSPX")
  list(APPEND sources
    ${PROJECT_SOURCE_DIR}/src/DetectorConstruction_DSPX.cc
    ${PROJECT_SOURCE_DIR}/src/Geometry/CryostatBuilder.cc
    ${PROJECT_SOURCE_DIR}/src/Geometry/Shapes.cc
  )
else()
  message(FATAL_ERROR "Unsupported QARRAY_DETECTOR_GEOMETRY=${QARRAY_DETECTOR_GEOMETRY}; expected CURIE, LEIDEN_II, or DSPX")
endif()
```

The important technical choice is to make detector geometry a configure-time
selection, not a run-time switch between separately compiled
`DetectorConstruction` classes with the same symbols. `LEIDEN_II` should remain
the default until DSPX is implemented and validated.

## Design Constraints

- Keep the public integration point small.
- Keep `DetectorConstruction` in charge of application-level geometry choices.
- Avoid importing a second command framework.
- Avoid importing a second output framework.
- Avoid namespace aliases that hide ownership.
- Do not rename the whole cached `Geometry` module into QArray. Translate only
  the cryostat subset we actually need.
- Do keep small local geometry primitives when they reduce repeated Geant4
  boilerplate and do not pull in framework dependencies.

## Open Questions Before Coding

- Which cached cryostat configuration is the target: `RicochetFinalDesign`,
  `RicochetRUN013`, `RicochetRUN014`, `RicochetRUN015`, or `RicochetRUN016`?
- Does current chip/detector placement need access to the OVC vacuum logical
  volume or another internal cryostat logical volume?
- Should the new cryostat replace the current `ConstructFridge()` geometry in
  one step, or should it be gated behind a metadata option for comparison?
