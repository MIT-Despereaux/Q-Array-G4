---
date: 2026-05-22
type: reference
tags: [cadmesh, stl, freecad, dspx, geometry, workflow]
status: active
branch: feature/dspx-cryostat-geometry
---

# STL Mesh Placement Workflow (CADMesh + Geant4)

How to load FreeCAD-exported STL files into the DSPX cryostat geometry via
CADMesh. Covers origin control, extent inspection, placement math, and
iterative verification.

---

## The Core Problem

An STL file is a bare list of triangles with XYZ coordinates and no metadata.
When CADMesh loads a file, Geant4 treats those coordinates as-is in mm. The
origin `(0,0,0)` of the resulting `G4TessellatedSolid` is **exactly wherever
`(0,0,0)` was in FreeCAD at export time** — no axis convention, no mounting
metadata, no hierarchy information is preserved.

The full workflow has three stages:

1. Control the origin in FreeCAD **before** export.
2. Inspect the bounding box in Geant4 **after** loading.
3. Verify placement iteratively with visualization and overlap checks.

---

## Stage 1: Control the Origin in FreeCAD

Deliberately position each part so a **physically meaningful point** is at FreeCAD's `(0,0,0)`.

### Recommended origin convention for DSPX

```
All STL parts exported with Z axis = cryostat vertical (up = +Z).
Origin = center of the bottom face of the part.
Units  = mm (FreeCAD default mesh export).
```

Matches the `Bucket` / `FilledTube` origin semantics used in `CryostatBuilder.cc`.

### How to set the origin in FreeCAD

In the Part or Part Design workbench, select the body, then in the Model tab Properties panel:

```
Placement > Position > x = 0   y = 0   z = 0
Placement > Angle    = 0        Axis  = (0, 0, 1)
```

### Recommended FreeCAD export settings

| Setting | Recommended value | Why |
|---|---|---|
| Surface deviation | <= 0.1 mm | Adequate for curved cryostat surfaces |
| Angular deviation | <= 0.5 deg | Avoids obvious faceting on cylinders |
| Format | ASCII or Binary `.stl` | Must be `.stl` — FreeCAD ASCII export saves as `.ast`, rename before committing |

---

## Stage 2: Inspect Mesh Extents in Geant4

After loading, print the bounding box to verify scale and axis alignment.

```cpp
#include "CADMesh.hh"
#include "G4TessellatedSolid.hh"
#include "G4VisExtent.hh"

auto mesh = CADMesh::TessellatedMesh::Read("bracket_top.stl");
mesh->SetScale(mm);
auto* solid = mesh->GetSolid();

G4VisExtent ext = solid->GetExtent();
G4cout << "[CADMesh] bracket_top.stl extent (mm):"
       << "\n  X: [" << ext.GetXmin()/mm << ", " << ext.GetXmax()/mm << "]"
       << "\n  Y: [" << ext.GetYmin()/mm << ", " << ext.GetYmax()/mm << "]"
       << "\n  Z: [" << ext.GetZmin()/mm << ", " << ext.GetZmax()/mm << "]"
       << G4endl;
```

### What to check

| Check | Expected result |
|---|---|
| Z range ~ part height from drawings | Confirms mm scale is correct |
| Z min ~ 0 | Confirms bottom-face-at-origin convention |
| X/Y symmetric around 0 | Confirms part is centered on Z axis |
| All values finite and non-zero | Confirms file was read successfully |

---

## Stage 3: Placement Math

The position passed to `G4PVPlacement` is the **offset of the mesh origin
relative to the parent logical volume's own origin**.

### General formula

```
pos_in_parent = target_position_in_cryostat_frame
              - parent_origin_in_cryostat_frame
```

### DSPX parent volume origins (cryostat frame)

| Parent logical volume | Local z=0 in cryostat frame |
|---|---|
| `ovcVacuumLogical` | +143.55 mm (G4Tubs center = inner_bot + h/2) |
| `ovcLogical` | z = -372.3 mm (OVC disk bottom) |
| `plate10mKLogical` | z = +2.0 mm (plate center = 0 + e/2) |

