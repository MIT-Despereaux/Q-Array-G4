#ifndef QRDetectorConstruction_h
#define QRDetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "G4GenericMessenger.hh"
#include "G4NistManager.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"

#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;

/// Detector construction class to define materials and geometry.

namespace QR
{
  class DetectorConstruction : public G4VUserDetectorConstruction
  {
  public:
    DetectorConstruction();
    virtual ~DetectorConstruction() override;

    virtual G4VPhysicalVolume *Construct() override;

    // Construct the sensitive detectors
    virtual void ConstructSDandField() override;

    // static G4int GetNDet1() { return fNDet1; };
    // static G4int GetNDet2() { return fNDet2; };

  private:
    // Private methods that construct the fridge
    // Returns the logical volume of the OVC
    void ConstructFridge();
    void ConstructLab();
    void ConstructTable(int iTableVer, bool bAddPb, G4ThreeVector offset);
    void AddExpPad(int iTableVer);

    // void AddScintI(G4String n);
    // void AddScintII(G4String n);

    void DefineCommands();

  private:
    G4GenericMessenger *mMessenger = nullptr;
    G4NistManager *mNistManager = nullptr;

    G4LogicalVolume *mDet1Logical = nullptr;
    G4LogicalVolume *mDet2Logical = nullptr;
    G4LogicalVolume *mDet3Logical = nullptr;
    G4LogicalVolume *mChipLogical = nullptr;

    // Useful logical volumes
    G4LogicalVolume *mFridgeLogical = nullptr;
    G4LogicalVolume *mStageOVCVacLogical = nullptr;
    G4double fMXCStageZ;
    G4LogicalVolume *mWorldLogical = nullptr;
    G4LogicalVolume *mTableLogical = nullptr;
    G4LogicalVolume *mTableSpaceLogical = nullptr;

    // Number of copies for detectors
    // static G4int fNDet1;
    // static G4int fNDet2;

    // Physical detector vector
    // std::vector<G4String> fDetNames;
    // std::vector<G4RotationMatrix *> fDetRotations;
    // std::vector<G4ThreeVector *> fDetPositions;
    // std::vector<G4VPhysicalVolume *> vecDetectors;

    // G4ThreeVector fGlobalOffset = G4ThreeVector(0., 0., 0.);
  };
}

#endif
