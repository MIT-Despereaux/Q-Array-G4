#ifndef QRDetectorConstruction_DSPX_h
#define QRDetectorConstruction_DSPX_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4GenericMessenger.hh"
#include "G4NistManager.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class for the DSPX geometry variant.
/// Delegates cryostat construction to Geometry::CryostatBuilder.

namespace QArray
{
  class DetectorConstruction : public G4VUserDetectorConstruction
  {
  public:
    DetectorConstruction();
    virtual ~DetectorConstruction() override;

    virtual G4VPhysicalVolume *Construct() override;
    virtual void ConstructSDandField() override;

  private:
    void ConstructFridge();
    void ConstructLab();
    void ConstructTable(int iTableVer, bool bAddPb, G4ThreeVector offset);
    void AddExpPad(int iTableVer);
    void DefineCommands();

  private:
    G4GenericMessenger *mMessenger = nullptr;
    G4NistManager *mNistManager = nullptr;

    // Useful logical volumes
    G4LogicalVolume *mFridgeLogical = nullptr;
    G4LogicalVolume *mStageOVCVacLogical = nullptr;
    G4double fMXCStageZ;
    G4LogicalVolume *mWorldLogical = nullptr;
  };
}

#endif
