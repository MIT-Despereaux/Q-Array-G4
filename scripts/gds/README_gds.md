# GDS / JSON to Geant4 Extrusion Pipeline

This folder contains the geometry definitions (JSON/GDS) and the verification scripts required to map 2D layouts into 3D Geant4 volumes.

## Workflow

1. **Verify Geometry:** Run the Python script to convert the raw JSON coordinates into a GDSII file. Open this in KLayout to visually verify the dimensions and curves.
   `python verify_geometry.py`
2. **Export to C++:** The Geant4 module `ExtrudedLayerBuilder` expects an array of 2D coordinates. You can parse the JSON directly in Geant4 using a library like `nlohmann/json`, or dump it to a simple flat text file.
3. **Build in Geant4:** The C++ class `ExtrudedLayerBuilder` will automatically read the vertices, correct the winding order (forcing Clockwise to prevent G4ExtrudedSolid crashes), and place the volumes on the specified mother logic volume.

## Integration in DetectorConstruction

You do not need to modify the core legacy code heavily. Simply instantiate the builder, define the G4CMP/NIST material, and call the build function.

```cpp
// Example usage in DetectorConstruction.cc
#include "ExtrudedLayerBuilder.hh"

ExtrudedLayerBuilder builder;
builder.SetMaterial(myNiobiumMaterial);
builder.SetThickness(50.0 * nm);
builder.SetZOffset(100.0 * um);
builder.BuildLayer(logicSiliconBase, parsedPolygonsVector);
```
