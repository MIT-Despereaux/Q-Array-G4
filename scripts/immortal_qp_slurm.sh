#!/bin/bash

# Job Flags
#SBATCH --job-name="G4CMP_Job_Tobias"
#SBATCH --output=out_%j.txt
#SBATCH --error=err_%j.txt
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=tclassen@mit.edu

#SBATCH --time=12:00:00
#SBATCH -N 1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --array=0-9
#SBATCH --mem=16G
#SBATCH --partition=mit_normal

# Set up environment
module load cmake
module load gcc/12.2.0
module load miniforge

# Run your application
source /home/tclassen/dependencies/geant4-install/bin/geant4.sh
cd /home/tclassen/projects/build-dspx
export G4CMP_CRYSTAL_MAPS="/home/tclassen/dependencies/g4cmp-install/share/G4CMP/CrystalMaps"


MACRO=../macros/QP_test/Immortal_QP_${SLURM_ARRAY_TASK_ID}.mac
./main $MACRO \
    > logs/run_${SLURM_ARRAY_TASK_ID}.log 2>&1