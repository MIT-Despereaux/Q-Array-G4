#!/bin/bash

# Job Flags
#SBATCH --job-name="Edep_Final_AmBe_NoGCMP"
#SBATCH --output=logs/out_%A_%a.txt
#SBATCH --error=logs/err_%A_%a.txt
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=tclassen@mit.edu

#SBATCH --time=11:59:00
#SBATCH -N 1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=16
#SBATCH --array=0-4
#SBATCH --mem=16G
#SBATCH --partition=mit_normal

# Set up environment
module load cmake
module load gcc/12.2.0
module load miniforge

source /home/tclassen/dependencies/geant4-install/bin/geant4.sh
export G4CMP_CRYSTAL_MAPS="/home/tclassen/dependencies/g4cmp-install/share/G4CMP/CrystalMaps"

# Setup Directories
PROJECT_DIR="/home/tclassen/projects/build-dspx"
cd $PROJECT_DIR
mkdir -p logs

# Read the CSV (Offset by 2 to skip the header rows)
CSV="/home/tclassen/projects/scripts/Seeds.csv"
LINE=$(sed -n "$((SLURM_ARRAY_TASK_ID+2))p" "$CSV")
IFS=',' read SEED1 SEED2 <<< "$LINE"

# Create an isolated workspace for this specific job to prevent CSV collisions
WORK_DIR="job_workspace_${SLURM_ARRAY_TASK_ID}"
mkdir -p $WORK_DIR
cd $WORK_DIR

# Link the executable and crystal maps into the workspace
ln -sfn /home/tclassen/dependencies/g4cmp-install/share/G4CMP/CrystalMaps/* ./
ln -sf ${PROJECT_DIR}/main ./
ln -sf ${PROJECT_DIR}/template_source_*.mac ./

# Replace placeholders in the macro and save it locally
TMPMACRO="run_macro_${SLURM_ARRAY_TASK_ID}.mac"
sed \
    -e "s/@SEED1@/${SEED1}/g" \
    -e "s/@SEED2@/${SEED2}/g" \
    ${PROJECT_DIR}/../macros/temporary_multi_source.mac > "$TMPMACRO"

# Execute the job
./main "$TMPMACRO"

# Rename and move the hardcoded output file back to the main directory
if [ -f "SpectrumEnergySummary.csv" ]; then
    mv "SpectrumEnergySummary.csv" "${PROJECT_DIR}/SpectrumEnergySummary_${SLURM_ARRAY_TASK_ID}.csv"
fi

# Clean up workspace
cd $PROJECT_DIR
rm -rf $WORK_DIR