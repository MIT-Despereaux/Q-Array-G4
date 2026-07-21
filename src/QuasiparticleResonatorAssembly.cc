#include "QuasiparticleResonatorAssembly.hh"
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

using namespace QuasiparticleDetectorParameters;

QuasiparticleResonatorAssembly::QuasiparticleResonatorAssembly(
    G4RotationMatrix* pRot, const G4ThreeVector& tLate, const G4String& pName,
    G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo,
    G4LatticeManager* LM, std::map<std::string,G4LatticeLogical*> logicalLatticeContainer,
    std::map<std::string,G4CMPSurfaceProperty*> borderContainer, G4bool pSurfChk)
{
    ConstructResonatorAssembly(pRot, tLate, pName, pMotherLogical, pMany, pCopyNo, LM, logicalLatticeContainer, borderContainer, pSurfChk);
}

QuasiparticleResonatorAssembly::~QuasiparticleResonatorAssembly() {}

void QuasiparticleResonatorAssembly::ConstructResonatorAssembly(
    G4RotationMatrix* pRot, const G4ThreeVector& tLate, const G4String& pName,
    G4LogicalVolume* pMotherLogical, G4bool pMany, G4int pCopyNo,
    G4LatticeManager* LM, std::map<std::string,G4LatticeLogical*> logicalLatticeContainer,
    std::map<std::string,G4CMPSurfaceProperty*> borderContainer, G4bool pSurfChk)
{
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");

    // 1. Native "+" Cross Solid
    std::vector<G4TwoVector> crossPolygon;
    G4double w = 30.0 * CLHEP::um / 2.0; 
    G4double l = 240.0 * CLHEP::um / 2.0;

    crossPolygon.push_back(G4TwoVector(-w, -l));
    crossPolygon.push_back(G4TwoVector( w, -l));
    crossPolygon.push_back(G4TwoVector( w, -w));
    crossPolygon.push_back(G4TwoVector( l, -w));
    crossPolygon.push_back(G4TwoVector( l,  w));
    crossPolygon.push_back(G4TwoVector( w,  w));
    crossPolygon.push_back(G4TwoVector( w,  l));
    crossPolygon.push_back(G4TwoVector(-w,  l));
    crossPolygon.push_back(G4TwoVector(-w,  w));
    crossPolygon.push_back(G4TwoVector(-l,  w));
    crossPolygon.push_back(G4TwoVector(-l, -w));
    crossPolygon.push_back(G4TwoVector(-w, -w));

    G4ExtrudedSolid* plusCross = new G4ExtrudedSolid("PlusCross", crossPolygon, dp_groundPlaneDimZ/2.0, G4TwoVector(0,0), 1.0, G4TwoVector(0,0), 1.0);

    // 2. Single Resonator Construction (G4MultiUnion)
    G4Box* strSeg = new G4Box("ResStr", 10.0*CLHEP::um/2.0, 300.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0);
    G4Tubs* topArc = new G4Tubs("ResTArc", 45.0*CLHEP::um, 55.0*CLHEP::um, dp_groundPlaneDimZ/2.0, 0*CLHEP::deg, 180.0*CLHEP::deg);
    G4Tubs* botArc = new G4Tubs("ResBArc", 45.0*CLHEP::um, 55.0*CLHEP::um, dp_groundPlaneDimZ/2.0, 180.0*CLHEP::deg, 180.0*CLHEP::deg);

    G4MultiUnion* singleRes = new G4MultiUnion(pName + "_Solid");
    
    // Straight start segment at local origin
    singleRes->AddNode(*strSeg, G4Transform3D(G4RotationMatrix(), G4ThreeVector(0, 0, 0)));

    for (int j = 0; j < 6; ++j) {
        G4double cx = j * 100.0 * CLHEP::um + 50.0 * CLHEP::um;
        if (j % 2 == 0) { 
            singleRes->AddNode(*topArc, G4Transform3D(G4RotationMatrix(), G4ThreeVector(cx, 150.0*CLHEP::um, 0)));
            singleRes->AddNode(*strSeg, G4Transform3D(G4RotationMatrix(), G4ThreeVector(cx + 50.0*CLHEP::um, 0, 0)));
        } else { 
            singleRes->AddNode(*botArc, G4Transform3D(G4RotationMatrix(), G4ThreeVector(cx, -150.0*CLHEP::um, 0)));
            singleRes->AddNode(*strSeg, G4Transform3D(G4RotationMatrix(), G4ThreeVector(cx + 50.0*CLHEP::um, 0, 0)));
        }
    }

    singleRes->AddNode(*plusCross, G4Transform3D(G4RotationMatrix(), G4ThreeVector(650.0*CLHEP::um, 270.0*CLHEP::um, 0)));
    singleRes->Voxelize();

    // 3. Placement (Exactly one per class instantiation)
    G4LogicalVolume* log_Resonator = new G4LogicalVolume(singleRes, aluminum, pName + "_Log");
    
    // We pass tLate and pRot directly from the parent, no extra offsets needed
    G4VPhysicalVolume* resPhys = new G4PVPlacement(pRot, tLate, log_Resonator, pName + "_Phys", pMotherLogical, pMany, pCopyNo, pSurfChk);
    
    fFundamentalVolumeList.push_back(std::make_tuple("Aluminum", pName, resPhys));
}

std::vector<std::tuple<std::string, G4String, G4VPhysicalVolume*>> 
QuasiparticleResonatorAssembly::GetListOfAllFundamentalSubVolumes()
{
    return fFundamentalVolumeList;
}