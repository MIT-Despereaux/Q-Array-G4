#include "Geometry/CryostatBuilder.hh"
#include "Geometry/Shapes.hh"

#include "G4SDManager.hh"
#include "SensitiveDetector.hh"

#include "CADMesh.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4ios.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PhysicalConstants.hh"
#include "G4PVPlacement.hh"
#include "G4RotationMatrix.hh"
#include "G4SystemOfUnits.hh"
#include "G4TessellatedSolid.hh"
#include "G4Tubs.hh"
#include "G4UnionSolid.hh"
#include "G4VisAttributes.hh"
#include "G4VisExtent.hh"
#include "G4VSolid.hh"
#include <array>
#include <cmath>
#include <filesystem>
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalBorderSurface.hh"  // <-- IMPORTANT: Added for precise volume-to-volume interface
#include "G4OpticalSurface.hh"

#include "Metadata.hh"
#include "QuasiparticleDetectorParameters.hh"
#include "QuasiparticleSensitivity.hh"
#include "QuasiparticleQubitHousing.hh"
#include "QuasiparticleTransmissionLine.hh"
#include "QuasiparticleResonatorAssembly.hh"
#include "G4CMPLogicalBorderSurface.hh"
#include "G4CMPSurfaceProperty.hh"
#include "G4LatticeManager.hh"
#include "G4LatticeLogical.hh"
#include "G4PhysicalVolumeStore.hh"

#include "G4ExtrudedSolid.hh"

namespace QArray::Geometry
{
  namespace
  {
    struct PlateSpec
    {
      const char* name;
      G4double radius;
      G4double thickness;
      G4double zBottom;
    };

    struct ScreenSpec
    {
      const char* name;
      G4double innerRadius;
      G4double thickness;
      G4double height;
      G4double zInnerBottom;
    };

    G4VisAttributes* Vis(const G4Colour& colour)
    {
      return new G4VisAttributes(colour);
    }

    void Place(G4LogicalVolume* logical,
               const G4String& name,
               G4LogicalVolume* mother,
               const G4ThreeVector& position,
               G4bool checkOverlaps)
    {
      new G4PVPlacement(nullptr, position, logical, name, mother, false, 0, checkOverlaps);
    }

    G4Material* BuildMuMetal(G4NistManager* nist)
    {
      if (auto* existing = G4Material::GetMaterial("MuMetal", false))
      {
        return existing;
      }

      auto* muMetal = new G4Material("MuMetal", 8.7 * g / cm3, 3);
      muMetal->AddElement(nist->FindOrBuildElement("Ni"), 0.80);
      muMetal->AddElement(nist->FindOrBuildElement("Mo"), 0.05);
      muMetal->AddElement(nist->FindOrBuildElement("Fe"), 0.15);
      return muMetal;
    }

    constexpr G4double kOvcInnerRadius = 37.1 * cm / 2.;
    constexpr G4double kOvcOuterCircumference = 120.5 * cm;
    constexpr G4double kOvcOuterRadius = kOvcOuterCircumference / (2. * pi);
    constexpr G4double kOvcWallThickness = kOvcOuterRadius - kOvcInnerRadius;
    constexpr G4double kOvcHeight = 103. * cm;
    constexpr G4double kOvcTopZ = 654.40 * mm;
    constexpr G4double kOvcInnerBottomZ = kOvcTopZ - kOvcHeight;
    constexpr G4double kOvcVacuumCenterZ = kOvcInnerBottomZ + kOvcHeight / 2.;
    constexpr G4double kOvcTopRingOuterRadius = 41.6 * cm / 2.;
    constexpr G4double kOvcTopRingHeight = 2.7 * cm;

    constexpr G4double kScreen1KInnerRadius = 26.1 * cm / 2.;
    constexpr G4double kInch = 25.4 * mm;
    constexpr G4double kScreen1KWallThickness = 0.04 * kInch;
    constexpr G4double kScreen1KTotalHeight = 48.5 * cm;
    constexpr G4double kScreen1KHeight = kScreen1KTotalHeight - kScreen1KWallThickness;
    constexpr G4double kScreen1KTopZ = 181.20 * mm;
    constexpr G4double kScreen1KInnerBottomZ = kScreen1KTopZ - kScreen1KHeight;
    constexpr G4double kScreen1KFlangeOuterRadius = 28.2 * cm / 2.;
    constexpr G4double kScreen1KTopAlRingThickness = 0.09 * kInch;
    constexpr G4double kScreen1KMuMetalFlangeThickness = kScreen1KWallThickness;
    constexpr G4double kScreen1KBottomAlRingThickness = 0.375 * kInch;

    constexpr G4double kScreen4KInnerRadius = 30.0 * cm / 2.;
    constexpr G4double kScreen4KWallThickness = 2.0 * mm;
    constexpr G4double kScreen4KHeight = 62.3 * cm;
    constexpr G4double kScreen4KTopZ = 313.20 * mm;
    constexpr G4double kScreen4KInnerBottomZ = kScreen4KTopZ - kScreen4KHeight;
    constexpr G4double kScreen4KFlangeOuterRadius = 32.5 * cm / 2.;
    constexpr G4double kScreen4KFlangeThickness = 8.0 * mm;

    constexpr G4double kScreen50KInnerRadius = 33.5 * cm / 2.;
    constexpr G4double kScreen50KWallThickness = 2.0 * mm;
    constexpr G4double kScreen50KHeight = 84.0 * cm;
    constexpr G4double kScreen50KTopZ = 522.70 * mm;
    constexpr G4double kScreen50KInnerBottomZ = kScreen50KTopZ - kScreen50KHeight;
    constexpr G4double kScreen50KFlangeOuterRadius = 36.0 * cm / 2.;
    constexpr G4double kScreen50KFlangeThickness = 1.3 * cm;

