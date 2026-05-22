# STL Mesh Placement Workflow (CADMesh + Geant4)

Reference document for loading FreeCAD-exported STL files into the DSPX
cryostat geometry via CADMesh. Covers origin control, extent inspection,
placement math, and iterative verification.

---

## The Core Problem

An STL file is a bare list of triangles with XYZ coordinates and no metadata.
When CADMesh loads a file, Geant4 treats those coordinates as-is in mm. The
origin `(0,0,0)` of the resulting `G4TessellatedSolid` is **exactly wherever
`(0,0,0)` was in FreeCAD at export time** -- no axis convention, no mounting
metadata, no hierarchy information is preserved.

The full workflow has three stages:

1. Control the origin in FreeCAD **before** export.
2. Inspect the bounding box in Geant4 **after** loading.
3. Verify placement iteratively with visualization and overlap checks.

---

## Stage 1: Control the Origin in FreeCAD

This is the most important step. Before exporting, deliberately position each
part so that a **physically meaningful point** is at FreeCAD's `(0,0,0)`.

### Recommended origin convention for DSPX

```
All STL parts exported with Z axis = cryostat vertical (up = +Z).
Origin = center of the bottom face of the part.
Units  = mm (FreeCAD default mesh export).
```

This matches the `Bucket` / `FilledTube` origin semantics already used in
`CryostatBuilder.cc`, so placement offsets are consistent throughout the code.

### How to set the origin in FreeCAD

In the Part or Part Design workbench, select the body, then in the Model tab
Properties panel set:

```
Placement > Position > x = 0   y = 0   z = 0
Placement > Angle    = 0        Axis  = (0, 0, 1)
```

Or use **Part > Move** to reposition the body before export. The exported STL
coordinates will directly reflect the current Placement. Do this **before**
File > Export.

### Recommended FreeCAD mesh export settings

Open File > Export, choose STL format, then in the export dialog:

| Setting           | Recommended value | Why                                       |
|-------------------|-------------------|-------------------------------------------|
| Surface deviation | <= 0.1 mm         | Adequate for curved cryostat surfaces     |
| Angular deviation | <= 0.5 deg        | Avoids obvious faceting on cylinders      |
| Format            | ASCII or Binary `.stl` | **Must be named `.stl`** -- FreeCAD ASCII export saves as `.ast`, rename before committing. CADMesh only recognises `.stl` extension. |

### Document every part in `src/Geometry/data/README.md`

For each STL file add a row:

```markdown
| Filename         | STEP part name     | Origin point         | Material | Parent LV           |
|------------------|--------------------|----------------------|----------|---------------------|
| bracket_top.stl  | TopMountingBracket | Bottom face center   | G4_Cu    | ovcVacuumLogical    |
| support_rod.stl  | ColdFinger_Rod     | Bottom end center    | G4_Fe    | plate10mKLogical    |
```

---

## Stage 2: Inspect the Mesh Extents in Geant4

After loading, print the bounding box to verify scale and axis orientation
before attempting placement.

### Extent query snippet

```cpp
#include "CADMesh.hh"
#include "G4TessellatedSolid.hh"
#include "G4VisExtent.hh"

auto mesh = CADMesh::TessellatedMesh::Read("bracket_top.stl");
mesh->SetScale(mm);                   // FreeCAD exports in mm; G4 mm unit = 1.0
auto* solid = mesh->GetSolid();       // returns G4TessellatedSolid*

G4VisExtent ext = solid->GetExtent();
G4cout << "[CADMesh] bracket_top.stl extent (mm):"
       << "\n  X: [" << ext.GetXmin()/mm << ", " << ext.GetXmax()/mm << "]"
       << "\n  Y: [" << ext.GetYmin()/mm << ", " << ext.GetYmax()/mm << "]"
       << "\n  Z: [" << ext.GetZmin()/mm << ", " << ext.GetZmax()/mm << "]"
       << G4endl;
```

Add this inside `CryostatBuilder::BuildMeshComponents()` behind a verbose
guard so it can be toggled without recompiling:

```cpp
if (fVerbose > 0) {
    G4VisExtent ext = solid->GetExtent();
    G4cout << "[CryostatBuilder] " << name << " extent (mm):"
           << " X[" << ext.GetXmin()/mm << "," << ext.GetXmax()/mm << "]"
           << " Y[" << ext.GetYmin()/mm << "," << ext.GetYmax()/mm << "]"
           << " Z[" << ext.GetZmin()/mm << "," << ext.GetZmax()/mm << "]"
           << G4endl;
}
```

Enable at runtime by calling `builder.SetVerbose(1)` in
`DetectorConstruction_DSPX.cc`.

### What to check

| Check                              | Expected result                         |
|------------------------------------|-----------------------------------------|
| Z range ~ part height from drawings | Confirms mm scale is correct            |
| Z min ~ 0                          | Confirms bottom-face-at-origin convention |
| X/Y symmetric around 0             | Confirms part is centered on Z axis     |
| All values finite and non-zero     | Confirms file was read successfully     |

If the Z range is ~1000x too large or too small the unit is wrong -- adjust
`mesh->SetScale(mm)` or `mesh->SetScale(cm)` accordingly.

