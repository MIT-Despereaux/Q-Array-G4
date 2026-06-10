---
date: 2026-05-21
type: reference
tags: [geant4, tutorial, geometry, materials, kernel, scoring]
status: reference
---

# Geant4 Tutorial PDFs - Structured Technical Summary

Extracted from: Vienna Workshop on Simulations (VIEWS24), April 2024
Presenters: Pablo Cirrone (INFN-LNS), Luciano Pandola (INFN-LNS), Carlo Mancini Terracciano (UniRoma1)

---

## PDF 1: Cirrone - Geometry.pdf
### Topic: Materials & Geometry

---

### 1. Mandatory User Classes

At initialization (registered to run manager):
  - G4VUserDetectorConstruction  -- defines all geometry, materials, sensitive detectors
  - G4VUserPhysicsList           -- defines physics processes
  - G4VUserActionInitialization  -- registers all action classes

At execution (optional user hooks):
  - G4VUserPrimaryGeneratorAction  (mandatory at execution)
  - G4UserRunAction
  - G4UserEventAction
  - G4UserStackingAction
  - G4UserTrackingAction
  - G4UserSteppingAction

The detector class must implement:
  - virtual G4VPhysicalVolume* Construct()  [pure virtual, must return world phys vol]
  - virtual void ConstructSDandField()      [for sensitive detectors and fields]

CAVEAT: Do NOT delete the MyDetector instance yourself. The run manager does it automatically.

---

### 2. Units System

Base units (from CLHEP via G4SystemOfUnits.hh or CLHEP/SystemOfUnits.hh):
  - Length:  millimeter (mm)
  - Time:    nanosecond (ns)
  - Energy:  megaelectronvolt (MeV)
  - Charge:  eplus
  - Temperature: kelvin
  - All others derived from these

CONVENTION: Always multiply by a unit when assigning values:
  G4double width = 12.5 * m;
  G4double density = 2.7 * g/cm3;

Output convention -- divide by unit to get numeric value in that unit:
  G4cout << dE / MeV << " (MeV)" << G4endl;

Smart output: G4BestUnit selects the most appropriate unit automatically:
  G4cout << G4BestUnit(StepSize, "Length");

Custom units via G4UnitDefinition:
  G4UnitDefinition("grammpercm2", "g/cm2", "MassThickness", g/cm2);
  -- registers to G4UnitsTable, available via /units/list UI command

USE: Prefer G4double, G4int, G4bool, G4String, G4ThreeVector over primitive C++ types
     for cross-platform compatibility.

---

### 3. Materials

Hierarchy: G4Isotope --> G4Element --> G4Material

G4Isotope / G4Element: atomic-level properties (Z, N, molar mass, cross-sections per atom)
G4Material: macroscopic properties (density, temperature, pressure, radiation/absorption length)
  -- used by tracking, geometry, and physics

Material density is MANDATORY. Temperature, pressure, state are optional.

#### Single-element material
  G4Material* lAr = new G4Material("liquidAr", z=18, a=39.95*g/mole, density=1.39*g/cm3);

#### Molecule (by atom count)
  G4Material* H2O = new G4Material("Water", density=1.0*g/cm3, ncomponents=2);
  H2O->AddElement(elH, natoms=2);
  H2O->AddElement(elO, natoms=1);

#### Compound/mixture (by mass fraction)
  Air->AddElement(elN, 70.0*perCent);
  Air->AddElement(elO, 30.0*perCent);

#### Mix of materials
  aerogel->AddMaterial(SiO2, fractionmass=62.5*perCent);
  aerogel->AddMaterial(H2O,  fractionmass=37.4*perCent);
  aerogel->AddElement(elC,   fractionmass= 0.1*perCent);

#### Gas (specify T and P, affects dE/dx)
  G4Material* CO2 = new G4Material("CO2Gas", density, ncomponents=2,
                                   kStateGas, temperature, pressure);

#### Vacuum (cannot use rho=0)
  Model as very-low-density gas:
  G4Material* Vacuum = new G4Material("interGalactic", atomicNumber=1,
      massOfMole=1.008*g/mole, density=1.e-25*g/cm3,
      kStateGas, T=2.73*kelvin, P=3.e-18*pascal);

#### NIST database (recommended)
  G4NistManager* man = G4NistManager::Instance();
  G4Material* H2O = man->FindOrBuildMaterial("G4_WATER");
  G4Material* vac = man->FindOrBuildMaterial("G4_Galactic");
  -- covers H to Cf (Z=1..98), ~hundreds of compounds (e.g. "G4_ADIPOSE_TISSUE_IRCP")
  -- best accuracy for density, mean excitation potential, isotope composition
  -- can mix NIST and user-defined materials
  UI commands: /material/nist/printElement, /material/nist/listMaterials

