#!/usr/bin/env python3
"""
geometry_converter.py

Parses a structured JSON file containing metadata and polygon coordinates.
Provides a boolean toggle to export either as a 3D STL (new approach) or 
a 2D GDSII file (old approach).

Updated to handle topological holes (e.g., rings/donuts).
"""

import json
import argparse
import sys
import numpy as np
import gdspy
import trimesh
from shapely.geometry import Polygon
from trimesh.creation import extrude_polygon

def convert_to_gds(geometry_data, output_path, default_layer=1):
    """Fallback to the original GDS output logic, now with hole support."""
    layer_num = geometry_data.get("metadata", {}).get("layer", default_layer)
    polygons = geometry_data.get("polygons", [])

    lib = gdspy.GdsLibrary()
    cell = lib.new_cell('DETECTOR_GEOMETRY')

    valid_polygons = 0
    for index, item in enumerate(polygons):
        if not item:
            continue
        try:
            # Handle new dictionary format with holes
            if isinstance(item, dict):
                exterior = item.get("exterior", [])
                holes = item.get("holes", [])
                
                if len(exterior) < 3:
                    continue
                
                # Create the main outer polygon
                poly = gdspy.Polygon(exterior, layer=layer_num)
                
                # If there are holes, we need to subtract them
                if holes:
                    hole_polys = [gdspy.Polygon(h, layer=layer_num) for h in holes if len(h) >= 3]
                    if hole_polys:
                        poly = gdspy.boolean(poly, hole_polys, 'not', layer=layer_num)
                
                if poly:
                    cell.add(poly)
                    valid_polygons += 1

            # Handle old flat list format (backward compatibility)
            elif isinstance(item, list) and len(item) >= 3:
                poly = gdspy.Polygon(item, layer=layer_num)
                cell.add(poly)
                valid_polygons += 1

        except Exception as e:
            print(f"Error processing polygon at index {index}: {e}")

    lib.write_gds(output_path)
    print(f"[*] Success: {valid_polygons} polygons written to {output_path} (GDS).")

def convert_to_stl(geometry_data, output_path):
    """Converts 2D JSON polygons (and their holes) into an extruded 3D STL mesh."""
    # Convert nm to um for the thickness extrusion
    thickness_nm = geometry_data.get("metadata", {}).get("thickness_nm", 200.0)
    thickness_um = thickness_nm / 1000.0 
    polygons = geometry_data.get("polygons", [])

    meshes = []
    valid_polygons = 0

    print(f"[*] Extruding polygons to {thickness_um} um thickness...")
    for index, item in enumerate(polygons):
        if not item:
            continue
            
        try:
            # Handle new dictionary format with holes
            if isinstance(item, dict):
                exterior = item.get("exterior", [])
                holes = item.get("holes", [])
                if len(exterior) < 3:
                    continue
                # Shapely inherently understands how to subtract holes from a shell
                shapely_poly = Polygon(shell=exterior, holes=holes)
                
            # Handle old flat list format (backward compatibility)
            elif isinstance(item, list) and len(item) >= 3:
                shapely_poly = Polygon(item)
                
            else:
                continue
            
            # Extrude into 3D mesh
            mesh = extrude_polygon(shapely_poly, height=thickness_um)
            meshes.append(mesh)
            valid_polygons += 1
            
        except Exception as e:
            print(f"Error meshing polygon at index {index}: {e}")

    if not meshes:
        print("Error: No valid 3D meshes could be generated.")
        sys.exit(1)

    # Concatenate all sub-meshes into a single scene/mesh and export
    combined_mesh = trimesh.util.concatenate(meshes)
    combined_mesh.export(output_path, file_type='stl_ascii')
    print(f"[*] Success: {valid_polygons} polygons extruded and written to {output_path} (STL).")

def main():
    parser = argparse.ArgumentParser(description="Convert JSON chip geometry to STL or GDS.")
    parser.add_argument("input_json", help="Path to the input JSON file.")
    parser.add_argument(
        "--stl", 
        action="store_true", 
        help="Boolean toggle: If set, exports a 3D STL file. Otherwise, defaults to 2D GDS."
    )
    
    args = parser.parse_args()

    # Load JSON
    try:
        with open(args.input_json, 'r') as f:
            geometry_data = json.load(f)
    except Exception as e:
        print(f"Fatal Error reading JSON: {e}")
        sys.exit(1)

    if "polygons" not in geometry_data or not geometry_data["polygons"]:
        print("Fatal Error: No polygons found in the JSON file.")
        sys.exit(1)

    # Toggle based on boolean flag
    if args.stl:
        output_file = args.input_json.replace(".json", ".stl")
        convert_to_stl(geometry_data, output_file)
    else:
        output_file = args.input_json.replace(".json", ".gds")
        convert_to_gds(geometry_data, output_file)

if __name__ == "__main__":
    main()