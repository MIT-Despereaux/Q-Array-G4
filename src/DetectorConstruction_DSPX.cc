#include "DetectorConstruction_DSPX.hh"
#include "Geometry/CryostatBuilder.hh"
#include "Metadata.hh"
#include "SensitiveDetector.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4VisAttributes.hh"

#include <cctype>

namespace QArray
{
  namespace
  {
    constexpr G4double kLabInteriorHeight = 3.5 * m;
    constexpr G4double kLabInteriorWidth = 8.25 * m;
    constexpr G4double kLabInteriorLength = 7.4 * m;
    constexpr G4double kLabWallThickness = 5.0 * 2.54 * cm;
    constexpr G4double kFridgeRightWallDistance = 160. * cm;
    constexpr G4double kFridgeBottomWallDistance = 480. * cm;
    constexpr G4double kFridgeCenterHeight = 1760.6 * mm;

    G4ThreeVector LabCenter()
    {
      return G4ThreeVector(0., 0., kLabInteriorHeight / 2.);
    }

    G4ThreeVector FridgeCenterInLab()
    {
      const auto x = kLabInteriorWidth / 2. - kFridgeRightWallDistance;
      const auto y = -kLabInteriorLength / 2. + kFridgeBottomWallDistance;
      return G4ThreeVector(x, y, kFridgeCenterHeight);
    }

    G4LogicalVolume* ConstructHollowBox(const G4String& name,
                                        G4Material* material,
                                        G4VisAttributes* vis,
                                        G4double dx,
                                        G4double dy,
                                        G4double dz,
                                        G4double thickness)
    {
      auto* outer = new G4Box(name + "Outer", dx / 2., dy / 2., dz / 2.);
      auto* inner = new G4Box(name + "Inner",
                              dx / 2. - thickness,
                              dy / 2. - thickness,
                              dz / 2. - thickness);
      auto* solid = new G4SubtractionSolid(name, outer, inner);
      auto* logical = new G4LogicalVolume(solid, material, name + "Logical");
      logical->SetVisAttributes(vis);
      return logical;
    }

    G4LogicalVolume* ConstructLabWithOpening(G4Material* material, G4VisAttributes* vis)
    {
      constexpr G4double labX = kLabInteriorWidth + 2. * kLabWallThickness;
      constexpr G4double labY = kLabInteriorLength + 2. * kLabWallThickness;
      constexpr G4double labZ = kLabInteriorHeight + 2. * kLabWallThickness;

      auto* outer = new G4Box("labOuter", labX / 2., labY / 2., labZ / 2.);
      auto* inner = new G4Box("labInner",
                              kLabInteriorWidth / 2.,
                              kLabInteriorLength / 2.,
                              kLabInteriorHeight / 2.);
      auto* shell = new G4SubtractionSolid("labShell", outer, inner);

      constexpr G4double openingYLength = 4.3 * m;
      constexpr G4double openingZHeight = 2.17 * m;
      constexpr G4double openingXThickness = 2. * kLabWallThickness;
      const G4double openingXCenter = kLabInteriorWidth / 2. + kLabWallThickness / 2.;
      const G4double openingYCenter = kLabInteriorLength / 2. - 0.9 * m - openingYLength / 2.;
      const G4double openingZCenter = -kLabInteriorHeight / 2. + openingZHeight / 2.;

      auto* opening = new G4Box("labOpening",
                                openingXThickness / 2.,
                                openingYLength / 2.,
                                openingZHeight / 2.);
      auto* labSolid =
          new G4SubtractionSolid("lab", shell, opening, nullptr, G4ThreeVector(openingXCenter, openingYCenter, openingZCenter));
      auto* logical = new G4LogicalVolume(labSolid, material, "labLogical");
      logical->SetVisAttributes(vis);
      return logical;
    }

    G4String ScoringDetectorName(const G4String& logicalName)
    {
      G4String name = "dspx_";
      for (const char ch : logicalName)
      {
        name += std::isalnum(static_cast<unsigned char>(ch)) ? ch : '_';
      }
      return name;
    }
  }

  DetectorConstruction::DetectorConstruction()
      : G4VUserDetectorConstruction()
  {
    mNistManager = G4NistManager::Instance();
    DefineCommands();
  }

  DetectorConstruction::~DetectorConstruction()
  {
    delete mMessenger;
  }

