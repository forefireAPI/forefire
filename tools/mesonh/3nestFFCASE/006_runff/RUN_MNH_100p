#!/bin/sh
#SBATCH -J FIRECAST 
#SBATCH -n 150 
#SBATCH --partition=intel
#SBATCH --time=30:00:00
#SBATCH --mail-user=batti.filippi@@@@gmail.com
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
rm -rf MODEL1/*
rm -rf MODEL2/*
rm -rf MODEL3/*
rm -rf vtkout1/*
rm -rf vtkout2/*
rm -rf vtkout3/*
rm -rf ForeFire/Outputs/*
rm -rf parallel/*.domain*
rm -rf parallel/1/*
rm -rf parallel/0/*


ln -sf ../001_pgd/PGD_D*.nested.* .
ln -sf ../002_real/M1.* .
ln -sf ../003_run/RUN12.1.PRUN1.* .
ln -sf ../004_SpawnReal/M2.* .
ln -sf ../005_SpawnReal/M3.* .

export MPIRUN="mpirun -np 120"

set -x
set -e


time ${MPIRUN} MESONH${XYZ}

mv OUTPUT_LISTING0  OUTPUT_LISTING0_run1
mv OUTPUT_LISTING1  OUTPUT_LISTING1_run1
mv OUTPUT_LISTING2  OUTPUT_LISTING2_run1
mv OUTPUT_LISTING3  OUTPUT_LISTING3_run1 
