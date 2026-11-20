# Forked from geo2kml-0.0.3
# MIT License

# Copyright (c) 2021 peterlv8

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.



 


class WrongFormatGeoJson(Exception):
    """
    Exception raise when geoJson type is not in [Point, MultiPoint, LineString, MultiLineString,
                                                 Polygon, MultiPolygon, GeometryCollection]
    """


def to_kml(geo_json: dict):
    """
    Convert to kml from geoJson
    """
    return '<?xml version="1.0" encoding="UTF-8"?>' + tag('kml', tag('Document', gen_kml_data(geo_json)))

def to_timed_kml(geo_jsonAndStamp: list,croll=1):
    """
    Convert to kml from geoJsons and timestamps
    """
    distinct_colors = [
        '#FF0000FF',  # 
        '#FF333333',  # 
        '#FF333333',  # 
        '#FF0000AA',  # 
        '#FF333333',  # 
        '#FF333333',  # 
    ]
    tdata = ""
    nc=0
    for geo_json, tstamp in geo_jsonAndStamp:
        kmlLineStyle = 	"<Style>	<LineStyle>	<color>%s</color>	<width>3</width> </LineStyle><PolyStyle>	<fill>0</fill>	</PolyStyle></Style>"%distinct_colors[int(nc/croll)]
        tdata=tdata+"\n"+gen_kml_data(geo_json, tstamp=tstamp,kmlLineStyle=kmlLineStyle)
        nc=nc+1
        if int(nc/croll) >= len(distinct_colors):
            nc=0

       
    return '<?xml version="1.0" encoding="UTF-8"?>' + tag('kml', tag('Document', tdata))

def gen_kml_data(geo_json: dict, tstamp=None,kmlLineStyle=""):
    """
    Generate kml data based on geojson type.
    """
    geo_type = geo_json.get('type')
    if not geo_type:
        return ''
    if geo_type == 'FeatureCollection':
        features = geo_json.get('features')
        if not features:
            return ''
        return ''.join([geo_feature(feature, tstamp=tstamp,kmlLineStyle=kmlLineStyle) for feature in features])
    elif geo_type == 'Feature':
        return geo_feature(geo_json, tstamp=tstamp,kmlLineStyle=kmlLineStyle)
    else:
        return geo_feature({'type': 'Feature', 'geometry': geo_json}, tstamp=tstamp,kmlLineStyle=kmlLineStyle)


def geo_feature(geo_data: dict, tstamp=None,kmlLineStyle=""):
    """
    Generate Placemark tag kml
    """
    geometry = geo_data.get('geometry')
    if geometry is None:
        return ''
    if not is_geometry_valid(geometry):
        return ''
    kml_str = geometry_converter(geometry)
    if not kml_str:
        return ''
    if( tstamp==None):
        return tag('Placemark', kml_str)
    return tag('Placemark',tag('TimeStamp', tag('when',tstamp)) + kml_str+ kmlLineStyle)

def geo_point(geo_data: dict):
    """
    Generate Point tag kml
    """
    coords = geo_data.get('coordinates', [])
    
    return tag('Point', tag('coordinates', ','.join(coords)))


def geo_multi_point(geo_data: dict):
    """
    Generate MultiGeometry tag kml
    """
    coords = geo_data.get('coordinates', [])
    
    if not len(coords):
        return ''
    return tag('MultiGeometry', ''.join([geo_point({'coordinates': coord}) for coord in coords]))


def geo_line_string(geo_data: dict):
    """
    Generate LineString tag kml
    """
    coords = geo_data.get('coordinates', [])
    return tag('LineString', tag('coordinates', gen_linear_ring(coords)))


def geo_multi_line_string(geo_data: dict):
    """
    Generate MultiGeometry tag kml
    """
    coords = geo_data.get('coordinates', [])
    if not len(coords):
        return ''
    return tag('MultiGeometry', ''.join([geo_line_string({'coordinates': coord}) for coord in coords]))


def geo_polygon(geo_data: dict):
    """
    Generate Polygon tag kml
    """
    coordsxyz = geo_data.get('coordinates', [])

   
    if not len(coordsxyz):
        return ''
    coords = [[(x, y) for x, y, _ in sublist] for sublist in coordsxyz]
    outer = coords[0]
    inners = coords[1:]
    outer_ring = tag('outerBoundaryIs', tag('LinearRing', tag('coordinates', gen_linear_ring(
        outer))))
    inner_rings = ''.join(
        [tag('innerBoundaryIs',
             tag('LinearRing',
                 tag('coordinates', gen_linear_ring(inner)))) for inner in inners])

    return tag('Polygon',"<tessellate>1</tessellate>  <altitudeMode>clampToGround</altitudeMode>" +outer_ring + inner_rings)


def geo_multi_polygon(geo_data: dict):
    """
    Generate MultiGeometry tag kml
    """
    coords = geo_data.get('coordinates', [])
    if not len(coords):
        return ''
    return tag('MultiGeometry', ''.join([geo_polygon({'coordinates': coord}) for coord in coords]))


def geo_geometry_collection(geo_data: dict):
    """
    Generate MultiGeometry tag kml from geometries geojson
    """
    geometries = geo_data.get('geometries', [])
    if not len(geometries):
        return ''
    return tag('MultiGeometry', ''.join([geometry_converter(geometry) for geometry in geometries]))


def geometry_converter(geometry: dict):
    """
    Choose generation function to convert these field in geojson to kml
    """
    geo_type = geometry.get('type', '')
    geo_mapping = {
        'Point': geo_point,
        'MultiPoint': geo_multi_point,
        'LineString': geo_line_string,
        'MultiLineString': geo_multi_line_string,
        'Polygon': geo_polygon,
        'MultiPolygon': geo_multi_polygon,
        'GeometryCollection': geo_geometry_collection
    }
    try:
        geo_fn = geo_mapping[geo_type]
    except KeyError:
        raise WrongFormatGeoJson(f'{geo_type} is not a valid type in geojson file')
    else:
        return geo_fn(geometry)


def is_geometry_valid(geo_data: dict):
    """
    Check geojson valid
    """
    return ((geo_data.get('type') is not None
             and geo_data.get('coordinates') is not None)
            or (geo_data.get('GeometryCollection') is not None
                and geo_data.get('geometries') is not None
                and all([is_geometry_valid(geometry) for geometry in geo_data.get('geometries')])))


def gen_linear_ring(coords: list):
    """
    Generate linear ring
    """
    return '\n'.join([','.join(map(str, coord)) for coord in coords])


def tag(tag_name: str, data: str):
    """
    Generate tag include tag name and tag data
    """
    return '<' + tag_name + '>' + data + '</' + tag_name + '>'
