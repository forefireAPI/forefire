#!/bin/sh
#SBATCH -J FCAST_SPA1
#SBATCH -N 1 
#SBATCH -n 1  
#SBATCH --partition=intel
#SBATCH --time=10:00:00
#SBATCH --mail-user=batti.filippi@@gmail.com
#SBATCH --mail-type=all

#Should we go to next step at the end 
continue_cycle=${1}

echo "SCRIPT SPAWN_MESONH " ${XYZ} " EN COURS"

# Echo des commandes
ulimit -c 0
ulimit -s unlimited
# Arrete du job des la premiere erreur
# Nom de la machine

. ~/runMNH 
ln -sf ../001_pgd/PGD_D*A.nested.* .
ln -sf ../003_run/RUN12.1.PRUN1.* .
export MPIRUN="mpirun -np 1"

time ${MPIRUN} SPAWNING${XYZ}
 
export MPIRUN="mpirun -np 20"

time ${MPIRUN} PREP_REAL_CASE${XYZ}

