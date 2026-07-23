#include "ExtrudedLayerBuilder.hh"
#include "G4ExtrudedSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VSensitiveDetector.hh"
#include <algorithm>

ExtrudedLayerBuilder::ExtrudedLayerBuilder()
  : fMaterial(nullptr), fZOffset(0.0), fNamePrefix("ExtrudedLayer")
{}

ExtrudedLayerBuilder::~ExtrudedLayerBuilder() {}

void ExtrudedLayerBuilder::SetMaterial(G4Material* material) { fMaterial = material; }
void ExtrudedLayerBuilder::SetZOffset(G4double zOffset) { fZOffset = zOffset; }
void ExtrudedLayerBuilder::SetNamePrefix(const G4String& name) { fNamePrefix = name; }

void ExtrudedLayerBuilder::EnsureClockwiseWinding(std::vector<G4TwoVector>& vertices)
{
    double area = 0.0;
    size_t n = vertices.size();
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        area += (vertices[i].x() * vertices[j].y() - vertices[j].x() * vertices[i].y());
    }
    
    // Reverse if counter-clockwise
    if (area > 0.0) {
        std::reverse(vertices.begin(), vertices.end());
    }
}

// ExtrudedLayerBuilder.cc
std::vector<G4VPhysicalVolume*> ExtrudedLayerBuilder::BuildLayer(
    G4LogicalVolume* motherVolume, 
    const std::vector<std::vector<G4TwoVector>>& polygons,
    G4double totalThicknessFromJSON)
{
    std::vector<G4VPhysicalVolume*> placedPhysVolumes;
    if (!fMaterial || !motherVolume) return placedPhysVolumes;

    G4double halfThickness = totalThicknessFromJSON / 2.0;
    G4TwoVector offset(0,0);
    G4int counter = 0;

    fCreatedLogicals.clear(); 

    G4cout << "\n[DEBUG-EXTRUDED] Starting creation of " << polygons.size() << " diced chunks..." << G4endl;

    for (auto polygon : polygons) {
        EnsureClockwiseWinding(polygon);

        G4cout << "[DEBUG-EXTRUDED] Chunk " << counter << " | Vertices: " << polygon.size() << " | Creating Solid..." << std::flush;
        
        G4String solidName = fNamePrefix + "_Solid_" + std::to_string(counter);
        G4ExtrudedSolid* solid = new G4ExtrudedSolid(
            solidName, polygon, halfThickness, offset, 1.0, offset, 1.0);
            
        G4cout << " DONE." << G4endl;

        G4cout << "[DEBUG-EXTRUDED] Chunk " << counter << " | Creating Logical Volume..." << std::flush;
        G4String logicName = fNamePrefix + "_Logic_" + std::to_string(counter);
        G4LogicalVolume* logic = new G4LogicalVolume(solid, fMaterial, logicName);
        fCreatedLogicals.push_back(logic);
        G4cout << " DONE." << G4endl;

        // CRITICAL FIX: The last parameter here is the overlap checker. 
        // We are hardcoding it to 'false' to prevent infinite hangs on complex extruded shapes.
        G4cout << "[DEBUG-EXTRUDED] Chunk " << counter << " | Placing Physical Volume (Overlap Check: OFF)..." << std::flush;
        G4String physName = fNamePrefix + "_Phys_" + std::to_string(counter);
        G4VPhysicalVolume* phys = new G4PVPlacement(
            0, 
            G4ThreeVector(0, 0, fZOffset), 
            logic, 
            physName, 
            motherVolume, 
            false, 
            counter, 
            false // <--- SET TO FALSE
        );
        G4cout << " DONE." << G4endl;
        
        placedPhysVolumes.push_back(phys);
        counter++;
    }
    
    G4cout << "[DEBUG-EXTRUDED] Successfully built all " << counter << " extruded chunks.\n" << G4endl;
    
    return placedPhysVolumes;
}

void ExtrudedLayerBuilder::AssignSensitiveDetector(G4VSensitiveDetector* sd)
{
    if (!sd) return;
    
    // This makes every individual polygon act as ONE unified detector
    for (auto* logicVol : fCreatedLogicals) {
        logicVol->SetSensitiveDetector(sd);
    }
}