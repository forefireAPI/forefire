 
from pyproj import Proj, transform, Transformer
import numpy as np 
from shapely.geometry import Polygon 
from datetime import timedelta,datetime
 
import os
import re
from PIL import Image, ImageDraw, ImageFont 
import matplotlib.cm as cm
    
import glob
import pandas as pd
import xarray as xr

 
from fastkml import kml, styles
from shapely.geometry import shape 
import math

from .geo_to_kml import *

class ffData:
    def isPoint(element):
  
        if len(element) == 3:
            if isinstance(element[0],float) and isinstance(element[1],float) and isinstance(element[2],float) :
                return True
        return False
    
    def __init__(self, filenamein=None,mode="batchWeb",destProj='epsg:4326',baseDate=datetime(2020, 1, 1, 0, 0, 0, 0),wsen=None,lbrt=None):
        
        self.fname = filenamein
 
        self.inXYpolygon = []
        self.inLLpolygon = []
        
        if mode == "batchWeb" :
            simpleFname = filenamein.split("/")[-1]
            self.frontCount=int(simpleFname.split("-")[0])
            frontProj=int(simpleFname[simpleFname.find("Z")+2:simpleFname.find(".ff")].split("-")[1])
            epsgstring=('epsg:%d')%frontProj
            self.mode = "batchWeb"
            self.projSRC=Proj(epsgstring)
            self.projDest=Proj(destProj) 
            self.area=0
            laDate=simpleFname[simpleFname.find("-")+1:simpleFname.find("Z")].split("T")
             
            self.frontDate = "%s %s UTC"%(laDate[0],laDate[1].replace('-', ':'))
     
        if mode == "mnhPGD" :
            
            self.frontCount=1
            self.projSRC=None
            self.projDest=None
            self.wsen = wsen
            self.lbrt = lbrt
            self.area=0
            self.localDate = baseDate 
            if filenamein is not None :
                self.localDate = baseDate + timedelta(seconds=int(filenamein.split(".")[-1]))
            self.mode="mnhPGD"
            self.frontDate = self.localDate.isoformat()
            

            
          

            
        if filenamein is not None :
            self.parse()
        self.atime = None

    def lalo2xy(self,lalo):
        if self.projSRC is None or self.projDest is None:
            wsen = self.wsen
            lbrt = self.lbrt
            if wsen is None or lbrt is None:
                return lalo
            x = lbrt[0]+((lalo[1] - wsen[0])/(wsen[2] - wsen[0]))*(lbrt[2]-lbrt[0])
            y = lbrt[1]+((lalo[0] - wsen[1])/(wsen[3] - wsen[1]))*(lbrt[3]-lbrt[1])
            return (x,y,lalo[2])
        return transform(self.projDest,self.projSRC,point[0],point[1])
    
    def xy2lalo(self,xy):
        if self.projSRC is None or self.projDest is None:
            wsen = self.wsen
            lbrt = self.lbrt
            if wsen is None or lbrt is None:
                return xy
            lo = wsen[0]+(float(xy[0] - lbrt[0])/float(lbrt[2] - lbrt[0]))*(wsen[2]-wsen[0])
            la = wsen[1]+(float(xy[1] - lbrt[1])/float(lbrt[3] - lbrt[1]))*(wsen[3]-wsen[1])
       
            return (la,lo,xy[2])       
        return transform(self.projSRC,self.projDest,point[0],point[1])
    
    def setATimeMatrix(self,at):
        self.atime = at
        
    def getATimeMatrix(self):
        return self.atime     

    def getROSMatrix(self, max_speed_filter=1.0):
        if self.atime is  None:
            return None
        
        l,b,r,t = self.lbrt
        norm_data = np.copy(self.atime )
         
        #norm_data = np.where(self.atime < -9990, np.nan, self.atime)
        norm_data[norm_data < 0] =  np.nan
        
        BMapresolution = float( (r-l) / norm_data.shape[0] )
        gradient_y, gradient_x = np.gradient(norm_data, 1)
        dspeed = np.sqrt(gradient_x**2 + gradient_y**2)
        print("computing ROS at resolution :",BMapresolution, dspeed.shape, norm_data.shape, " fitering averything over (in m/s):",max_speed_filter)
        #dspeed[dspeed < 4] = np.nan
        #dspeed[dspeed > 1] = 1
        
        speed = BMapresolution / dspeed
        speed[speed > max_speed_filter] = max_speed_filter

        return speed
    

    
    def getLLPolygon(self):
        def process_front(front, is_main_front=True):
            processed_front = []
            for element in front:
                if ffData.isPoint(element):
                    processed_front.append(self.xy2lalo(element))
                else:
                    # Process nested fronts (holes) and keep them separate
                    sub_front = process_front(element, is_main_front=False)
                    if sub_front:
                        if is_main_front:
                            # Add sub-fronts as holes in the main front
                            processed_front.append(sub_front)
                        else:
                            # Add sub-fronts to the polygon list directly
                            self.inLLpolygon.append(sub_front)
            return processed_front
    
        if len(self.inXYpolygon) > len(self.inLLpolygon):
            for front in self.inXYpolygon:
                main_front = process_front(front)
                if main_front:
                    self.inLLpolygon.append(main_front)
    
        return self.inLLpolygon
    
           
    def getLLPolygonOrigin(self, ):
        if len(self.inXYpolygon) > len(self.inLLpolygon):
            for front in self.inXYpolygon:
                fa = []
                for element in front:
                    if ffData.isPoint(element):
                        fa.append(self.xy2lalo(element))
                    else:
                        fb = [] 
                        for el2 in element:
                            if ffData.isPoint(el2):
                                fb.append(self.xy2lalo(el2))
                            else:
                                print("WARNING SUB-SUB-SUB front not parsed")
                        fa.append(fb)
                self.inLLpolygon.append(fa)
                    
        return self.inLLpolygon
    
    def getCartesianPolygon(self, ):
        
        return self.inXYpolygon
    
    def toKML(self, ):
        return to_kml(self.toGeoJson())
 
    
    def toVTK(self, ):
        return 0


    
    
    def getFlattenPathArrays(self,projString = 'epsg:32632'): # default UTM32
        import matplotlib.path as mpath
     
        wgs84 = Proj('epsg:4326')
        lambert4_carto = Proj(projString)
        transformer = Transformer.from_proj(wgs84, lambert4_carto)
        
        def latlon_to_lambert4(lat, lon):
            # Convertir de WGS84 Lat/Lon à Lambert 4 Carto
            x, y = transformer.transform( lat,lon)
            return x, y
        
        def polygon_area(path):
            # Calculer l'aire d'un polygone défini par un Path
            vertices = path.vertices
            n = len(vertices)
            area = 0.5 * np.sum(vertices[i][0]*vertices[(i+1)%n][1] - vertices[(i+1)%n][0]*vertices[i][1] for i in range(n))
            return area
        
        def create_pathes_from_polygon(polygons):
            mainfronts = []
            otherfronts = []
        
            # Séparer les points principaux et les autres fronts
            for polygon in polygons:
          
                newFront = []
                for elem in polygon:
                    if (len(elem) == 3):
                        newFront.append(elem)  # Inverser lat, lon si nécessaire
                    else:
                        
                        otherfronts.append(elem)
                mainfronts.append(newFront[::-1])
                
            for polygon in otherfronts:
             
                newFront = []
                for elem in polygon:
                    if (len(elem) == 3):
                        newFront.append(elem)  # Inverser lat, lon si nécessaire
                    else:
                        print("------should not be here ",len(elem))
                       
                mainfronts.append(newFront[::-1])
                
        
   
        
            
            # Créer un Path pour chaque front
            paths = []
            for front in mainfronts:
                lambert_points = [latlon_to_lambert4(lat, lon) for lat, lon, _ in front]
                paths.append(mpath.Path(np.array(lambert_points, dtype=np.float64)))
        
            return sorted(paths, key=polygon_area)[::-1]
         
        


        ll = self.getLLPolygon()
        
        boollArrays = []
        areaArrays = []
        lambertArrays = create_pathes_from_polygon(ll)
        for path in lambertArrays:
            area = polygon_area(path)
            is_ccw = area > 0
            boollArrays.append(is_ccw)
            areaArrays.append(area)
    
        return lambertArrays, boollArrays, areaArrays 
    
    def toGeoJson(self, ):
        llPoly = self.getLLPolygon()
        
        geoJSonStyleData = []
        
        for front in llPoly:
            mainfront = []
            otherfronts = []
            for element in front:
                if ffData.isPoint(element): 
                    mainfront.append((element[1],element[0],element[2]))
                else:
                    otherfronts.append(element)
 
            geoJSonStyleData.append(mainfront[::-1])
            
            for nfront in otherfronts:
                holesFronts  = []
                for element in nfront:
                    if ffData.isPoint(element): 
                        holesFronts.append((element[1],element[0],element[2]))
                    else:
                        print("Error Json Parsing fronts, too many holes in holes") 
                geoJSonStyleData.append(holesFronts[::-1])
                
   
   
        geometryInfo={}   
        geometryInfo["numberOfPolygons"]=  len(geoJSonStyleData)
 
 
            
        self.area = self.metadata["totalArea"]
        data = {
        "type": "FeatureCollection",
        "date": self.frontDate, 
        "details":self.metadata ,
        "features": [
        { "type": "Feature", "properties": { "numberOfPolygons": len(geoJSonStyleData) }, 

        "geometry": { "type": "MultiPolygon", "coordinates": [geoJSonStyleData]
        } 
        }
        ]
        }
        return data
        return 0
    
    def toFFPrint(self, ):
        return 0        
    
    def getDomainInitString(self, secondsSinceMidnight=0):
        l,b,r,t = self.lbrt
        
        return "FireDomain[sw=(%d,%f,0);ne=(%f,%f,0);t=%d]"%(l,b,r,t,secondsSinceMidnight)
      
    def parse(self, ):            

        
        def getLocationFromLine(line):
            match = re.search(r"loc=\(([^,]+),([^,]+),([^)]+)\)", line)
            if match:
                x, y, z = map(float, match.groups())
                return (x, y, z)
            else:
                return None
        
            return (float(llr[0]),float(llr[1]),float(llr[2]))
        
        def getMarkPropFromLine(line):
      
            Mpos = getLocationFromLine(line)
           # Mvel = (1,1)#getLocationFromLine(line,pattern="vel=(")
         #   print("here ", Mvel)
            return Mpos#, Mvel #+ Mvel
    
        def printToPolygons(linePrinted, level=1):
            if level > 8:
                 return 

            fronts = linePrinted.split("\n%sFireFront"%('\t'*level))

            pointsMap = []
            if len(fronts)>0:
                nodes = fronts[0].split("FireNode")
                if len(nodes) > 1:
                    for node in nodes[1:]:
                        #ptl, sptl = getMarkPropFromLine(node)
                        pointsMap.append(getMarkPropFromLine(node))
                    #ptl, sptl =  
                    pointsMap.append(getMarkPropFromLine(nodes[1]))

                for subline in fronts[1:]:
                    pointsMap.append(printToPolygons(subline,level+1))

            return pointsMap
        def process_fronts(fronts):
            main_front = []
            other_fronts = []
        
            def process_subfronts(front, level=0):
                subfront = []
                for element in front:
                    if ffData.isPoint(element):
                        if level == 0:
                            main_front.append(element)
                        else:
                            subfront.append(element)
                    else:
                        other_fronts.append(process_subfronts(element, level+1))
                return subfront if subfront else None
        
            for front in fronts:
                subfront = process_subfronts(front)
                if subfront:
                    other_fronts.append(subfront)
        
            # Clean other_fronts to remove empty or None entries
            other_fronts = [front for front in other_fronts if front]
        
            return main_front, other_fronts
        # Usage
        
        self.inXYpolygon = []
        f = open(self.fname, 'r')
        rawdata = f.read()
        firstLine = rawdata.partition('\n')[0]
        print(firstLine)
        llv = firstLine.split("sw=(")
        llr = llv[1].split("ne=(");
        self.lbrt = [  float( llr[0].split(",")[0]), float(llr[0].split(",")[1]), float(llr[1].split(",")[0]), float(llr[1].split(",")[1]) ]
        
        self.inXYpolygon = printToPolygons(rawdata)
        
        f.close()
        
        metaHelper = []
        mainfront, otherfronts = process_fronts(self.inXYpolygon)
        # for front in self.inXYpolygon :
        #     mainfront = []
        #     otherfronts = []
        #     for element in front:
        #         if ffData.isPoint(element): 
        #             mainfront.append(element)
        #         else:
        #             newFront= []
        #             for subelement in element:
        #                 if ffData.isPoint(subelement): 
        #                     newFront.append(subelement)
        #                 else:
        #                     for subsubelement in subelement:
        #                         if ffData.isPoint(subelement): 
        #                             .......
        #                         else:
        #                             print("OmitingSubfront")
        #             otherfronts.append(newFront)
 
        metaHelper.append(mainfront[::-1])
        for element in otherfronts:
            metaHelper.append(element[::-1])
                            
        self.metadata = {}   
        self.metadata["numberOfPolygons"]=  len(metaHelper)
        totalArea = 0
        totalLength = 0
        
        for na,a in enumerate(metaHelper):
            pgon = Polygon(a)
            self.metadata["P%d"%na]={"area":pgon.area,"perimeter":pgon.length, "directWinding":pgon.exterior.is_ccw}
       
            totalLength = totalLength+pgon.length
            totalArea += pgon.area if pgon.exterior.is_ccw else -pgon.area

        
        self.metadata["totalLength"]=  totalLength
        self.metadata["totalArea"]=  totalArea
       