---

### 4. Geometry Construction -- Three Conceptual Layers

Layer 1 -- G4VSolid:       shape and size
Layer 2 -- G4LogicalVolume: material, sensitivity, magnetic field, daughter hierarchy
Layer 3 -- G4VPhysicalVolume: position and rotation (same logical vol can be placed many times)

#### Step-by-step pattern
  // Step 1: create solid
  G4VSolid* pBoxSolid = new G4Box("aBoxSolid", 1.*m, 2.*m, 3.*m);

  // Step 2: create logical volume (assign material)
  G4LogicalVolume* pBoxLog = new G4LogicalVolume(pBoxSolid, pBoxMaterial, "aBoxLog");

  // Step 3: place physical volume in mother coordinate system
  G4VPhysicalVolume* aBoxPhys = new G4PVPlacement(
      pRotation, G4ThreeVector(posX, posY, posZ),
      pBoxLog, "aBoxPhys", pMotherLog, false, copyNo);

---

### 5. Solids

CSG solids (analogous to GEANT3):
  G4Box, G4Tubs, G4Cons, G4Trd, G4Torus, G4Trap, G4Para, G4Sphere, G4Orb

IMPORTANT -- G4Box uses HALF-lengths:
  G4Box(name, halfX, halfY, halfZ)

G4Tubs: G4Tubs(name, Rmin, Rmax, halfDz, startPhi, deltaPhi)
  -- Rmin=0 for solid cylinder; deltaPhi=twopi for full cylinder

Specific/advanced CSG:
  G4Polycone, G4Polyhedra, G4Hype, G4TwistedTubs, G4TwistedTrap

BREP solids: G4BREPSolidPolycone, G4BSplineSurface (any order surface)

Boolean solids (can nest):
  G4UnionSolid, G4SubtractionSolid, G4IntersectionSolid
  -- require 2 solids, 1 boolean op, optional transform for 2nd solid
  -- 2nd solid is positioned relative to the 1st solid's coordinate system
  -- result is itself a G4VSolid, can be re-used in further booleans
  CAVEAT: navigation cost for boolean solids is proportional to the number
          of constituent CSG solids (can be slow for complex geometries)

---

### 6. Logical Volumes

G4LogicalVolume(G4VSolid* pSolid, G4Material* pMaterial, const G4String& name,
                G4FieldManager* pFM=0, G4VSensitiveDetector* pSD=0,
                G4UserLimits* pUL=0, G4bool optimise=true);

  -- solid and material pointers must NOT be nullptr
  -- contains hierarchy of daughter volumes
  -- multiple physical volumes can share the same logical volume

---

### 7. Physical Volumes and Placement Rules

#### Placement (G4PVPlacement) -- single positioned instance
  Passive transformation (rotation of mother frame):
    G4PVPlacement(G4RotationMatrix* pRot, G4ThreeVector tlate,
                  G4LogicalVolume* pCurrent, G4String pName,
                  G4LogicalVolume* pMother, G4bool pMany,
                  G4int pCopyNo, G4bool pSurfChk=false);

  pMany: NOT USED -- always set to false
  pCopyNo: unique arbitrary integer identifying this copy
  pSurfChk: optional overlap check (CPU-intensive, worth enabling at least once)

  Active transformation (using G4Transform3D):
    G4PVPlacement(G4Transform3D(G4RotationMatrix& pRot, G4ThreeVector tlate),
                  G4LogicalVolume* pDaughter, ...);

Repeated volumes (memory-efficient):
  G4PVReplica       -- simple repetition along an axis
  G4PVParameterised -- complex pattern with parameterized geometry
  G4PVDivision      -- division of a volume

---

### 8. Geometry Hierarchy Rules (CRITICAL)

  - The World volume is the root physical volume; must uniquely contain all others
  - World volume defines the global coordinate system (origin at center)
  - All positions are relative to the LOCAL coordinate system of the mother volume
  - Origin of mother's local coord system is at the CENTER of the mother volume
  - DAUGHTERS CANNOT PROTRUDE from their mother volume
  - DAUGHTERS CANNOT OVERLAP with each other
  - If a logical volume is placed more than once, all its daughters appear in each copy
  - Track positions are given in the GLOBAL coordinate system

---

### 9. Geometry Overlap Checking

Geant4 does not auto-detect overlaps; behavior is unpredictable with malformed geometry.

