


These scripts create the files necessary to run a coupled simulation with MesoNH [http://mesonh.aero.obs-mip.fr/mesonh56]

to couple the simulation with MesoNH you should also edit the &NAM_FOREFIRE namelist in the EXSEG file [http://mesonh.aero.obs-mip.fr/mesonh56/BooksAndGuides?action=AttachFile&do=get&target=forefire.pdf] 

#  Preprocessing Scripts

List of preprocessing scripts:
- ***clickImageToLocation.py*** sets the coordinates for the Init.ff file 
- ***genForeFireCase.py*** writes the **NetCDFfile** landscape (fuel, elevation, wind, flux models) from numpy arrays
- ***genPrepIdeal.py*** creates the  .nam for mesonh ideal case ???
- ***kmlDomain.py*** extracts kml files from a netcdf file
- ***PGD2Init.py*** creates the **InitFile**=Init.ff in the **ForeFireDataDirectory** directory
- ***pngs2bmap.py*** creates the burning map file **BMapFiles** starting from kml contours
- ***prealCF2Case.py*** creates the **NetCDFfile**=data.nc file in the **ForeFireDataDirectory** directory


### genForeFireCase

Lives in `preprocessing/genForeFireCase.py` and contains the preprocessing
routine *FiretoNC*, used to generate a packed data landscape in cdf format for
ForeFire.

**Usage:** FiretoNC(filename, file name
         domainProperties, the domain extension (map matching forefire parameters SWx, SWy, SWz, Lx, Ly, Lz, t0, Lt)
     parametersProperties, the simulation date and duration. All of date, duration, refYear, refDay, year, month and day are required
             fuelModelMap, a numpy integer array containing the indexes of fuel type
           elevation=None, a numpy real array with the elevation
                wind=None, a map with a "zonal" and "meridian" numpy real array values
       fluxModelMap=None): a map with a ("table" and "name" ) and fMap "data" numpy int array values containing indices to the corresponding flux model

Field arrays are indexed outermost axis first: `(NY, NX)`, or `(NZ, NY, NX)`
and `(NT, NZ, NY, NX)` when they vary with height or time — the same
convention `prealCF2Case.py` uses.

`tests/python/test_genforefirecase.py` builds a landscape, loads it and runs a
short simulation; it doubles as a worked example.


### PGD2Init

reates the **InitFile**=Init.ff in the **ForeFireDataDirectory** directory.
The **InitFile** file contains the location of the fire front at initialization (non-parallel init).
**Usage:** PGD2Init.py inMNH_PGD_File.nc lat lon startTimeInseconds > pathtoForeFireDataDirectory/Init.ff
- INPUT:
    - inMNH_PGD_File.nc  (ex:/001_pgd/PGD_D80mA.nc)
    - lat lon : latitude and longitude (ex: 39.7556 -8.1736) 
    - time of ignition in seconds (ex: 50000)
- OUTPUT:
    - the **InitFile**=Init.ff file in the **ForeFireDataDirectory**


### pngs2bmap 

creates the bmap file **BMapFiles** starting from given kml contours
TO BE IMPLEMENTED:
- parameters *otime*,*FLx*,*FLy*,*FSWx* ,*FSWy*,*FSWx*,*FSWy*, *FrefYear*, *FrefDay*, *FrefYear*, *FrefDay*,*NHours* should be all given at the head of the modules
- finds the kml source files automatically for a given directory  
 
 
### prealCF2Case

creates the **NetCDFfile**  file containing the data needed by the simulation (data.nc), otput uses the *FiretoNC* routine
**Usage:** python prealCF2Case inMNH_PGD_File.nc outFFDCaseFile.nc   opt:pngFuelFile"
- INPUT: the pgd file at the specified path *(inMNH_PGD_File.nc)*
- OUTPUT: the data.nc file at the specified path *(outFFDCaseFile.nc)*
- OPTIONAL: the png on which the fuel distribution is constructed *(opt:pngFuelFile)*

ouutput is generated from the *FiretoNC* routine in *genForeFireCase.py*

reads dimensions from the PGD imput file

example of NetCDF "data.nc":
```
dimensions:
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
```
TO BE IMPLEMENTED:
- a routine that given the coordinates, extract a colormap for fuels
- we need to translate the colormap in integer numbers

## kmlDomain
creates the kml file showing the geographical domain extensions starting from a  netCDF domain

**Usage:** 
```
python3.9 kmlDomain.py PGDFile >  kmlfile
```
- INPUT: the netCDF containing the informations of the modelled domain (ex: *KTEST_2D/001_pgd/PGD_D160mA.nc*)
- OUTPUT:the desired kml file (ex: *KTEST_2D/001_pgd/kmlD160.kml*)


#  Processing Scripts

List of postprocessing scripts:

- ***plotcontour.py***

#  Postprocessing Scripts

List of postprocessing scripts:
- ***bmapToVTK.py***
- ***ffToGeoJson.py*** 
- ***pMNHFF2VTK.py***

## pMNHFF2VTK

Builds vtk from forefire outputs for parallels runs

**Usage:**  

python PathtoScript pathtoForefireOutput pathForefireOutputGeom pathtodirOutput
- on a single node:  
    - computes all steps:
    
```
   python3.9 ~/soft/firefront/tools/postprocessing/pMNHFF2VTK.py  MODEL3/output ForeFire/Outputs/output vtkout3/*
```
   
    - computes step number n=4
    
```
   python3.9 ~/soft/firefront/tools/postprocessing/pMNHFF2VTK.py  MODEL3/output ForeFire/Outputs/output vtkout3/ -steps 4 5
```
   
- on multiple nodes (SLURM):  
you should launch pMNHFF2VTK by an opportune script (**sbatchRunVtkFilesM3** for model 3)

example of script:
```
seqSTEP=$1
stepLength=$2
startSTEP=$(($stepLength * $SLURM_ARRAY_TASK_ID))
endSTEP=$(($stepLength * ($SLURM_ARRAY_TASK_ID + 1) ))
echo "ID $SLURM_ARRAY_TASK_ID computing $startSTEP to $endSTEP"

for i in `seq ${startSTEP} ${seqSTEP} ${endSTEP}`; 
do
srun -n1 --exclusive python3.9 ~/soft/firefront/tools/postprocessing/pMNHFF2VTK.py  MODEL3/output ForeFire/Outputs/output vtkout3/ -steps $i $(($i + $seqSTEP)) & 
done
wait
```
run with `sbatch --array=0-2 script_vtk_array 2 10` :  will launch 3 tasks SLURMID (0, 1 and 2) of 10 steps each consisting of 10/2 srun processes of length 2 on each task
 so ID 0 will compute 0-2 3-4 5-6 7-8 9-10
 so ID 1 will compute 11-12 13-14 15-16 17-18 19-20
 so ID 2 will compute 21-22 23-24 25-26 27-28 29-30
 Typically fo 2000 outputs on 10 tasks : "sbatch --array=0-9 script_vtk_array 20 200"
 
When all steps are computed, run another time:
```
 python3.9 ~/soft/firefront/tools/postprocessing/pMNHFF2VTK.py  MODEL3/output ForeFire/Outputs/output vtkout3/
```

with that all time steps will be sincg=hronized with MesoNH time 

#  BUGFIX

## When launching a simulation do always the following steps
 
- check that all directories called in RUN_MNH_Nproc are really there, otherwise this will cause errors difficult to debug:

```
  ~/runMNH 
rm -rf MODEL/*
rm -rf vtkout/*
rm -rf ForeFire/Outputs/*
rm -rf parallel/*.domain*
rm -rf parallel/1/*
rm -rf parallel/0/*


ln -sf ../001_pgd/PGD_D* .
ln -sf ../002_real/AND1.* .
ln -sf ../003_run/JUL19.1.RUN1D.008* .
#ln -sf ../004_SpawnReal/AND2.* .
#ln -sf ../005_SpawnReal/AND3.* .

```
- When launching the simultaions, sometiles bmap is not good!! check with
```
grep bmap | logfile.out
```
if everything is ok you will have something of the type:
```
 Domain 0    size:202:202:52 coord:24240:24240 my bmap is 2424:2424 res 10:10
```
then you can comment the following line in the RUN_MNH script to avoid such problem in the future:
```
rm -rf parallel/*.domain*
```

- The vtk postprocesser pMNHFF2VTK gives the error:
```
found no domains
```
if is so you should move all the output.1*  output.2* ... (with the exception of the output.0* files) in a new directory *pathForefireOutputGeom* and use that one in the post processing   
