# DSPX STL Mesh Inventory

Canonical location for all STL mesh files used by the DSPX geometry
(`src/Geometry/obj/`). Files here are copied into the build directory
automatically by CMake and loaded at runtime via CADMesh.

See `.agents/stl_placement_workflow.md` for the full FreeCAD export and
Geant4 placement workflow.

---

## Export Convention

| Parameter            | Value                                                                       |
|----------------------|-----------------------------------------------------------------------------|
| Format               | ASCII or Binary STL, **named `.stl`**. FreeCAD's ASCII STL export saves as `.ast` -- rename to `.stl` before committing. CADMesh only recognises `.stl`. |
| Z axis               | Cryostat vertical (up = +Z)                                                 |
| Origin               | Top face center of the part (see note below)                                |
| Units                | mm (FreeCAD default)                                                        |
| Surface deviation    | <= 0.1 mm                                                                   |
| Angular deviation    | <= 0.5 deg                                                                  |

> **Note on origin convention:** The `Experimental_Paddle.stl` is exported
> with the **top face center at (0,0,0)** — the part hangs downward into
> negative Z. This differs from the bottom-face convention used by the CSG
> shapes (`Bucket`, `FilledTube`). All future parts should be documented
> clearly below. When in doubt, run the extent check (see workflow doc).

---

## Mesh Inventory

| Filename                  | STEP part name       | Origin point             | Bounding box (mm)                    | Material | Parent LV                  | Notes                                                        |
|---------------------------|----------------------|--------------------------|--------------------------------------|----------|----------------------------|--------------------------------------------------------------|
| Experimental_Paddle.stl  | Experimental_Paddle  | Top face center          | X[-125,125] Y[-80,50] Z[-163,0]      | G4_Cu    | ovcVacuumLogical           | ASCII STL; top face at Plate10mK bottom (z=0)                |
| Ricochet_Box_Platform.stl | Ricochet_Box_Platform | Platform reference       | X[-35,35] Y[-35,35] Z[-30.31,0]      | G4_Cu    | DetectorBoxAssemblyLogical | Platform origin placed -3.1 mm below detector reference      |
| Detector_Base.stl        | Detector_Base        | Detector reference       | X[-32.5,32.5] Y[-32.5,32.5] Z[-3,3.5] | G4_Cu    | DetectorBoxAssemblyLogical | Detector reference at (13.1,-35.5,-76.75) mm in cryostat frame |
| Detector_Cover.stl       | Detector_Cover       | Cover reference          | X[-22.5,22.5] Y[-22.5,22.5] Z[0,33.1] | G4_Cu    | DetectorBoxAssemblyLogical | Cover origin placed +1.7 mm above detector reference; rotated 180 deg around Z |

The Sn detector cube is CSG, not a mesh: `19.3 x 19.3 x 15.4 mm`, with its
bottom face 3.0 mm above the detector-cover origin.

`DetectorBoxAssemblyLogical` is a vacuum mother volume sized to the current
children with symmetric Z about the detector baseplate reference point:
half-lengths `(35,35,34.8) mm`.

Detector-box alignment: the box origin is offset from the screw-hole alignment
point by `(+25.4,0,-88.9) mm`. The current alignment offset is
`(-12.3,-35.5,+12.15) mm`, giving detector reference position
`(13.1,-35.5,-76.75) mm` in the cryostat frame.

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
