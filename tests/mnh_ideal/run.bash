. $HOME/runMNH
export MPIRUN="mpiexec -np 4"
ncgen -o ForeFire/lpsimple.nc ForeFire/lpsimple.cdl
mkdir ForeFire/Outputs
mkdir MODEL1
time ${MPIRUN} MESONH${XYZ}
# Check that ForeFire.0.nc exists and is larger than a minimal size
min_size=15000   # bytes;  
if [ ! -s ForeFire/Outputs/ForeFire.0.nc ]; then
    echo "ForeFire.0.nc is missing."
    exit 1
fi
# Get file size in bytes  
actual_size=$(wc -c < ForeFire/Outputs/ForeFire.0.nc | tr -d '[:space:]')
if [ "$actual_size" -lt "$min_size" ]; then
    echo "ForeFire.0.nc is too small (${actual_size} bytes; expected > ${min_size})."
    exit 1
fi