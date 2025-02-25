import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# Fuel palette string
fuelPalette = """# S2GLC - Land cover classification of Sentinel-2
# CBK PAN, http://s2glc.cbk.waw.pl/extension
# Detailed changelog can be found at: http://s2glc.cbk.waw.pl/extension
#
# QGIS Generated Color Map Export File
# Pixel value, R, G, B, Opacity, Class name
INTERPOLATION:INTERPOLATED
0,255,255,255,255,Clouds
10,0,100,0,255,Tree cover
20,255,187,34,255,Shrubland
30,255,255,76,255,Grassland
40,240,150,255,255,Cropland
50,250,0,0,255,Built-up
60,180,180,180,255,Bare / sparse vegetation
70,240,240,240,255,Snow and Ice
62,210,0,0,255,Artificial surfaces and constructions
73,253,211,39,255,Cultivated areas
75,176,91,16,255,Vineyards
80,0,100,200,255,Permanent water bodies
82,35,152,0,255,Broadleaf tree cover
83,8,98,0,255,Coniferous tree cover
90,0,150,160,255,Herbaceous wetland
95,0,207,117,255,Mangroves
100,250,230,160,255,Moss and lichen
102,249,150,39,255,Herbaceous vegetation
103,141,139,0,255,Moors and Heathland
104,95,53,6,255,Sclerophyllous vegetation
105,149,107,196,255,Marshes
106,77,37,106,255,Peatbogs
121,154,154,154,255,Natural material surfaces
123,106,255,255,255,Permanent snow covered surfaces
162,20,69,249,255,Water bodies
255,255,255,255,255,No data"""

def parse_fuel_palette(palette_str):
    """
    Parse the fuelPalette string into a dictionary mapping pixel values to (r, g, b, a, class_name).
    For pixel values 0 and 255 the alpha is forced to 0.
    """
    palette_dict = {}
    for line in palette_str.splitlines():
        line = line.strip()
        if not line or line.startswith("#") or line.startswith("INTERPOLATION"):
            continue
        parts = line.split(',')
        if len(parts) < 6:
            continue
        try:
            pixel = int(parts[0])
            r = int(parts[1])
            g = int(parts[2])
            b = int(parts[3])
            a = int(parts[4])
        except ValueError:
            continue
        class_name = parts[5].strip()
        # Force transparency for indices 0 and 255
        if pixel in (0, 255):
            a = 0
        palette_dict[pixel] = (r, g, b, a, class_name)
    return palette_dict

def generate_colormap_header_generic(colormaps, num_samples=10, scalemap=[0, 0.9, 1],
                                     alpha=[0.3, 0.6, 0.8, 1], cbarPath=None, cbarTextRange=[2, 90]):
    """
    Generate a C++ header snippet for a list of matplotlib colormaps.
    For each colormap, create an array with num_samples values.
    If cbarPath is provided, a colorbar image is generated.
    """
    header = ""
    # Prepare interpolation arrays for scaling
    scale_input = np.linspace(0, 1, len(scalemap))
    alpha_input = np.linspace(0, 1, len(alpha))
    
    for cmap_name in colormaps:
        try:
            cmap = plt.get_cmap(cmap_name)
        except ValueError:
            continue

        header += f"static const ColorEntry {cmap_name}Colormap[colormapSize] = {{\n"
        for i in range(num_samples):
            idx = i / (num_samples - 1)
            idx_scaled = np.interp(idx, scale_input, scalemap)
            color = cmap(idx_scaled)
            a_interp = np.interp(idx, alpha_input, alpha)
            r, g, b = [int(255 * x) for x in color[:3]]
            a_val = int(255 * a_interp)
            header += f"    {{{r}, {g}, {b}, {a_val}}},\n"
        header += "};\n\n"

        if cbarPath is not None:
            fig, ax = plt.subplots(figsize=(2, 10))
            bounds = np.linspace(0, 1, num_samples + 1)
            norm = mcolors.BoundaryNorm(bounds, cmap.N)
            sm = plt.cm.ScalarMappable(norm=norm, cmap=cmap)
            cb = plt.colorbar(sm, cax=ax, ticks=bounds)
            scaled_positions = [np.interp(i/num_samples, scale_input, scalemap) *
                                (cbarTextRange[1]-cbarTextRange[0]) + cbarTextRange[0]
                                for i in range(num_samples+1)]
            cb.set_ticklabels([f"{v:.2f}" for v in scaled_positions])
            plt.tight_layout()
            os.makedirs(cbarPath, exist_ok=True)
            plt.savefig(os.path.join(cbarPath, f"{cmap_name}.png"), bbox_inches='tight', dpi=300)
            plt.close()
    return header