    G4VSolid* ConstructScreenSolid(const ScreenSpec& screen)
    {
      if (screen.name == G4String("Screen1K"))
      {
        auto* bucket = new Bucket("Screen1KBucket", screen.innerRadius, screen.thickness, screen.height);
        auto* muMetalFlange = new G4Tubs("Screen1KMuMetalFlange",
                                         screen.innerRadius,
                                         kScreen1KFlangeOuterRadius,
                                         kScreen1KMuMetalFlangeThickness / 2.,
                                         0.,
                                         twopi);
        return new G4UnionSolid("Screen1K",
                                bucket,
                                muMetalFlange,
                                nullptr,
                                G4ThreeVector(0.,
                                              0.,
                                              screen.height - kScreen1KTopAlRingThickness -
                                                  kScreen1KMuMetalFlangeThickness / 2.));
      }

      if (screen.name == G4String("Screen4K"))
      {
        auto* bucket = new Bucket("Screen4KBucket", screen.innerRadius, screen.thickness, screen.height);
        auto* topFlange = new G4Tubs("Screen4KTopFlange",
                                     screen.innerRadius,
                                     kScreen4KFlangeOuterRadius,
                                     kScreen4KFlangeThickness / 2.,
                                     0.,
                                     twopi);
        return new G4UnionSolid("Screen4K",
                                bucket,
                                topFlange,
                                nullptr,
                                G4ThreeVector(0., 0., screen.height - kScreen4KFlangeThickness / 2.));
      }

      if (screen.name != G4String("Screen50K"))
      {
        return new Bucket(screen.name, screen.innerRadius, screen.thickness, screen.height);
      }

      auto* bucket = new Bucket("Screen50KBucket", screen.innerRadius, screen.thickness, screen.height);
      auto* topFlange = new G4Tubs("Screen50KTopFlange",
                                   screen.innerRadius,
                                   kScreen50KFlangeOuterRadius,
                                   kScreen50KFlangeThickness / 2.,
                                   0.,
                                   twopi);
      return new G4UnionSolid("Screen50K",
                              bucket,
                              topFlange,
                              nullptr,
                              G4ThreeVector(0., 0., screen.height - kScreen50KFlangeThickness / 2.));
    }
  }

  void CryostatBuilder::SetCheckOverlaps(G4bool value)
  {
    mCheckOverlaps = value;
  }

  void CryostatBuilder::SetVerbose(G4int value)
  {
    mVerbose = value;
  }

  void CryostatBuilder::SetAddPb(G4bool value)
  {
    mAddPb = value;
  }

  void CryostatBuilder::SetMeshDataPath(const G4String& path)
  {
    mMeshDataPath = path;
  }

