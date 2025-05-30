export MPIRUN="mpirun -np 2"
mkdir ForeFire/Outputs
mkdir MODEL1
mkdir MODEL2
time ${MPIRUN} MESONH${XYZ}
if ! diff ForeFire/ForeFire.0.nc.ref ForeFire/Outputs/ForeFire.0.nc; then
    echo "Burning map does not match reference in mnh_real case."
    exit 1
fi
