#!/usr/bin/env python3
"""
gds_extract_chip.py

Extracts a specific region of a GDS file on a specific layer/datatype, 
merges the polygons to prevent Geant4 volume overlaps, and saves the 
vertex data to a JSON file for downstream Geant4/g4cmp geometry generation.
"""

import gdstk
import json
import os
import csv

def get_bounding_box(coords):
    """Calculate min and max X/Y from a list of (X,Y) coordinates."""
    x_coords = [p[0] for p in coords]
    y_coords = [p[1] for p in coords]
    return min(x_coords), max(x_coords), min(y_coords), max(y_coords)

def load_coords_from_csv(csv_path):
    """
    Generalizable function for future use: 
    Reads a CSV file containing X, Y coordinates and returns a list of tuples.
    """
    coords = []
    if os.path.exists(csv_path):
        with open(csv_path, 'r') as f:
            reader = csv.reader(f)
            for row in reader:
                if row: # Skip empty rows
                    coords.append((float(row[0]), float(row[1])))
    return coords

def extract_chip_geometry(gds_path, layer, datatype, region_coords, output_path):
    min_x, max_x, min_y, max_y = get_bounding_box(region_coords)
    
    print(f"[*] Loading GDS file: {gds_path}")
    if not os.path.exists(gds_path):
        raise FileNotFoundError(f"GDS file not found at {gds_path}")

    lib = gdstk.read_gds(gds_path)
    top_cell = lib.top_level()[0]
    
    print("[*] Flattening cell hierarchy to resolve SREFs/AREFs...")
    top_cell.flatten()
    
    # Resolves: ValueError: Filtering is only enabled if both layer and datatype are set.
    print(f"[*] Extracting polygons for Layer {layer}, Datatype {datatype}...")
    polygons = top_cell.get_polygons(layer=layer, datatype=datatype)
    
    print(f"[*] Filtering polygons within bounding box: X[{min_x}, {max_x}], Y[{min_y}, {max_y}]")
    filtered_polygons = []
    
    for poly in polygons:
        poly_bb = poly.bounding_box()
        if poly_bb is None:
            continue
            
        p_min_x, p_min_y = poly_bb[0]
        p_max_x, p_max_y = poly_bb[1]
        
        # Check if the polygon's bounding box overlaps with our target region
        overlap_x = (p_min_x <= max_x) and (p_max_x >= min_x)
        overlap_y = (p_min_y <= max_y) and (p_max_y >= min_y)
        
        if overlap_x and overlap_y:
            filtered_polygons.append(poly)
            
    print(f"[*] Found {len(filtered_polygons)} polygons in the specified region.")
    
    if len(filtered_polygons) == 0:
        print("[!] No polygons found. Exiting.")
        return

    # CRITICAL GEANT4 FIX: Merge overlapping polygons using a boolean OR.
    # In GDS design, metals are often drawn overlapping to ensure electrical continuity.
    # If exported directly to Geant4, overlapping G4ExtrudedSolids will cause navigation 
    # track errors and particles will get "stuck". We must merge them into disjoint shapes first.
    print("[*] Merging overlapping polygons to prevent Geant4 volume overlap errors...")
    merged_polygons = gdstk.boolean(filtered_polygons, [], "or")
    
    print(f"[*] Post-merge, there are {len(merged_polygons)} distinct disjoint polygons.")
    
    # Extract points for JSON serialization
    export_data = []
    for poly in merged_polygons:
        # poly.points is a numpy array; convert to a standard python list of [x, y] lists
        export_data.append(poly.points.tolist())
        
    # Save out to a JSON file so it can be parsed by your C++ auto-generator
    with open(output_path, 'w') as f:
        json.dump(export_data, f, indent=4)
        
    print(f"[*] Successfully saved extracted polygons to {output_path}")


if __name__ == "__main__":
    # 1. Define inputs
    gds_file = "RQB1_gap_eng_paper.gds"
    output_file = "../../output/extracted_chip_5_0.json"
    
    # 2. Define target layer and datatype
    target_layer = 5
    target_datatype = 0
    
    # 3. Define the region of interest
    # Grouped as a list of tuples to keep it generalizable for future CSV ingestion
    region = [
        (-75, -5525), 
        (-75, -10225), 
        (-5075, -5225), 
        (-5075, -10255)
    ]
    
    # Example of how a CSV could be loaded in the future:
    # csv_file = "chip_coordinates.csv"
    # if os.path.exists(csv_file):
    #     region = load_coords_from_csv(csv_file)
    
    # 4. Execute extraction
    extract_chip_geometry(gds_file, target_layer, target_datatype, region, output_file)