  CryostatVolumes CryostatBuilder::Build()
  {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* copper = nist->FindOrBuildMaterial("G4_Cu");
    auto* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");
    auto* aluminum = nist->FindOrBuildMaterial("G4_Al");
    auto* muMetal = BuildMuMetal(nist);

    constexpr G4double containerXY = 1.0 * m;
    constexpr G4double containerZ = 1.8 * m;  // extended to accommodate Pb base layers
    constexpr G4double containerCenterZ = 315. * mm;

    auto* fridgeSolid = new G4Box("dspxFridgeBox", containerXY / 2., containerXY / 2., containerZ / 2.);
    auto* fridgeLogical = new G4LogicalVolume(fridgeSolid, air, "fridgeLogical");
    fridgeLogical->SetVisAttributes(new G4VisAttributes(false));

    CryostatVolumes volumes;
    volumes.fridgeLogical = fridgeLogical;
    volumes.plate10mKCenter = G4ThreeVector(0., 0., 2. * mm - containerCenterZ);

    const G4Colour copperColour(184 / 255., 115 / 255., 51 / 255.);
    const G4Colour screenColour(180 / 255., 125 / 255., 70 / 255., 0.35);
    const G4Colour steelColour(137 / 255., 148 / 255., 153 / 255.);
    const G4Colour aluminumColour(80 / 255., 140 / 255., 240 / 255., 0.45);
    const G4Colour muMetalColour(70 / 255., 180 / 255., 170 / 255., 0.45);
    const G4Colour vacuumColour(0., 0., 1., 0.08);

    const PlateSpec plates[] = {
        {"Plate10mK", 125.0 * mm, 4.0 * mm, 0.0 * mm},
        {"Plate100mK", 125.0 * mm, 3.0 * mm, 74.20 * mm},
        {"Plate1K", 142.0 * mm, 3.0 * mm, 181.20 * mm},
        {"Plate4K", 161.5 * mm, 6.0 * mm, 313.20 * mm},
        {"Plate50K", 180.0 * mm, 6.0 * mm, 522.70 * mm},
    };

    constexpr G4double ovcVacuumEpsilon = 0.001 * mm;

    auto* ovcVacuumSolid = new G4Tubs("ScreenOVCVacuum",
                                      0.,
                                      kOvcInnerRadius - ovcVacuumEpsilon,
                                      kOvcHeight / 2. - ovcVacuumEpsilon,
                                      0.,
                                      twopi);
    auto* ovcVacuumLogical = new G4LogicalVolume(ovcVacuumSolid, vacuum, "stageOVCVacLogical");
    ovcVacuumLogical->SetVisAttributes(Vis(vacuumColour));
    volumes.ovcVacuumLogical = ovcVacuumLogical;

    const ScreenSpec screens[] = {
        {"Screen1K", kScreen1KInnerRadius, kScreen1KWallThickness, kScreen1KHeight, kScreen1KInnerBottomZ},
        {"Screen4K", kScreen4KInnerRadius, kScreen4KWallThickness, kScreen4KHeight, kScreen4KInnerBottomZ},
        {"Screen50K", kScreen50KInnerRadius, kScreen50KWallThickness, kScreen50KHeight, kScreen50KInnerBottomZ},
        {"ScreenOVC", kOvcInnerRadius, kOvcWallThickness, kOvcHeight, kOvcInnerBottomZ},
    };

    for (const auto& screen : screens)
    {
      auto* solid = ConstructScreenSolid(screen);
      const G4bool useSteelMaterial = screen.name == G4String("ScreenOVC");
      const G4bool useMuMetalMaterial = screen.name == G4String("Screen1K");
      auto* logical = new G4LogicalVolume(solid,
                                          useMuMetalMaterial ? muMetal : (useSteelMaterial ? steel : copper),
                                          G4String(screen.name) + "Logical");
      logical->SetVisAttributes(Vis(useMuMetalMaterial ? muMetalColour : (useSteelMaterial ? steelColour : screenColour)));
      Place(logical,
            G4String(screen.name) + "Physical",
            screen.name == G4String("ScreenOVC") ? fridgeLogical : ovcVacuumLogical,
            G4ThreeVector(0.,
                          0.,
                          screen.name == G4String("ScreenOVC")
                              ? screen.zInnerBottom - containerCenterZ
                              : screen.zInnerBottom - kOvcVacuumCenterZ),
            mCheckOverlaps);

      if (screen.name == G4String("ScreenOVC"))
      {
        volumes.ovcLogical = logical;
      }
    }

    auto* screen1KAlRingSolid = new G4Tubs("Screen1KAlRing",
                                           kScreen1KInnerRadius + kScreen1KWallThickness,
                                           kScreen1KFlangeOuterRadius,
                                           kScreen1KTopAlRingThickness / 2.,
                                           0.,
                                           twopi);
    auto* screen1KTopAlRingLogical = new G4LogicalVolume(screen1KAlRingSolid,
                                                         aluminum,
                                                         "Screen1KTopAlRingLogical");
    screen1KTopAlRingLogical->SetVisAttributes(Vis(aluminumColour));
    Place(screen1KTopAlRingLogical,
          "Screen1KTopAlRingPhysical",
          ovcVacuumLogical,
          G4ThreeVector(0.,
                        0.,
                        kScreen1KTopZ - kScreen1KTopAlRingThickness / 2. - kOvcVacuumCenterZ),
          mCheckOverlaps);

    auto* screen1KBottomAlRingSolid = new G4Tubs("Screen1KBottomAlRing",
                                                 kScreen1KInnerRadius + kScreen1KWallThickness,
                                                 kScreen1KFlangeOuterRadius,
                                                 kScreen1KBottomAlRingThickness / 2.,
                                                 0.,
                                                 twopi);
    auto* screen1KBottomAlRingLogical = new G4LogicalVolume(screen1KBottomAlRingSolid,
                                                            aluminum,
                                                            "Screen1KBottomAlRingLogical");
    screen1KBottomAlRingLogical->SetVisAttributes(Vis(aluminumColour));
    Place(screen1KBottomAlRingLogical,
          "Screen1KBottomAlRingPhysical",
          ovcVacuumLogical,
          G4ThreeVector(0.,
                        0.,
                        kScreen1KTopZ - kScreen1KTopAlRingThickness -
                            kScreen1KMuMetalFlangeThickness -
                            kScreen1KBottomAlRingThickness / 2. - kOvcVacuumCenterZ),
          mCheckOverlaps);

    Place(ovcVacuumLogical,
          "stageOVCVacPhysical",
          fridgeLogical,
          G4ThreeVector(0., 0., kOvcVacuumCenterZ - containerCenterZ),
          mCheckOverlaps);

    for (const auto& plate : plates)
    {
      auto* solid = new FilledTube(plate.name, plate.radius, plate.thickness);
      auto* logical = new G4LogicalVolume(solid, copper, G4String(plate.name) + "Logical");
      logical->SetVisAttributes(Vis(copperColour));
      Place(logical,
            G4String(plate.name) + "Physical",
            ovcVacuumLogical,
            G4ThreeVector(0., 0., plate.zBottom + plate.thickness / 2. - kOvcVacuumCenterZ),
            mCheckOverlaps);

      if (plate.name == G4String("Plate10mK"))
      {
        volumes.mixingChamberLogical = logical;
      }
    }

    constexpr G4double brideInnerRadius  = 187.0 * mm;
    constexpr G4double brideCircumRadius = 240.2 * mm;
    constexpr G4double brideInnerHeight  = 97.0 * mm;
    constexpr G4double brideTopThickness = 14.0 * mm;
    constexpr G4double brideBottomZ      = kOvcTopZ;
    constexpr G4double brideVacEpsilon   = 0.1 * mm;

    auto* brideSolid = new HexBride("Bride",
                                    brideInnerRadius,
                                    brideCircumRadius,
                                    brideInnerHeight,
                                    brideTopThickness);
    auto* brideLogical = new G4LogicalVolume(brideSolid, steel, "BrideLogical");
    brideLogical->SetVisAttributes(Vis(steelColour));
    auto* brideRotation = new G4RotationMatrix();
    brideRotation->rotateZ(30. * deg);
    new G4PVPlacement(brideRotation,
                      G4ThreeVector(0., 0., brideBottomZ - containerCenterZ),
                      brideLogical,
                      "BridePhysical",
                      fridgeLogical,
                      false,
                      0,
                      mCheckOverlaps);

    auto* brideBoreVacSolid = new G4Tubs("BrideBoreVacuum",
                                         0.,
                                         brideInnerRadius - brideVacEpsilon,
                                         (brideInnerHeight - brideVacEpsilon) / 2.,
                                         0.,
                                         twopi);
    auto* brideBoreVacLogical = new G4LogicalVolume(brideBoreVacSolid, vacuum, "BrideBoreVacLogical");
    brideBoreVacLogical->SetVisAttributes(Vis(vacuumColour));
    Place(brideBoreVacLogical,
          "BrideBoreVacPhysical",
          fridgeLogical,
          G4ThreeVector(0., 0., brideBottomZ + brideInnerHeight / 2. - containerCenterZ),
          mCheckOverlaps);

    auto* ovcTopRingSolid = new G4Tubs("OVCTopRing",
                                       kOvcOuterRadius,
                                       kOvcTopRingOuterRadius,
                                       kOvcTopRingHeight / 2.,
                                       0.,
                                       twopi);
    auto* ovcTopRingLogical = new G4LogicalVolume(ovcTopRingSolid, steel, "OVCTopRingLogical");
    ovcTopRingLogical->SetVisAttributes(Vis(steelColour));
    Place(ovcTopRingLogical,
          "OVCTopRingPhysical",
          fridgeLogical,
          G4ThreeVector(0., 0., kOvcTopZ - kOvcTopRingHeight / 2. - containerCenterZ),
          mCheckOverlaps);

    if (mVerbose > 0)
    {
      G4cout << "Built DSPX cryostat geometry in fridgeLogical" << G4endl;
    }

    BuildMeshComponents(volumes);

    // -----------------------------------------------------------------------
    // Lead bricks around the OVC
    // -----------------------------------------------------------------------
    if (mAddPb)
    {
      auto* lead = nist->FindOrBuildMaterial("G4_Pb");
      const G4Colour leadColour(50 / 255., 155 / 255., 36 / 255.);

      constexpr G4double pbLong  = 8. * 25.4 * mm;
      constexpr G4double pbMid   = 4. * 25.4 * mm;
      constexpr G4double pbShort = 2. * 25.4 * mm;

      constexpr G4double ovcBaseZ_local   = kOvcInnerBottomZ - containerCenterZ;
      constexpr G4double ovcTrueBot_local = ovcBaseZ_local - kOvcWallThickness - 0.1 * mm;

      constexpr G4double pbRingR1  = kOvcOuterRadius + 5. * mm + pbMid / 2.;
      constexpr G4double pbRingR2  = 305. * mm;
      constexpr G4int    pbNLayers = 8;

      auto* pbRingBrickSolid   = new G4Box("PbRingBrick", pbMid / 2., pbLong / 2., pbShort / 2.);
      auto* pbRingBrickLogical = new G4LogicalVolume(pbRingBrickSolid, lead, "PbRingBrickLogical");
      pbRingBrickLogical->SetVisAttributes(Vis(leadColour));

      for (G4int iLayer = 0; iLayer < pbNLayers; ++iLayer)
      {
        const G4double layerCenterZ = ovcBaseZ_local + (iLayer + 0.5) * pbShort;

        for (G4int i = 0; i < 4; ++i)
        {
          const G4double angle = i * 90. * deg;
          auto* rot = new G4RotationMatrix();
          rot->rotateZ(angle);
          new G4PVPlacement(rot,
                            G4ThreeVector(pbRingR1 * std::cos(angle),
                                          pbRingR1 * std::sin(angle),
                                          layerCenterZ),
                            pbRingBrickLogical,
                            "PbRingBrickPhysical",
                            fridgeLogical,
                            false,
                            iLayer * 8 + i,
                            mCheckOverlaps);
        }

        for (G4int i = 0; i < 4; ++i)
        {
          const G4double angle = (45. + i * 90.) * deg;
          auto* rot = new G4RotationMatrix();
          rot->rotateZ(-angle);
          new G4PVPlacement(rot,
                            G4ThreeVector(pbRingR2 * std::cos(angle),
                                          pbRingR2 * std::sin(angle),
                                          layerCenterZ),
                            pbRingBrickLogical,
                            "PbRingBrickPhysical",
                            fridgeLogical,
                            false,
                            iLayer * 8 + 4 + i,
                            mCheckOverlaps);
        }
      }

      constexpr G4int pbBaseNX = 7;
      constexpr G4int pbBaseNY = 3;

      auto* pbBaseBrickSolid = new G4Box("PbBaseBrick", pbMid / 2., pbLong / 2., pbShort / 2.);
      auto* pbBaseBrickLogical = new G4LogicalVolume(pbBaseBrickSolid, lead, "PbBaseBrickLogical");
      pbBaseBrickLogical->SetVisAttributes(Vis(leadColour));

      for (G4int iLayer = 0; iLayer < 2; ++iLayer)
      {
        const G4double layerCenterZ = ovcTrueBot_local - (iLayer + 0.5) * pbShort;
        for (G4int ix = 0; ix < pbBaseNX; ++ix)
        {
          for (G4int iy = 0; iy < pbBaseNY; ++iy)
          {
            const G4double cx = (ix - (pbBaseNX - 1) / 2.) * pbMid;
            const G4double cy = (iy - (pbBaseNY - 1) / 2.) * pbLong;
            new G4PVPlacement(nullptr,
                              G4ThreeVector(cx, cy, layerCenterZ),
                              pbBaseBrickLogical,
                              "PbBaseBrickPhysical",
                              fridgeLogical,
                              false,
                              iLayer * pbBaseNX * pbBaseNY + ix * pbBaseNY + iy,
                              mCheckOverlaps);
          }
        }
      }
    }

    return volumes;
  }

