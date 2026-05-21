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
  struct HexBride;      // new: hexagonal prism lid for DSPX
}
```

For the cryostat merge, start with only the shapes needed by the cryostat:

- `FilledTube`
- `Bucket`
- `ReversedBucket`  (kept for reference but NOT used for DSPX bride -- see below)
- `HexBride`        (new, replaces ReversedBucket for DSPX)

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

### HexBride shape (DSPX-specific)

The DSPX bride is a hexagonal prism lid, not a cylinder. The cached
`ReversedBucket` (G4Polycone) cannot represent a hexagonal outer wall.

Use `G4Polyhedra` directly in `CryostatBuilder.cc` or wrap it in a
`HexBride` struct in `Shapes.hh`:

```cpp
namespace QArray::Geometry
{
  // Inverted hexagonal cup (lid). Origin = open mouth center (bottom).
  // Wall section: ri=innerRadius, ro=circumRadius, height=Bride_h
  // Lid disk:     ri=0, ro=circumRadius, thickness=Bride_e
  // Total height: Bride_h + Bride_e
  //
  // G4Polyhedra parameters:
  //   phiStart = 0, phiTotal = 360*deg, numSide = 6
  //   z-planes (4): [0, Bride_h, Bride_h, Bride_h + Bride_e]
  //   rInner:       [innerRadius, innerRadius, 0, 0]
  //   rOuter:       [circumRadius, circumRadius, circumRadius, circumRadius]
  struct HexBride : G4Polyhedra
  {
    HexBride(const G4String& name,
             double innerRadius,   // bore radius = 187 mm for DSPX
             double circumRadius,  // hex vertex-to-center = 240.2 mm for DSPX
             double Bride_h,       // inner cavity height = 97 mm
             double Bride_e);      // top disk thickness  = 14 mm
  };
}
```

DSPX confirmed values:
  innerRadius   = 187 mm
  flat-to-flat  = 416 mm  ->  inradius = 208 mm
  circumRadius  = 208 / cos(30 deg) = 240.2 mm
  Bride_h       = 97 mm
  Bride_e       = 14 mm   (= 111 mm total - 97 mm inner height)
  Total height  = 111 mm

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

Implement DSPX as a configure-time geometry variant, not as a direct mutation of
the current Leiden-II geometry. Keep every phase small enough that the project
can configure and build at the end of the phase.

High-level order:

1. Establish the geometry-selection build seam.
2. Add local geometry primitives and material helpers.
3. Build the DSPX cryostat geometry as a standalone builder.
4. Integrate the builder through `DetectorConstruction_DSPX.cc`.
5. Validate with a short batch macro and document the validated geometry.

## Phased Implementation Plan

### Phase 0 -- Source Audit And Locked Geometry Inputs

Description: Confirm that `.agents/diagram_cryostat_dspx.txt` is the single
source of truth for DSPX plate, screen, OVC, and bride values, and map any cache
code that will be translated rather than imported.

Tasks:

- [ ] Review `cache/Simulation-develop/Geometry` only for construction patterns and
  shape semantics.
- [ ] Record any discrepancy between cached values and
  `.agents/diagram_cryostat_dspx.txt` before coding.
- [ ] Keep the resolved DSPX dimensions in the plan and diagram synchronized.

Acceptance criteria:

- [ ] No ROOT, SimuOutput, messenger, or cache framework dependency is required for
  the planned implementation.
- [ ] All DSPX numeric constants used by later phases are present in
  `.agents/diagram_cryostat_dspx.txt`.
- [ ] Any remaining unknown is explicitly listed in this plan before implementation
  starts.

Verification:

- [ ] Read-only review of `Cryostat.cc`, `GenericCryostatBuilder.cc`, and
  `.agents/diagram_cryostat_dspx.txt`.
- [ ] No code changes required in this phase.

Dependencies: None.

Estimated scope: Small.

### Phase 1 -- Configure-Time Geometry Selection

Description: Add `QARRAY_DETECTOR_GEOMETRY` to CMake so `CURIE`, `LEIDEN_II`,
and `DSPX` are selected by source list. Keep `LEIDEN_II` as the default.

Tasks:

- [ ] Replace the current ad hoc exclusion of `DetectorConstruction_CURIE.cc` with
  explicit source selection.
- [ ] Add a placeholder `DetectorConstruction_DSPX.cc` only if needed to prove
  source selection before the full builder exists.
- [ ] Keep shared files and existing macros available to all geometry variants.

Acceptance criteria:

- [ ] `cmake -S . -B build` configures with default `LEIDEN_II`.
- [ ] `cmake -S . -B build-dspx -DQARRAY_DETECTOR_GEOMETRY=DSPX` selects the DSPX
  source path without compiling duplicate `DetectorConstruction` symbols.
- [ ] `CURIE` selection remains expressible through the same cache variable.

Verification:

- [ ] `cmake -S . -B build`
- [ ] `cmake --build build`
- [ ] `cmake -S . -B build-dspx -DQARRAY_DETECTOR_GEOMETRY=DSPX`

Dependencies: Phase 0.

Estimated scope: Medium.

### Phase 2 -- Local Geometry Primitives

Description: Add the small `QArray::Geometry` helper layer needed by the DSPX
builder, without importing the cached geometry framework.

Tasks:

- [ ] Add `FilledTube`, `Bucket`, `ReversedBucket`, and `HexBride` in local
  `include/Geometry/Shapes.hh` and `src/Geometry/Shapes.cc`.
- [ ] Add a compact placement helper only if it materially reduces repeated Geant4
  boilerplate.
- [ ] Keep all helper types in `QArray::Geometry`.

Acceptance criteria:

- [ ] Shape helpers compile independently of ROOT, SimuOutput, cached utility
  classes, and cached messenger classes.
- [ ] `HexBride` uses `G4Polyhedra` with the DSPX hexagonal outer profile.
- [ ] `Bucket` represents a hollow cylindrical wall plus solid bottom disk with the
  origin semantics used by the DSPX diagram.

Verification:

- [ ] `cmake --build build-dspx`
- [ ] Inspect generated solids in code for matching z-plane/origin semantics before
  integrating into `CryostatBuilder`.

Dependencies: Phase 1.

Estimated scope: Medium.

### Phase 3 -- DSPX Cryostat Builder

Description: Implement `QArray::Geometry::CryostatBuilder` as the single owner
of DSPX cryostat internals. It should place plates, screens, OVC, and bride into
a provided mother logical volume and return the logical volumes needed by
detector placement.

Tasks:

- [ ] Add `include/Geometry/CryostatBuilder.hh` and
  `src/Geometry/CryostatBuilder.cc`.
- [ ] Translate the plate stack, screen cascade, OVC, and hex bride from the locked
  diagram values.
- [ ] Populate `CryostatVolumes` with at least `ovcLogical`, `ovcVacuumLogical`,
  and `mixingChamberLogical`.
- [ ] Keep materials local and minimal, using existing NIST material patterns where
  practical.

Acceptance criteria:

- [ ] The builder API is small and does not expose cache framework types.
- [ ] Plate and screen positions match `.agents/diagram_cryostat_dspx.txt`.
- [ ] The OVC top rim aligns with the bride open mouth.
- [ ] `checkOverlaps` is configurable and passed to placements.

Verification:

- [ ] `cmake --build build-dspx`
- [ ] Run a short DSPX macro once `DetectorConstruction_DSPX.cc` delegates to the
  builder in Phase 4.

Dependencies: Phase 2.

Estimated scope: Medium.

### Phase 4 -- DetectorConstruction DSPX Integration

Description: Add the DSPX detector construction variant that owns the world,
lab/table/fridge placement, global offsets, and detector-array placement while
delegating cryostat internals to `CryostatBuilder`.

Tasks:

- [ ] Add or complete `src/DetectorConstruction_DSPX.cc`.
- [ ] Preserve existing application-level placement behavior unless the DSPX geometry
  explicitly requires a documented change.
- [ ] Use `CryostatVolumes` for detector/chip placement inside the OVC vacuum or the
  appropriate 10mK reference region.

Acceptance criteria:

- [ ] `LEIDEN_II` still builds and runs unchanged as the default.
- [ ] `DSPX` builds without duplicate `DetectorConstruction` definitions.
- [ ] Detector placement is expressed relative to returned cryostat reference
  volumes or documented reference positions.

Verification:

- [ ] `cmake --build build`
- [ ] `ctest --test-dir build --output-on-failure`
- [ ] `cmake --build build-dspx`

Dependencies: Phase 3.

Estimated scope: Medium.

### Phase 5 -- Validation And Documentation Checkpoint

Description: Validate the DSPX geometry with a smoke macro and update the
handoff docs with what was actually built.

Tasks:

- [ ] Run overlap checks for the DSPX geometry.
- [ ] Run `geometry_lab_proton.mac` to visualize lab/fridge geometry with one
  10 MeV proton from the world top center.
- [ ] Run `geometry_lab_proton_batch.mac` to test the same one-proton setup
  without opening a visualization window.
- [ ] Inspect any generated CSV/JSON outputs relevant to geometry metadata.
- [ ] Update `.agents/diagram_cryostat_dspx.txt` if implementation exposes a
  mismatch in the drawing.
- [ ] Update `CHANGELOG.md` if this lands with a version bump.

Acceptance criteria:

- [ ] Default `LEIDEN_II` smoke tests still pass.
- [ ] DSPX batch run completes without fatal Geant4 overlap errors.
- [ ] The plan, diagram, and implementation agree on plate/screen/OVC/bride
  dimensions.

Verification:

- [ ] `cmake -S . -B build`
- [ ] `cmake --build build`
- [ ] `ctest --test-dir build --output-on-failure`
- [ ] `cmake -S . -B build-dspx -DQARRAY_DETECTOR_GEOMETRY=DSPX`
- [ ] `cmake --build build-dspx`
- [ ] `cd build-dspx && ./main run_test.mac`
- [ ] `cd build-dspx && ./main geometry_lab_proton.mac`
- [ ] `cd build-dspx && ./main geometry_lab_proton_batch.mac`

Dependencies: Phase 4.

Estimated scope: Small.

### Parallelization Notes

Safe to parallelize after Phase 1:

- [ ] Shape-helper implementation and focused review of DSPX numeric constants.
- [ ] Documentation updates and CMake source-selection cleanup.

Must remain sequential:

- [ ] `CryostatBuilder` should wait for shape-helper origin semantics to settle.
- [ ] `DetectorConstruction_DSPX.cc` should wait for the builder API to stabilize.
- [ ] Final validation should run after all DSPX source selection and placement work
  is integrated.

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
  **(resolved -- see Resolved Decisions below)**
- Does current chip/detector placement need access to the OVC vacuum logical
  volume or another internal cryostat logical volume?
  **(resolved -- see Resolved Decisions below)**
- Should the new cryostat replace the current `ConstructFridge()` geometry in
  one step, or should it be gated behind a metadata option for comparison?
  **(resolved -- see Resolved Decisions below)**

## Resolved Decisions

**Q1 -- Target configuration: CryoConcept2020 plate dimensions + Bucket screen construction**

DSPX screens have no bolt flanges, so the `BuildScreen()` flanged-cylinder path
(RicochetFinalDesign) does not match the hardware. Use `Bucket` G4Polycone
shapes for all screens and the OVC instead.

Plate dimensions: `CryoConcept2020` (updated inter-plate gaps from CROSS baseline).
Screen construction: `Bucket(innerRadius, thickness, height)` for Screen1K,
Screen4K, Screen50K, and OVC. No Screen10mK. No floating plate.
Bride: `HexBride` (G4Polyhedra, hexagonal outer, NOT ReversedBucket -- see HexBride
section below).
Position cascade: CryoConcept/CROSS-style gap chain (not the single absolute
`DisPlate10mK_InnerBottomOVC` anchor used by RicochetFinalDesign).

Screen heights are fully DERIVED from the gap cascade -- not free parameters.
Only two genuinely unknown DSPX values remain:
  - `DisPlate10mK_BottomScreen1K`: how far below Plate10mK bottom the Screen1K
    inner bottom center sits (replaces the Screen10mK cascade since there is none).
  - `OVC_h`: total height of the OVC Bucket wall.
All inter-screen gap distances default to CROSS values (1.95 cm, 1.98 cm, 2.0 cm)
until DSPX drawings confirm otherwise.

See `.agents/diagram_cryostat_bucket.txt` for the full annotated diagram.

**Q2 -- Downstream logical volume access: required**

`DetectorConstruction` needs logical volume pointers back from `CryostatBuilder`.
`CryostatVolumes` must be populated. At minimum populate:
- `ovcLogical` -- OVC shell logical volume
- `ovcVacuumLogical` -- vacuum interior of the OVC (detector array lives here)
- `mixingChamberLogical` -- region around the 10mK plate

Add `plate10mKCenter` as a `G4ThreeVector` convenience field if chip placement
is expressed relative to the 10mK plate origin rather than the cryostat Build()
position.

**Q3 -- Replacement strategy: CMake configure-time selection**

Gate the new builder behind `QARRAY_DETECTOR_GEOMETRY=DSPX`. `LEIDEN_II` remains
the default and is untouched during development. Do not use run-time switching
between separately compiled `DetectorConstruction` classes with the same symbols.