Tools:
  1. pSurfChk=true in G4PVPlacement/G4PVParameterised constructors
     -- samples random points on surface; CPU-intensive but worthwhile
  2. UI commands at runtime:
     /geometry/test/run  or  /geometry/test/grid_test
       -- checks first depth level only
     /geometry/test/recursive_test
       -- all depth levels (very CPU-intensive)
  3. DAVID graphical tool

---

### 10. Regions

A G4Region is a sub-set of geometry with:
  - Its own production thresholds (range cuts)
  - User limits (max step length, max steps, min kinetic energy, ...)
  - Field manager
  - The world logical volume is the default region
  - CAVEAT: User cannot define a region for the world volume

---

## PDF 2: Pandola - Kernel Lecture 1.pdf
### Topic: Geant4 Kernel -- Runs, Events, Tracks, Steps, User Actions

---

### 1. Terminology Hierarchy

  Run > Events > Tracks > Steps > Processes

  Run:   collection of events with the SAME detector and physics conditions
         -- geometry optimized for navigation at start of run
         -- cross section tables (re)calculated at start of run
         -- managed by G4RunManager; user hook: G4UserRunAction

  Event: basic unit of simulation
         -- primary tracks generated and pushed to stack
         -- tracks popped one-by-one, tracked until stack is empty
         -- G4Event stores: primary vertices/particles (input), hits, trajectories (output)
         -- user hook: G4UserEventAction

  Track: snapshot of a particle at a moment in time (G4Track)
         -- stores current energy, momentum, position, polarization
         -- updated after every step
         -- deleted when: exits world vol, disappears in interaction,
                          slowed to zero KE with no AtRest process, manually killed
         -- NO track object persists at end of event
         -- user hook: G4UserTrackingAction

  Step:  "flight" between two subsequent interactions (G4Step)
         -- has pre-step and post-step points
         -- stores delta information (energy loss, etc.)
         -- updated each time a process is invoked
         -- CANNOT cross a boundary (geometry-limited steps are allowed)
         -- user hook: G4UserSteppingAction

---

### 2. Boundary Convention

  When a step is limited by a volume boundary:
    -- the end point PHYSICALLY STANDS ON the boundary
    -- it BELONGS TO THE NEXT VOLUME [official Geant4 convention]

  To detect boundary crossing:
    -- compare physical volume of pre and post step points (if different = boundary)
    -- or check step status: fGeomBoundary (does not apply to world volume)
    -- pre-step status = status of PREVIOUS step; post-step status = status of CURRENT step

  Code pattern:
    if (postStepPoint->GetStepStatus() == fGeomBoundary) { /* step ends on boundary */ }
    G4Material* nextMaterial = step->GetPostStepPoint()->GetMaterial();

---

### 3. Key G4Step Information

  step->GetTotalEnergyDeposit()    -- total edep (includes secondaries below cut)
  step->GetDeltaEnergy()           -- energy difference pre to post
  step->GetDeltaPosition()         -- displacement vector
  step->GetStepLength()
  step->GetTrack()                 -- pointer to G4Track
  track->GetDynamicParticle()      -- G4DynamicParticle
  dynParticle->GetDefinition()     -- G4ParticleDefinition (static info, name, etc.)
  dynParticle->GetKineticEnergy()  -- KE after step
  track->GetTrackID()              -- 1 = primary
  track->GetParentID()             -- 0 for primary, 1 for secondary of primary, etc.
  track->GetCreatorProcess()->GetProcessName()

---

### 4. User Action Registration (MT-aware)

  All user actions registered through G4VUserActionInitialization:
    void Build() const {                     // called per worker thread
        SetUserAction(new MyPrimaryGeneratorAction());
        SetUserAction(new MyEventAction());
        SetUserAction(new MyRunAction());
    }
    void BuildForMaster() const {            // called for master thread only
        SetUserAction(new MyMasterRunAction());
    }

  In MT mode: geometry and physics are READONLY (shared); action objects are thread-LOCAL.
  Two RunActions allowed: one for master, one per worker thread.
  G4UserRunAction: BeginOfRunAction(), EndOfRunAction(), GenerateRun()
  G4UserEventAction: BeginOfEventAction(), EndOfEventAction()
  G4UserStackingAction: ClassifyNewTrack(), NewStage(), PrepareNewEvent()
  G4UserTrackingAction: PreUserTrackingAction(), PostUserTrackingAction()
  G4UserSteppingAction: UserSteppingAction()

---

