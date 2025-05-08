# ForeFire

![logo](./doc/forefire.jpg)

ForeFire is an [open-source code for wildland fire spread models](https://www.researchgate.net/publication/278769168_ForeFire_open-source_code_for_wildland_fire_spread_models), developed and maintained by Université de Corse Pascal Paoli.

Access the [demo simulator here](http://forefire.univ-corse.fr/sim)

![demo](./doc/sim-forefire.jpg)


It has been designed and runs on Unix systems. Three modules can be built with the source code.

The main binaries are  
  - An interpreter (executable)
  - A shared library (with C/C++/Java and Fortran bindings)

##  Preprocessing Scripts

These scripts create the files necessary to run a coupled simulation with MesoNH [http://mesonh.aero.obs-mip.fr/mesonh56]

to couple the simulation with MesoNH you should also edit the &NAM_FOREFIRE namelist in the EXSEG file [http://mesonh.aero.obs-mip.fr/mesonh56/BooksAndGuides?action=AttachFile&do=get&target=forefire.pdf] 

List of scripts:
- ***clickImageToLocation.py *** sets the coordinates for the Init.ff file 
- ***genForeFireCase.py*** contains routines for  addFieldToNcFile
- ***genPrepIdeal.py*** creates the  .nam for mesonh ideal case ???
- ***prealCF2Case.py*** creates the **NetCDFfile**=data.nc file in the **ForeFireDataDirectory** directory
- ***pngs2bmap.py*** creates the burning map file **BMapFiles** starting from kml contours
- ***kmlDomain.py*** extracts kml files from a netcdf file

### genForeFireCase

Contains the preprocessing routine *FiretoNC*:
to be used in order to generate a packed data landscape data in cdf format for ForeFire.
Usage: FiretoNC(filename, file name
         domainProperties, the domain extension (map matching forefire parameters SWx, SWy, SWz, Lx, Ly, Lz, t0, Lt)
     parametersProperties, the other optional properties you may want to put in the list
             fuelModelMap, a numpy integer array containing the indexes of fuel type
           elevation=None, a numpy real array with the elevation
                wind=None, a map with a "zonal" and "meridian" numpy real array values
       fluxModelMap=None): a map with a ("table" and "name" ) and fMap "data" numpy int array values containing indices to the corresponding flux model
 
### prealCF2Case

creates the netCDF file containing the data needed by the simulation (data.nc), otput uses the *FiretoNC* routine in 
INPUT: the pgd file at the specified path
OUTPUT: the data.nc file at the specified path

ouutput is generated from the *FiretoNC* routine in *genForeFireCase.py*

reads dimensions from the PGD imput file

example of NetCDF "data.nc":

*dimensions:
	DIMX = 302 ;
	DIMY = 302 ;
	DIMZ = 1 ;
	DIMT = 1 ;
variables:
	int fuel(DIMT, DIMZ, DIMY, DIMX) ;
		fuel:type = "fuel" ;
	int heatFlux(DIMT, DIMZ, DIMY, DIMX) ;
		heatFlux:type = "flux" ;
		heatFlux:model0name = "IndFlux" ;
		heatFlux:indices = 0 ;
	int vaporFlux(DIMT, DIMZ, DIMY, DIMX) ;
		vaporFlux:type = "flux" ;
		vaporFlux:model1name = "IndVap" ;
		vaporFlux:indices = 1 ;
	char domain ;
		domain:type = "domain" ;
		domain:SWx = 288520. ;
		domain:SWy = 282520. ;
		domain:SWz = 0 ;
		domain:Lx = 24160. ;
		domain:Ly = 24160. ;
		domain:Lz = 0 ;
		domain:t0 = 0 ;
		domain:Lt = Infinityf ;
	char parameters ;
		parameters:type = "parameters" ;
		parameters:projectionproperties = "41.551998,8.828396,41.551998,8.828396" ;
		parameters:date = "2017-06-17_14:00:00" ;
		parameters:duration = 100000 ;
		parameters:projection = "OPENMAP" ;
		parameters:refYear = 2017 ;
		parameters:refDay = 168 ;

// global attributes:
		:version = "FF.1.0" ;
*
TO BE IMPLEMENTED:
- a routine that given the coordinates, extract a colormap for fuels
- we need to translate the colormap in integer numbers

### pngs2bmap 
createsthe bmap file **BMapFiles** starting from given kml contours
TO BE IMPLEMENTED:
- parameters *otime*,*FLx*,*FLy*,*FSWx* ,*FSWy*,*FSWx*,*FSWy*, *FrefYear*, *FrefDay*, *FrefYear*, *FrefDay*,*NHours* should be all given at the head of the modules
- finds the kml source files automatically for a given directory 

```
