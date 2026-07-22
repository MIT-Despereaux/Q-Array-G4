#include "DetectorConstruction_DSPX.hh"
#include "Geometry/CryostatBuilder.hh"
#include "Metadata.hh"
#include "SensitiveDetector.hh"
#include "G4Tubs.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4PhysicalVolumeStore.hh" // <-- Added to search for the crystal
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4VisAttributes.hh"

#include "G4SDManager.hh"
#include "SensitiveDetector.hh"
#include "G4LogicalVolumeStore.hh"

#include <cctype>

#ifdef USE_G4CMP
  #include "G4LatticeManager.hh"
  #include "G4LatticePhysical.hh"
#endif

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

      return worldPhysical;
    }

  void DetectorConstruction::ConstructSDandField()
  {
#ifdef USE_G4CMP
    auto* LM = G4LatticeManager::GetLatticeManager();
    auto meta = Metadata::GetInstance();
    auto* store = G4PhysicalVolumeStore::GetInstance();

    // Safe multi-threaded lookup matching dynamic macro configurations
    if (meta->GetBool("/QR/geom/useQubitArray"))
    {
      // 1. Map tracking substrate
      auto* physSiliconChip = store->GetVolume("SiliconChip", false);
      if (physSiliconChip) {
        auto* latSilicon = LM->LoadLattice(physSiliconChip->GetLogicalVolume()->GetMaterial(), "Si");
        auto* crystalSilicon = new G4LatticePhysical(latSilicon);
        crystalSilicon->SetMillerOrientation(1, 0, 0);
        LM->RegisterLattice(physSiliconChip, crystalSilicon);
      }

      // 2. Map copper housing structure 
      // *** COMMENTED OUT TO PREVENT PHONON CRASH IN MACROSCOPIC METAL ***
      /*
      auto* physQubitHousing = store->GetVolume("QubitHousing", false);
      if (physQubitHousing) {
        auto* latHousing = LM->LoadLattice(physQubitHousing->GetLogicalVolume()->GetMaterial(), "Cu");
        auto* crystalHousing = new G4LatticePhysical(latHousing);
        crystalHousing->SetMillerOrientation(1, 0, 0);
        LM->RegisterLattice(physQubitHousing, crystalHousing);
      }
      */

      // 3. Map dynamic superconducting ground plane(s) - UPDATED FOR MULTIPLE EXTRUDED VOLUMES
      for (auto* physVol : *store) {
          G4String pName = physVol->GetName();
          // This will match "GroundPlane", "GroundPlane_Extruded_Phys_0", etc.
          if (pName.find("GroundPlane") != std::string::npos) {
              using namespace QuasiparticleDetectorParameters;
              auto* latGround = LM->LoadLattice(physVol->GetLogicalVolume()->GetMaterial(), "Al");
              auto* crystalGround = new G4LatticePhysical(latGround, dp_polycryElScatMFP_Al, dp_scDelta0_Al, dp_scTeff_Al, dp_scDn_Al, dp_scTauQPTrap_Al);
              crystalGround->SetMillerOrientation(1, 0, 0);
              LM->RegisterLattice(physVol, crystalGround);
          }
      }
    else
    {
      // Classic Calibration Run Fallback Mode
      auto* crystalPhys = store->GetVolume("DetectorSnCubePhysical", false);
      if (crystalPhys) {
        auto* latticeLogical = LM->LoadLattice(crystalPhys->GetLogicalVolume()->GetMaterial(), "Si");
        auto* crystalLattice = new G4LatticePhysical(latticeLogical);
        crystalLattice->SetMillerOrientation(1, 0, 0);
        LM->RegisterLattice(crystalPhys, crystalLattice);
      }
    }
#endif

// 1. Fetch the Logical Volume Store and SD Manager
    auto* lvStore = G4LogicalVolumeStore::GetInstance();
    auto* sdm = G4SDManager::GetSDMpointer();

    // 2. Keep your existing Silicon setup (assuming it works fine)
    auto* siliconSD = new QArray::SensitiveDetector("SiliconSD", true, "SiliconHits");
    sdm->AddNewDetector(siliconSD);
    auto* log_siliconChip = lvStore->GetVolume("SiliconChip_log", false);
    if (log_siliconChip) {
        log_siliconChip->SetSensitiveDetector(siliconSD);
    }

    // 3. Dynamically assign unique SDs to all Ground, Transmission, and Resonator volumes
    for (auto* lv : *lvStore) 
    {
        G4String name = lv->GetName();
        
        // Skip SiliconChip_log since we just handled it
        if (name == "SiliconChip_log") continue;

        // If the logical volume name contains any of our target keywords...
        // If the logical volume name contains any of our target keywords...
        if (name.find("GroundPlane") != std::string::npos ||
            name.find("TransmissionLine") != std::string::npos ||
            name.find("ResonatorAssembly") != std::string::npos ||
            name.find("Chip_Gate") != std::string::npos) // <-- ADD THIS  
        {
            // Make sure we haven't already assigned an SD to this volume
            if (lv->GetSensitiveDetector() == nullptr) 
            {
                // Create a completely unique SD specifically for this logical volume
                G4String sdName = name + "_SD";
                G4String hcName = name + "_Hits";
                
                auto* uniqueSD = new QArray::SensitiveDetector(sdName, true, hcName);
                sdm->AddNewDetector(uniqueSD);
                lv->SetSensitiveDetector(uniqueSD);
            }
        }
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

      // CONFIGURABLE SYSTEM FLAG REGISTRATION
      meta->AddParamCommand<G4UIcmdWithABool>("/QR/geom/useQubitArray",
                                              "Toggle active target architecture: True = Qubit Matrix, False = Bulk Calibration Block",
                                              "useQubitArray");
      meta->Set("/QR/geom/useQubitArray", false); 

      meta->AddParamCommand<G4UIcmdWithABool>("/QR/geom/isHorizontal",
                                              "Toggle orientation: True = Horizonal, False = Vertical",
                                              "isHorizontal");
      meta->Set("/QR/geom/isHorizontal", true); 

      meta->AddParamCommand<G4UIcmdWithABool>("/QR/physics/killLowEnergyPhonons",
                                              "Toggle <2*Gap phonons existing: True = reaper, False = No reaper",
                                              "killLowEnergyPhonons");
      meta->Set("/QR/physics/killLowEnergyPhonons", false); 
    } 
} // <-- THIS IS THE FINAL CLOSING BRACKET OF THE FILE