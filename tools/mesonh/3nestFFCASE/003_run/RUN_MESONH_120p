#!/bin/sh
#SBATCH -J FCAST_MNH 
#SBATCH -N 3 
#SBATCH -n 120  
#SBATCH --partition=intel
#SBATCH --time=10:00:00
#SBATCH --mail-user=batti.filippi@@gmail.com
#SBATCH --mail-type=all

#Should we go to next step at the end 
continue_cycle=${1}

echo "SCRIPT RUN_MESONH " ${XYZ} " EN COURS"

# Echo des commandes
ulimit -c 0
ulimit -s unlimited
# Arrete du job des la premiere erreur
# Nom de la machine

. ~/runMNH 
ln -sf ../001_pgd/PGD_*nested.* .
ln -sf ../002_real/M1* .
export MPIRUN="mpirun -np 120"


time ${MPIRUN} MESONH${XYZ}

mv OUTPUT_LISTING0  OUTPUT_LISTING0_run1
mv OUTPUT_LISTING1  OUTPUT_LISTING1_run1
 
