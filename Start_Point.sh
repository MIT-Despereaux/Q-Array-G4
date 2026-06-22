#!/bin/bash

# Stop execution immediately if any step throws an error
set -e

# --- CLEANUP FUNCTION FOR FORCED EXITS ---
cleanup() {
    echo -e "\n[Trap] Script interrupted! Cleaning up macro files..."
    
    # Restore init_vis.mac if a backup exists
    if [ -f init_vis.mac.bak ]; then
        mv init_vis.mac.bak init_vis.mac
    fi
    
    # Clean up trailing multi-source backups
    rm -f template_multi_source.mac.orig
    
    exit 1
}

# Assign the cleanup function to intercept SIGINT (Ctrl+C) and SIGTERM
trap cleanup SIGINT SIGTERM


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
    
    [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
    echo "/control/execute ./Set_Up.mac.txt" >> init_vis.mac
    ./build/main
    
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

elif [ "$2" == "multi" ]; then
    echo "Multiple particle sources initiated"
    [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
    
    rm -f G4History.macro
    
    NUM_EVENTS=$3
    CSV_FILE=$4
    
    ./Multi_Source_Set_Up.sh "$NUM_EVENTS" "$CSV_FILE"
    echo "Multi Source creation finished."
    
    ./build/main
    
    rm -f template_multi_source.mac.orig
    
    if [ -f init_vis.mac.bak ]; then
        mv init_vis.mac.bak init_vis.mac
    else
        rm -f init_vis.mac
    fi

else
    echo "Invalid mode selection: '$2'"
    exit 1
fi

echo "=========================================="
echo "Simulation Finished. Starting File Sorter..."
echo "=========================================="

python3 sort_sim_data.py "$DIST_TYPE"

echo "=========================================="
echo "Pipeline complete!"
echo "=========================================="