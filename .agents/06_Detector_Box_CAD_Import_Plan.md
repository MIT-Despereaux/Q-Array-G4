---
date: 2026-05-22
type: plan
tags: [q-array-g4, dspx, detector-box, cad, mesh, plan]
---

# Detector Box CAD Import Plan

> For Hermes: use this as the implementation reference for the detector-box assembly import in DSPX/q-array-g4.

**Goal:** Define the detector-box assembly as a mixed CAD + CSG geometry with a stable detector origin and explicit per-part material ownership.

**Architecture:** Use the baseplate STL as the anchor mesh for the assembly. Build the smaller copper and sapphire pieces plus the Sn detector volume as CSG solids placed relative to the detector origin. Import the box cover as its own STL mesh so materials remain one-part-per-volume and easy to assign in Geant4.

**Tech Stack:** Geant4 C++17, CADMesh STL import, G4LogicalVolume/G4PVPlacement, CSG solids for simple internal components.

---

## Detector origin convention

- Detector origin = bottom-side center of the baseplate assembly
- Coordinate convention: STL exported with correct Geant4 Z orientation
- All internal placements should be recorded relative to this detector origin
- Recommended implementation: create a dedicated detector assembly mother LV and place all child parts into it

## Draft implementation parts table

| Part ID | Part name                       | Geometry source       | Material                         | Proposed Geant4 solid/LV strategy                                                       | Parent LV                                                          | Origin / anchor convention                                                               | Placement notes                                                                                                     | Status |
| ------- | ------------------------------- | --------------------- | -------------------------------- | --------------------------------------------------------------------------------------- | ------------------------------------------------------------------ | ---------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- | ------ |
| DB-001  | Baseplate                       | FreeCAD export -> STL | G4_Cu                            | Import as CADMesh tessellated solid; one LV for baseplate                               | Detector assembly mother LV                                        | Mesh origin at detector origin = bottom-side center of baseplate                         | Primary anchor part for the whole detector-box assembly; all other offsets measured from this frame                 | Draft  |
| DB-002  | Small copper pieces             | CSG in code           | G4_Cu                            | Build as simple CSG solids (`G4Box`, `G4Tubs`, or booleans only if necessary)           | Detector assembly mother LV or baseplate-relative placement helper | Place relative to detector origin, using measured offsets from baseplate reference frame | Prefer one LV per distinct part unless exact repeated copies justify reuse                                          | Draft  |
| DB-003  | Small sapphire pieces           | CSG in code           | Sapphire / Al2O3 custom material | Build as simple CSG solids; define or reuse a sapphire material once in materials setup | Detector assembly mother LV or baseplate-relative placement helper | Place relative to detector origin, using measured offsets from baseplate reference frame | Keep each physically separate sapphire element as its own LV for clear material ownership and debugging             | Draft  |
| DB-004  | Detector volume                 | CSG in code           | Tin (Sn) custom material         | Single `G4Box` solid for detector cube                                                  | Detector assembly mother LV                                        | Cube center placed relative to detector origin                                           | Sits above baseplate and centered laterally; final Z offset should be measured from baseplate top/interface surface | Draft  |
| DB-005  | Box cover (wall + cap combined) | FreeCAD export -> STL | G4_Cu                            | Import as CADMesh tessellated solid; one LV for combined cover mesh                     | Detector assembly mother LV                                        | Mesh origin should be defined and documented relative to detector origin before export   | Treat as a separate enclosing part; verify clearances against detector cube and internal CSG pieces                 | Draft  |

## Open data to capture before implementation

1. Exact file names for exported STL assets
   - baseplate STL
   - box cover STL
2. Final material mapping
   - exact baseplate material
   - exact box cover material
   - sapphire material definition parameters
   - tin material definition parameters
3. Relative placements from detector origin
   - baseplate top surface Z
   - each small copper piece center or anchor transform
   - each sapphire piece center or anchor transform
   - detector cube center transform
   - cover transform
4. Whether the detector assembly should get its own mother LV for easier reuse and offsetting inside DSPX

## Recommended implementation order

1. Create detector assembly mother LV and document the detector-origin frame.
2. Import and place the baseplate STL at the detector origin.
3. Add the Sn detector cube as CSG and verify offsets.
4. Add copper and sapphire CSG sub-parts one by one.
5. Import and place the combined box-cover STL.
6. Run overlap checks and inspect in visualization.

## Notes

- Keep the one-part-per-material-region rule.
- Prefer CSG for simple internal parts to keep navigation cheaper than full mesh import.
- Avoid combining distinct materials into a single imported mesh.
- If FreeCAD exports ASCII STL as `.ast`, rename to `.stl` before use with CADMesh.