---

## Stage 3: Placement Math

The position passed to `G4PVPlacement` is the **offset of the mesh origin
relative to the parent logical volume's own origin**.

### General formula

```
pos_in_parent = target_position_in_cryostat_frame
              - parent_origin_in_cryostat_frame
```

### DSPX parent volume origins (cryostat frame, z=0 at Plate10mK bottom face)

| Parent logical volume   | Origin in cryostat frame        |
|-------------------------|---------------------------------|
| `ovcVacuumLogical`      | z = -367.3 mm (OVC inner bottom)|
| `ovcLogical`            | z = -372.3 mm (OVC disk bottom) |
| `plate10mKLogical`      | z =    0.0 mm (Plate10mK bot)   |

Example: a bracket whose bottom face should sit at z = -200 mm in the
cryostat frame, placed inside `ovcVacuumLogical`:

```
pos_z = -200 mm - (-367.3 mm) = +167.3 mm
```

```cpp
G4ThreeVector pos(0, 0, 167.3*mm);
new G4PVPlacement(nullptr, pos, lv, "Bracket_PV",
                  volumes.ovcVacuumLogical, false, 0, fCheckOverlaps);
```

### Handling rotation

If the STL was exported Z-up and the cryostat Z is also up, no rotation is
needed (`nullptr` as the first argument to `G4PVPlacement`).

If the CAD model was in a different orientation at export time:

```cpp
auto* rot = new G4RotationMatrix();
rot->rotateX(90.*deg);   // part was lying on its side in FreeCAD
new G4PVPlacement(rot, pos, lv, "Bracket_PV", motherLV,
                  false, 0, fCheckOverlaps);
```

Prefer fixing the orientation in FreeCAD before export over compensating with
rotations in C++. Correcting in FreeCAD keeps the code clean and the README
accurate.

---

## Stage 4: Iterative Verification Loop

```
1. Export STL from FreeCAD with deliberate origin (Stage 1).
2. cmake --build build-dspx
3. Run with verbose=1 to print G4VisExtent -- confirm scale and axis.
4. Set placement position to zero first and run visualization:
       cd build-dspx && ./main geometry_lab_proton.mac
5. In the OpenGL window:
       /vis/viewer/set/style wireframe
       /vis/scene/add/axes 0 0 0 200 mm    <- XYZ triad at world origin
   Zoom in to see where the mesh landed relative to the CSG plates.
6. Compute the correct offset using the formula above, update the position,
   rebuild, and re-visualize.
7. Once placement looks correct, enable overlap checking:
       /run/beamOn 0      <- triggers geometry rebuild + G4 overlap check
   This runs even without a primary particle and reports all interpenetrations.
8. Minor touching-surface warnings (< 1 um) from Bucket/screen interfaces
   are acceptable. Document them in src/Geometry/data/README.md.
   Fatal overlaps (large mm-scale) indicate a wrong position or scale.
```

### Useful Geant4 UI commands during verification

```
/vis/viewer/set/style wireframe          show all volumes as wireframes
/vis/scene/add/axes 0 0 0 200 mm         draw XYZ triad at world origin
/vis/scene/add/axes 0 0 -367 200 mm      draw triad at ovcVacuumLogical origin
/geometry/test/run                        explicit overlap check (same as beamOn 0)
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

// Offset: shift the entire mesh origin (use as a last resort;
// prefer fixing the origin in FreeCAD before export)
mesh->SetOffset(G4ThreeVector(0, 0, -bottom_z*mm));

// Get the solid (G4TessellatedSolid*)
auto* solid = mesh->GetSolid();

// Optional: name the solid explicitly
mesh->GetSolid()->SetName("BracketTop");

// Then build logical volume and place as normal:
auto* lv = new G4LogicalVolume(solid, material, "BracketTop_LV");
new G4PVPlacement(rot, pos, lv, "BracketTop_PV", motherLV,
                  false, 0, checkOverlaps);
```

`SetOffset` is the escape hatch if you cannot re-export from FreeCAD. Use it
only when documented in `src/Geometry/data/README.md` -- silent offsets in C++
make the placement math hard to audit later.

---

## Common Pitfalls

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| Mesh appears 1000x too large | File in m, not mm | `mesh->SetScale(mm/1000)` or re-export in mm |
| Mesh appears at wrong Z (deep below geometry) | FreeCAD origin was not reset before export | Re-export with corrected Placement |
| Mesh rotated 90 deg | CAD model was lying flat in FreeCAD | Add `rot->rotateX(90.*deg)` or fix in FreeCAD |
| Fatal overlap on first run | Wrong parent LV or position sign error | Print extent, recalculate pos_in_parent |
| Touching-surface warnings only | CSG plate top face = mesh bottom face | Expected; document in README, set tolerance |
| `GetSolid()` returns nullptr | File not found at runtime | Check CMake copy rule; confirm file is in build dir |
| `ReaderCantReadError` at startup | File has wrong extension (e.g. `.ast`) | FreeCAD ASCII STL exports as `.ast`; rename to `.stl` before committing |
