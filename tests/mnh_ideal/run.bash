. $HOME/runMNH
export MPIRUN="mpiexec -np 4"
ncgen -o ForeFire/lpsimple.nc ForeFire/lpsimple.cdl
mkdir ForeFire/Outputs
mkdir MODEL1
time ${MPIRUN} MESONH${XYZ}
if ! diff ForeFire/ForeFire.0.nc.ref ForeFire/Outputs/ForeFire.0.nc; then
    echo "Burning map does not match reference in mnh_ideal case."
    exit 1
fi