def get_WSEN_LBRT_ZS_From_Pgd(pgd_path):
    pgd = xr.load_dataset(pgd_path)
    dlon = float(pgd.longitude[0,1]-pgd.longitude[0,0])
    dlat = float(pgd.latitude[1,0]-pgd.latitude[0,0])
    dx = float(pgd.XHAT[1]-pgd.XHAT[0])
    dy = float(pgd.YHAT[1]-pgd.YHAT[0])
 
    return (
        (
        float(pgd.longitude.min())-dlon/2,
        float(pgd.latitude.min())-dlat/2,
        float(pgd.longitude.max())+dlon/2,
        float(pgd.latitude.max())+dlon/2
        ),
        (
        float(pgd.XHAT.min())-dx/2,
        float(pgd.YHAT.min())-dy/2,
        float(pgd.XHAT.max())+dx/2,
        float(pgd.YHAT.max())+dy/2,
        ),
        pgd.ZS
    )

 
def areaBurnt():    
    contours1  = glob.glob('/Users/filippi_j/data/2022/pedrogao/OutputsFullCoupled/output.0.*')
    contours2  = glob.glob('/Users/filippi_j/data/2022/pedrogao/OutputsNoCoupled/output.0.*')
    

        
    for contour in contours1:
        ffGSon= ffToGeoJson(contour,mode = "mnhPGD", baseDate=baseDate)
        d = ffGSon.parse()
        area = d["details"]["P0"]["area"] 
        date = datetime.fromisoformat(d["date"])
        v = [date , area]
        lCnt.append( v )
    
        
    
    dfc = pd.DataFrame(lCnt, columns=['date','HaCoupled'])
    dfc["HaCoupled"] = dfc["HaCoupled"]/10000
    
    dfc = dfc.set_index("date")
    dfc.sort_values(by=['date'], inplace=True)
    
    lCnt = []
    for contour in contours2:
        ffGSon= ffToGeoJson(contour,mode = "mnhPGD",baseDate=baseDate)
        d = ffGSon.parse()
        area = d["area"] 
        date = datetime.fromisoformat(d["date"])
        v = [date , area]
        lCnt.append( v )
            
    
    dnc = pd.DataFrame(lCnt, columns=['date','HaNoCoupled'])
    dnc["HaNoCoupled"] = dnc["HaNoCoupled"]/10000
    
    dnc = dnc.set_index("date")
    dnc.sort_values(by=['date'], inplace=True)
    
     
    
     
    totalBurnArea = np.count_nonzero(dBmap.arrival_time_of_front > 000000)
    mSize = (dBmap.domain.Lx/dBmap.arrival_time_of_front.shape[0]) * (dBmap.domain.Ly/dBmap.arrival_time_of_front.shape[1])
    lCnt = []
    for lDate in dnc.index:
        yetBurnt =( totalBurnArea - np.count_nonzero(dBmap.arrival_time_of_front > (lDate-baseDate).seconds))*mSize
        lCnt.append( [lDate, yetBurnt ])
     
    
    doc = pd.DataFrame(lCnt, columns=['date','HaObserved'])
    doc["HaObserved"] = doc["HaObserved"]/10000
    
    doc = doc.set_index("date")
    doc.sort_values(by=['date'], inplace=True)
    
    dac = pd.concat([dfc,dnc,doc], join='inner', axis=1)
    dac.plot()       


