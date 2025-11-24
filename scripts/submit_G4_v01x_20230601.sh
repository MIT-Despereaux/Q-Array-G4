#!/bin/bash

# This is the submission script for simulations.

# Slurm SBATCH settings
# No settings allowed if we use LLsub with triplets
# Run with LLsub ./script.sh [#_node,#_task_per_node,#_cpu_per_task] -- script_args

# Setting up the environments
source /etc/profile
ulimit -c unlimited

echo "Task ID: " $LLSUB_RANK
echo "Total number of tasks: " $LLSUB_SIZE

# Initialize variables
folder=""
overwrite=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -f)
            folder="$2"
            shift # Skip the argument value
            ;;
        --overwrite|-O)
            overwrite=true
            ;;
        *)
            echo "Invalid option: $key" >&2
            exit 1
            ;;
    esac
    shift # Move to the next argument
done

# Check if folder name is provided
if [ -z "$folder" ]; then
    echo "Folder name is required. Please provide a folder using the '-f' option." >&2
    exit 1
fi

directory=$HOME/GitProjects/MIT-Desperaux/Qurad-G4-sim/output/G4_simulation/v01x/20230601/$folder

# Check if folder exists
if [ ! -d "$directory" ]; then
    echo "Folder '$directory' does not exist!"
    exit 1
fi

cd $directory

if [ "$overwrite" = true ]; then
    echo "Overwriting previous simulation..."
    rm *.csv *.json > /dev/null 2>&1
fi

~/GitProjects/MIT-Desperaux/Qurad-G4-sim/build/main run_prod.mac
