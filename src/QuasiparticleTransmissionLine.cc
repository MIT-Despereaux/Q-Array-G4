#include "QuasiparticleTransmissionLine.hh"
#include "QuasiparticleDetectorParameters.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4ExtrudedSolid.hh"
#include "G4MultiUnion.hh"
#include "G4Transform3D.hh"
#include "G4RotationMatrix.hh"
#include "G4ThreeVector.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4ios.hh"

using namespace QuasiparticleDetectorParameters;

QuasiparticleTransmissionLine::QuasiparticleTransmissionLine(
    G4RotationMatrix* pRot, const G4ThreeVector& tLate, const G4String& pName,
    G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo,
    G4LatticeManager* LM, std::map<std::string,G4LatticeLogical*> logicalLatticeContainer,
    std::map<std::string,G4CMPSurfaceProperty*> borderContainer, G4bool pSurfChk)
{
    ConstructTransmissionLine(pRot, tLate, pName, pMotherLogical, pMany, pCopyNo, LM, logicalLatticeContainer, borderContainer, pSurfChk);
}

QuasiparticleTransmissionLine::~QuasiparticleTransmissionLine() {}

void QuasiparticleTransmissionLine::ConstructTransmissionLine(
    G4RotationMatrix* pRot, const G4ThreeVector& tLate, const G4String& pName,
    G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo,
    G4LatticeManager* LM, std::map<std::string,G4LatticeLogical*> logicalLatticeContainer,
    std::map<std::string,G4CMPSurfaceProperty*> borderContainer, G4bool pSurfChk)
{
    G4cout << "=== [DEBUG] Starting TransmissionLine Construction ===" << G4endl;

    G4NistManager* nist = G4NistManager::Instance();
    G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");

    std::vector<G4TwoVector> padPolygon;
    padPolygon.push_back(G4TwoVector(-5.0*CLHEP::um, 0.0));
    padPolygon.push_back(G4TwoVector( 5.0*CLHEP::um, 0.0));
    padPolygon.push_back(G4TwoVector( 75.0*CLHEP::um, 200.0*CLHEP::um));
    padPolygon.push_back(G4TwoVector( 75.0*CLHEP::um, 400.0*CLHEP::um));
    padPolygon.push_back(G4TwoVector(-75.0*CLHEP::um, 400.0*CLHEP::um));
    padPolygon.push_back(G4TwoVector(-75.0*CLHEP::um, 200.0*CLHEP::um));

    G4ExtrudedSolid* padMetal = new G4ExtrudedSolid("PadMetal", padPolygon, dp_groundPlaneDimZ/2.0, G4TwoVector(0,0), 1.0, G4TwoVector(0,0), 1.0);

    G4Box* tlCenter = new G4Box("TLCenter", 4308.0*CLHEP::um/2.0, 10.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0);
    G4Tubs* lCurve  = new G4Tubs("LCurve", 45.0*CLHEP::um, 55.0*CLHEP::um, dp_groundPlaneDimZ/2.0, 90.0*CLHEP::deg, 90.0*CLHEP::deg);
    G4Tubs* rCurve  = new G4Tubs("RCurve", 45.0*CLHEP::um, 55.0*CLHEP::um, dp_groundPlaneDimZ/2.0, 270.0*CLHEP::deg, 90.0*CLHEP::deg);
    G4Box* extBox   = new G4Box("ExtBox", 10.0*CLHEP::um/2.0, 300.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0);

    G4MultiUnion* tlMulti = new G4MultiUnion(pName + "_Solid");
    tlMulti->AddNode(*tlCenter, G4Transform3D(G4RotationMatrix(), G4ThreeVector(0, 0, 0)));
    tlMulti->AddNode(*lCurve, G4Transform3D(G4RotationMatrix(), G4ThreeVector(-2154.0*CLHEP::um, -45.0*CLHEP::um, 0)));
    tlMulti->AddNode(*extBox, G4Transform3D(G4RotationMatrix(), G4ThreeVector(-2199.0*CLHEP::um, -195.0*CLHEP::um, 0)));

    G4RotationMatrix rotPadLeft;
    rotPadLeft.rotateZ(180.0 * CLHEP::deg);
    tlMulti->AddNode(*padMetal, G4Transform3D(rotPadLeft, G4ThreeVector(-2199.0*CLHEP::um, -345.0*CLHEP::um, 0)));

    tlMulti->AddNode(*rCurve, G4Transform3D(G4RotationMatrix(), G4ThreeVector(2154.0*CLHEP::um, 45.0*CLHEP::um, 0)));
    tlMulti->AddNode(*extBox, G4Transform3D(G4RotationMatrix(), G4ThreeVector(2199.0*CLHEP::um, 195.0*CLHEP::um, 0)));
    tlMulti->AddNode(*padMetal, G4Transform3D(G4RotationMatrix(), G4ThreeVector(2199.0*CLHEP::um, 345.0*CLHEP::um, 0)));

    G4cout << "=== [DEBUG] Starting TransmissionLine Voxelization... ===" << G4endl;
    tlMulti->Voxelize();
    G4cout << "=== [DEBUG] Finished TransmissionLine Voxelization ===" << G4endl;

    G4LogicalVolume* log_TL = new G4LogicalVolume(tlMulti, aluminum, pName + "_Log");
    fPhys_output = new G4PVPlacement(pRot, tLate, log_TL, pName + "_Phys", pMotherLogical, pMany, pCopyNo, pSurfChk);

    fFundamentalVolumeList.push_back(std::make_tuple("Aluminum", pName, fPhys_output));
    
    G4cout << "=== [DEBUG] Finished TransmissionLine Construction ===" << G4endl;
}

std::vector<std::tuple<std::string, G4String, G4VPhysicalVolume*>> 
QuasiparticleTransmissionLine::GetListOfAllFundamentalSubVolumes()
{
    return fFundamentalVolumeList;
}