  // ---------------------------------------------------------------------------
  // BuildMeshComponents
  // ---------------------------------------------------------------------------
  void CryostatBuilder::BuildMeshComponents(const CryostatVolumes& volumes)
  {
    constexpr G4double ovcVacLocalCenterZ = kOvcVacuumCenterZ;

    struct MeshSpec
    {
      const char* filename;     
      const char* solidName;
      const char* lvName;
      const char* pvName;
      const char* material;     
      G4LogicalVolume* CryostatVolumes::* parentLV;
      G4ThreeVector    position;
      G4double         rotationZ = 0.;
      G4double         alpha = 0.8;
      G4double         red = 184 / 255.;
      G4double         green = 115 / 255.;
      G4double         blue = 51 / 255.;
    };

    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* tin = nist->FindOrBuildMaterial("G4_Sn");
    constexpr G4double detectorAssemblyHalfZ = 35. * mm;
    auto* detectorAssemblySolid = new G4Box("DetectorBoxAssemblySolid",
                                            35. * mm,
                                            35. * mm,
                                            detectorAssemblyHalfZ);
    auto* detectorAssemblyLogical =
        new G4LogicalVolume(detectorAssemblySolid, vacuum, "DetectorBoxAssemblyLogical");
    
    // Explicit visibility for the mother bounding box so you can debug boundaries if needed
    auto* assemblyVis = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.1));
    assemblyVis->SetVisibility(true);
    detectorAssemblyLogical->SetVisAttributes(assemblyVis);

    const G4ThreeVector detectorBoxFromAlignment(2. * 12.7 * mm, 0., -7. * 12.7 * mm);
    const G4ThreeVector detectorAlignmentOffset(-12.3 * mm, -35.5 * mm, +12.15 * mm);
    const G4ThreeVector detectorOriginInCryostat = detectorAlignmentOffset + detectorBoxFromAlignment;

    auto meta = Metadata::GetInstance();

    // =========================================================================
    // THE "LAB" ROTATION: 
    // To rotate the entire detector box relative to the cryostat, 
    // simply apply a rotation matrix to this G4PVPlacement!
    // =========================================================================
    G4VPhysicalVolume* detectorAssemblyPhysical = new G4PVPlacement(
                      nullptr, // <-- Replace 'nullptr' with a G4RotationMatrix* here if you want to rotate the whole box
                      G4ThreeVector(detectorOriginInCryostat.x(),
                                    detectorOriginInCryostat.y(),
                                    detectorOriginInCryostat.z() - ovcVacLocalCenterZ),
                      detectorAssemblyLogical,
                      "DetectorBoxAssemblyPhysical",
                      volumes.ovcVacuumLogical,
                      false,
                      0,
                      mCheckOverlaps);

    constexpr G4double detectorBoxFloorZ = 3.5 * mm;
    constexpr G4double detectorCrystalClearance = 1.7 * mm;

    if (!meta->GetBool("/QR/geom/useQubitArray"))
    {
      // =================================================================
      // MODE 0: CALIBRATION CONFIGURATION (SOLID BLOCK)
      // =================================================================
      auto* detectorCrystalSolid = new G4Box("DetectorSnCubeSolid",
                                             19.3 * mm / 2.,
                                             19.3 * mm / 2.,
                                             15.4 * mm / 2.);
      auto* detectorCrystalLogical =
          new G4LogicalVolume(detectorCrystalSolid, tin, "DetectorSnCubeLogical");
      detectorCrystalLogical->SetVisAttributes(new G4VisAttributes(G4Colour(0.10, 0.85, 0.95, 1.0)));

      new G4PVPlacement(
          nullptr,
          G4ThreeVector(0.,
                        0.,
                        detectorBoxFloorZ + detectorCrystalClearance + 15.4 * mm / 2.),
          detectorCrystalLogical,
          "DetectorSnCubePhysical",
          detectorAssemblyLogical,
          false,
          0,
          mCheckOverlaps);
    }
    else
    {
      // =================================================================
      // MODE 1: MULTI-STAGE G4CMP DETECTOR CHIP ARRAY (ORIENTATION SWITCH)
      // =================================================================
      using namespace QuasiparticleDetectorParameters;

      // Read macro/metadata command flag for orientation layout toggle

      G4bool isHorizontal = meta->GetBool("/QR/geom/isHorizontal");

      G4ThreeVector userPos;
      G4RotationMatrix* baseRot = new G4RotationMatrix();
      G4RotationMatrix* housingRot = nullptr;

      if (isHorizontal)
      {
        // Horizontal Layout: Co-linear nested geometry transformation
        //userPos = G4ThreeVector(0. * mm, 8000. * mm, 12.9 * mm); 
        userPos = G4ThreeVector(0. * mm, 0. * mm, 12.9 * mm); 
        baseRot->rotateX(0. * deg);
        baseRot->rotateY(0. * deg);
        baseRot->rotateZ(0. * deg);

        housingRot = new G4RotationMatrix(*baseRot);
        housingRot->rotateX(0. * deg); 
      }
      else
      {
        // Vertical Layout: Default orientation process as originally constructed
        userPos = G4ThreeVector(0. * mm, 0. * mm, 12.9 * mm); 
        baseRot->rotateX(90. * deg);
        baseRot->rotateY(0. * deg);
        baseRot->rotateZ(0. * deg);

        housingRot = new G4RotationMatrix(*baseRot);
        housingRot->rotateX(180. * deg); 
      }

      G4cout << "[CryostatBuilder] Qubit Array Configured Layout: " 
             << (isHorizontal ? "Horizontal" : "Vertical") << " | Position: " << userPos << G4endl;

      auto* fSilicon  = nist->FindOrBuildMaterial("G4_Si");
      auto* fAluminum = nist->FindOrBuildMaterial("G4_Al");
      auto* fCopper   = nist->FindOrBuildMaterial("G4_Cu");
      auto* fNiobium  = nist->FindOrBuildMaterial("G4_Nb");

      std::map<std::string, G4CMPSurfaceProperty*> fBorderContainer;
      std::map<std::string, G4LatticeLogical*> fLogicalLatticeContainer;
      
      auto* LM = G4LatticeManager::GetLatticeManager();
      fLogicalLatticeContainer.emplace("Silicon",  LM->LoadLattice(fSilicon, "Si"));
      fLogicalLatticeContainer.emplace("Aluminum", LM->LoadLattice(fAluminum, "Al"));
      fLogicalLatticeContainer.emplace("Copper",   LM->LoadLattice(fCopper, "Cu"));
      fLogicalLatticeContainer.emplace("Niobium",  LM->LoadLattice(fNiobium, "Nb")); 

      const G4double GHz = 1e9 * hertz;
      const std::vector<G4double> anhCoeffs = {0, 0, 0, 0, 0, 1.51e-14};
      const std::vector<G4double> diffCoeffs, specCoeffs;
      const G4double anhCutoff = 520., reflCutoff = 350.;

      // =========================================================================
      // G4CMP SURFACE BOUNDARY PHYSICS
      // Constructor: (Name, PhAbs, PhRefl, PhTrans, PhBscat, QpAbs, QpRefl, QpTrans, QpBscat, eAbs, hAbs)
      // =========================================================================

      // Si/Al Interface: Phonons 50/50 transmit/reflect. QPs MUST 100% reflect back into Al.
      auto* fSiAlInterface = new G4CMPSurfaceProperty("SiAlSurf", 
          0.0, 0.5, 0.5, 0.0,  
          0.0, 1.0, 0.0, 0.0,  
          0.0, 1.0);
      fSiAlInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiAl", fSiAlInterface);

      // Si/Vac Interface: Phonons 100% reflect. QPs 100% reflect.
      auto* fSiVacInterface = new G4CMPSurfaceProperty("SiVacSurf", 
          0.0, 1.0, 0.0, 0.0,  
          0.0, 1.0, 0.0, 0.0,  
          0.0, 1.0);
      fSiVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiVac", fSiVacInterface);

      // Si/Cu Interface: Phonons 100% reflect. QPs 100% reflect.
      // (Copper is passive, so phonons must bounce off the wall to stay inside the active chip)
      auto* fSiCuInterface = new G4CMPSurfaceProperty("SiCuSurf", 
          0.0, 1.0, 0.0, 0.0,  // <--- Changed to 1.0 Reflect, 0.0 Transmit
          0.0, 1.0, 0.0, 0.0,  
          0.0, 1.0);
      fSiCuInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiCu", fSiCuInterface);

      // Cu/Vac Interface
      /*
      auto* fCuVacInterface = new G4CMPSurfaceProperty("CuVacSurf", 
          0.0, 1.0, 0.0, 0.0, 
          1.0, 1.0, 0.0, 0.0, 
          0.0, 1.0);
      fCuVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("CuVac", fCuVacInterface);
      */

      // Al/Vac Interface
      auto* fAlVacInterface = new G4CMPSurfaceProperty("AlVacSurf", 
          0.0, 1.0, 0.0, 0.0, 
          0.0, 1.0, 0.0, 0.0, 
          0.0, 1.0);
      fAlVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("AlVac", fAlVacInterface);

      // Al/Al Interface: SAME MATERIAL. Phonons and QPs MUST 100% transmit.
      auto* fAlAlInterface = new G4CMPSurfaceProperty("AlAlSurf", 
          0.0, 0.0, 1.0, 0.0,  
          0.0, 0.0, 1.0, 0.0,  
          0.0, 0.0);
      fAlAlInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("AlAl", fAlAlInterface);

      // Vac/Vac Interface
      auto* fVacVacInterface = new G4CMPSurfaceProperty("VacVacSurf", 
          0.0, 1.0, 0.0, 0.0, 
          0.0, 1.0, 0.0, 0.0, 
          0.0, 1.0);
      fVacVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("VacVac", fVacVacInterface);

      // 4. Substrate Chip 
      auto* solid_siliconChip = new G4Box("QubitChip_solid", 0.5 * dp_siliconChipDimX, 0.5 * dp_siliconChipDimY, 0.5 * dp_siliconChipDimZ);
      auto* log_siliconChip = new G4LogicalVolume(solid_siliconChip, fSilicon, "SiliconChip_log");
      
      auto* siVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.6));
      siVis->SetVisibility(true);
      siVis->SetForceSolid(true);
      log_siliconChip->SetVisAttributes(siVis);

      G4ThreeVector localSiPos(0, 0, 0.5 * (dp_housingDimZ - dp_siliconChipDimZ) + dp_eps);
      G4ThreeVector globalSiPos = userPos + (*baseRot)(localSiPos);

      auto* physSiliconChip = new G4PVPlacement(new G4RotationMatrix(*baseRot), globalSiPos, log_siliconChip, "SiliconChip", detectorAssemblyLogical, false, 0, mCheckOverlaps);

      if (LM && fLogicalLatticeContainer["Silicon"]) {
                auto* siLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Silicon"]);
                siLatticePhys->SetMillerOrientation(1,0,0);
                LM->RegisterLattice(physSiliconChip, siLatticePhys);
            }

      if (detectorAssemblyPhysical && physSiliconChip) {
        new G4CMPLogicalBorderSurface("border_siliconChip_env", physSiliconChip, detectorAssemblyPhysical, fSiVacInterface);
        new G4CMPLogicalBorderSurface("border_env_siliconChip", detectorAssemblyPhysical, physSiliconChip, fSiVacInterface);
      }

      // 5. Guard Housing
      G4VPhysicalVolume* physQubitHousing = nullptr;
      if (dp_useQubitHousing) {
        G4ThreeVector localHousingPos(0, 0, 0);
        G4ThreeVector globalHousingPos = userPos + (*baseRot)(localHousingPos);

        auto* qubitHousing = new QuasiparticleQubitHousing(housingRot, globalHousingPos, "QubitHousing", detectorAssemblyLogical, false, 0, mCheckOverlaps);
        physQubitHousing = qubitHousing->GetPhysicalVolume();

        // -------------------------------------------------------------
        // COPPER QUBIT HOUSING LATTICE REGISTRATION
        // -------------------------------------------------------------
        /*
        if (LM && fLogicalLatticeContainer["Copper"] && physQubitHousing) {
            auto* cuLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Copper"]);
            cuLatticePhys->SetMillerOrientation(1,0,0);
            LM->RegisterLattice(physQubitHousing, cuLatticePhys);
        }
        */

        if (physQubitHousing && physSiliconChip) {
          new G4CMPLogicalBorderSurface("border_siliconChip_qubitHousing", physSiliconChip, physQubitHousing, fSiCuInterface);
          new G4CMPLogicalBorderSurface("border_qubitHousing_siliconChip", physQubitHousing, physSiliconChip, fSiCuInterface);
          // if (detectorAssemblyPhysical) {
          //   new G4CMPLogicalBorderSurface("border_qubitHousing_env", physQubitHousing, detectorAssemblyPhysical, fCuVacInterface);
          //   new G4CMPLogicalBorderSurface("border_env_qubitHousing", detectorAssemblyPhysical, physQubitHousing, fCuVacInterface);
          // }
        }
      }

      // 6. Ground Plane & Sub-Components
      if (dp_useGroundPlane) {
        auto* solid_groundPlane = new G4Box("GroundPlane_solid", 0.5 * dp_groundPlaneDimX, 0.5 * dp_groundPlaneDimY, 0.5 * dp_groundPlaneDimZ);
        auto* log_groundPlane = new G4LogicalVolume(solid_groundPlane, fNiobium, "GroundPlane_log");

        auto* gpVis = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0, 0.4));
        gpVis->SetVisibility(true);
        gpVis->SetForceSolid(true);
        log_groundPlane->SetVisAttributes(gpVis);

        G4ThreeVector localGpPos(0, 0, 0.5 * dp_housingDimZ + dp_eps + dp_groundPlaneDimZ * 0.5);
        G4ThreeVector globalGpPos = userPos + (*baseRot)(localGpPos);

        auto* physGroundPlane = new G4PVPlacement(new G4RotationMatrix(*baseRot), globalGpPos, log_groundPlane, "GroundPlane", detectorAssemblyLogical, false, 0, mCheckOverlaps);

        // -------------------------------------------------------------
        // NIOBIUM GROUND PLANE LATTICE REGISTRATION
        // -------------------------------------------------------------
        if (LM && fLogicalLatticeContainer["Niobium"]) {
            auto* nbLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Niobium"],
                                                        dp_polycryElScatMFP_Al, // Temporarily using Al parameters
                                                        dp_scDelta0_Al,         
                                                        dp_scTeff_Al,
                                                        dp_scDn_Al,
                                                        dp_scTauQPTrap_Al);
            nbLatticePhys->SetMillerOrientation(1,0,0);
            LM->RegisterLattice(physGroundPlane, nbLatticePhys);
        }

        if (physSiliconChip && physGroundPlane) {
          new G4CMPLogicalBorderSurface("border_siliconChip_groundPlane", physSiliconChip, physGroundPlane, fSiAlInterface);
          new G4CMPLogicalBorderSurface("border_groundPlane_siliconChip", physGroundPlane, physSiliconChip, fSiAlInterface);
        }
        if (detectorAssemblyPhysical && physGroundPlane) {
          new G4CMPLogicalBorderSurface("border_env_groundPlane", detectorAssemblyPhysical, physGroundPlane, fAlVacInterface);
          new G4CMPLogicalBorderSurface("border_groundPlane_env", physGroundPlane, detectorAssemblyPhysical, fAlVacInterface);
        }

        // Transmission Lines
        if (dp_useTransmissionLine) {
          auto* tLine = new QuasiparticleTransmissionLine(nullptr, G4ThreeVector(0,0,0), "TransmissionLine", log_groundPlane, false, 0, LM, fLogicalLatticeContainer, fBorderContainer, mCheckOverlaps);
          for (const auto& subVol : tLine->GetListOfAllFundamentalSubVolumes()) {
            G4String matName = std::get<0>(subVol);
            G4String volName = std::get<1>(subVol);
            auto* subPhys = std::get<2>(subVol);

            if (!subPhys) continue;

            if (matName.find("Vacuum") != std::string::npos) {
              if (physSiliconChip) {
                new G4CMPLogicalBorderSurface("b_si_v_" + volName, physSiliconChip, subPhys, fSiVacInterface);
                new G4CMPLogicalBorderSurface("b_v_si_" + volName, subPhys, physSiliconChip, fSiVacInterface);
              }
              if (detectorAssemblyPhysical) {
                new G4CMPLogicalBorderSurface("b_env_v_" + volName, detectorAssemblyPhysical, subPhys, fVacVacInterface);
                new G4CMPLogicalBorderSurface("b_v_env_" + volName, subPhys, detectorAssemblyPhysical, fVacVacInterface);
              }
              if (physGroundPlane) {
                new G4CMPLogicalBorderSurface("b_gp_v_" + volName, physGroundPlane, subPhys, fAlVacInterface);
                new G4CMPLogicalBorderSurface("b_v_gp_" + volName, subPhys, physGroundPlane, fAlVacInterface);
              }
            }
            if (matName.find("Aluminum") != std::string::npos) {
              
              // -------------------------------------------------------------
              // ALUMINUM TRANSMISSION LINE LATTICE REGISTRATION
              // -------------------------------------------------------------
              if (LM && fLogicalLatticeContainer["Aluminum"]) {
                  auto* alLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Aluminum"],
                                                              dp_polycryElScatMFP_Al,
                                                              dp_scDelta0_Al,
                                                              dp_scTeff_Al,
                                                              dp_scDn_Al,
                                                              dp_scTauQPTrap_Al);
                  alLatticePhys->SetMillerOrientation(1,0,0);
                  LM->RegisterLattice(subPhys, alLatticePhys);
              }

              if (physSiliconChip) {
                new G4CMPLogicalBorderSurface("b_si_al_" + volName, physSiliconChip, subPhys, fSiAlInterface);
                new G4CMPLogicalBorderSurface("b_al_si_" + volName, subPhys, physSiliconChip, fSiAlInterface);
              }
              if (detectorAssemblyPhysical) {
                new G4CMPLogicalBorderSurface("b_env_al_" + volName, detectorAssemblyPhysical, subPhys, fAlVacInterface);
                new G4CMPLogicalBorderSurface("b_al_env_" + volName, subPhys, detectorAssemblyPhysical, fAlVacInterface);
              }
            }
          }
        }

        // Resonators
        if (dp_useResonatorAssembly) {
          for (int iR = 0; iR < 8; ++iR) {
            G4ThreeVector resTranslate(0,0,0);
            
            // KEEP your existing rotation logic exactly as you have it
            G4RotationMatrix* rotAssembly = new G4RotationMatrix();
            // [Your existing orientation code here, if any]

            // KEEP your existing X positioning
            G4double xOffset = -1575.0 * CLHEP::um + (iR % 4) * 1050.0 * CLHEP::um; 
            
            // UPDATE only the Y shift to 570 um
            if (iR < 4) {
              // Top half of the chip (+y direction)
              // Shifts the origin to +570, placing the outer quarter-circle edge perfectly at +775
              resTranslate = G4ThreeVector(xOffset, 0.0 * CLHEP::um, 0.0);
              rotAssembly->rotateZ(90.0 * CLHEP::deg); // Face +y
            } else {
              // Bottom half of the chip (-y direction)
              // Shifts the origin to -570, placing the outer quarter-circle edge perfectly at -775
              resTranslate = G4ThreeVector(xOffset, 0.0 * CLHEP::um, 0.0);
              rotAssembly->rotateZ(-90.0 * CLHEP::deg); // Face +y
            }

//check if this is on axis or offset


            G4String resonatorAssemblyName = "ResonatorAssembly_" + std::to_string(iR);
            auto* resonatorAssembly = new QuasiparticleResonatorAssembly(rotAssembly, resTranslate, resonatorAssemblyName, log_groundPlane, false, 0, LM, fLogicalLatticeContainer, fBorderContainer, mCheckOverlaps);

            // ... (leave the border surface tracking loop intact below this)

        // ... (leave the border surface tracking loop completely intact below this)
            for (const auto& subVol : resonatorAssembly->GetListOfAllFundamentalSubVolumes()) {
              G4String matName = std::get<0>(subVol);
              G4String volName = std::get<1>(subVol);
              auto* subPhys = std::get<2>(subVol);

              if (!subPhys) continue;

              if (matName.find("Vacuum") != std::string::npos) {
                if (physSiliconChip) {
                  new G4CMPLogicalBorderSurface("b_si_vac_" + volName, physSiliconChip, subPhys, fSiVacInterface);
                  new G4CMPLogicalBorderSurface("b_vac_si_" + volName, subPhys, physSiliconChip, fSiVacInterface);
                }
                if (detectorAssemblyPhysical) {
                  new G4CMPLogicalBorderSurface("b_env_vac_" + volName, detectorAssemblyPhysical, subPhys, fVacVacInterface);
                  new G4CMPLogicalBorderSurface("b_vac_env_" + volName, subPhys, detectorAssemblyPhysical, fVacVacInterface);
                }
              }
              if (matName.find("Aluminum") != std::string::npos) {
                
                // -------------------------------------------------------------
                // ALUMINUM RESONATOR LATTICE REGISTRATION
                // -------------------------------------------------------------
                if (LM && fLogicalLatticeContainer["Aluminum"]) {
                  auto* alLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Aluminum"],
                                                              dp_polycryElScatMFP_Al,
                                                              dp_scDelta0_Al,
                                                              dp_scTeff_Al,
                                                              dp_scDn_Al,
                                                              dp_scTauQPTrap_Al);
                  alLatticePhys->SetMillerOrientation(1,0,0);
                  LM->RegisterLattice(subPhys, alLatticePhys);
                }

                if (physSiliconChip) {
                  new G4CMPLogicalBorderSurface("b_si_al_" + volName, physSiliconChip, subPhys, fSiAlInterface);
                  new G4CMPLogicalBorderSurface("b_al_si_" + volName, subPhys, physSiliconChip, fSiAlInterface);
                }
                if (detectorAssemblyPhysical) {
                  new G4CMPLogicalBorderSurface("b_env_al_" + volName, detectorAssemblyPhysical, subPhys, fAlVacInterface);
                  new G4CMPLogicalBorderSurface("b_al_env_" + subPhys->GetName(), subPhys, detectorAssemblyPhysical, fAlVacInterface);
                }
              }
              if (volName.find("tlCouplingConductor") != std::string::npos || volName == resonatorAssemblyName) {
                if (physGroundPlane) {
                  new G4CMPLogicalBorderSurface("b_gp_al_" + volName, physGroundPlane, subPhys, fAlAlInterface);
                  new G4CMPLogicalBorderSurface("b_al_gp_" + volName, subPhys, physGroundPlane, fAlAlInterface);
                }
              }
              if (volName.find("tlCouplingEmpty") != std::string::npos) {
                if (physGroundPlane) {
                  new G4CMPLogicalBorderSurface("b_gp_vac_" + volName, physGroundPlane, subPhys, fAlVacInterface);
                  new G4CMPLogicalBorderSurface("b_vac_gp_" + volName, subPhys, physGroundPlane, fAlVacInterface);
                }
              }
            }
          }
        }
        // -------------------------------------------------------------
        // 7. Gate Contact Pads (Global Placement on the Chip)
        // -------------------------------------------------------------
