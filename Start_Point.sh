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
    
    # Restore template_multi_source.mac if a backup exists
    if [ -f template_multi_source.mac.bak_multi ]; then
        mv template_multi_source.mac.bak_multi template_multi_source.mac
    fi
    
    exit 1
}

# Assign the cleanup function to intercept SIGINT (Ctrl+C) and SIGTERM
trap cleanup SIGINT SIGTERM


# --- 1. Check for inputs ---
if [[ -z "$1" || -z "$2" ]]; then
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

#ToDo multiple source creation and specification beyond just simple 2 particle type/number of sources.
elif [ "$2" == "multi" ]; then
    if [[ -z "$3" || -z "$4" ]]; then
        echo "Error: Multi-source mode requires event number and CSV file path."
        echo "Usage: ./Start_Point.sh <dist> multi <events> <csv_file>"
        exit 1
    fi

    echo "Multiple particle sources initiated"
    [ -f init_vis.mac ] && cp init_vis.mac init_vis.mac.bak
    
    # Securely backup your template_multi_source base macro here before appending data
    [ -f template_multi_source.mac ] && cp template_multi_source.mac template_multi_source.mac.bak_multi
    
    rm -f G4History.macro
    
    NUM_EVENTS=$3
    # Handles filenames with unquoted spaces automatically by pulling all trailing elements
    CSV_FILE="${@:4}"
    
    ./Multi_Source_Set_Up.sh "$NUM_EVENTS" "$CSV_FILE"
    echo "Multi Source creation finished."
    
    ./build/main
    
    # Restore the multi_source template back to its pristine state now that Geant4 execution is complete
    if [ -f template_multi_source.mac.bak_multi ]; then
        mv template_multi_source.mac.bak_multi template_multi_source.mac
    fi
    
    # clean up
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

# python to get json and csv to correct location
python3 sort_sim_data.py "$DIST_TYPE"

echo "=========================================="
echo "Pipeline complete!"
echo "=========================================="