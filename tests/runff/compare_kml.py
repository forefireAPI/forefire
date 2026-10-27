import sys
import math
import re
import xml.etree.ElementTree as ET

# The front is compared by shape, not node by node: the number of nodes
# changes with the CPU (630 on the Linux runner, 636 on macOS, 637 elsewhere
# for the same case), while the shape stays put: the runners land 16-30 m and
# 0.01-0.34% of area from the reference, the pre-#195 front 416 m and 37%.
# --- Configuration ---
MAX_DISTANCE_M = 50.0      # symmetric Hausdorff distance, about two cells of ForeFire.0.nc
MAX_AREA_REL_DIFF = 0.01   # relative difference of the enclosed area
# ---

EARTH_RADIUS_M = 6371000.0


def read_rings(path):
    """Returns each <coordinates> element of the KML as a list of (lon, lat)."""
    root = ET.parse(path).getroot()
    elements = root.findall('.//{http://www.opengis.net/kml/2.2}coordinates') or root.findall('.//coordinates')
    rings = []
    for element in elements:
        ring = []
        for point in re.split(r'\s+', (element.text or '').strip()):
            if point:
                coords = [float(c) for c in point.split(',')]
                ring.append((coords[0], coords[1]))
        if ring:
            rings.append(ring)
    return rings


def to_metres(rings, lon0, lat0):
    """Local equirectangular projection around (lon0, lat0)."""
    kx = math.radians(1) * EARTH_RADIUS_M * math.cos(math.radians(lat0))
    ky = math.radians(1) * EARTH_RADIUS_M
    return [[((lon - lon0) * kx, (lat - lat0) * ky) for lon, lat in ring] for ring in rings]


def area(rings):
    total = 0.0
    for ring in rings:
        s = 0.0
        for (x1, y1), (x2, y2) in zip(ring, ring[1:] + ring[:1]):
            s += x1 * y2 - x2 * y1
        total += abs(s) / 2
    return total


def segments(rings):
    return [seg for ring in rings for seg in zip(ring, ring[1:] + ring[:1])]


def point_segment_distance(p, a, b):
    (px, py), (ax, ay), (bx, by) = p, a, b
    dx, dy = bx - ax, by - ay
    length2 = dx * dx + dy * dy
    t = 0.0 if length2 == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length2))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def directed_distance(rings_from, rings_to):
    segs = segments(rings_to)
    return max(min(point_segment_distance(p, a, b) for a, b in segs)
               for ring in rings_from for p in ring)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare_kml.py <generated_kml_file> <reference_kml_file>", file=sys.stderr)
        sys.exit(2)
    file1_path, file2_path = sys.argv[1], sys.argv[2]
    try:
        rings1, rings2 = read_rings(file1_path), read_rings(file2_path)
    except (ET.ParseError, ValueError, FileNotFoundError) as e:
        print(f"Error reading KML: {e}", file=sys.stderr)
        sys.exit(1)
    if not rings1 or not rings2:
        print("Error: Could not extract coordinates.", file=sys.stderr)
        sys.exit(1)

    lon0, lat0 = rings2[0][0]
    m1, m2 = to_metres(rings1, lon0, lat0), to_metres(rings2, lon0, lat0)
    distance = max(directed_distance(m1, m2), directed_distance(m2, m1))
    area1, area2 = area(m1), area(m2)
    area_diff = abs(area1 - area2) / area2

    print(f"Fronts: {sum(map(len, rings1))} vs {sum(map(len, rings2))} nodes, "
          f"Hausdorff distance {distance:.2f} m (max {MAX_DISTANCE_M}), "
          f"area {area1:.0f} vs {area2:.0f} m2, {area_diff:.2%} apart (max {MAX_AREA_REL_DIFF:.0%})")
    if distance <= MAX_DISTANCE_M and area_diff <= MAX_AREA_REL_DIFF:
        print(f"KML comparison successful: {file1_path} matches {file2_path} within tolerance.")
        sys.exit(0)
    print(f"KML comparison failed: {file1_path} differs from {file2_path}.", file=sys.stderr)
    sys.exit(1)
