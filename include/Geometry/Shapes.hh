#ifndef QARRAY_GEOMETRY_SHAPES_HH
#define QARRAY_GEOMETRY_SHAPES_HH

#include "G4Polycone.hh"
#include "G4Polyhedra.hh"
#include "G4Tubs.hh"
#include "globals.hh"

namespace QArray::Geometry
{
  struct FilledTube : G4Tubs
  {
    FilledTube(const G4String& name, G4double radius, G4double height);
  };

  struct Bucket : G4Polycone
  {
    Bucket(const G4String& name, G4double innerRadius, G4double thickness, G4double height);
  };

  struct ReversedBucket : G4Polycone
  {
    ReversedBucket(const G4String& name, G4double innerRadius, G4double thickness, G4double height);
  };

  struct HexBride : G4Polyhedra
  {
    HexBride(const G4String& name,
             G4double innerRadius,
             G4double circumRadius,
             G4double innerHeight,
             G4double topThickness);
  };
}

#endif