def generate_fuel_header(palette_str, cbarPath=None):
    """
    Generate a C++ header snippet for the 'fuel' colormap.
    The fuel colormap is an array of 256 ColorEntry values; for indices not defined
    in the palette the default transparent color {255,255,255,0} is used.
    """
    palette = parse_fuel_palette(palette_str)
    fuel_size = 256
    header = f"static const size_t fuelColormapSize = {fuel_size};\n\n"
    header += f"static const ColorEntry fuelColormap[fuelColormapSize] = {{\n"
    default_color = (255, 255, 255, 0)
    for i in range(fuel_size):
        if i in palette:
            r, g, b, a, class_name = palette[i]
            header += f"    {{{r}, {g}, {b}, {a}}}, // {i} - {class_name}\n"
        else:
            header += f"    {{{default_color[0]}, {default_color[1]}, {default_color[2]}, {default_color[3]}}}, // {i} - no value\n"
    header += "};\n\n"
    
    if cbarPath is not None:
        # Build a list of colors and labels for defined indices only
        colors = []
        tick_labels = []
        for i in range(fuel_size):
            if i in palette:
                r, g, b, a, class_name = palette[i]
                colors.append((r/255, g/255, b/255, a/255))
                tick_labels.append(class_name)
            else:
                colors.append((default_color[0]/255, default_color[1]/255, default_color[2]/255, default_color[3]/255))
                tick_labels.append("")
        cmap = mcolors.ListedColormap(colors, name="fuel")
        fig, ax = plt.subplots(figsize=(2, 10))
        bounds = np.linspace(0, fuel_size, fuel_size+1)
        norm = mcolors.BoundaryNorm(bounds, cmap.N)
        sm = plt.cm.ScalarMappable(norm=norm, cmap=cmap)
        cb = plt.colorbar(sm, cax=ax, ticks=np.arange(0.5, fuel_size, 1))
        cb.ax.set_yticklabels(tick_labels)
        plt.tight_layout()
        os.makedirs(cbarPath, exist_ok=True)
        plt.savefig(os.path.join(cbarPath, "fuel.png"), bbox_inches='tight', dpi=300)
        plt.close()
    return header

def generate_combined_header(output_file, generic_colormaps, num_samples=50,
                             scalemap=[0, 0.2, 0.5, 1], alpha=[0.3, 0.5, 0.9, 1],
                             cbarPath="cmaps/"):
    """
    Generate the complete C++ header including generic colormaps and the fuel colormap,
    plus a single static map with all colormaps.
    """
    # Start header guard and common includes
    header_content = """#ifndef COLORMAP_H
#define COLORMAP_H

#include <array>
#include <map>
#include <string>

typedef std::array<unsigned char, 4> ColorEntry;
constexpr size_t colormapSize = %d;

""" % num_samples

    # Generate generic colormaps
    header_content += generate_colormap_header_generic(generic_colormaps, num_samples, scalemap, alpha, cbarPath)

    # Generate fuel colormap (with size fixed to 256)
    header_content += generate_fuel_header(fuelPalette, cbarPath)

    # Build the single colormapMap with all entries
    header_content += "static const std::map<std::string, const ColorEntry*> colormapMap = {\n"
    for cmap_name in generic_colormaps:
        header_content += f'    {{"{cmap_name}", {cmap_name}Colormap}},\n'
    header_content += '    {"fuel", fuelColormap},\n'
    header_content += "};\n"
    header_content += "#endif // COLORMAP_H\n"

    # Write header to file
    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    with open(output_file, "w") as f:
        f.write(header_content)
    print(f"Header file written to {output_file}")

# --- Usage Example ---

generic_colormaps = ['hot', 'viridis', 'plasma', 'plasma_r', 'coolwarm', 'grey', 'hot_r', 'spring', 'jet', 'turbo']
output_header_file = os.path.join("..", "src", "colormap.h")
generate_combined_header(output_header_file, generic_colormaps, num_samples=256,
                         scalemap=[0, 0.2, 0.5, 1], alpha=[0.3, 0.5, 0.9, 1],
                         cbarPath="cmaps/")