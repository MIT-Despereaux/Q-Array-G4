#include "ExtrudedLayerBuilder.hh"
#include "G4ExtrudedSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VSensitiveDetector.hh"
#include "G4UnionSolid.hh"
#include <algorithm>

ExtrudedLayerBuilder::ExtrudedLayerBuilder()
  : fMaterial(nullptr), 
    fPosition(0, 0, 0), 
    fRotation(nullptr), 
    fNamePrefix("ExtrudedLayer"), 
    fUnifiedLogical(nullptr)
{}

ExtrudedLayerBuilder::~ExtrudedLayerBuilder() {}

void ExtrudedLayerBuilder::SetMaterial(G4Material* material) { fMaterial = material; }
void ExtrudedLayerBuilder::SetPosition(const G4ThreeVector& position) { fPosition = position; }
void ExtrudedLayerBuilder::SetRotation(G4RotationMatrix* rotation) { fRotation = rotation; }
void ExtrudedLayerBuilder::SetNamePrefix(const G4String& name) { fNamePrefix = name; }

void ExtrudedLayerBuilder::EnsureClockwiseWinding(std::vector<G4TwoVector>& vertices)
{
    double area = 0.0;
    size_t n = vertices.size();
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        area += (vertices[i].x() * vertices[j].y() - vertices[j].x() * vertices[i].y());
    }
    if (area > 0.0) {
        std::reverse(vertices.begin(), vertices.end());
    }
}

G4VPhysicalVolume* ExtrudedLayerBuilder::BuildUnifiedLayer(
    G4LogicalVolume* motherVolume, 
    const std::vector<std::vector<G4TwoVector>>& polygons,
    G4double totalThicknessFromJSON)
{
    if (!fMaterial || !motherVolume || polygons.empty()) return nullptr;

    G4double halfThickness = totalThicknessFromJSON / 2.0;
    G4TwoVector offset(0,0);
    std::vector<G4VSolid*> subSolids;

    G4cout << "\n[DEBUG-EXTRUDED] Extruding " << polygons.size() << " polygons..." << G4endl;

    for (size_t i = 0; i < polygons.size(); ++i) {
        auto poly = polygons[i];
        EnsureClockwiseWinding(poly);
        G4String sName = fNamePrefix + "_SubSolid_" + std::to_string(i);
        subSolids.push_back(new G4ExtrudedSolid(sName, poly, halfThickness, offset, 1.0, offset, 1.0));
    }


    
    // Binary Tree Union: Merges solids pairwise to avoid stack overflow
    G4cout << "[DEBUG-EXTRUDED] Unifying solids into single compound geometry..." << G4endl;
    while (subSolids.size() > 1) {
        std::vector<G4VSolid*> nextLevel;
        for (size_t i = 0; i < subSolids.size(); i += 2) {
            if (i + 1 < subSolids.size()) {
                G4String uName = fNamePrefix + "_Union_" + std::to_string(i);
                nextLevel.push_back(new G4UnionSolid(uName, subSolids[i], subSolids[i+1]));
            } else {
                nextLevel.push_back(subSolids[i]);
            }
        }
        subSolids = nextLevel;
    }

    G4VSolid* unifiedSolid = subSolids[0];
    G4String logicName = fNamePrefix + "_Logic";
    fUnifiedLogical = new G4LogicalVolume(unifiedSolid, fMaterial, logicName);

    G4String physName = fNamePrefix + "_Phys";
    G4VPhysicalVolume* phys = new G4PVPlacement(
        fRotation, 
        fPosition, 
        fUnifiedLogical, 
        physName, 
        motherVolume, 
        false, 
        0, 
        false
    );

    G4cout << "[DEBUG-EXTRUDED] Successfully built unified physical volume: " << physName << G4endl;
    return phys;
    /*
    // --- ADD THIS TEMPORARILY ---
    for (size_t i = 0; i < subSolids.size(); i++) {
        G4LogicalVolume* debugLog = new G4LogicalVolume(
            subSolids[i], fMaterial, fNamePrefix + "_DebugLog_" + std::to_string(i));
        
        new G4PVPlacement(
            fRotation, 
            fPosition, 
            debugLog, 
            fNamePrefix + "_DebugPhys_" + std::to_string(i), 
            motherVolume, 
            false, 
            i, 
            false);
    }
    return nullptr; // We don't return a unified volume here.
    */
}

void ExtrudedLayerBuilder::AssignSensitiveDetector(G4VSensitiveDetector* sd)
{
    if (sd && fUnifiedLogical) {
        fUnifiedLogical->SetSensitiveDetector(sd);
    }
}