def geojson_to_placemark(geojson_obj, timestamp):
    # Convertir la géométrie GeoJSON en géométrie Shapely
    geom = shape(geojson_obj['geometry'])

    # Créer un Placemark KML
    pm = kml.Placemark()
    pm.geometry = geom
    pm.timestamp = timestamp

    # Créer et définir le style (contour orange de 2 points, pas de remplissage)
    style = styles.Style()
    style.linestyle.color = styles.Color.rgb(255, 165, 0)  # Orange
    style.linestyle.width = 2  # 2 points de large
    style.polystyle.fill = 0  # Pas de remplissage
    pm.style = style

    return pm

def geojsons_to_kml(geojson_objs):
    # Créer un KML
    k = kml.KML()

    # Créer un document pour contenir les Placemarks
    doc = kml.Document()
    k.append(doc)

    # Convertir chaque objet GeoJSON en Placemark et l'ajouter au document
    for gj in geojson_objs:
        timestamp_str = gj['properties']['timestamp']  # Assumer une propriété 'timestamp'
        timestamp = datetime.fromisoformat(timestamp_str.replace("Z", "+00:00"))  # Convertir la chaîne en datetime
        pm = geojson_to_placemark(gj, timestamp)
        doc.append(pm)

    return k.to_string(prettyprint=True)

