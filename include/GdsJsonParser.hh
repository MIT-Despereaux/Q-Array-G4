#ifndef GdsJsonParser_h
#define GdsJsonParser_h 1

#include "globals.hh"
#include "G4TwoVector.hh"
#include <vector>
#include <string>

class GdsJsonParser
{
  public:
    GdsJsonParser();
    ~GdsJsonParser();

    // Loads and parses the JSON file. Returns true if successful.
    bool LoadFile(const std::string& filepath);
    
    // Getters to pass directly to your ExtrudedLayerBuilder
    std::vector<std::vector<G4TwoVector>> GetPolygons() const;
    G4double GetThickness() const;

  private:
    std::vector<std::vector<G4TwoVector>> fPolygons;
    G4double fThickness; // Stored in Geant4 standard units (mm)
};

#endif