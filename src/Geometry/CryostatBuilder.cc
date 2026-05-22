#include "Geometry/CryostatBuilder.hh"
#include "Geometry/Shapes.hh"

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
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include <array>
#include <cmath>

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

  CryostatVolumes CryostatBuilder::Build()
  {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* copper = nist->FindOrBuildMaterial("G4_Cu");
    auto* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

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
    const G4Colour vacuumColour(0., 0., 1., 0.08);

    const PlateSpec plates[] = {
        {"Plate10mK", 125.0 * mm, 4.0 * mm, 0.0 * mm},
        {"Plate100mK", 125.0 * mm, 3.0 * mm, 74.20 * mm},
        {"Plate1K", 142.0 * mm, 5.0 * mm, 181.20 * mm},
        {"Plate4K", 161.5 * mm, 6.0 * mm, 313.20 * mm},
        {"Plate50K", 180.0 * mm, 6.0 * mm, 522.70 * mm},
    };

    constexpr G4double ovcInnerBottomZ = -367.30 * mm;
    constexpr G4double ovcHeight = 1021.70 * mm;
    constexpr G4double ovcVacuumEpsilon = 0.001 * mm;
    constexpr G4double ovcVacuumCenterZ = ovcInnerBottomZ + ovcHeight / 2.;

    auto* ovcVacuumSolid = new G4Tubs("ScreenOVCVacuum",
                                      0.,
                                      190.5 * mm - ovcVacuumEpsilon,
                                      ovcHeight / 2. - ovcVacuumEpsilon,
                                      0.,
                                      twopi);
    auto* ovcVacuumLogical = new G4LogicalVolume(ovcVacuumSolid, vacuum, "stageOVCVacLogical");
    ovcVacuumLogical->SetVisAttributes(Vis(vacuumColour));
    volumes.ovcVacuumLogical = ovcVacuumLogical;

    const ScreenSpec screens[] = {
        {"Screen1K", 130.5 * mm, 2.0 * mm, 483.20 * mm, -302.00 * mm},
        {"Screen4K", 151.5 * mm, 2.0 * mm, 636.70 * mm, -323.50 * mm},
        {"Screen50K", 170.0 * mm, 2.0 * mm, 868.00 * mm, -345.30 * mm},
        {"ScreenOVC", 190.5 * mm, 5.0 * mm, 1021.70 * mm, -367.30 * mm},
    };

    for (const auto& screen : screens)
    {
      auto* solid = new Bucket(screen.name, screen.innerRadius, screen.thickness, screen.height);
      auto* logical = new G4LogicalVolume(solid,
                                          screen.name == G4String("ScreenOVC") ? steel : copper,
                                          G4String(screen.name) + "Logical");
      logical->SetVisAttributes(Vis(screen.name == G4String("ScreenOVC") ? steelColour : screenColour));
      Place(logical,
            G4String(screen.name) + "Physical",
            screen.name == G4String("ScreenOVC") ? fridgeLogical : ovcVacuumLogical,
            G4ThreeVector(0.,
                          0.,
                          screen.name == G4String("ScreenOVC")
                              ? screen.zInnerBottom - containerCenterZ
                              : screen.zInnerBottom - ovcVacuumCenterZ),
            mCheckOverlaps);

      if (screen.name == G4String("ScreenOVC"))
      {
        volumes.ovcLogical = logical;
      }
    }

    Place(ovcVacuumLogical,
          "stageOVCVacPhysical",
          fridgeLogical,
          G4ThreeVector(0., 0., ovcVacuumCenterZ - containerCenterZ),
          mCheckOverlaps);

    for (const auto& plate : plates)
    {
      auto* solid = new FilledTube(plate.name, plate.radius, plate.thickness);
      auto* logical = new G4LogicalVolume(solid, copper, G4String(plate.name) + "Logical");
      logical->SetVisAttributes(Vis(copperColour));
      Place(logical,
            G4String(plate.name) + "Physical",
            ovcVacuumLogical,
            G4ThreeVector(0., 0., plate.zBottom + plate.thickness / 2. - ovcVacuumCenterZ),
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
    constexpr G4double brideBottomZ      = 654.40 * mm;
    constexpr G4double brideVacEpsilon   = 0.1 * mm;

    auto* brideSolid = new HexBride("Bride",
                                    brideInnerRadius,
                                    brideCircumRadius,
                                    brideInnerHeight,
                                    brideTopThickness);
    auto* brideLogical = new G4LogicalVolume(brideSolid, steel, "BrideLogical");
    brideLogical->SetVisAttributes(Vis(steelColour));
    Place(brideLogical,
          "BridePhysical",
          fridgeLogical,
          G4ThreeVector(0., 0., brideBottomZ - containerCenterZ),
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

    if (mVerbose > 0)
    {
      G4cout << "Built DSPX cryostat geometry in fridgeLogical" << G4endl;
    }

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
      // The ScreenOVC Bucket solid has a 5mm solid disc flange below z=0 (local),
      // so the true bottom of the OVC including flange is at ovcInnerBottomZ - 5mm.
      // Base bricks must start below this to avoid overlapping the flange.
      constexpr G4double ovcFlange       = 5.0 * mm;  // ScreenOVC Bucket thickness at base
      constexpr G4double ovcBaseZ_local  = ovcInnerBottomZ - containerCenterZ;         // -682.3 mm
      constexpr G4double ovcTrueBot_local = ovcBaseZ_local - ovcFlange - 0.1 * mm;     // -687.4 mm

      // ---- Ring: 8 bricks × 8 layers around the OVC side ----
      // All bricks oriented with 8" tangential (local y) and 4" radial (local x).
      // G4Box(pbMid/2, pbLong/2, pbShort/2): local x = 4" radial, local y = 8" tangential.
      // rotateZ(θ) points local x radially at angle θ.
      //
      // Set 1 — axial (0°, 90°, 180°, 270°): 5mm gap from OVC outer wall (195.5mm).
      //   r1 = 195.5 + 5 + pbMid/2 = 251.3mm
      //
      // Set 2 — diagonal (45°, 135°, 225°, 315°): placed as close as possible to Set 1.
      //   SAT separating axis (0,1) for 45° brick vs 0° brick:
      //     r2/√2 > pbLong/2 + (pbMid+pbLong)/(2√2) + gap = 101.6 + 107.8 + 5 = 214.4mm
      //     → r2 > 303.2mm  →  use 305mm  (gap = 305/√2 − 209.4 = 6.2mm)

      constexpr G4double ovcOuterR = 195.5 * mm;   // ScreenOVC innerR(190.5) + wall(5.0)
      constexpr G4double pbRingR1  = ovcOuterR + 5. * mm + pbMid / 2.;  // 251.3mm
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
          rot->rotateZ(angle);  // rotate 5° to create gap from OVC wall
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
        for (G4int i = 0; i < 4; ++i)
        {
          const G4double angle = (45. + i * 90.) * deg;
          auto* rot = new G4RotationMatrix();
          rot->rotateZ(-angle);  // the object rotation is an "active" one
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
}