def otherJsonToKml():
    # Utilisation :
    geojson_objs = [
        # Vos objets GeoJSON vont ici
    ]
    kml_str = geojsons_to_kml(geojson_objs)
    with open('output.kml', 'w') as f:
        f.write(kml_str)

def create_kml(west, south, east, north, Name, pngfile, outkml_path, pngcbarfile=None):
    kml_template = '''<?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2" xmlns:gx="http://www.google.com/kml/ext/2.2">
        <Document id="1">'''
    
    if pngcbarfile:
        kml_template += '''
        <ScreenOverlay>
            <name>Légende</name>
            <Icon>
                <href>{pngcbarfile}</href>
            </Icon>
            <overlayXY x="0" y="0" xunits="fraction" yunits="fraction"/>
            <screenXY x="0.05" y="0.05" xunits="fraction" yunits="fraction"/>
            <rotationXY x="0.5" y="0.5" xunits="fraction" yunits="fraction"/>
            <size x="150" y="400" xunits="pixels" yunits="pixels"/>
        </ScreenOverlay>'''
    
    kml_template += '''
            <GroundOverlay id="2">
                <name>{Name}</name>
                <Icon id="3">
                    <href>{pngfile}</href>
                </Icon>
                <LatLonBox>
                    <north>{north}</north>
                    <south>{south}</south>
                    <east>{east}</east>
                    <west>{west}</west>
                </LatLonBox>
            </GroundOverlay>
        </Document>
    </kml>'''

    kml_content = kml_template.format(Name=Name, pngfile=pngfile, north=north, south=south, east=east, west=west, pngcbarfile=pngcbarfile)
    
    with open(outkml_path, "w") as f:
        f.write(kml_content)



