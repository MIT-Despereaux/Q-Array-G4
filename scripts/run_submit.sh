#!/bin/bash

# Job Flags
#SBATCH --job-name="G4CMP_Job_Tobias"
#SBATCH --output=out_%j.txt
#SBATCH --error=err_%j.txt
#SBATCH --mail-type=END,FAIL
#SBATCH --mail-user=tclassen@mit.edu

#SBATCH --time=4-00:00:00
#SBATCH -N 1                  # 1 node
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8     # 8 CPUs allocated to that single task
#SBATCH --mem=16G             # adjust as needed
#SBATCH --partition=mit_normal     # your 

# Set up environment
module load cmake
module load gcc/12.2.0
module load miniforge

# Run your application
cd /home/tclassen/projects/build-dspx
./main ../macros/neutron_gun_batch.mac