import json
import gdspy
import sys

def convert_json_to_gds(input_json, output_gds, layer_num=1):
    """
    Parses a JSON file containing lists of polygon coordinates and 
    exports them as a GDSII file for KLayout visualization.
    """
    try:
        with open(input_json, 'r') as f:
            geometry_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: Could not find {input_json}.")
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Error: {input_json} is not a valid JSON file.")
        sys.exit(1)

    # Initialize a new GDSII library
    lib = gdspy.GdsLibrary()

    # Create a new top-level cell
    cell = lib.new_cell('DETECTOR_GEOMETRY')

    valid_polygons = 0
    # Iterate through the arrays of coordinates
    for index, coords in enumerate(geometry_data):
        if not coords or len(coords) < 3:
            print(f"Warning: Skipping item {index} - not enough vertices to form a polygon.")
            continue
        
        try:
            # gdspy automatically handles the closing of the polygon
            poly = gdspy.Polygon(coords, layer=layer_num)
            cell.add(poly)
            valid_polygons += 1
        except Exception as e:
            print(f"Error processing polygon at index {index}: {e}")

    # Write the compiled geometry to a GDS file
    lib.write_gds(output_gds)
    print(f"Success: {valid_polygons} polygons written to {output_gds}.")
    print("You can now open this file in KLayout to verify the curves and dimensions.")

if __name__ == "__main__":
    # Save your provided array into a file named 'geometry.json' in the same directory.
    INPUT_FILE = "../../output/extracted_chip_5_0.json"
    OUTPUT_FILE = "verified_geometry.gds"
    
    convert_json_to_gds(INPUT_FILE, OUTPUT_FILE)