def draw_arrows_on_image(image, vx, vy, scale=10):
    draw = ImageDraw.Draw(image)
    nj, ni = vx.shape
    dx_space = image.width / ni
    dy_space = image.height / nj
    
    for j in range(nj):
        for i in range(ni):
            position = (i * dx_space + dx_space/2, j * dy_space + dy_space/2)
            dx = vx[j, i] * scale
            dy = vy[j, i] * scale
            arrow_start = position
            arrow_end = (position[0] + dx, position[1] - dy)
            draw.line((arrow_start, arrow_end), fill='black', width=2)

            angle = math.atan2(dy, dx)
            arrow_head_length = 5 * (scale/10)
            arrow_point1 = (arrow_end[0] - arrow_head_length * math.cos(angle - math.pi/4),
                            arrow_end[1] + arrow_head_length * math.sin(angle - math.pi/4))
            arrow_point2 = (arrow_end[0] - arrow_head_length * math.cos(angle + math.pi/4),
                            arrow_end[1] + arrow_head_length * math.sin(angle + math.pi/4))

            draw.line((arrow_end, arrow_point1), fill='black', width=2)
            draw.line((arrow_end, arrow_point2), fill='black', width=2)




def arrayToPng(data, output_filename, output_cbar_png=None, cmap_str = "viridis",vx=None, vy=None, scale=10, tickRatio=100000):


    # Normaliser les données entre 0 et 255
 
    vmin = np.nanmin(data)
    vmax = np.nanmax(data)
     
    print(vmin, vmax)
    imsize=(data.shape[0]*scale, data.shape[1]*scale)
    revdata = np.flipud(data)
    
    norm_data = ((revdata  - vmin) / (vmax - vmin))
    
    cmap = cm.get_cmap(cmap_str)
    rgba_data = (cmap(norm_data) * 255).astype(np.uint8)
    
    # Rendre les valeurs NaN transparentes
    rgba_data[np.isnan(revdata)] = (0, 0, 0, 0)
    
    # Créer et sauvegarder l'image
    image = Image.fromarray(rgba_data, "RGBA")
    image = image.resize(imsize)
    
    # Si les vecteurs vx et vy sont fournis, dessiner les flèches
    if vx is not None and vy is not None:
        draw_arrows_on_image(image, vx, vy, scale)
    # Enregistrer l'image
    image.save(output_filename)
    
    if output_cbar_png is not None:
        # Créer un gradient 1D de la colormap
        height, width = 200, 50
        gradient = np.linspace(1, 0, height).reshape(-1, 1)  # 1 at the top to 0 at the bottom
        gradient = np.hstack([gradient]*width)  # Repeat for each column
        
        # Apply the colormap to the gradient
        rgba_gradient = (cmap(gradient) * 255).astype(np.uint8)
        
        # Create an extended version of the gradient for ticks and labels
        #extra_width = 50  # 50 extra pixels for ticks and labels
        #extended_rgba_gradient = np.hstack([rgba_gradient, rgba_gradient[:, :extra_width]])
        
        # Create a PIL image from the extended array
        colorbar_image = Image.fromarray(rgba_gradient, "RGBA")
        
        # Prepare the object for drawing and the font
        draw = ImageDraw.Draw(colorbar_image)
        default_font = ImageFont.load_default()
        
        tick_positions = [0, height // 4, height // 2, 3 * height // 4, height - 1]
         
        tick_labels = [
            str(tickRatio*(vmax)), 
            str(tickRatio*(vmin + 0.75 * (vmax - vmin))), 
            str(tickRatio*((vmax + vmin) / 2)), 
            str(tickRatio*(vmin + 0.25 * (vmax - vmin))), 
            str(tickRatio*(vmin))
        ]
        
        # Draw the ticks and labels
        for pos, label in zip(tick_positions, tick_labels):
            font = default_font
            fill = "black"
            vertical_adjustment = -5
            
            # Special formatting for top and bottom labels
            if pos == 0:
                fill = "white"
                vertical_adjustment = 2
            elif pos == height - 1:
                fill = "white"
                vertical_adjustment = -15
            
            # Draw a line for the tick
            draw.line([(5, pos), (5 + 10, pos)], fill="black", width=1)
            # Draw the text for the label
            draw.text((5 + 15, pos + vertical_adjustment), label, fill=fill, font=font)

        # Sauvegarder l'image modifiée
        colorbar_image.save(output_cbar_png)

    
def normalize_rgb(rgb_tuple):
    """
    Normalize the RGB values to [0, 1] range for matplotlib.
    """
    return tuple(np.array(rgb_tuple) / 255.0)

def generate_indexed_png_and_legend(legend_file_path, tif_file_path, output_indexed_png_path, output_legend_png_path=None):
    import matplotlib.pyplot as plt
    import rasterio
    from PIL import Image
    import numpy as np
    
    def normalize_rgb(color):
        return [c / 255.0 for c in color]

    # Read the legend file
    color_palette = {}
    class_names = {}
    with open(legend_file_path, 'r') as f:
        lines = f.readlines()
        lines = [line.strip() for line in lines if line.strip() and line.strip()[0].isdigit()]
        for line in lines:
            parts = line.split(',')
            index = int(parts[0])
            color = tuple(map(int, parts[1:4]))  # R, G, B values
            class_name = parts[-1]  # Class name
            color_palette[index] = color
            class_names[index] = class_name

    # Read the TIFF file
    with rasterio.open(tif_file_path) as src:
        tif_data = src.read(1)  # Reading the first band

    # Generate the indexed PNG
    img_shape = (tif_data.shape[1], tif_data.shape[0])
    img = Image.new('P', img_shape)
    
    # Prepare the palette
    palette = [0] * 768  # 256 colors * 3 (R, G, B)
    for idx, (r, g, b) in color_palette.items():
        palette[idx*3 : idx*3+3] = [r, g, b]
    
    img.putpalette(palette)
    
    # Convert the 2D NumPy array to a 1D array with values in the same order as the image's pixels
    img_data_1d = tif_data.flatten().tolist()
    
    # Populate the image with the 1D array
    img.putdata(img_data_1d)
    
    # Save the image
    img.save(output_indexed_png_path, compress_level=9)

    if output_legend_png_path is None:
        return

    # Find unique values in the TIFF data
    unique_values = np.unique(tif_data)
    if len(unique_values) < 2:
        print("WARNING ::: Found ",len(unique_values), " in fuel data")
    
    # Filter color_palette and class_names to include only unique values
    filtered_color_palette = {key: color_palette[key] for key in unique_values if key in color_palette}
    filtered_class_names = {key: class_names[key] for key in unique_values if key in class_names}
    
    normalized_colors = [normalize_rgb(filtered_color_palette[val]) for val in sorted(filtered_color_palette.keys())]
    all_labels = [f"{val}:{filtered_class_names[val]}" for val in sorted(filtered_class_names.keys())]
    
    # Create the colorbar figure
    fig, ax = plt.subplots(figsize=(4, 10))  # Increase the figure width
    
    # Hide axis and set transparent background
    ax.axis('off')
    fig.patch.set_visible(False)
    ax.axis('off')
    
    # Draw rectangles and text labels
    for i, (idx, color, label) in enumerate(zip(sorted(filtered_color_palette.keys()), normalized_colors, all_labels)):
        rect = plt.Rectangle((0, i), 1, 1, facecolor=color)
        ax.add_patch(rect)
        ax.text(1.2, i + 0.5, label, va='center', fontsize=18, backgroundcolor=(1, 1, 1, 0))  # Increase fontsize
    
    # Set limits and aspect ratio
    ax.set_xlim(0, 4)  # Adjust limits to prevent clipping
    ax.set_ylim(0, len(all_labels))
    ax.set_aspect('auto')
    plt.tight_layout()
    plt.savefig(output_legend_png_path, transparent=True,bbox_inches='tight')

 

def plotRos(PGDFILE, BMAPFILE,max_speed_filter=1.0):
    import matplotlib.pyplot as plt
    wsen, lbrt, ZS = get_WSEN_LBRT_ZS_From_Pgd(PGDFILE)
    dBmap = xr.open_dataset(BMAPFILE)
    fdat = ffData(mode = "mnhPGD", wsen=wsen, lbrt = lbrt)   
    fdat.setATimeMatrix(dBmap.arrival_time_of_front.values)
    ros = fdat.getROSMatrix(max_speed_filter=max_speed_filter)
    vmin,vmax = np.nanmin(ros),np.nanmax(ros)
    print(vmin,vmax)
    #ros[ros == np.nan] = vmax
    plt.imshow(ros[:,:],vmin=vmin,vmax=vmax)



def extract_last_number(filepath):
    import re

    # Extraire le nom de base du fichier (sans le chemin)
    base = os.path.basename(filepath)
    
    # Utiliser une expression régulière pour trouver tous les nombres dans le nom de fichier
    numbers = re.findall(r'\d+', base)
    
    # Renvoyer le dernier nombre trouvé, converti en entier
    return int(numbers[-1]) if numbers else 0

def genCostDict(PGDFILE, BMAPFILE, FFINPUTPATTERN,everyNFronts=1,btslice=None):
    dBmap = xr.open_dataset(BMAPFILE)
    baseDate= datetime(int(dBmap.domain.refYear), 1, 1, 0, 0, 0, 0)
    baseDate= baseDate+timedelta(days=int(dBmap.domain.refDay))
    wsen, lbrt, ZS = get_WSEN_LBRT_ZS_From_Pgd(PGDFILE)
    west, south, east, north = wsen

    
    contours1  = glob.glob(FFINPUTPATTERN)
        
    lCnt=[]
    larea=[]
 
    selectionSorted =  sorted(contours1 ,key=extract_last_number)  
    if btslice is not None:
         selectionSorted = selectionSorted[btslice[0]:btslice[1]]
         
    selectionCutSorted =  sorted(selectionSorted[::everyNFronts],key=extract_last_number) 
 
    nSample = len(selectionCutSorted)
    
    costTimeFormat = '%Y%m%d-%H%M%S'
    oD = {}
    oD['start_datetime'] = baseDate.strftime(costTimeFormat)
    oD['indices'] = ['43']
    oD['scalar_input_data'] = []    
    oD['scalar_label'] = []
    oD['temporal_label'] = ['u', 'v', 'Ta', 'Md', 'Wind speed norm [m/s]']
    oD['temporal_input_data'] =  list(np.zeros((5,nSample)))

    contour_datetime = [None] * nSample 
    area = [None] * nSample 
    positive = [None] * nSample 
    contour = [None] * nSample 

    print(nSample, " contours to process")
    
    for i,contourFile  in enumerate(selectionCutSorted):
        f = ffData(contourFile,mode = "mnhPGD", baseDate=baseDate,wsen=wsen)
        pathes,booleans,surfaces = f.getFlattenPathArrays()
        contour_datetime[i] = f.localDate.strftime(costTimeFormat)
        area[i] = surfaces
        positive[i] = booleans
        contour[i] = pathes
   
    
    oD['contour_datetime'] = contour_datetime
    oD['area'] = [area]
    oD['positive'] = [positive]
    oD['contour'] = [contour]

    return oD
    
    
    # if btslice is not None:
    #     selectionSorted = selectionSorted[btslice[0]:btslice[1]]
    # for contour in selectionSorted[::everyNFronts]:
    #     f = ffData(contour,mode = "mnhPGD", baseDate=baseDate,wsen=wsen)
    #     lCnt.append((f.toGeoJson(),f = ffData(contour,mode = "mnhPGD", baseDate=baseDate,wsen=wsen)))
    #     larea.append((f.metadata["totalArea"],f.frontDate))
    # with open(frontsKMLOUT, "w") as text_file:
    #     text_file.write(to_timed_kml(lCnt, croll=change_color_every))
     

def genKMLFiles(PGDFILE, BMAPFILE, FFINPUTPATTERN, BMAPKMLOUT,frontsKMLOUT, everyNFronts=1,change_color_every=6,btslice=None,tickRatio=60000):

    dBmap = xr.open_dataset(BMAPFILE)
    baseDate= datetime(int(dBmap.domain.refYear), 1, 1, 0, 0, 0, 0)
    baseDate= baseDate+timedelta(days=int(dBmap.domain.refDay))
    wsen, lbrt, ZS = get_WSEN_LBRT_ZS_From_Pgd(PGDFILE)
    west, south, east, north = wsen
     
    
    contours1  = glob.glob(FFINPUTPATTERN)
        
    lCnt=[]
    larea=[]
    import os
    selectionSorted =  sorted(contours1, key=extract_last_number)  
    if btslice is not None:
        selectionSorted = selectionSorted[btslice[0]:btslice[1]]
    for contour in selectionSorted[::everyNFronts]:
        f = ffData(contour,mode = "mnhPGD", baseDate=baseDate,wsen=wsen)
        lCnt.append((f.toGeoJson(),f.frontDate))
        larea.append((f.metadata["totalArea"],f.frontDate))
    with open(frontsKMLOUT, "w") as text_file:
        text_file.write(to_timed_kml(lCnt, croll=change_color_every))
    
    fdat = ffData(mode = "mnhPGD", wsen=wsen, lbrt = lbrt)
    
   
    fdat.setATimeMatrix(dBmap.arrival_time_of_front.values)
    ros = fdat.getROSMatrix(max_speed_filter=3)

    arrayToPng(ros, BMAPKMLOUT+"ROS.png", output_cbar_png=BMAPKMLOUT+"ROSCBAR.png",tickRatio=tickRatio)
    
    create_kml(west, south, east, north,"ROS", BMAPKMLOUT+"ROS.png",  BMAPKMLOUT, pngcbarfile=BMAPKMLOUT+"ROSCBAR.png")
  
    #bmap2kml(dBmap,wsen,BMAPKMLOUT+"2.kml")

def ffFromPgd(PGDFILE,domainDate=None,ignitions = None, fuel_test = None):
    wsen, lbrt, ZS = get_WSEN_LBRT_ZS_From_Pgd(PGDFILE)
    fdat = ffData(mode = "mnhPGD", wsen=wsen, lbrt = lbrt)
    secsDomain = 0
    dom =""
    if domainDate is not None :
        secsDomain =  (domainDate - domainDate.replace(hour=0, minute=0, second=0, microsecond=0)).total_seconds()
        #dom+=f"setParameters[year={domainDate.year};month={domainDate.month};day={domainDate.day}]\n"
    dom+= fdat.getDomainInitString(secsDomain)
    
    if ignitions is not None :   
        for ig in ignitions:
            igT = datetime.fromisoformat(ig["when"])
            print("ignition at ",igT)
            igSecs =  (igT - domainDate.replace(hour=0, minute=0, second=0, microsecond=0)).total_seconds()
            print("ignition at ",igT,igSecs)
            ix,iy,iz = fdat.lalo2xy((ig["latitude"],ig["longitude"],0))
            dom+=f"\nstartFire[loc=({ix},{iy},0.);t={igSecs}]"
            
    if fuel_test is not None :
        l,b,r,t = lbrt
        ix = np.linspace(l, r, len(fuel_test)+2)
 
        iy = np.ones(ix.shape) * (b + (t - b) / 4)
        
        iz = np.zeros(ix.shape)
        
        # Si vous souhaitez les coordonnées comme des paires (ix[i], iy[i])
        points = np.array(list(zip(ix, iy, iz)))
        for ix, iy, iz in points[1:-1]:
            dom+=f"\nstartFire[loc=({ix},{iy},0.);t={secsDomain}]"
        
    return dom
   

    
 
