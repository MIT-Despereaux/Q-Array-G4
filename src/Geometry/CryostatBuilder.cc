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
    // ovcVacuumLogical is a G4Tubs: local z=0 is at its GEOMETRIC CENTER.
    // Center in cryostat frame = kOvcInnerBottomZ + kOvcHeight/2 = +139.40 mm.
    // pos_in_parent = originInCryostatZ - ovcVacLocalCenterZ
    constexpr G4double ovcVacLocalCenterZ = kOvcVacuumCenterZ;

    struct MeshSpec
    {
      const char*      filename;     // relative to mMeshDataPath
      const char*      solidName;
      const char*      lvName;
      const char*      pvName;
      const char*      material;     // NIST name
      G4LogicalVolume* CryostatVolumes::* parentLV;
      // Position of the part's own origin in the parent LV frame.
      G4ThreeVector    position;
      // Rotation around Z axis applied to the mesh before placement.
      // 0 = no rotation (default), 180*deg = flipped around Z.
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

    // Detector origin = base mesh reference origin.  The screw-hole alignment
    // offset is adjustable; the box origin is 2 x 12.7 mm in +X and
    // 7 x 12.7 mm in -Z from that offset.
    const G4ThreeVector detectorBoxFromAlignment(2. * 12.7 * mm, 0., -7. * 12.7 * mm);
    const G4ThreeVector detectorAlignmentOffset(-12.3 * mm, -35.5 * mm, +12.15 * mm);
    const G4ThreeVector detectorOriginInCryostat = detectorAlignmentOffset + detectorBoxFromAlignment;
    new G4PVPlacement(nullptr,
                      G4ThreeVector(detectorOriginInCryostat.x(),
                                    detectorOriginInCryostat.y(),
                                    detectorOriginInCryostat.z() - ovcVacLocalCenterZ),
                      detectorAssemblyLogical,
                      "DetectorBoxAssemblyPhysical",
                      volumes.ovcVacuumLogical,
                      false,
                      0,
                      mCheckOverlaps);

    constexpr G4double detectorCoverOriginZ = 1.7 * mm;

    auto* detectorCrystalSolid = new G4Box("DetectorSnCubeSolid",
                                           19.3 * mm / 2.,
                                           19.3 * mm / 2.,
                                           15.4 * mm / 2.);
    auto* detectorCrystalLogical =
        new G4LogicalVolume(detectorCrystalSolid, tin, "DetectorSnCubeLogical");
    detectorCrystalLogical->SetVisAttributes(new G4VisAttributes(G4Colour(0.10, 0.85, 0.95, 1.0)));
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0.,
                                    0.,
                                    detectorCoverOriginZ + 3.0 * mm + 15.4 * mm / 2.),
                      detectorCrystalLogical,
                      "DetectorSnCubePhysical",
                      detectorAssemblyLogical,
                      false,
                      0,
                      mCheckOverlaps);

    // -------------------------------------------------------------------------
    // Mesh inventory -- matches src/Geometry/obj/README.md
    // -------------------------------------------------------------------------
    // Experimental_Paddle: top face nominally at Plate10mK bottom (z=0).
    // Shifted 0.1mm below (z=-0.1mm) to avoid tessellation-precision overlap
    // with the Plate10mK CSG solid which also sits at z=0.
    // Parent: ovcVacuumLogical (G4Tubs center at +143.55mm in cryostat frame).
    // pos_in_parent_z = -0.1mm - (+143.55mm) = -143.65mm
    const MeshSpec kMeshSpecs[] = {
      {
        "Experimental_Paddle.stl",
        "ExperimentalPaddleSolid",
        "ExperimentalPaddle_LV",
        "ExperimentalPaddle_PV",
        "G4_Cu",
        &CryostatVolumes::ovcVacuumLogical,
        G4ThreeVector(0., 0., -0.1 * mm - ovcVacLocalCenterZ),
        180. * deg,  // rotated 180° around Z to match DSPX orientation
        0.25
      },
      {
        "Ricochet_Box_Platform.stl",
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
        "Detector_Base.stl",
        "DetectorBaseSolid",
        "DetectorBase_LV",
        "DetectorBase_PV",
        "G4_Cu",
        nullptr,
        G4ThreeVector(0., 0., 0.),
        0.,
        0.8
      },
      {
        "Detector_Cover.stl",
        "DetectorCoverSolid",
        "DetectorCover_LV",
        "DetectorCover_PV",
        "G4_Cu",
        nullptr,
        G4ThreeVector(0., 0., detectorCoverOriginZ),
        180. * deg,
        0.8,
        0.55,
        0.35,
        0.95
      },
    };

    for (const auto& spec : kMeshSpecs)
    {
      // Build full path
      std::filesystem::path stlPath =
          std::filesystem::path(std::string(mMeshDataPath)) / spec.filename;

      if (!std::filesystem::exists(stlPath))
      {
        G4cerr << "[CryostatBuilder] WARNING: mesh file not found, skipping: "
               << stlPath.string() << G4endl;
        continue;
      }

      // Load mesh
      auto mesh = CADMesh::TessellatedMesh::FromSTL(stlPath.string());
      mesh->SetScale(mm);  // FreeCAD exports in mm; Geant4 mm unit = 1.0

      auto* solid = mesh->GetSolid();
      solid->SetName(spec.solidName);

      // Print extent when verbose
      if (mVerbose > 0)
      {
        G4VisExtent ext = solid->GetExtent();
        G4cout << "[CryostatBuilder] " << spec.filename << " extent (mm):"
               << " X[" << ext.GetXmin() / mm << "," << ext.GetXmax() / mm << "]"
               << " Y[" << ext.GetYmin() / mm << "," << ext.GetYmax() / mm << "]"
               << " Z[" << ext.GetZmin() / mm << "," << ext.GetZmax() / mm << "]"
               << G4endl;
      }

      // Material
      auto* material = nist->FindOrBuildMaterial(spec.material);
      if (!material)
      {
        G4cerr << "[CryostatBuilder] WARNING: material not found: "
               << spec.material << " -- skipping " << spec.filename << G4endl;
        continue;
      }

      // Logical volume
      auto* lv = new G4LogicalVolume(solid, material, spec.lvName);
      lv->SetVisAttributes(new G4VisAttributes(G4Colour(spec.red, spec.green, spec.blue, spec.alpha)));

      // Parent logical volume.  Detector box meshes live in their assembly
      // mother; cryostat-level meshes reference CryostatVolumes explicitly.
      G4LogicalVolume* parentLV =
          spec.parentLV ? volumes.*(spec.parentLV) : detectorAssemblyLogical;
      if (!parentLV)
      {
        G4cerr << "[CryostatBuilder] WARNING: parent LV is null for "
               << spec.filename << " -- skipping" << G4endl;
        continue;
      }

      // Build rotation if needed
      G4RotationMatrix* rot = nullptr;
      if (spec.rotationZ != 0.)
      {
        rot = new G4RotationMatrix();
        rot->rotateZ(spec.rotationZ);
      }

      new G4PVPlacement(
          rot,
          spec.position,
          lv,
          spec.pvName,
          parentLV,
          false, 0,
          mCheckOverlaps);

      if (mVerbose > 0)
      {
        G4cout << "[CryostatBuilder] Placed " << spec.pvName
               << " in " << parentLV->GetName()
               << " at (" << spec.position.x() / mm << ", "
               << spec.position.y() / mm << ", "
               << spec.position.z() / mm << ") mm (parent frame)" << G4endl;
      }
    }
  }

} // namespace QArray::Geometry
