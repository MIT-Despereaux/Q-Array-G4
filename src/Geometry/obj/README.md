# DSPX STL Mesh Inventory

Canonical location for all STL mesh files used by the DSPX geometry
(`src/Geometry/obj/`). Files here are copied into the build directory
automatically by CMake and loaded at runtime via CADMesh.

See `.agents/stl_placement_workflow.md` for the full FreeCAD export and
Geant4 placement workflow.

---

## Export Convention

| Parameter            | Value                                         |
|----------------------|-----------------------------------------------|
| Z axis               | Cryostat vertical (up = +Z)                   |
| Origin               | Top face center of the part (see note below)  |
| Units                | mm (FreeCAD default)                          |
| Surface deviation    | <= 0.1 mm                                     |
| Angular deviation    | <= 0.5 deg                                    |
| Format               | Binary STL                                    |

> **Note on origin convention:** The `Experimental_Paddle.stl` is exported
> with the **top face center at (0,0,0)** — the part hangs downward into
> negative Z. This differs from the bottom-face convention used by the CSG
> shapes (`Bucket`, `FilledTube`). All future parts should be documented
> clearly below. When in doubt, run the extent check (see workflow doc).

---

## Mesh Inventory

| Filename                   | STEP part name       | Origin point      | Bounding box (mm)                    | Material | Parent LV          | Notes                                          |
|----------------------------|----------------------|-------------------|--------------------------------------|----------|--------------------|------------------------------------------------|
| Experimental_Paddle.stl    | Experimental_Paddle  | Top face center   | X[-125,125] Y[-80,50] Z[-163,0]      | G4_Cu    | ovcVacuumLogical   | Placed with top face at Plate10mK bottom (z=0) |

---

## Adding a New Part

1. In FreeCAD, set `Placement > Position = (0,0,0)` at your chosen origin
   before exporting (see workflow doc Stage 1).
2. Export: `File > Export > STL`, surface deviation <= 0.1 mm.
3. Copy the `.stl` file into this directory.
4. Add a row to the inventory table above.
5. Add a `MeshSpec` entry in `CryostatBuilder::BuildMeshComponents()`.
6. Commit the `.stl` file alongside the code change (STL files are tracked in git).
7. `cmake --build build-dspx` — CMake copies new STL files automatically.
8. Verify with `G4VisExtent` print (set `builder.SetVerbose(1)`) and
   visualization before committing.