std::vector<G4TwoVector> padPolygon;
        padPolygon.push_back(G4TwoVector(-5.0*CLHEP::um, 0.0));
        padPolygon.push_back(G4TwoVector( 5.0*CLHEP::um, 0.0));
        padPolygon.push_back(G4TwoVector( 75.0*CLHEP::um, 200.0*CLHEP::um));
        padPolygon.push_back(G4TwoVector( 75.0*CLHEP::um, 400.0*CLHEP::um));
        padPolygon.push_back(G4TwoVector(-75.0*CLHEP::um, 400.0*CLHEP::um));
        padPolygon.push_back(G4TwoVector(-75.0*CLHEP::um, 200.0*CLHEP::um));

        auto* gatePadSolid = new G4ExtrudedSolid("GatePadSolid", padPolygon, dp_groundPlaneDimZ/2.0, G4TwoVector(0,0), 1.0, G4TwoVector(0,0), 1.0);
        
        auto* log_GatePad = new G4LogicalVolume(gatePadSolid, fAluminum, "GatePad_log");
        auto* gateVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.8));
        gateVis->SetVisibility(true);
        gateVis->SetForceSolid(true);
        log_GatePad->SetVisAttributes(gateVis);

        // Locked Y axis to 2355 as specified
        std::vector<G4ThreeVector> gatePositions = {
            G4ThreeVector(170.0*CLHEP::um, 2355.0*CLHEP::um, 0), G4ThreeVector(-170.0*CLHEP::um, 2355.0*CLHEP::um, 0),
            G4ThreeVector(1260.0*CLHEP::um, 2355.0*CLHEP::um, 0), G4ThreeVector(-1260.0*CLHEP::um, 2355.0*CLHEP::um, 0),
            G4ThreeVector(170.0*CLHEP::um, -2355.0*CLHEP::um, 0), G4ThreeVector(-170.0*CLHEP::um, -2355.0*CLHEP::um, 0),
            G4ThreeVector(1260.0*CLHEP::um, -2355.0*CLHEP::um, 0), G4ThreeVector(-1260.0*CLHEP::um, -2355.0*CLHEP::um, 0)
        };

        for (size_t k = 0; k < gatePositions.size(); ++k) {
            G4String gateName = "Chip_Gate_" + std::to_string(k);
            G4RotationMatrix* rotGate = new G4RotationMatrix();
            
            if (gatePositions[k].y() > 0) {
                rotGate->rotateZ(0.0 * CLHEP::deg);
            } else {
                rotGate->rotateZ(180.0 * CLHEP::deg);
            }
            
            auto* physGatePad = new G4PVPlacement(rotGate, gatePositions[k], log_GatePad, gateName + "_Phys", log_groundPlane, false, 0, mCheckOverlaps);
            // -------------------------------------------------------------
            // ALUMINUM GATE PAD LATTICE REGISTRATION
            // -------------------------------------------------------------
            if (LM && fLogicalLatticeContainer["Aluminum"]) {
                auto* alLatticePhys = new G4LatticePhysical(fLogicalLatticeContainer["Aluminum"],
                                                            dp_polycryElScatMFP_Al,
                                                            dp_scDelta0_Al,
                                                            dp_scTeff_Al,
                                                            dp_scDn_Al,
                                                            dp_scTauQPTrap_Al);
                alLatticePhys->SetMillerOrientation(1,0,0);
                LM->RegisterLattice(physGatePad, alLatticePhys);
            }
            
            // G4CMP Logical Border Surfaces (Matches how you track QP boundaries for resonators)
            if (physGroundPlane) {
                new G4CMPLogicalBorderSurface("b_gp_gate_" + gateName, physGroundPlane, physGatePad, fAlAlInterface);
                new G4CMPLogicalBorderSurface("b_gate_gp_" + gateName, physGatePad, physGroundPlane, fAlAlInterface);
            }
        }
      } // <--- THIS IS THE EXISTING CLOSING BRACE FOR: if (dp_useGroundPlane)
    }

    // =========================================================================================
    // Mesh Specifications Array (WITH 'obj/' SUBFOLDER DIRECTORIES ADDED)
    // =========================================================================================
    const MeshSpec kMeshSpecs[] = {
      {
        "obj/Experimental_Paddle.stl",
        "ExperimentalPaddleSolid",
        "ExperimentalPaddle_LV",
        "ExperimentalPaddle_PV",
        "G4_Cu",
        &CryostatVolumes::ovcVacuumLogical,
        G4ThreeVector(0., 0., -0.1 * mm - ovcVacLocalCenterZ),
        180. * deg,
        0.25
      },
      {
        "obj/Ricochet_Box_Platform.stl",
        "RicochetBoxPlatformSolid",
        "RicochetBoxPlatform_LV",
        "RicochetBoxPlatform_PV",
        "G4_Cu",
        nullptr,
        G4ThreeVector(0., 0., -3.1 * mm),
        0.,
        0.8,
        0.20,
        0.75,
        0.35
      },
      {
        "obj/Detector_Box.stl",
        "DetectorBoxSolid",
        "DetectorBox_LV",
        "DetectorBox_PV",
        "G4_Cu",
        nullptr,
        G4ThreeVector(0., 0., 0.),
        180. * deg,
        0.8,
        0.55,
        0.35,
        0.95
      },
    };

    // =========================================================================================
    // The STL Mesh Loading Loop
    // =========================================================================================
    for (const auto& spec : kMeshSpecs)
    {
      if (std::string(spec.filename) == "obj/Detector_Box.stl" && meta->GetBool("/QR/geom/useQubitArray"))
      {
        G4cout << "[CryostatBuilder] Skipping Detector_Box.stl because Qubit Array is enabled." << G4endl;
        continue; 
      }

      std::filesystem::path stlPath = std::filesystem::path("../src/Geometry") / spec.filename;

      // =========================================================================================
      // CRITICAL DEBUG: Print the absolute system path so you know exactly where it's looking
      // =========================================================================================
      G4cout << "[DEBUG-STL] Attempting to load mesh from absolute path: " 
             << std::filesystem::absolute(stlPath).string() << G4endl;

      if (!std::filesystem::exists(stlPath))
      {
        G4cerr << "[CryostatBuilder] CRITICAL WARNING: Mesh file not found at path: " 
               << stlPath.string() << " -- Skipping placement!" << G4endl;
        continue;
      }

      auto solid = CADMesh::TessellatedMesh::FromSTL(stlPath.string())->GetSolid();
      if (!solid) { continue; }
      
      auto* material = nist->FindOrBuildMaterial(spec.material);
      auto* lv = new G4LogicalVolume(solid, material, spec.lvName);
      lv->SetVisAttributes(new G4VisAttributes(G4Colour(spec.red, spec.green, spec.blue, spec.alpha)));
      
      G4LogicalVolume* parentLV = spec.parentLV ? volumes.*(spec.parentLV) : detectorAssemblyLogical;
      if (!parentLV) { continue; }
      
      G4RotationMatrix* rot = nullptr;
      if (spec.rotationZ != 0.) {
        rot = new G4RotationMatrix();
        rot->rotateZ(spec.rotationZ);
      }
      
      new G4PVPlacement(rot, spec.position, lv, spec.pvName, parentLV, false, 0, mCheckOverlaps);
      G4cout << "[CryostatBuilder] Successfully loaded and placed mesh: " << spec.filename << G4endl;
    }
  }
}