  G4VPhysicalVolume* DetectorConstruction::Construct()
  {
    auto meta = Metadata::GetInstance();
    auto* air = mNistManager->FindOrBuildMaterial("G4_AIR");

    constexpr G4bool checkOverlaps = true;
    constexpr G4double worldSize = 30.0 * m;

    auto* worldSolid = new G4Box("worldBox", worldSize / 2., worldSize / 2., worldSize / 2.);
    mWorldLogical = new G4LogicalVolume(worldSolid, air, "worldLogical");

    auto* worldPhysical = new G4PVPlacement(nullptr,
                                            G4ThreeVector(),
                                            mWorldLogical,
                                            "worldPhysical",
                                            nullptr,
                                            false,
                                            0,
                                            checkOverlaps);

    if (meta->GetBool("/QR/geom/constructLab"))
    {
      ConstructLab();
    }

    if (meta->GetBool("/QR/geom/constructFridge"))
    {
      ConstructFridge();
      new G4PVPlacement(nullptr,
                        meta->GetThreeVector("/QR/geom/globalOffset") + FridgeCenterInLab(),
                        mFridgeLogical,
                        "fridgePhysical",
                        mWorldLogical,
                        false,
                        0,
                        checkOverlaps);
    }
// =======================================================
    // HARDCODED GPS VISUAL TRACKING VOLUME
    // =======================================================
    // Dimensions matching your macro: radius = 5 cm, half-z = 10 cm
    auto* solidVisualSource = new G4Tubs("GPS_Visual_Solid", 
                                         0.*cm, 
                                         1.12*cm, 
                                         1.55*cm, 
                                         0.*deg, 
                                         360.*deg);
    
    // Match the background material (Air) so it has no physical effect on tracking
    auto* logicalVisualSource = new G4LogicalVolume(solidVisualSource, 
                                                    air, 
                                                    "GPS_Visual_Logical");

    // Make it a transparent cyan wireframe box
    auto* visAtt = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0, 0.4)); // Cyan with 40% opacity
    visAtt->SetForceWireframe(true);
    logicalVisualSource->SetVisAttributes(visAtt);

    // Hardcoded center coordinates matching your macro: (0., -32., -12.7) cm
    G4ThreeVector hardcodedSourcePos(0.*cm, -27.5*cm, -12.7*cm);

    // Place it directly inside the world logical volume
    new G4PVPlacement(nullptr,
                      hardcodedSourcePos,
                      logicalVisualSource,
                      "GPS_Visual_Physical",
                      mWorldLogical,
                      false,
                      0,
                      false); // Overlap checking is false so it doesn't trigger warnings
    // =======================================================

    return worldPhysical;
  }

  void DetectorConstruction::ConstructSDandField()
  {
    for (auto* logical : *G4LogicalVolumeStore::GetInstance())
    {
      const auto& logicalName = logical->GetName();
      const auto& materialName = logical->GetMaterial()->GetName();
      const auto detectorName = ScoringDetectorName(logicalName);
      const G4bool registrationOnly =
          materialName == "G4_AIR" || materialName == "G4_Galactic";

      logical->SetSensitiveDetector(new SensitiveDetector(detectorName));
      G4cout << "DSPX_SCORING logical=" << logicalName
             << " detector=" << detectorName
             << " material=" << materialName
             << " requirement=" << (registrationOnly ? "registration" : "positive")
             << G4endl;
    }
  }

  void DetectorConstruction::ConstructFridge()
  {
    Geometry::CryostatBuilder builder;
    builder.SetCheckOverlaps(true);
    auto meta = Metadata::GetInstance();
    builder.SetAddPb(meta->GetBool("/QR/geom/addPb"));
    auto volumes = builder.Build();
    mFridgeLogical = volumes.fridgeLogical;
    mStageOVCVacLogical = volumes.ovcVacuumLogical;
    fMXCStageZ = volumes.plate10mKCenter.z();
  }

  void DetectorConstruction::ConstructLab()
  {
    auto meta = Metadata::GetInstance();
    auto* concrete = mNistManager->FindOrBuildMaterial("G4_CONCRETE");
    auto* labLogical = ConstructLabWithOpening(concrete, new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.35)));

    new G4PVPlacement(nullptr,
                      meta->GetThreeVector("/QR/geom/globalOffset") + LabCenter(),
                      labLogical,
                      "labPhysical",
                      mWorldLogical,
                      false,
                      0,
                      true);
  }

  void DetectorConstruction::ConstructTable(int, bool, G4ThreeVector)
  {
  }

  void DetectorConstruction::AddExpPad(int)
  {
  }

  void DetectorConstruction::DefineCommands()
  {
    mMessenger = new G4GenericMessenger(this, "/QR/geom/", "Geometry control");

    auto meta = Metadata::GetInstance();
    auto* globalOffset = meta->AddParamCommand<G4UIcmdWith3VectorAndUnit>(
        "/QR/geom/globalOffset", "Global offset for everything.");
    globalOffset->SetUnitCategory("Length");
    meta->Set("/QR/geom/globalOffset", G4ThreeVector(0, 0, 0));

    meta->AddParamCommand<G4UIcmdWithAnInteger>("/QR/geom/tableVer",
                                                "Geometry or table version",
                                                "tableVer");
    meta->Set("/QR/geom/tableVer", 20230601);

    meta->AddParamCommand<G4UIcmdWithABool>("/QR/geom/constructLab",
                                            "Construct Lab?",
                                            "cLab");
    meta->Set("/QR/geom/constructLab", true);

    meta->AddParamCommand<G4UIcmdWithABool>("/QR/geom/constructFridge",
                                            "Construct Fridge?",
                                            "cFridge");
    meta->Set("/QR/geom/constructFridge", true);

    meta->AddParamCommand<G4UIcmdWithABool>("/QR/geom/addPb",
                                            "Add Pb?",
                                            "addPb");
    meta->Set("/QR/geom/addPb", false);
  }
}
