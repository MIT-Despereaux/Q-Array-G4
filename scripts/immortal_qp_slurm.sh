#!/bin/bash

# Job Flags
#SBATCH --job-name="G4CMP_Job_Tobias"
#SBATCH --output=out_%A_%a.txt
#SBATCH --error=err_%A_%a.txt
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=tclassen@mit.edu

#SBATCH --time=12:00:00
#SBATCH -N 1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --array=0-7
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

CSV=../macros/launch_points.csv

# Read the CSV (Offset by 2 to skip the header row)
LINE=$(sed -n "$((SLURM_ARRAY_TASK_ID+2))p" "$CSV")
IFS=',' read SEED1 SEED2 X Y Z <<< "$LINE"
TMPMACRO=$(mktemp)

# Replace placeholders in the macro
sed \
    -e "s/@SEED1@/${SEED1}/g" \
    -e "s/@SEED2@/${SEED2}/g" \
    -e "s/@X@/${X}/g" \
    -e "s/@Y@/${Y}/g" \
    -e "s/@Z@/${Z}/g" \
    -e "s/@TASK_ID@/${SLURM_ARRAY_TASK_ID}/g" \
    ../macros/Immortal_QP.mac \
    > "$TMPMACRO"

mkdir -p logs

# Execute the temporary macro
./main "$TMPMACRO" \
    > logs/run_${SLURM_ARRAY_TASK_ID}.log 2>&1

# Clean up
rm "$TMPMACRO"