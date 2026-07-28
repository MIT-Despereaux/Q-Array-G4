#!/bin/bash

# Job Flags
#SBATCH --job-name="Immortal_QP_Full_Heatmap"
#SBATCH --output=logs/slurm_%A_%a.out
#SBATCH --error=logs/slurm_%A_%a.err
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=tclassen@mit.edu

#SBATCH --time=12:00:00
#SBATCH -N 1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --array=0-63               # 64 array tasks (change to 0-31 for 32 tasks)
#SBATCH --mem=4G
#SBATCH --partition=mit_normal

# Request SLURM to send SIGTERM 60 seconds before job walltime expires
#SBATCH --signal=B:SIGTERM@60

# Set up environment
module load cmake
module load gcc/12.2.0
module load miniforge

source /home/tclassen/dependencies/geant4-install/bin/geant4.sh
cd /home/tclassen/projects/build-dspx

export G4CMP_CRYSTAL_MAPS="/home/tclassen/dependencies/g4cmp-install/share/G4CMP/CrystalMaps"
ln -sfn /home/tclassen/dependencies/g4cmp-install/share/G4CMP/CrystalMaps/* ./

CSV=/home/tclassen/projects/scripts/launch_points.csv

# Read CSV parameters (Offset by 2 to skip header)
LINE=$(sed -n "$((SLURM_ARRAY_TASK_ID+2))p" "$CSV")
IFS=',' read SEED1 SEED2 X Y Z <<< "$LINE"

TMPMACRO=$(mktemp)

# Build parameterised macro
sed \
    -e "s/@SEED1@/${SEED1}/g" \
    -e "s/@SEED2@/${SEED2}/g" \
    -e "s/@X@/${X}/g" \
    -e "s/@Y@/${Y}/g" \
    -e "s/@Z@/${Z}/g" \
    -e "s/@TASK_ID@/${SLURM_ARRAY_TASK_ID}/g" \
    ../macros/Immortal_QP.mac \
    > "$TMPMACRO"

# Directories setup
mkdir -p logs
mkdir -p /home/tclassen/projects/output/matrices

MATRIX_CSV="/home/tclassen/projects/output/matrices/matrix_task_${SLURM_ARRAY_TASK_ID}.csv"

# Pipeline execution: Geant4 stdout streams directly into Python matrix parser
./main "$TMPMACRO" 2>&1 | python3 /home/tclassen/projects/scripts/extract_matrix.py --output "$MATRIX_CSV"

# Clean up temp macro
rm "$TMPMACRO"