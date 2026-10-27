import sys
import math
import xml.etree.ElementTree as ET
import re

# --- Configuration ---
RELATIVE_TOLERANCE = 1e-5
ABSOLUTE_TOLERANCE = 1e-8
# ---

def parse_coordinates(coord_string):
    """Parses the KML coordinate string into a list of [lon, lat, alt] floats."""
    points = []
    raw_points = [p for p in re.split(r'\s+', coord_string.strip()) if p]
    for point_str in raw_points:
        try:
            coords = [float(c) for c in point_str.split(',')]
            if len(coords) == 2: coords.append(0.0)
            if len(coords) == 3: points.append(coords)
            else: print(f"Warning: Skipping invalid coordinate tuple: {point_str}", file=sys.stderr)
        except ValueError: print(f"Warning: Skipping non-numeric coordinate data: {point_str}", file=sys.stderr)
    return points

def compare_coordinate_lists(list1, list2, rel_tol, abs_tol):
    """Compares two lists of coordinates point by point with tolerance."""
    if len(list1) != len(list2):
        print(f"Error: Coordinate lists have different lengths ({len(list1)} vs {len(list2)})", file=sys.stderr)
        return False
    for i, (p1, p2) in enumerate(zip(list1, list2)):
        if not math.isclose(p1[0], p2[0], rel_tol=rel_tol, abs_tol=abs_tol) or \
           not math.isclose(p1[1], p2[1], rel_tol=rel_tol, abs_tol=abs_tol) or \
           not math.isclose(p1[2], p2[2], rel_tol=rel_tol, abs_tol=abs_tol):
            print(f"Error: Mismatch at coordinate point {i+1}:\n  Got: {p1}\n  Expected: {p2}", file=sys.stderr)
            return False
    return True

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_kml.py <generated_kml_file> <reference_kml_file>", file=sys.stderr)
        sys.exit(2)
    file1_path, file2_path = sys.argv[1], sys.argv[2]
    try:
        tree1 = ET.parse(file1_path); root1 = tree1.getroot()
        coords1_elements = root1.findall('.//{http://www.opengis.net/kml/2.2}coordinates') or root1.findall('.//coordinates')
        coords1_text = "".join([c.text for c in coords1_elements if c.text])
        points1 = parse_coordinates(coords1_text)

        tree2 = ET.parse(file2_path); root2 = tree2.getroot()
        coords2_elements = root2.findall('.//{http://www.opengis.net/kml/2.2}coordinates') or root2.findall('.//coordinates')
        coords2_text = "".join([c.text for c in coords2_elements if c.text])
        points2 = parse_coordinates(coords2_text)

        if not points1 or not points2: print("Error: Could not extract coordinates.", file=sys.stderr); sys.exit(1)
        if compare_coordinate_lists(points1, points2, RELATIVE_TOLERANCE, ABSOLUTE_TOLERANCE):
            print(f"KML comparison successful: {file1_path} matches {file2_path} within tolerance.")
            sys.exit(0)
        else: print(f"KML comparison failed: {file1_path} differs from {file2_path}.", file=sys.stderr); sys.exit(1)
    except ET.ParseError as e: print(f"Error parsing XML: {e}", file=sys.stderr); sys.exit(1)
    except FileNotFoundError as e: print(f"Error: File not found - {e}", file=sys.stderr); sys.exit(1)
    except Exception as e: print(f"An unexpected error occurred: {e}", file=sys.stderr); sys.exit(1)