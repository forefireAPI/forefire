import fiona
import rasterio
import rasterio.plot
import matplotlib as mpl
from descartes import PolygonPatch
fireFile = "/Users/filippi_j/soft/firefront/Examples/villa/villa/progression_ViladeRei_200719.shp"
in_tif = "/Users/filippi_j/soft/firefront/Examples/villa/DomDB59.tif";



src = rasterio.open(in_tif)

with fiona.open(fireFile, "r") as shapefile:
    features = [feature["geometry"] for feature in shapefile]

#rasterio.plot.show((src, 1))
ax = mpl.pyplot.gca()
patches = [PolygonPatch(feature, edgecolor="red", facecolor="none", linewidth=2) for feature in features if feature is not None]
ax.add_collection(mpl.collections.PatchCollection(patches, match_original=True))
