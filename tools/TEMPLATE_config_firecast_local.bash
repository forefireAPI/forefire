    ## Configuration file 

    ulimit -c 0
    ulimit -s unlimited

    PREP_PGD_FILES="MESONH PHYSIO files path"
    BC_FILES="Where to store BC gribs"
    UNCOUPLED_CASE_PATH="where are the uncoupled case to couple"
    COUPLED_CASE_PATH="where to put your couplde cases"

    ## Optional http links from where to get the data in the 
    BASE_DATA_URL="where to get BC gribs from the web"
    HTTP_PGD="$BASE_DATA_URL/LANDSCAPE/"
    HTTP_BC="$BASE_DATA_URL/MARS/"
    HTTP_TILES="$BASE_DATA_URL/ff_tiles/"

    ### binaries
    export ECCODES_HOME="ECCODE library path"
    export NETCDF_HOME="/Netcdf library path"
    export FOREFIREHOME="ForeFire library path"
    export PYTHONEXE="a capable python interpreter"
    export MESONHROOT="root path of MesoNH"
    source ${MESONHROOT}/conf/profile_mesonh------your profile

    export MPIRUN="mpirun -np 8"
    export MONORUN="mpirun -np 1"