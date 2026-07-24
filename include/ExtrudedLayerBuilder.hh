#ifndef ExtrudedLayerBuilder_h
#define ExtrudedLayerBuilder_h 1

#include "globals.hh"
#include "G4TwoVector.hh"
#include "G4ThreeVector.hh"
#include "G4RotationMatrix.hh"
#include <vector>

class G4LogicalVolume;
class G4Material;
class G4VSensitiveDetector;
class G4VPhysicalVolume;

class ExtrudedLayerBuilder
{
  public:
    ExtrudedLayerBuilder();
    ~ExtrudedLayerBuilder();

    // Configuration Setters
    void SetMaterial(G4Material* material);
    void SetPosition(const G4ThreeVector& position);
    void SetRotation(G4RotationMatrix* rotation);
    void SetNamePrefix(const G4String& name);
    
    // Unifies all polygon solids into a single compound Physical Volume
    G4VPhysicalVolume* BuildUnifiedLayer(
        G4LogicalVolume* motherVolume, 
        const std::vector<std::vector<G4TwoVector>>& polygons,
        G4double totalThicknessFromJSON);

    // Call this after building to attach the Sensitive Detector to the unified logical volume
    void AssignSensitiveDetector(G4VSensitiveDetector* sd);

  private:
    void EnsureClockwiseWinding(std::vector<G4TwoVector>& vertices);

    G4Material*       fMaterial;
    G4ThreeVector     fPosition;
    G4RotationMatrix* fRotation;
    G4String          fNamePrefix;
    G4LogicalVolume*  fUnifiedLogical;
};

#endif