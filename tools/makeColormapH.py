import matplotlib.pyplot as plt
import numpy as np
import matplotlib.colors as mcolors

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

def generate_colormap_header(colormaps, num_samples=10, scalemap=[0, 0.9, 1], alpha=[0.3, 0.6, 0.8, 1], cbarPath=None, cbarTextRange=[2, 90]):
    header_content = """
#ifndef COLORMAP_H
#define COLORMAP_H

#include <array>
#include <map>
#include <string>

typedef std::array<unsigned char, 4> ColorEntry;
constexpr size_t colormapSize = {};

""".format(num_samples)

    scale_input = np.linspace(0, 1, len(scalemap))
    alpha_input = np.linspace(0, 1, len(alpha))

    for cmap_name in colormaps:
        try:
            cmap = plt.get_cmap(cmap_name)
        except ValueError:
            continue

        header_content += "static const ColorEntry {}Colormap[colormapSize] = {{\n".format(cmap_name)
        for i in range(num_samples):
            idx = i / (num_samples - 1)
            idx_scaled = np.interp(idx, scale_input, scalemap)
            color = cmap(idx_scaled)
            a_interp = np.interp(idx, alpha_input, alpha)
            r, g, b = [int(255 * x) for x in color[:3]]
            a = int(255 * a_interp)
            header_content += "    {{{}, {}, {}, {}}},\n".format(r, g, b, a)
        header_content += "};\n\n"

        if cbarPath is not None:
            fig, ax = plt.subplots(figsize=(2, 10))
            bounds = np.linspace(0, 1, num_samples + 1)
            norm = mcolors.BoundaryNorm(bounds, cmap.N)
            cb = plt.colorbar(plt.cm.ScalarMappable(norm=norm, cmap=cmap), cax=ax, ticks=bounds)
            scaled_positions = [np.interp(i / num_samples, scale_input, scalemap) * (cbarTextRange[1] - cbarTextRange[0]) + cbarTextRange[0] for i in range(num_samples + 1)]
            cb.set_ticklabels([f"{l:.2f}" for l in scaled_positions])
            plt.tight_layout()
            plt.savefig(f"{cbarPath}{cmap_name}.png", bbox_inches='tight', dpi=300)
            plt.close()

    header_content += "static const std::map<std::string, const ColorEntry*> colormapMap = {\n"
    for cmap_name in colormaps:
        header_content += '    {{"{}", {}Colormap}},\n'.format(cmap_name, cmap_name)
    header_content += "};\n"
    header_content += "#endif // COLORMAP_H\n"
    return header_content

# Usage
colormaps = ['hot', 'viridis', 'plasma', 'plasma_r', 'coolwarm', 'grey', 'hot_r', 'spring', 'jet', 'turbo']
header_content = generate_colormap_header(colormaps, 50, scalemap=[0, 0.2, 0.5, 1], alpha=[0.3, 0.5, 0.9, 1], cbarPath="cmaps/", cbarTextRange=[0, 8000])
with open("../src/colormap.h", "w") as file:
    file.write(header_content)