### 5. MT Mode Notes

  Worker threads independently process subsets of events.
  G4Run::Merge() called at end of each worker run by master to consolidate results.
  G4Accumulable<T>: thread-safe accumulation of scalar quantities:
    -- Register to G4AccumulableManager
    -- Reset at BeginOfRun, increment during run, Merge at EndOfRun
    -- Use IsMaster() guard before reporting

  Multiple actions of same type: G4MultiRunAction, G4MultiEventAction, etc.
  (No G4MultiStackingAction exists)

---

### 6. Analysis Tools (g4analysis)

  G4AnalysisManager singleton (thread-safe, unlike ROOT directly):
    -- Supports: ROOT, AIDA XML, CSV, HBOOK
    -- Switch format by changing one typedef/using line only

  Usage pattern:
    BeginOfRunAction: book histos/ntuples, open file
    EndOfEventAction: fill histos/ntuples
    EndOfRunAction: Write()
    main(): CloseFile()

  G4THitsMap<G4double>: container for primitive scorer data per event

---

### 7. Command-Based Scoring

  Requires only: G4ScoringManager::GetScoringManager() in main()
  No C++ needed for scoring -- all via UI commands:
    /score/create/boxMesh <name>
    /score/mesh/boxsize <dx> <dy> <dz>
    /score/mesh/nbin <nx> <ny> <nz>
    /score/quantity/eDep <scorer_name>
    /score/quantity/cellFlux <scorer_name>
    /score/filter/particle <filter> <particle_list>
    /score/filter/kinE <filter> <Emin> <Emax> <unit>
    /score/dump, /score/draw, /score/list

---

## PDF 3: kernel2.pdf
### Topic: Sensitive Detectors, Native Scorers, Hit Collection Retrieval

---

### 1. Sensitive Detectors

  A logical volume becomes sensitive by attaching a G4VSensitiveDetector pointer.
  Registration MUST be done in ConstructSDandField() of the detector construction class.

  Rules:
    -- SD objects must have UNIQUE detector names
    -- A logical volume can only have ONE SD object attached
    -- A single SD can be assigned to multiple logical volumes (as separate instances)
    -- Do NOT share a primitive scorer object between multiple G4MultiFunctionalDetector instances
       (results may mix up!)

  Pattern:
    G4VSensitiveDetector* mySensitive = new MySensitiveDetector("MyDetector");
    G4SDManager::GetSDMpointer()->AddNewDetector(mySensitive);
    logicVol->SetSensitiveDetector(mySensitive);
    // or: SetSensitiveDetector("LVname", mySensitive);

---

### 2. Native Scorers (G4MultiFunctionalDetector)

  G4MultiFunctionalDetector is a concrete G4VSensitiveDetector subclass.
  Takes an arbitrary number of G4VPrimitiveScorer instances.
  Each G4VPrimitiveScorer accumulates ONE physics quantity per physical volume per event.

  When to use native scorers vs. custom SD:
    -- Use native scorers when you want accumulated quantities per event/run (not per step)
    -- Use custom SD for full step-by-step control

  Available primitive scorers:
    Track length:    G4PSTrackLength, G4PSPassageTrackLength
    Energy deposit:  G4PSEnergyDeposit, G4PSDoseDeposit
    Current/Flux:    G4PSFlatSurfaceCurrent, G4PSSphereSurfaceCurrent,
                     G4PSPassageCurrent, G4PSFlatSurfaceFlux,
                     G4PSCellFlux, G4PSPassageCellFlux
    Others:          G4PSMinKinEAtGeneration, G4PSNofSecondary,
                     G4PSNofStep, G4PSCellCharge

  Hit collection name auto-generated as: <MFDname>/<ScorerName>
  operator+= provided on hit collections for automatic aggregation (no manual looping needed)

---

### 3. Scorer Filters (G4VSDFilter)

  Filter which tracks contribute to a scorer:
    G4SDChargeFilter        -- charged particles only
    G4SDNeutralFilter       -- neutral particles only
    G4SDKineticEnergyFilter -- particles in a KE range
    G4SDParticleFilter      -- specific particle type(s)
    G4VSDFilter             -- base class for custom filters

  Usage:
    G4VSDFilter* protonFilter = new G4SDParticleFilter("protonFilter");
    protonFilter->Add("proton");
    protonSurfFlux->SetFilter(protonFilter);

---

