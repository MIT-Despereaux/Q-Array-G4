#!/usr/bin/env python3
"""
gds_extract_chip.py

Extracts a region of a GDS file, merges polygons, aggressively slices them 
into a fine grid to prevent Geant4 concave triangulation hangs, and cleans 
collinear/microscopic vertices before exporting to JSON.
"""

import gdstk
import json
import os
import csv
import math

def get_bounding_box(coords):
    x_coords = [p[0] for p in coords]
    y_coords = [p[1] for p in coords]
    return min(x_coords), max(x_coords), min(y_coords), max(y_coords)

def clean_polygon(points, dist_tol=1e-4, area_tol=1e-6):
    """
    Cleans a polygon by removing microscopically close points and 
    mathematically collinear vertices to prevent Geant4 infinite loops.
    """
    if len(points) < 3:
        return points
        
    # 1. Remove adjacent duplicate/microscopically close points
    deduped = [points[0]]
    for pt in points[1:]:
        if math.hypot(pt[0] - deduped[-1][0], pt[1] - deduped[-1][1]) > dist_tol:
            deduped.append(pt)
            
    # Check first and last points
    if len(deduped) > 1 and math.hypot(deduped[-1][0] - deduped[0][0], deduped[-1][1] - deduped[0][1]) <= dist_tol:
        deduped.pop()
        
    if len(deduped) < 3:
        return []

    # 2. Remove collinear points using the triangle area method
    cleaned = []
    n = len(deduped)
    for i in range(n):
        p1 = deduped[i-1]
        p2 = deduped[i]
        p3 = deduped[(i+1)%n]
        
        # Calculate determinant (2x triangle area)
        area = abs(p1[0]*(p2[1] - p3[1]) + p2[0]*(p3[1] - p1[1]) + p3[0]*(p1[1] - p2[1]))
        
        # If the area is greater than tolerance, the points form a valid corner
        if area > area_tol:
            cleaned.append(p2)
            
    return cleaned

def extract_chip_geometry(gds_path, layer, datatype, region_coords, output_path):
    min_x, max_x, min_y, max_y = get_bounding_box(region_coords)
    
    print(f"[*] Loading GDS file: {gds_path}")
    if not os.path.exists(gds_path):
        raise FileNotFoundError(f"GDS file not found at {gds_path}")

    lib = gdstk.read_gds(gds_path)
    top_cell = lib.top_level()[0]
    
    print("[*] Flattening cell hierarchy...")
    top_cell.flatten()
    
    print(f"[*] Extracting polygons for Layer {layer}, Datatype {datatype}...")
    polygons = top_cell.get_polygons(layer=layer, datatype=datatype)
    
    filtered_polygons = []
    for poly in polygons:
        poly_bb = poly.bounding_box()
        if poly_bb is None: continue
            
        p_min_x, p_min_y = poly_bb[0]
        p_max_x, p_max_y = poly_bb[1]
        
        if (p_min_x <= max_x) and (p_max_x >= min_x) and (p_min_y <= max_y) and (p_max_y >= min_y):
            filtered_polygons.append(poly)
            
    print(f"[*] Found {len(filtered_polygons)} polygons in region.")
    if not filtered_polygons: return

    print("[*] Merging overlapping polygons...")
    merged_polygons = gdstk.boolean(filtered_polygons, [], "or")
    
    # -------------------------------------------------------------------------
    # HIGH-DENSITY DICING: 25x25 grid to break concave shapes into rectangles
    # -------------------------------------------------------------------------
    print("[*] Dicing layout into a high-density grid to prevent Geant4 voxelization hangs...")
    diced_polygons = []
    
    grid_x_count = 25
    grid_y_count = 25
    
    step_x = (max_x - min_x) / grid_x_count
    step_y = (max_y - min_y) / grid_y_count
    
    for i in range(grid_x_count):
        for j in range(grid_y_count):
            tile_min_x = min_x + i * step_x
            tile_max_x = min_x + (i + 1) * step_x
            tile_min_y = min_y + j * step_y
            tile_max_y = min_y + (j + 1) * step_y
            
            tile_rect = gdstk.rectangle((tile_min_x, tile_min_y), (tile_max_x, tile_max_y))
            tile_intersection = gdstk.boolean(merged_polygons, tile_rect, "and")
            diced_polygons.extend(tile_intersection)
            
    print(f"[*] Post-dicing, geometry split into {len(diced_polygons)} chunks.")
    
    center_x = (min_x + max_x) / 2.0
    center_y = (min_y + max_y) / 2.0
    
    print("[*] Cleaning and re-centering extracted polygons...")
    export_polygons = []
    
    for poly in diced_polygons:
        # 1. Zero-center the points
        centered_pts = [[pt[0] - center_x, pt[1] - center_y] for pt in poly.points]
        
        # 2. Aggressively clean floating-point artifacts and collinearities
        cleaned_pts = clean_polygon(centered_pts)
        
        # 3. Only keep valid polygons
        if len(cleaned_pts) >= 3:
            export_polygons.append(cleaned_pts)
            
    print(f"[*] Final export count after cleaning: {len(export_polygons)} valid chunks.")
        
    json_output = {
        "metadata": {
            "source_gds": os.path.basename(gds_path),
            "layer": layer,
            "datatype": datatype,
            "units": "um",
            "thickness_nm": 200.0,
            "bounding_box_um": [min_x, max_x, min_y, max_y]
        },
        "polygons": export_polygons
    }

    with open(output_path, 'w') as f:
        json.dump(json_output, f, indent=4)

if __name__ == "__main__":
    gds_file = "RQB1_gap_eng_paper.gds"
    output_file = "../../output/extracted_chip_5_0.json"
    
    target_layer = 5
    target_datatype = 0
    
    region = [
        (-75, -5525), 
        (-75, -10225), 
        (-5075, -5225), 
        (-5075, -10255)
    ]
    
    extract_chip_geometry(gds_file, target_layer, target_datatype, region, output_file)