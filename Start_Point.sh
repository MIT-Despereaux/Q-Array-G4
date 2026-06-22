#!/bin/bash

# Stop execution immediately if any step throws an error
set -e

# --- 1. Check for inputs ---
if [[ -z "$1" && -z "$2" ]]; then
    echo "Error: You must supply a distribution type argument."
    echo "Usage: ./Start_Point.sh <test_hist | ISO_spectrum | mono> <single | dual | multi>"
    exit 1
fi

DIST_TYPE=$1
SOURCE_MODE=$2

echo "=========================================="
echo "Starting Geant4 Simulation Engine..."
echo "=========================================="

# 2. Run your compiled Geant4 simulation binary using the macro file
if [ "$2" == "single" ]; then
    echo "Single particle source initiated"
./build/main << EOF
/control/execute ./Set_Up.mac.txt
EOF
elif [ "$2" == "double" ]; then
    echo "Two particle sources initiated"
./build/main << EOF
/control/execute ./Proto_Full_Set_Up.mac.txt
EOF
# ToDo multiple source creation and specification beyond just simple 2 particle type/number of sources.
# elif [ "$2" == "multi" ]; then
#     echo "Multiple particle sources initiated"
# ./build/main << EOF
# /control/execute ./Set_Up.mac.txt
# EOF 
else
    echo "Invalid mode selection."
    exit 1
fi

echo "=========================================="
echo "Simulation Finished. Starting File Sorter..."
echo "=========================================="

# 3. Execute python sorting routine passing down the environment logic token
python3 sort_sim_data.py "$DIST_TYPE"

echo "=========================================="
echo "Pipeline complete!"
echo "=========================================="