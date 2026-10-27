std::string commandHelp = R"(
## FireDomain
    FireDomain[sw=(x,y,z);ne=(x,y,z);t=seconds]
    Create the main FireDomain; fronts and nodes will be created within this FireDomain
    Example: FireDomain[sw=(0,0,0);ne=(64000,64000,0);t=2400]
    Arguments:
    - 'sw': Southwest point (x,y,z) in meters
    - 'ne': Northeast point (x,y,z) in meters
    - 't': Time associated with the fire domain in seconds (no 'date=' here)
    To georeference the domain, set BBoxWSEN before creating it, e.g.
    setParameter[BBoxWSEN=8.36215875,41.711125,9.1366311,42.28667]
    FireDomain does not read BBoxWSEN; without the parameter the domain is placed at DefaultSWLngLat (Corsica)


##     FireFront
    FireFront[id=<front_id>;domain=<domain_id>;t=<time>]
    Creates a FireFront within a FireDomain or another FireFront. The prefixed spacing defines its hierarchical level.
    A FireFront in a FireDomain has 4 spaces before the command; an inner FireFront adds 4 more spaces.
    Example: FireFront[id=26;domain=0;t=0]
    Arguments:
    - 'id': Identifier for the fire front
    - 'domain': Domain ID where the front is created
    - 't': Time at which the fire front is created;

##         FireNode
    FireNode[loc=(x,y,z);vel=(x,y,z);t=seconds]
    Creates a FireNode within a FireFront. The prefixed spacing defines the hierarchical level of the element.
    A FireNode inside a FireFront is defined with 4 additional spaces relative to its parent.
    Example: FireNode[domain=0;id=1;fdepth=2;kappa=0.1;loc=(3.5,2.6,1.1);vel=(-0.1,-0.03,0.01);t=2.1;state=moving;frontId=2]
    Arguments:
     - 'loc': Spatial coordinates (x,y,z) where the node is created
     - 'vel': Initial velocity (x,y,z) of the node
     - 't': Time associated with the fire node in seconds
     - 'opt:domain': Domain ID where the node is created
     - 'opt:id': Identifier for the fire node
     - 'opt:fdepth': Initial fire depth in meters
     - 'opt:kappa': Initial curvature factor (tan value)
     - 'opt:state': State of the node (init, moving, splitting, merging, final);

## @
    @t=seconds
    Schedule operator to trigger the command at time t=seconds or nowplus=seconds
    Example: print[]@t=1200\n will print the current simulation state at sim time 1200 seconds
    Example: save[]@nowplus=1200\n will save the current map at current sim time plus 1200 
    Arguments:
     - 't': Time in seconds when the scheduled command should execute
     - 'nowplus': Delta duration in seconds for the scheduled command to execute;

## startFire
    startFire[loc=(x,y,z)|lonlat=(lon,lat,z);t=seconds|date=YYYY-MM-DDTHH:MM:SSZ]
    Creates the smallest possible triangular fire front at the specified location and time
    Example: startFire[loc=(0.0,0.0,0.0);t=0.]
    Arguments:
     - 'loc': Starting location of the fire (coordinates x,y,z)
     - 'lonlat': WGS coordinates of the ignition point (lon,lat,z); all three values are required
     - 't': Time when the fire is started (in seconds or as an absolute ISO GMT date);

## step
    step[dt=seconds]
    Advances the simulation by the specified time duration
    Example: step[dt=5.]
    Arguments:
     - 'dt': Duration (in seconds) for which the simulation will run;

## addLayer
    addLayer[name=]
    add a constant layer of type type (default data) with name and value V (default search for parameter of same name, then 0), optional modelName = with bounds matching the FireDomain 
    Example: addLayer[name=heatFlux;type=flux;modelName=heatFluxBasic;value=3];


## trigger
    trigger[wind;vel=(vx,vy,vz)]
    trigger[fuelIndice;loc=(x,y,z);fuelType=<int>]
    Triggers a change in simulation data, such as fuel or dynamic wind conditions
    The first argument, alone without '=', selects the change. Schedule it with @t=, a 't=' inside the brackets is not read
    Example: trigger[wind;vel=(5.0,2.0,0.0)]@t=1800
    Example: trigger[fuelIndice;loc=(10,20,0);fuelType=1]
    Arguments:
     - 'wind': Sets the wind to 'vel', the vector (vx,vy,vz) in m/s
     - 'fuelIndice': Sets the fuel index at 'loc' (x,y,z) to the integer 'fuelType';

## emit
    emit[layer=<flux_layer>;loc=(x,y,z);radius=<m>;duration=<seconds>;flux=<per_m2>]
    Injects a flux into an existing flux layer, over an area, for a time span starting at the current domain time
    Example: emit[layer=heatFlux;loc=(5000,5000,0);radius=50;duration=600;frp=120]
    Arguments:
     - 'layer': Name of the flux layer to emit into, as given to addLayer. 'name' is accepted as an alias
     - 'loc': Centre of the emission in domain coordinates. Alternatives: 'lonlat' in WGS84, or a 'sw'/'ne' box, or a 'swlonlat'/'nelonlat' box
     - 'radius': Radius in metres, giving a disc of area pi*r^2. Alternatives: 'surface' or 'area' in m2, or the area of the sw/ne box
     - 'duration': Length of the emission in seconds, required. It starts at the current domain time
     - 'flux': The emitted quantity per square metre per second, in the layer's own units. Alternatives, any one of: 'frp' in MW, converted using the FRPToWatts parameter and divided by the area; 'value' or 'val', a power divided by the area; 'total', an energy divided by area and duration;

