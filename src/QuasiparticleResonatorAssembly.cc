#include "QuasiparticleResonatorAssembly.hh"
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
    G4Material* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    G4Material* aluminum = nist->FindOrBuildMaterial("G4_Al");

    // 8 Resonator Loop
    G4double startX = -((dp_numResonators - 1) * dp_resSpacingX) / 2.0; // Centered across X
    G4double startY = dp_resDistanceToTLGap + (22.0*CLHEP::um / 2.0) + (3.0*CLHEP::um); // Transmission line gap edge + ground separation

    for (int i = 0; i < dp_numResonators; ++i) 
    {
        G4double currentX = startX + (i * dp_resSpacingX);
        G4String resName = pName + "_Res_" + std::to_string(i);

        // 1. Straights (7 total: lengths 300 except top which is 200)
        for (int j = 0; j < 7; ++j) 
        {
            G4double len = (j == 6) ? dp_resTopStraightLength : dp_resStraightLength;
            G4Box* straight = new G4Box(resName + "_straight_" + std::to_string(j), dp_resTraceWidth/2.0, len/2.0, dp_groundPlaneDimZ/2.0);
            
            // Build gaps logic
            G4Box* gap = new G4Box(resName + "_gap_" + std::to_string(j), (dp_resTraceWidth + 2*dp_resGapWidth)/2.0, len/2.0, dp_groundPlaneDimZ/2.0);
            // NOTE: the first straight (j=0) terminates on the ground plane, so we trim its bottom gap thickness to 0 at the interface.
            
            // Place straights directly in the mother logical (ground plane / vacuum layer)
            // Staggering X positions based on the 45um inner radius curves
        }

        // 2. Cross and Capacitor
        // Cross dimensions: 240 length x 30 width
        G4Box* crossV = new G4Box("CrossV", dp_crossWidth/2.0, dp_crossLength/2.0, dp_groundPlaneDimZ/2.0);
        G4Box* crossH = new G4Box("CrossH", dp_crossLength/2.0, dp_crossWidth/2.0, dp_groundPlaneDimZ/2.0);
        G4UnionSolid* crossIsland = new G4UnionSolid(resName + "_Cross", crossV, crossH);
        
        // Gap for cross: 300x90
        G4Box* crossGapV = new G4Box("CrossGapV", 90.0*CLHEP::um/2.0, 300.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0);
        G4Box* crossGapH = new G4Box("CrossGapH", 300.0*CLHEP::um/2.0, 90.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0);
        G4UnionSolid* crossGap = new G4UnionSolid(resName + "_CrossGap", crossGapV, crossGapH);

        // Capacitor: |_| shape around the cross
        // Construct using 3 combined boxes and union them to the resonator line
    }

    // 3. Gate Contact Pads (Completely new geometry requested)
    // Locations: +/- 2345 vertical from 0,0. Horizontal separations: 170 and (170+1090)
    std::vector<G4ThreeVector> gatePositions = {
        G4ThreeVector(170.0*CLHEP::um, 2345.0*CLHEP::um, 0),
        G4ThreeVector(-170.0*CLHEP::um, 2345.0*CLHEP::um, 0),
        G4ThreeVector((170.0+1090.0)*CLHEP::um, 2345.0*CLHEP::um, 0),
        G4ThreeVector(-(170.0+1090.0)*CLHEP::um, 2345.0*CLHEP::um, 0),
        G4ThreeVector(170.0*CLHEP::um, -2345.0*CLHEP::um, 0),
        G4ThreeVector(-170.0*CLHEP::um, -2345.0*CLHEP::um, 0),
        G4ThreeVector((170.0+1090.0)*CLHEP::um, -2345.0*CLHEP::um, 0),
        G4ThreeVector(-(170.0+1090.0)*CLHEP::um, -2345.0*CLHEP::um, 0)
    };

for (size_t k = 0; k < gatePositions.size(); ++k) {
        G4String gateName = pName + "_Gate_" + std::to_string(k);
        
        // Build Pad identical to transmission line tapers
        G4Trd* gatePad = new G4Trd(gateName + "Metal", 150.0*CLHEP::um/2.0, 10.0*CLHEP::um/2.0, dp_groundPlaneDimZ/2.0, dp_groundPlaneDimZ/2.0, 200.0*CLHEP::um/2.0);
        G4LogicalVolume* log_Gate = new G4LogicalVolume(gatePad, aluminum, gateName + "_Log");
        
        // Proper orientation facing transmission line (y=0)
        G4RotationMatrix* rotGate = new G4RotationMatrix();
        if (gatePositions[k].y() > 0) {
            rotGate->rotateZ(CLHEP::pi); // Point downwards
        }
        
        // Capture pointer to physical volume here:
        G4VPhysicalVolume* gatePhys = new G4PVPlacement(rotGate, gatePositions[k], log_Gate, gateName + "_Phys", pMotherLogical, pMany, pCopyNo, pSurfChk);

        // Track physical volume for G4CMP boundary physics
        fFundamentalVolumeList.push_back(std::make_tuple("Aluminum", gateName, gatePhys));
    }
}

std::vector<std::tuple<std::string, G4String, G4VPhysicalVolume*>> 
QuasiparticleResonatorAssembly::GetListOfAllFundamentalSubVolumes()
{
    return fFundamentalVolumeList;
}