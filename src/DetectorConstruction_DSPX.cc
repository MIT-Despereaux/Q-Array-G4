#include "DetectorConstruction.hh"
#include "Geometry/CryostatBuilder.hh"
#include "Metadata.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4VisAttributes.hh"

namespace QArray
{
  namespace
  {
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
                        meta->GetThreeVector("/QR/geom/globalOffset") + G4ThreeVector(0., 0., 1.0 * m),
                        mFridgeLogical,
                        "fridgePhysical",
                        mWorldLogical,
                        false,
                        0,
                        checkOverlaps);
    }

    return worldPhysical;
  }

  void DetectorConstruction::ConstructSDandField()
  {
  }

  void DetectorConstruction::ConstructFridge()
  {
    Geometry::CryostatBuilder builder;
    builder.SetCheckOverlaps(true);
    auto volumes = builder.Build();
    mFridgeLogical = volumes.fridgeLogical;
    mStageOVCVacLogical = volumes.ovcVacuumLogical;
    mChipLogical = volumes.mixingChamberLogical;
    fMXCStageZ = volumes.plate10mKCenter.z();
  }

  void DetectorConstruction::ConstructLab()
  {
    auto meta = Metadata::GetInstance();
    auto* concrete = mNistManager->FindOrBuildMaterial("G4_CONCRETE");
    constexpr G4double labX = 7.8 * m;
    constexpr G4double labY = 6.63 * m;
    constexpr G4double labZ = 4.0 * m;
    constexpr G4double labT = 0.5 * m;

    auto* labLogical = ConstructHollowBox("lab",
                                          concrete,
                                          new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 1.0)),
                                          labX,
                                          labY,
                                          labZ,
                                          labT);

    new G4PVPlacement(nullptr,
                      meta->GetThreeVector("/QR/geom/globalOffset") +
                          G4ThreeVector(0., 0., (labZ - 2. * labT) / 2.),
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
