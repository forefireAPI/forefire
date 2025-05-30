     #!python
import sys
import netCDF4
import numpy as np
import sys


#fnames = ["/Users/filippi_j/Volumes/orsu/firecaster/2023/nest150Ref/001_pgd/PGD_D2000mA.nested.nc","/Users/filippi_j/Volumes/orsu/firecaster/2023/nest150Ref/001_pgd/PGD_D400mA.nested.nc","/Users/filippi_j/Volumes/orsu/firecaster/2023/nest150Ref/001_pgd/PGD_D80mA.nested.nc"]
#foutName = "/Users/filippi_j/data/2023/corbara20230727/domains.kml"
#if len(sys.argv) != 2:
#    print("usage kmlDomain PGDFile1 2....")
#else :
#    fnames = sys.argv[1:]

def pgds_to_KML_nc4(fnames, fout):
    dbo={}
    kmlString = str("""<?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2" xmlns:gx="http://www.google.com/kml/ext/2.2" xmlns:kml="http://www.opengis.net/kml/2.2" xmlns:atom="http://www.w3.org/2005/Atom">
    <Document>
    	<name>kmldomain.kml</name>
    <Style id="doms"><PolyStyle><fill>0</fill></PolyStyle></Style>""")
    
    for fname in fnames:
        f = netCDF4.Dataset(fname,'r')
        
        dbo['LAT'] = f.variables["latitude"][:]
        dbo['LON'] = f.variables["longitude"][:]
        dbo['ALT'] = f.variables["ZSMT"][:]
    
    
        #with open('.coordinates.pickle', 'wb') as dbfile:
        #	pkl.dump(dbo, dbfile)
    
        #Recuperation de la taille de la matrice
        max_x = dbo['LAT'].shape[0]-1
        max_y = dbo['LAT'].shape[1]-1
        #Recuperation des latitudes/longitudes dans un tableau trie par ordre croissant
        tab_lat = dbo['LAT'][:,0]
        tab_lon = dbo['LON'][0,:]
    
        kmlString += str("""
        	<Placemark>
        		<name>MNH Bound in PGD</name>
        		<styleUrl>#doms</styleUrl>
        			<Polygon>
        			<tessellate>0</tessellate>
        			<outerBoundaryIs>
        				<LinearRing>
        					<coordinates>
        """)
        kmlString += str("%f,%f,0 %f,%f,0 %f,%f,0 %f,%f,0  %f,%f,0"%(dbo['LON'][0][0],dbo['LAT'][0][0],dbo['LON'][0][max_y],dbo['LAT'][0][max_y],dbo['LON'][max_x][max_y],dbo['LAT'][max_x][max_y],dbo['LON'][max_x][0],dbo['LAT'][max_x][0],dbo['LON'][0][0],dbo['LAT'][0][0]))
        
        kmlString += str("""
        					</coordinates>
        				</LinearRing>
        			</outerBoundaryIs>
        		</Polygon>
        	</Placemark>
        """)
        f.close()
    kmlString += str("""  
    </Document>
    </kml>""")
 
    
    with open(fout, "w") as text_file:
        text_file.write(kmlString)



def bounds_to_KML(boundaries, fout):
    kmlString = """<?xml version="1.0" encoding="UTF-8"?>
    <kml xmlns="http://www.opengis.net/kml/2.2" xmlns:gx="http://www.google.com/kml/ext/2.2" xmlns:kml="http://www.opengis.net/kml/2.2" xmlns:atom="http://www.w3.org/2005/Atom">
    <Document>
    	<name>kmldomain.kml</name>
    <Style id="doms"><PolyStyle><fill>0</fill></PolyStyle></Style>"""
 
    w, s, e, n = boundaries  

    kmlString += f"""
        <Placemark>
            <name>MNH Bound in PGD</name>
            <styleUrl>#doms</styleUrl>
            <Polygon>
                <tessellate>0</tessellate>
                <outerBoundaryIs>
                    <LinearRing>
                        <coordinates>
                            {w},{s},0 {e},{s},0 {e},{n},0 {w},{n},0 {w},{s},0
                        </coordinates>
                    </LinearRing>
                </outerBoundaryIs>
            </Polygon>
        </Placemark>
    """

    kmlString += """  
    </Document>
    </kml>"""

    with open(fout, "w") as text_file:
        text_file.write(kmlString)
