#ifndef ExtrudedLayerBuilder_h
#define ExtrudedLayerBuilder_h 1

#include "globals.hh"
#include "G4TwoVector.hh"
#include <vector>

class G4LogicalVolume;
class G4Material;
class G4VSensitiveDetector;
class G4VPhysicalVolume; // <--- Add this line here

class ExtrudedLayerBuilder
{
  public:
    ExtrudedLayerBuilder();
    ~ExtrudedLayerBuilder();

    // Configuration Setters
    void SetMaterial(G4Material* material);
    void SetZOffset(G4double zOffset);
    void SetNamePrefix(const G4String& name);
    
    // ExtrudedLayerBuilder.hh
    std::vector<G4VPhysicalVolume*> BuildLayer(
        G4LogicalVolume* motherVolume, 
        const std::vector<std::vector<G4TwoVector>>& polygons,
        G4double totalThicknessFromJSON);

    // Call this after building to unify all pieces into ONE detector readout
    void AssignSensitiveDetector(G4VSensitiveDetector* sd);

  private:
    void EnsureClockwiseWinding(std::vector<G4TwoVector>& vertices);

    G4Material* fMaterial;
    G4double    fZOffset;
    G4String    fNamePrefix;

    // Store logical volumes to attach the Sensitive Detector later
    std::vector<G4LogicalVolume*> fCreatedLogicals;
};

#endif