## goTo
    goTo[t=seconds]
    Advances the simulation to the specified time
    Example: goTo[t=56.2]
    Arguments:
     - 't': The desired simulation time to advance to;

## print
    print[opt:filename]
    Prints a representation of the current simulation state to console or in file un the current dumpMode
    dumpMode 'ff' is the native that could be reparsed by forefire, json is a compact cartesian json, geojson and kml are projected
    Example: print[front_state.ff]
    Arguments:
     - opt: the filename for output;

## save
    save[opt:filename=landscape_file.nc;opt:fields=(altitude,wind,fuel)]]
    without argument saves the current simulation arrival time state in netcdf format with default name ForeFire.'domainID'.nc
    Example: save[]
    Arguments:
     - opt:filename if given, landscape file is saved instead
     - opt:fields fields to save in landscape file in (altitude,wind,fuel);

## loadData
    loadData[landscape_file.nc;YYYY-MM-DDTHH:MM:SSZ]
    Loads a NetCDF data file into the simulation at a certain UTC date
    Example: loadData[data.nc;2020-02-10T17:35:54Z]
    Arguments:
     - a netcdf landscape file
     - the UTC date;

## plot
    plot[parameter=(speed|arrival_time|fuel|altitude|slope|windU|windV|Rothermel);
    filename=<fname.png/jpg/nc/asc>; 
    opt:range=(min,max); 
    opt:BBoxWSEN=(w_lon,s_lat,e_lon,n_lat) | opt:BBoxLBRT=(leftX,bottomY,rightX,topY) | opt:BBoxLBRT=active; 
    opt:size=eni,enj; 
    opt:cmap=<colormap>; 
    opt:histbins=<integer>; 
    opt:projectionOut=(json|<fname.kml>)]

    Generates a plot of the simulation parameter. The output file format is determined by the filename extension:
    - PNG/JPG: Creates an image file.
    - NC: Creates a NetCDF file containing the data matrix and latitude/longitude arrays.
    - ASC: Creates an ASCII file suitable for GIS applications.

    Optional projection output:
    - 'projectionOut=json': Outputs the bounding box as a JSON formatted string.
    - 'projectionOut=<fname.kml>': Saves a KML file with a GroundOverlay referencing the generated image.

    Example:
    plot[parameter=speed;filename=out.png;range=(0,0.1);BBoxWSEN=(8.36215875,41.711125,9.1366311,42.28667);cmap=viridis;histbins=50]

    Arguments ('opt:' marks an optional argument, do not type it):
    - 'parameter': Simulation parameter to plot (speed | arrival_time | fuel | altitude | slope | windU | windV | Rothermel)
    - 'filename': Output file name. The extension (.png, .jpg, .nc, .asc) selects the output format.
    - 'opt:range': Optional data range for the plot in the format (min,max).
    - 'opt:BBoxWSEN' / 'opt:BBoxLBRT': Optional area. 'BBoxWSEN' for geographic coordinates (west_lon, south_lat, east_lon, north_lat),
                'BBoxLBRT' for Cartesian coordinates (leftX, bottomY, rightX, topY), or 'BBoxLBRT=active' for the active domain subset.
    - 'opt:size': Optional matrix size as a comma-separated pair eni,enj, without parentheses. If not provided, default resolution is used.
    - 'opt:cmap': Optional colormap (e.g., viridis, turbo, fuel).
    - 'opt:histbins': Optional integer specifying the number of histogram bins between min and max.
    - 'opt:projectionOut': Optional projection output. Use 'json' for a JSON string of domain boundaries, or a filename ending in .kml
                            to generate a KML file with a GroundOverlay referencing the output image.;

## computeSpeed
    computeSpeed
    Computes and returns an array of speed values using the first registered propagation model
    Example: computeSpeed
    Arguments:
     - Uses the active propagation model to calculate simulation speeds;

## setParameter
    setParameter[param=value]
    Sets a single simulation parameter to a given value
    Example: setParameter[kappa=0.05]
    Arguments:
     - 'param': Name of the parameter to set
     - 'value': Value to assign to the parameter;

## setParameters
    setParameters[param1=val1;param2=val2;...;paramn=valn]
    Sets multiple simulation parameters at once
    Example: setParameters[kappa=0.05;fdepth=20]
    Arguments:
     - Sets each parameter to its specified value; separate parameters with semicolons;

## getParameter
    getParameter[key]
    Retrieves the value of a specified simulation parameter
    Example: getParameter[dumpMode]
    Arguments:
     - 'key': The name of the parameter to retrieve;

## include
    include[filename.ff]
    Executes FF commands from the specified file
    Example: include[commands.ff]
    Arguments:
     - 'filename.ff': Filename containing simulation commands to execute

## clear
    clear[]
    Frees the fire domain and cancels every scheduled event. Parameters are kept, so a new FireDomain can be created without setting them again
    Example: clear[]

## systemExec
    systemExec[command=<system_command>]
    Executes a system command
    Example: systemExec[command=ls -la]
    Arguments:
     - 'command': The system command to execute

## listenHTTP
    listenHTTP[host:port]
    Launches an HTTP server to listen for simulation commands
    Example: listenHTTP[localhost:8080]
    Arguments:
     - 'host': Hostname or IP address for the server
     - 'port': Port number on which the server will listen;

## quit
    quit[]
    Terminates the simulation
    Example: quit
    Arguments:
     - Ends the simulation execution;

)";
