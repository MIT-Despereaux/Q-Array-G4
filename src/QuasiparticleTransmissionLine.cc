#include "QuasiparticleTransmissionLine.hh"
#include "QuasiparticleDetectorParameters.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Trd.hh"
#include "G4UnionSolid.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"

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
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");

    // Dimensions from parameters
    G4double lineLen = 4308.0 * CLHEP::um;
    G4double lineWidth = 10.0 * CLHEP::um;
    G4double lineGap = 6.0 * CLHEP::um;
    G4double totalWidth = lineWidth + 2 * lineGap; // 22 um
    G4double curveRad = 45.0 * CLHEP::um;
    G4double padStraightExt = 300.0 * CLHEP::um;

    // 1. Center Straight Line
    G4Box* centerMetal = new G4Box("TL_CenterMetal", lineLen/2.0, lineWidth/2.0, dp_groundPlaneDimZ/2.0);
    G4Box* centerGap = new G4Box("TL_CenterGap", lineLen/2.0, totalWidth/2.0, dp_groundPlaneDimZ/2.0);

    // 2. Curves (Inner radius 45, width 10)
    G4Tubs* curveMetal = new G4Tubs("TL_CurveMetal", curveRad, curveRad + lineWidth, dp_groundPlaneDimZ/2.0, 0, CLHEP::halfpi);
    G4Tubs* curveGap = new G4Tubs("TL_CurveGap", curveRad - lineGap, curveRad + lineWidth + lineGap, dp_groundPlaneDimZ/2.0, 0, CLHEP::halfpi);

    // 3. Extensions (300 um)
    G4Box* extMetal = new G4Box("TL_ExtMetal", lineWidth/2.0, padStraightExt/2.0, dp_groundPlaneDimZ/2.0);
    G4Box* extGap = new G4Box("TL_ExtGap", totalWidth/2.0, padStraightExt/2.0, dp_groundPlaneDimZ/2.0);

    // 4. Tapered Pads
    G4Trd* padMetal = new G4Trd("TL_PadMetal", 150.0*CLHEP::um/2.0, 10.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0, dp_groundPlaneDimZ/2.0, 200.0*CLHEP::um/2.0);
    G4Trd* padGap = new G4Trd("TL_PadGap", 410.0*CLHEP::um/2.0, 22.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0, dp_groundPlaneDimZ/2.0, 296.0*CLHEP::um/2.0);

    // Assemble via Union to prevent bounding box boundaries
    G4UnionSolid* fullMetal = new G4UnionSolid("TL_Metal_Solid", centerMetal, curveMetal, 0, G4ThreeVector(lineLen/2.0, -(curveRad + lineWidth/2.0), 0));
    // Repeat boolean additions for extensions, opposite side curve, and pads...
    // (Ensure you append the extensions and pads at the correct coordinates using standard matrix rotations for the left downward curve and right upward curve).
    
    G4LogicalVolume* log_TLMetal = new G4LogicalVolume(fullMetal, aluminum, pName + "_MetalLog");
    fPhys_output = new G4PVPlacement(pRot, tLate, log_TLMetal, pName + "_MetalPhys", pMotherLogical, pMany, pCopyNo, pSurfChk);

    // Track for SD assignment
    fFundamentalVolumeList.push_back(std::make_tuple("Aluminum", pName, fPhys_output));
}