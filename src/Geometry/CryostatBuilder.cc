#include "Geometry/CryostatBuilder.hh"
#include "Geometry/Shapes.hh"

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

    // Vacuum plug filling the HexBride bore — sibling of BrideLogical in
    // fridgeLogical, mirroring the OVC / OVC-vacuum pattern.
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
    // Standard US lead brick: 8" (long) x 4" (mid) x 2" (short/height).
    // All bricks lay flat — the 2" face is vertical.
    if (mAddPb)
    {
      auto* lead = nist->FindOrBuildMaterial("G4_Pb");
      const G4Colour leadColour(50 / 255., 155 / 255., 36 / 255.);

      constexpr G4double pbLong  = 8. * 25.4 * mm;  // 203.2 mm — tangential
      constexpr G4double pbMid   = 4. * 25.4 * mm;  // 101.6 mm — radial
      constexpr G4double pbShort = 2. * 25.4 * mm;  //  50.8 mm — vertical

      // OVC base in fridgeLogical coordinates.
      // The ScreenOVC Bucket solid has a bottom disk below z=0 (local),
      // so the true bottom of the OVC is one wall thickness below the inner bottom.
      // Base bricks must start below this to avoid overlapping the flange.
      constexpr G4double ovcBaseZ_local   = kOvcInnerBottomZ - containerCenterZ;
      constexpr G4double ovcTrueBot_local = ovcBaseZ_local - kOvcWallThickness - 0.1 * mm;

      // ---- Ring: 8 bricks × 8 layers around the OVC side ----
      // All bricks oriented with 8" tangential (local y) and 4" radial (local x).
      // G4Box(pbMid/2, pbLong/2, pbShort/2): local x = 4" radial, local y = 8" tangential.
      // rotateZ(θ) points local x radially at angle θ.
      //
      // Set 1 — axial (0°, 90°, 180°, 270°): 5mm gap from OVC outer wall.
      //
      // Set 2 — diagonal (45°, 135°, 225°, 315°): placed as close as possible to Set 1.
      //   SAT separating axis (0,1) for 45° brick vs 0° brick:
      //     r2/√2 > pbLong/2 + (pbMid+pbLong)/(2√2) + gap = 101.6 + 107.8 + 5 = 214.4mm
      //     → r2 > 303.2mm  →  use 305mm  (gap = 305/√2 − 209.4 = 6.2mm)

      constexpr G4double pbRingR1  = kOvcOuterRadius + 5. * mm + pbMid / 2.;
      constexpr G4double pbRingR2  = 305. * mm;
      constexpr G4int    pbNLayers = 8;

      // Single shared logical — same physical brick for both sets.
      auto* pbRingBrickSolid   = new G4Box("PbRingBrick", pbMid / 2., pbLong / 2., pbShort / 2.);
      auto* pbRingBrickLogical = new G4LogicalVolume(pbRingBrickSolid, lead, "PbRingBrickLogical");
      pbRingBrickLogical->SetVisAttributes(Vis(leadColour));

      for (G4int iLayer = 0; iLayer < pbNLayers; ++iLayer)
      {
        const G4double layerCenterZ = ovcBaseZ_local + (iLayer + 0.5) * pbShort;

        // Set 1: axial bricks at 0°, 90°, 180°, 270°
        for (G4int i = 0; i < 4; ++i)
        {
          const G4double angle = i * 90. * deg;
          auto* rot = new G4RotationMatrix();
          rot->rotateZ(angle);  // at cardinal angles ±rotation is equivalent (box symmetry)
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

        // Set 2: diagonal bricks at 45°, 135°, 225°, 315°
        // rotateZ(θ) is an active clockwise rotation: local x̂ → (cosθ, −sinθ), ŷ → (sinθ, cosθ).
        // At θ=45°, rotateZ(+45°): local ŷ (8") → (1/√2, 1/√2) = NE = radial → wrong.
        // Using rotateZ(−45°): local ŷ (8") → (−1/√2, 1/√2) = tangential at 45° ✓.
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

      // ---- Base: rectangular layer below OVC, 2 layers ----
      // Footprint: 7×4" = 711.2mm in x (long side), 6×4" = 609.6mm in y (short side).
      // "6×4"" in y = 3 bricks × 8" each.  Bricks oriented 4"(x) × 8"(y) × 2"(z).
      // Shorter total side (24" = 609.6mm) is parallel to y as requested.
      constexpr G4int pbBaseNX = 7;  // 7 × 4" = 28" in x
      constexpr G4int pbBaseNY = 3;  // 3 × 8" = 24" in y (= 6 × 4")

      // Base brick is rotated vs ring brick: 4" in x, 8" in y, 2" in z.
      auto* pbBaseBrickSolid = new G4Box("PbBaseBrick", pbMid / 2., pbLong / 2., pbShort / 2.);
      auto* pbBaseBrickLogical = new G4LogicalVolume(pbBaseBrickSolid, lead, "PbBaseBrickLogical");
      pbBaseBrickLogical->SetVisAttributes(Vis(leadColour));

      for (G4int iLayer = 0; iLayer < 2; ++iLayer)
      {
        // Layer 0 is directly below the OVC flange bottom; layer 1 is below that.
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
  //
  // Loads each STL file listed in kMeshSpecs from mMeshDataPath and places it
  // as a G4TessellatedSolid inside the appropriate parent logical volume.
  // If a file is not found it is skipped with a warning -- the CSG-only
  // geometry is never broken by a missing mesh.
  //
  // Placement convention:
  //   Plate10mK bottom is z=0 in the cryostat frame.
  //   ovcVacuumLogical origin is at its geometric center, z=+139.40 mm in the
  //   cryostat frame.
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
    detectorAssemblyLogical->SetVisAttributes(new G4VisAttributes(false));

    const G4ThreeVector detectorBoxFromAlignment(2. * 12.7 * mm, 0., -7. * 12.7 * mm);
    const G4ThreeVector detectorAlignmentOffset(-12.3 * mm, -35.5 * mm, +12.15 * mm);
    const G4ThreeVector detectorOriginInCryostat = detectorAlignmentOffset + detectorBoxFromAlignment;
    
    // Ensure clean compilation with complete framework context visibility
    // AVOID PITFALL: Retain direct local pointers to eliminate global store parsing overheads
    G4VPhysicalVolume* detectorAssemblyPhysical = new G4PVPlacement(
                      nullptr,
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

    auto meta = Metadata::GetInstance();

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
      // MODE 1: MULTI-STAGE G4CMP DETECTOR CHIP ARRAY (FLAT TRANSFORM)
      // =================================================================
      using namespace QuasiparticleDetectorParameters;

      // 1. Initialize local variables with your HARDCODED DEFAULTS
      G4ThreeVector userPos(0. * mm, 0. * mm, 12.9 * mm); 
      G4RotationMatrix* baseRot = new G4RotationMatrix();
      baseRot->rotateX(90. * deg);
      baseRot->rotateY(0. * deg);
      baseRot->rotateZ(0. * deg);

      // 2. SAFE OVERRIDE: Check if the macro settings exist/are active.
      // If Metadata doesn't have a check function, we can check if they are non-zero,
      // or if you want to allow macro overrides, uncomment this block:
      /*
      if (meta->HasKey("/QR/geom/qubitPos")) { // Adjust to match your Metadata API if HasKey exists
          userPos = meta->GetThreeVector("/QR/geom/qubitPos");
          G4ThreeVector userRotAngles = meta->GetThreeVector("/QR/geom/qubitRot");
          baseRot->setIdentity();
          baseRot->rotateX(userRotAngles.x());
          baseRot->rotateY(userRotAngles.y());
          baseRot->rotateZ(userRotAngles.z());
          G4cout << "[CryostatBuilder] Using Macro overrides for Qubit Geometry." << G4endl;
      } else {
          G4cout << "[CryostatBuilder] Macro parameters not initialized. Falling back to hardcoded defaults." << G4endl;
      }
      */
      G4cout << "[CryostatBuilder] Qubit Array Position: " << userPos << G4endl;

      // 3. Setup Materials and Borders
      auto* nist = G4NistManager::Instance();
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

      const G4double GHz = 1e9 * hertz;
      const std::vector<G4double> anhCoeffs = {0, 0, 0, 0, 0, 1.51e-14};
      const std::vector<G4double> diffCoeffs, specCoeffs;
      const G4double anhCutoff = 520., reflCutoff = 350.;

      auto* fSiAlInterface = new G4CMPSurfaceProperty("SiAlSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
      fSiAlInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiAl", fSiAlInterface);

      auto* fSiVacInterface = new G4CMPSurfaceProperty("SiVacSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
      fSiVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiVac", fSiVacInterface);

      auto* fSiCuInterface = new G4CMPSurfaceProperty("SiCuSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);
      fSiCuInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("SiCu", fSiCuInterface);

      auto* fCuVacInterface = new G4CMPSurfaceProperty("CuVacSurf", 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0);
      fCuVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("CuVac", fCuVacInterface);

      auto* fAlVacInterface = new G4CMPSurfaceProperty("AlVacSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
      fAlVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("AlVac", fAlVacInterface);

      auto* fAlAlInterface = new G4CMPSurfaceProperty("AlAlSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
      fAlAlInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("AlAl", fAlAlInterface);

      auto* fVacVacInterface = new G4CMPSurfaceProperty("VacVacSurf", 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
      fVacVacInterface->AddScatteringProperties(anhCutoff, reflCutoff, anhCoeffs, diffCoeffs, specCoeffs, GHz, GHz, GHz);
      fBorderContainer.emplace("VacVac", fVacVacInterface);

      // 4. Substrate Chip (Rotated and Translated)
      auto* solid_siliconChip = new G4Box("QubitChip_solid", 0.5 * dp_siliconChipDimX, 0.5 * dp_siliconChipDimY, 0.5 * dp_siliconChipDimZ);
      auto* log_siliconChip = new G4LogicalVolume(solid_siliconChip, fSilicon, "SiliconChip_log");

      // Fix: Instantiate directly, mutate, and assign
      auto* siVis = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5, 0.6));
      siVis->SetVisibility(true);
      siVis->SetForceSolid(true);
      log_siliconChip->SetVisAttributes(siVis);

      G4ThreeVector localSiPos(0, 0, 0.5 * (dp_housingDimZ - dp_siliconChipDimZ) + (dp_eps + 0.001 * mm));
      G4ThreeVector globalSiPos = userPos + (*baseRot)(localSiPos); // Apply math safely

      auto* physSiliconChip = new G4PVPlacement(new G4RotationMatrix(*baseRot), globalSiPos, log_siliconChip, "SiliconChip", detectorAssemblyLogical, false, 0, mCheckOverlaps);

      if (detectorAssemblyPhysical && physSiliconChip) {
        new G4CMPLogicalBorderSurface("border_siliconChip_env", physSiliconChip, detectorAssemblyPhysical, fSiVacInterface);
        new G4CMPLogicalBorderSurface("border_env_siliconChip", detectorAssemblyPhysical, physSiliconChip, fSiVacInterface);
      }

      // 5. Guard Housing (Rotated and Translated)
      G4VPhysicalVolume* physQubitHousing = nullptr;
      if (dp_useQubitHousing) {
        G4ThreeVector localHousingPos(0, 0, 0);
        G4ThreeVector globalHousingPos = userPos + (*baseRot)(localHousingPos);

        auto* qubitHousing = new QuasiparticleQubitHousing(new G4RotationMatrix(*baseRot), globalHousingPos, "QubitHousing", detectorAssemblyLogical, false, 0, mCheckOverlaps);
        physQubitHousing = qubitHousing->GetPhysicalVolume();

        if (physQubitHousing && physSiliconChip) {
          new G4CMPLogicalBorderSurface("border_siliconChip_qubitHousing", physSiliconChip, physQubitHousing, fSiCuInterface);
          new G4CMPLogicalBorderSurface("border_qubitHousing_siliconChip", physQubitHousing, physSiliconChip, fSiCuInterface);
          if (detectorAssemblyPhysical) {
            new G4CMPLogicalBorderSurface("border_qubitHousing_env", physQubitHousing, detectorAssemblyPhysical, fCuVacInterface);
            new G4CMPLogicalBorderSurface("border_env_qubitHousing", detectorAssemblyPhysical, physQubitHousing, fCuVacInterface);
          }
        }
      }

      // 6. Ground Plane & Sub-Components (Rotated and Translated)
      if (dp_useGroundPlane) {
        auto* solid_groundPlane = new G4Box("GroundPlane_solid", 0.5 * dp_groundPlaneDimX, 0.5 * dp_groundPlaneDimY, 0.5 * dp_groundPlaneDimZ);
        auto* log_groundPlane = new G4LogicalVolume(solid_groundPlane, fNiobium, "GroundPlane_log");

        // Fix: Instantiate directly, mutate, and assign
        auto* gpVis = new G4VisAttributes(G4Colour(0.0, 1.0, 1.0, 0.4));
        gpVis->SetVisibility(true);
        gpVis->SetForceSolid(true);
        log_groundPlane->SetVisAttributes(gpVis);

        G4ThreeVector localGpPos(0, 0, 0.5 * dp_housingDimZ + dp_eps + dp_groundPlaneDimZ * 0.5);
        G4ThreeVector globalGpPos = userPos + (*baseRot)(localGpPos);

        auto* physGroundPlane = new G4PVPlacement(new G4RotationMatrix(*baseRot), globalGpPos, log_groundPlane, "GroundPlane", detectorAssemblyLogical, false, 0, mCheckOverlaps);

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
          for (int iR = 0; iR < 6; ++iR) {
            G4ThreeVector resTranslate(0,0,0);
            G4RotationMatrix* rotAssembly = nullptr;
            if (iR <= 2) {
              resTranslate = G4ThreeVector(dp_resonatorLateralSpacing * (iR - 1) + dp_centralResonatorOffsetX,
                                           0.5 * dp_resonatorAssemblyBaseAlDimY + 0.5 * dp_transmissionLineCavityFullWidth + dp_largeEpsilon, 0.0);
            } else {
              resTranslate = G4ThreeVector(dp_resonatorLateralSpacing * (iR - 4) - dp_centralResonatorOffsetX,
                                           -1.0 * (0.5 * dp_resonatorAssemblyBaseAlDimY + 0.5 * dp_transmissionLineCavityFullWidth + dp_largeEpsilon), 0.0);
              rotAssembly = new G4RotationMatrix();
              rotAssembly->rotateZ(180.0 * deg);
            }

            G4String resonatorAssemblyName = "ResonatorAssembly_" + std::to_string(iR);
            auto* resonatorAssembly = new QuasiparticleResonatorAssembly(rotAssembly, resTranslate, resonatorAssemblyName, log_groundPlane, false, 0, LM, fLogicalLatticeContainer, fBorderContainer, mCheckOverlaps);

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
      }
    }
  }
}