#include "Geometry/Shapes.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include <array>

namespace QArray::Geometry
{
  FilledTube::FilledTube(const G4String& name, G4double radius, G4double height)
      : G4Tubs(name, 0., radius, height / 2., 0., twopi)
  {
  }

  Bucket::Bucket(const G4String& name, G4double innerRadius, G4double thickness, G4double height)
      : G4Polycone(name,
                   0.,
                   twopi,
                   4,
                   std::array<G4double, 4>{-thickness, 0., 0., height}.data(),
                   std::array<G4double, 4>{0., 0., innerRadius, innerRadius}.data(),
                   std::array<G4double, 4>{innerRadius + thickness,
                                           innerRadius + thickness,
                                           innerRadius + thickness,
                                           innerRadius + thickness}.data())
  {
  }

  ReversedBucket::ReversedBucket(const G4String& name, G4double innerRadius, G4double thickness, G4double height)
      : G4Polycone(name,
                   0.,
                   twopi,
                   4,
                   std::array<G4double, 4>{0., height, height, height + thickness}.data(),
                   std::array<G4double, 4>{innerRadius, innerRadius, 0., 0.}.data(),
                   std::array<G4double, 4>{innerRadius + thickness,
                                           innerRadius + thickness,
                                           innerRadius + thickness,
                                           innerRadius + thickness}.data())
  {
  }

  HexBride::HexBride(const G4String& name,
                     G4double innerRadius,
                     G4double circumRadius,
                     G4double innerHeight,
                     G4double topThickness)
      : G4Polyhedra(name,
                    0.,
                    twopi,
                    6,
                    4,
                    std::array<G4double, 4>{0., innerHeight, innerHeight, innerHeight + topThickness}.data(),
                    std::array<G4double, 4>{innerRadius, innerRadius, 0., 0.}.data(),
                    std::array<G4double, 4>{circumRadius, circumRadius, circumRadius, circumRadius}.data())
  {
  }
}
