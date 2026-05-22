#ifndef QARRAY_GEOMETRY_CRYOSTAT_BUILDER_HH
#define QARRAY_GEOMETRY_CRYOSTAT_BUILDER_HH

#include "G4LogicalVolume.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

namespace QArray::Geometry
{
  struct CryostatVolumes
  {
    G4LogicalVolume* fridgeLogical = nullptr;
    G4LogicalVolume* ovcLogical = nullptr;
    G4LogicalVolume* ovcVacuumLogical = nullptr;
    G4LogicalVolume* mixingChamberLogical = nullptr;
    G4ThreeVector plate10mKCenter;
  };

  class CryostatBuilder
  {
  public:
    void SetCheckOverlaps(G4bool value);
    void SetVerbose(G4int value);
    void SetAddPb(G4bool value);

    // Path to directory containing STL mesh files (default: "." = build dir).
    // Set before calling Build() if files live elsewhere.
    void SetMeshDataPath(const G4String& path);

    CryostatVolumes Build();

  private:
    void BuildMeshComponents(const CryostatVolumes& volumes);

    G4bool   mCheckOverlaps = true;
    G4int    mVerbose       = 0;
    G4bool   mAddPb         = false;
    G4String mMeshDataPath  = ".";
  };
}

#endif
