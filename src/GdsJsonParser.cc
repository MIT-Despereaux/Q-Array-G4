#include "GdsJsonParser.hh"
#include <fstream>
#include <iostream>

// Include the nlohmann JSON library (make sure json.hpp is in your include path)
#include "json.hh" 
using json = nlohmann::json;

GdsJsonParser::GdsJsonParser() : fThickness(0.0) {}
GdsJsonParser::~GdsJsonParser() {}

bool GdsJsonParser::LoadFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "GdsJsonParser Error: Could not open " << filepath << std::endl;
        return false;
    }

    json j;
    try {
        file >> j;
        
        // 1. Extract and convert thickness (nm -> mm for Geant4)
        double thickness_nm = j["metadata"]["thickness_nm"].get<double>();
        fThickness = thickness_nm * 1e-6; // Geant4 default unit for length is mm

        // 2. Extract and convert polygons (um -> mm for Geant4)
        fPolygons.clear();
        for (const auto& polyJson : j["polygons"]) {
            std::vector<G4TwoVector> polygon;
            
            // Iterate over each vertex in the current polygon
            for (const auto& vertex : polyJson) {
                // Assuming your JSON stores x,y pairs in micrometers (um)
                double x_um = vertex[0].get<double>();
                double y_um = vertex[1].get<double>();
                
                // Convert um to mm
                polygon.push_back(G4TwoVector(x_um * 1e-3, y_um * 1e-3));
            }
            fPolygons.push_back(polygon);
        }
    } 
    catch (json::exception& e) {
        std::cerr << "GdsJsonParser Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

std::vector<std::vector<G4TwoVector>> GdsJsonParser::GetPolygons() const {
    return fPolygons;
}

G4double GdsJsonParser::GetThickness() const {
    return fThickness;
}