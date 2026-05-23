---
date: 2026-05-21
type: reference
tags: [geant4, tutorial, geometry, materials, kernel, scoring]
---

# Geant4 Tutorial Notes

Extracted from: Vienna Workshop on Simulations (VIEWS24), April 2024
Presenters: Pablo Cirrone (INFN-LNS), Luciano Pandola (INFN-LNS), Carlo Mancini Terracciano (UniRoma1)

---

## PDF 1: Cirrone - Geometry.pdf

### 1. Mandatory User Classes

At initialization (registered to run manager):
- G4VUserDetectorConstruction — defines all geometry, materials, sensitive detectors
- G4VUserPhysicsList — defines physics processes
- G4VUserActionInitialization — registers all action classes

At execution (optional user hooks):
- G4VUserPrimaryGeneratorAction (mandatory at execution)
- G4UserRunAction, G4UserEventAction, G4UserStackingAction
- G4UserTrackingAction, G4UserSteppingAction

**CAVEAT:** Do NOT delete the Detector instance yourself. The run manager does it automatically.

### 2. Units System

Base units (from CLHEP):
- Length: millimeter (mm)
- Time: nanosecond (ns)
- Energy: megaelectronvolt (MeV)

**CONVENTION:** Always multiply by a unit when assigning values:
```cpp
G4double width = 12.5 * m;
G4double density = 2.7 * g/cm3;
G4cout << dE / MeV << " (MeV)" << G4endl;
G4cout << G4BestUnit(StepSize, "Length");
```

### 3. Materials

Hierarchy: G4Isotope → G4Element → G4Material

NIST database (recommended):
```cpp
G4NistManager* man = G4NistManager::Instance();
G4Material* H2O = man->FindOrBuildMaterial("G4_WATER");
G4Material* vac = man->FindOrBuildMaterial("G4_Galactic");
```

### 4. Geometry Construction — Three Layers

1. **G4VSolid:** shape and size
2. **G4LogicalVolume:** material, sensitivity, magnetic field, daughter hierarchy
3. **G4VPhysicalVolume:** position and rotation (same logical vol can be placed many times)

**IMPORTANT:** G4Box uses HALF-lengths: `G4Box(name, halfX, halfY, halfZ)`

### 5. Geometry Hierarchy Rules

- World volume is root; defines global coordinate system (origin at center)
- All positions relative to mother's local coordinate system
- Daughters CANNOT protrude from mother volume
- Daughters CANNOT overlap with each other
- Track positions given in global coordinate system

### 6. Overlap Checking

- `pSurfChk=true` in G4PVPlacement constructor
- UI commands: `/geometry/test/run`, `/geometry/test/recursive_test`

---

## PDF 2: Pandola - Kernel Lecture 1.pdf

### Terminology: Run → Events → Tracks → Steps → Processes

- **Run:** collection of events with same detector and physics conditions
- **Event:** basic simulation unit; primary tracks pushed to stack
- **Track:** snapshot of particle at a moment in time (G4Track)
- **Step:** "flight" between two subsequent interactions; CANNOT cross a boundary

### Boundary Convention

When a step is limited by volume boundary, the end point physically stands on boundary and **belongs to the next volume** (official Geant4 convention).

### Key G4Step Info

```cpp
step->GetTotalEnergyDeposit()    // total edep
step->GetDeltaEnergy()           // energy difference pre to post
step->GetStepLength()
step->GetTrack()->GetTrackID()   // 1 = primary
```

### MT Mode

Worker threads independently process subsets of events. G4Run::Merge() called at end of each worker run. Use G4Accumulable<T> for thread-safe accumulation.

---

## PDF 3: kernel2.pdf — Sensitive Detectors & Scorers

### Sensitive Detectors

- SD objects must have UNIQUE names
- A logical volume can only have ONE SD attached
- Same SD can be assigned to multiple logical volumes

### Native Scorers (G4MultiFunctionalDetector)

Available primitive scorers:
- Track length: G4PSTrackLength, G4PSPassageTrackLength
- Energy deposit: G4PSEnergyDeposit, G4PSDoseDeposit
- Current/Flux: G4PSFlatSurfaceCurrent, G4PSCellFlux

### Scorer Filters

- G4SDParticleFilter — specific particle type(s)
- G4SDKineticEnergyFilter — particles in a KE range
- G4SDChargeFilter — charged particles only

---

## PDF 4: Cirrone - ApplicationBuild.pdf

### Recommended Directory Structure

```
MyApp/
  CMakeLists.txt      -- build script
  MyApp.cc            -- main() function
  include/            -- header files (.hh)
  src/                -- source files (.cc)
  *.mac               -- Geant4 macro files
```

### CMake Build System

```cmake
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})
add_executable(exampleB1 ${sources} ${headers})
target_link_libraries(exampleB1 ${Geant4_LIBRARIES})
```