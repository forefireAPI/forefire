from __future__ import print_function
from __future__ import division
from builtins import range
from builtins import object
from past.utils import old_div
import os
import sys
from pyproj import Proj, transform
import json
import datetime
import pdb 
import geopandas as gpd 
import shapely 
import pyproj
import geojson
from functools import partial
import copy

#http://dev.firecaster.valabre.fr/forefireAPI.php?command=ignition&longitude=42.348570491488999&date_simulation=2020-01-04T09%3A37%3A09Z&date_eclosion=2020-02-04T09%3A37%3A09Z&latitude=9.017890207687699&pathdate=2020-01-07_T10_47_15Z
    
#http://dev.firecaster.valabre.fr/forefireAPI.php?command=sequence&duree_h=4&vent_direction_degre=20&vent_vitesse_kmh=50&temperature=10&hygrometrie=10&pathdate=2020-01-07_T10_47_15Z
    
    
#     $duree_h=$_REQUEST['duree_h'];
# 11 $vent_direction_degre=$_REQUEST['vent_direction_degre'];
# 12 $vent_vitesse_kmh=$_REQUEST['vent_vitesse_kmh'];
# 13 $hygrometrie=$_REQUEST['hygrometrie'];
# 14 $temperature=$_REQUEST['temperature'];
# 15 $pathdate=$_REQUEST['pathdate'];

#################################
def projGeometry2WGS84(feature,projin):

    # the shapely projection function
    datum_wgs84 = pyproj.Proj(init='EPSG:4326') 
    projection_wm_func = partial(pyproj.transform, projin, datum_wgs84)

    # project GPS lat/lon coordinates to web mercator for each polygon in the geojson
    polygon = shapely.geometry.shape(feature['geometry'])
    return [point for polygon in shapely.ops.transform(projection_wm_func, polygon) for point in polygon.exterior.coords[:-1]]  

#################################
class FFGEO(object):
        
    def __init__(self, name, proj):
        self.json = {"type": "FeatureCollection",
                       "name": "Front name:%s"%name,
                       "features": [] }
        self.proj = proj
        self.fronts  = []

    def addFront(self,ff):
        
        def parse(dataString):

            def proj(point,inProj = None, outProj = None):
                if inProj is None or outProj is None:
                    return point
                return transform(inProj,outProj,point[0],point[1])

            def isPoint(element):
                if len(element) == 2:
                    if isinstance(element[0],float) and isinstance(element[1],float) :
                        return True
                return False

            def getLocationFromLine(line,pattern="loc=("):
                llv = line.split(pattern)
                if len(llv) < 2: 
                    return None
                llr = llv[1].split(",");
                if len(llr) < 3: 
                    return None
                return (float(llr[0]),float(llr[1]))
            '''
            def printToPolygons(self, linePrinted, inProj = None, outProj = None, level=1):
                if level > 8:
                     return 

                #pointsMap

                fronts = linePrinted.split("\n%sFireFront"%('\t'*level))
                
                #out = []

                if len(fronts)>0:
                    nodes = fronts[0].split("FireNode")
                    if len(nodes) > 1:
                        pointsMap = []
                        for node in nodes[1:]:
                            pointsMap.append(proj(getLocationFromLine(node),inProj, outProj))
                        pointsMap.append(proj(getLocationFromLine(nodes[1]),inProj, outProj))
                        self.fronts.append([level, pointsMap])
                        print level

                    for subline in fronts[1:]:
                        self.fronts.append(printToPolygons(self,subline,inProj, outProj, level+1))
            '''
            def printToPolygons(linePrinted, inProj = None, outProj = None, level=1):
                if level > 8:
                     return 
                fronts = linePrinted.split("\n%sFireFront"%('\t'*level))
                
                out = []
                if len(fronts)>0:
                    nodes = fronts[0].split("FireNode")
                    if len(nodes) > 1:
                        pointsMap = []
                        for node in nodes[1:]:
                            pointsMap.append(proj(getLocationFromLine(node),inProj, outProj))
                        pointsMap.append(proj(getLocationFromLine(nodes[1]),inProj, outProj))
                        out.append([level,pointsMap])

                    for subline in fronts[1:]:
                        out.append(printToPolygons(subline,inProj, outProj, level+1))

                return out 
           
            def splitPoly(self, treePoly, dadLevel=999, featId_here=0):
                
                sousFront = []
                for poly in treePoly[0]:
                    
                    if (len(poly)==2) & (isinstance(poly[0], int) ):
                        levelHere = old_div(poly[0],2)
                        if levelHere > dadLevel: 
                            self.out.append([])
                            self.featId += 1
                            featId_here = self.featId
                        #print levelHere, dadLevel, self.featId, featId_here
                        self.out[featId_here].append(poly[1])
                        dadLevel=levelHere 
                    
                    else:
                        splitPoly(self, [poly], dadLevel=dadLevel, featId_here=featId_here)

            
            treePoly = printToPolygons(dataString)
            self.out   =  [[]]
            self.featId = 0 
            splitPoly(self, treePoly)
           
            return self.out

        
        fronts_polygons = parse(ff.execute("print[]"))
        
        feature_template = { "type": "Feature", 
                             "properties": { "area": 999, 
                                             "date": 777,
                                             "fill":"#ffffff",
                                             "fill-opacity":0,
                                             "stroke": "#555555",
                                             "stroke-opacity": 1,
                                             "stroke-width":"2",
                                      },
                             "geometry": { "type": "MultiPolygon", 
                                         }, 
                           }

        for front_coords in fronts_polygons:

            feature = copy.deepcopy(feature_template)
            try:
                feature['properties']['area'] = shapely.geometry.Polygon(front_coords[0]).area*1.e-4
                feature['properties']['date'] =ff.getString("ISOdate")
                feature['geometry']['coordinates'] = [front_coords]
            except: 
                pdb.set_trace()
            self.json['features'].append(feature)

        #re-compute geopanda dataframe from geojson
        try:
            self.pd = gpd.GeoDataFrame.from_features(self.json["features"])
            self.pd['date'] = [datetime.datetime.strptime(date, '%Y-%m-%dT%H:%M:%SZ') for date in self.pd['date'] ]
            self.pd['timetofront'] = [ (self.pd['date'].iloc[-1]-date).total_seconds() for date in self.pd['date'] ]
            self.pd = self.pd.sort_values(by='timetofront', ascending=True)
            self.pd.loc[self.pd['timetofront']>4.*3600, 'timetofront'] = 4.*3600
        except:
            pdb.set_trace()

    def dumgeojson(self,filename):

        # project geojson to 4326
        ffgeojson = dict(self.json)
        for i in range(len(ffgeojson['features'])):
            ffgeojson['features'][i]['geometry']['coordinates'] = [[projGeometry2WGS84(ffgeojson['features'][i],self.proj)]]

        #and save 
        with open(filename, 'w') as f:
            geojson.dump(ffgeojson, f)


if __name__ == '__main__':
    
    ff = forefire.PLibForeFire()
    ffFronts = ffgeojson('test')
    ffFronts.addFront(ff)
    
    print(ffFronts.json)
