#!/bin/bash

# Stop execution immediately if any step throws an error
set -e

# --- 1. Check for inputs ---
if [ -z "$1" ]; then
    echo "Error: You must supply a distribution type argument."
    echo "Usage: ./run_sim.sh <test_hist | ISO_spectrum | mono>"
    exit 1
fi

DIST_TYPE=$1

echo "=========================================="
echo "Starting Geant4 Simulation Engine..."
echo "=========================================="

# 2. Run your compiled Geant4 simulation binary using the macro file
./build/main Set_Up.mac.txt

echo "=========================================="
echo "Simulation Finished. Starting File Sorter..."
echo "=========================================="

# 3. Execute python sorting routine passing down the environment logic token
python3 sort_sim_data.py "$DIST_TYPE"

echo "=========================================="
echo "Pipeline complete!"
echo "=========================================="