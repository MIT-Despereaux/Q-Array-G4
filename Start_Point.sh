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

# 2. Run your compiled Geant4 simulation binary using the native init_vis.mac file
if [ "$2" == "single" ]; then
    echo "Single particle source initiated"
    
    # 1. Back up your original init_vis.mac if it exists so we don't destroy it
    [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
    
    # 2. Append your macro execution command to the end of init_vis.mac
    echo "/control/execute ./Set_Up.mac.txt" >> init_vis.mac
    
    # 3. Launch with zero arguments. Geant4 stays in GUI mode and naturally reads init_vis.mac
    ./build/main
    
    # 4. Restore your original init_vis.mac file to keep your folder clean
    if [ -f init_vis.mac.bak ]; then
        mv init_vis.mac.bak init_vis.mac
    else
        rm -f init_vis.mac
    fi

elif [ "$2" == "double" ]; then
    echo "Two particle sources initiated"
    
    [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
    echo "/control/execute ./Proto_Full_Set_Up.mac.txt" >> init_vis.mac
    ./build/main
    
    if [ -f init_vis.mac.bak ]; then
        mv init_vis.mac.bak init_vis.mac
    else
        rm -f init_vis.mac
    fi

# ToDo multiple source creation and specification beyond just simple 2 particle type/number of sources.
# elif [ "$2" == "multi" ]; then
#     echo "Multiple particle sources initiated"
#     [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
#     echo "/control/execute ./MULTIPLE_FUTURE_IMPLEMENTATION_FEATURE.mac.txt" >> init_vis.mac
#     ./build/main
#     if [ -f init_vis.mac.bak ]; then
#         mv init_vis.mac.bak init_vis.mac
#     else
#         rm -f init_vis.mac
#     fi

else
    echo "Invalid mode selection: '$2'"
    exit 1
fi

echo "=========================================="
echo "Simulation Finished. Starting File Sorter..."
echo "=========================================="

# python to get json and csv to correct location
python3 sort_sim_data.py "$DIST_TYPE"

echo "=========================================="
echo "Pipeline complete!"
echo "=========================================="