### 4. Retrieving Scorer Data

  Hit collections keyed by (copy number, scored quantity):
    -- copy number = logical volume copy number (map key)
    -- quantity = accumulated value (map value)

  Retrieval pattern (e.g. in EndOfEventAction):
    // Step 1: get collection ID by name (do once, cache the ID)
    G4int collID = G4SDManager::GetSDMpointer()
                     ->GetCollectionID("myCellScorer/TotalSurfFlux");

    // Step 2: get all hit collections for this event
    G4HCofThisEvent* HCE = event->GetHCofThisEvent();

    // Step 3: get specific collection (cast required)
    G4THitsMap<G4double>* evtMap =
        static_cast<G4THitsMap<G4double>*>(HCE->GetHC(collID));

    // Step 4: iterate
    for (auto pair : *(evtMap->GetMap())) {
        G4double value  = *(pair.second);
        G4int copyNb    = pair.first;   // copy number
    }

  Hit collections can be accessed in EventAction or RunAction.

---

## PDF 4: Cirrone - ApplicationBuild.pdf
### Topic: Building a Geant4 Application

---

### 1. Recommended Directory Structure

  MyApp/
    CMakeLists.txt      -- build script
    MyApp.cc            -- main() function
    include/            -- header files (.hh)
    src/                -- source files (.cc)
    *.mac               -- Geant4 macro files

  Note: structure is recommended but NOT enforced by Geant4.

---

### 2. CMake Build System

  CMake is the standard (GNU make is possible but discouraged).
  CMakeLists.txt must:
    1. Set cmake minimum version and project name
    2. Find Geant4 package (optionally with UI and visualization):
         find_package(Geant4 REQUIRED ui_all vis_all)
         include(${Geant4_USE_FILE})
    3. Include header directories
    4. Glob source and header files
    5. Define executable and link Geant4 libraries:
         add_executable(exampleB1 exampleB1.cc ${sources} ${headers})
         target_link_libraries(exampleB1 ${Geant4_LIBRARIES})
    6. Copy macro files to build directory (configure_file with COPYONLY)

---

### 3. Build Workflow

  Step 1 -- Copy example to home (if modifying official examples):
    cp -r /usr/local/geant4/.../examples/basic/B1 ~

  Step 2 -- Create a SEPARATE build directory (out-of-source build, strongly recommended):
    mkdir -p ~/B1-build && cd ~/B1-build

  Step 3 -- Run CMake pointing to Geant4 install and source dir:
    cmake -DGeant4_DIR=/path/to/geant4/lib/Geant4-X.Y.Z/ ~/B1/

  Step 4 -- Compile:
    make
    -- executable created in build dir
    -- macro files copied to build dir

  Step 5 -- Run:
    ./exampleB1             -- launches GUI (Qt, GAG, tcsh, csh depending on config)
    ./exampleB1 run.mac     -- batch mode with macro

  CAVEAT: Never compile inside the source directory (possible but not recommended).
  CAVEAT: Do not leave out the DGeant4_DIR if Geant4 is not in the default system paths.

---

## Cross-Cutting Notes and Caveats

1. UNITS: Never use bare numbers. Always multiply by a Geant4 unit. Failure to do so
   causes silent wrong results (no runtime error).

2. VACUUM: rho=0 is illegal. Always model vacuum as a very-low-density gas.

3. WORLD VOLUME: Must be returned from Construct(). Must fully contain all other volumes.
   Its center is the origin of the global coordinate system.

4. OVERLAPS: Geant4 does NOT automatically catch overlapping volumes. Navigation becomes
   undefined. Always run overlap checks (pSurfChk=true or /geometry/test commands) at
   least once.

5. HALF-LENGTHS: G4Box (and G4Tubs Dz, G4Cons Dz) take HALF-dimensions. A common source
   of geometry bugs.

6. ROTATIONS: G4PVPlacement passive constructor rotates the MOTHER FRAME, not the
   daughter. Active constructor (via G4Transform3D) rotates the DAUGHTER. Know which
   convention you are using.

7. pMany flag in G4PVPlacement: set to false -- it is not used and has no effect.

8. MT THREADING: Geometry and physics lists are shared read-only between threads.
   User actions are thread-local. If you use ROOT or file I/O directly, you are
   responsible for thread safety. Use G4AnalysisManager or G4Accumulable instead.

9. SD MUST BE CREATED IN ConstructSDandField(), not in Construct().

10. BOOLEAN SOLIDS: Navigation cost scales with the number of constituent CSG solids.
    Avoid deeply nested boolean solids in performance-critical geometries.

11. REGIONS: Cannot assign a region to the world logical volume.

12. TRACK LIFETIME: No G4Track persists after the end of an event. Extract info
    in UserSteppingAction or hit collections during the event.