Example: a bracket whose bottom face should sit at z = -200 mm in cryostat frame, placed inside `ovcVacuumLogical`:

```
pos_z = -200 mm - (+143.55 mm) = -343.55 mm
```

```cpp
G4ThreeVector pos(0, 0, -343.55*mm);
new G4PVPlacement(nullptr, pos, lv, "Bracket_PV",
                  volumes.ovcVacuumLogical, false, 0, fCheckOverlaps);
```

### Handling rotation

If the STL was exported Z-up and cryostat Z is also up, no rotation is needed (`nullptr`).

If the CAD model was in a different orientation:

```cpp
auto* rot = new G4RotationMatrix();
rot->rotateX(90.*deg);   // part was lying on its side in FreeCAD
new G4PVPlacement(rot, pos, lv, "Bracket_PV", motherLV,
                  false, 0, fCheckOverlaps);
```

**Prefer fixing the orientation in FreeCAD before export** over compensating with rotations in C++.

---

## Stage 4: Iterative Verification Loop

1. Export STL from FreeCAD with deliberate origin (Stage 1)
2. `cmake --build build-dspx`
3. Run with verbose=1 to print G4VisExtent — confirm scale
4. Zero placement position first, run visualization:
   ```sh
   cd build-dspx && ./main geometry_lab_proton.mac
   ```
5. In the OpenGL window:
   ```
   /vis/viewer/set/style wireframe
   /vis/scene/add/axes 0 0 0 200 mm
   ```
6. Compute correct offset, update position, rebuild, re-visualize
7. Once placement looks correct, enable overlap checking:
   ```
   /run/beamOn 0
   ```
8. Document minor touching-surface warnings (< 1 um) in `src/Geometry/obj/README.md`

### Useful Geant4 UI Commands

```
/vis/viewer/set/style wireframe          show all volumes as wireframes
/vis/scene/add/axes 0 0 0 200 mm         draw XYZ triad at world origin
/geometry/test/run                        explicit overlap check
/geometry/test/tolerance 0.001 mm         set overlap detection tolerance
/vis/viewer/set/viewpointThetaPhi 90 0    side view (XZ plane)
/vis/viewer/set/viewpointThetaPhi 0 0     top view (XY plane)
```

---

## Quick Reference: CADMesh API (v2.0.x)

```cpp
// Load from file (STL, OBJ, PLY all supported)
auto mesh = CADMesh::TessellatedMesh::FromSTL("part.stl");

// Scale: set if file units differ from Geant4 internal units
mesh->SetScale(mm);    // FreeCAD mm export (most common)
mesh->SetScale(cm);    // if exported in cm

// Offset: shift the entire mesh origin (use sparingly)
mesh->SetOffset(G4ThreeVector(0, 0, -bottom_z*mm));

// Get the solid
auto* solid = mesh->GetSolid(); // G4TessellatedSolid*

// Build logical volume and place
auto* lv = new G4LogicalVolume(solid, material, "BracketTop_LV");
new G4PVPlacement(rot, pos, lv, "BracketTop_PV", motherLV,
                  false, 0, checkOverlaps);
```

---

## Common Pitfalls

| Symptom | Likely cause | Fix |
|---|---|---|
| Mesh appears 1000x too large | File in m, not mm | `mesh->SetScale(mm/1000)` or re-export in mm |
| Mesh appears at wrong Z | FreeCAD origin not reset before export | Re-export with corrected Placement |
| Mesh rotated 90 deg | CAD model was lying flat in FreeCAD | Add `rot->rotateX(90.*deg)` or fix in FreeCAD |
| Fatal overlap on first run | Wrong parent LV or position sign error | Print extent, recalculate pos_in_parent |
| Touching-surface warnings only | CSG plate top face = mesh bottom face | Expected; document in README, set tolerance |
| `GetSolid()` returns nullptr | File not found at runtime | Check CMake copy rule; confirm file in build dir |
| `ReaderCantReadError` at startup | File has wrong extension (`.ast`) | FreeCAD ASCII STL exports as `.ast`; rename to `.stl` |