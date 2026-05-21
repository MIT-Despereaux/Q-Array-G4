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
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include <array>

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

  CryostatVolumes CryostatBuilder::Build()
  {
    auto* nist = G4NistManager::Instance();
    auto* air = nist->FindOrBuildMaterial("G4_AIR");
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* copper = nist->FindOrBuildMaterial("G4_Cu");
    auto* steel = nist->FindOrBuildMaterial("G4_STAINLESS-STEEL");

    constexpr G4double containerXY = 1.0 * m;
    constexpr G4double containerZ = 1.4 * m;
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

    return volumes;
  }
}
