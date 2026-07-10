#!/bin/bash
# For this bash script input a path to log file it will extract the initial gamma and neutrons ONLY.

# Capture the first argument
log_file="$1"

# Check if the file exists and is a regular file
if [ ! -f "$log_file" ]; then
    echo "ERROR: Valid log file path required."
    echo "Usage: $0 <path_to_log_file>"
    exit 1 
fi

# Ensure the output directory exists so awk doesn't fail silently
mkdir -p ./output/initial_data/

awk '
    BEGIN {
        OFS = ","
        gamma_file = "./output/initial_data/primary_gammas.csv"
        neutron_file = "./output/initial_data/primary_neutrons.csv"
        
        print "Kinetic_Energy,Unit" > gamma_file
        print "Kinetic_Energy,Unit" > neutron_file
    }

    # When a track header appears, assign the state to the specific thread ($1)
    /\* G4Track Information:/ {
        thread = $1
        is_gamma[thread] = ($0 ~ /Parent ID = 0/ && $0 ~ /Particle = gamma/) ? 1 : 0
        is_neutron[thread] = ($0 ~ /Parent ID = 0/ && $0 ~ /Particle = neutron/) ? 1 : 0
    }
    
    # When an initStep appears, check if THIS specific thread has a primary particle queued
    /initStep/ {
        thread = $1
        
        if (is_gamma[thread] || is_neutron[thread]) {
            
            # 1. Find where the data starts (skip the Thread ID and Step 0)
            idx = 1
            for (i=1; i<=NF; i++) {
                if ($i == ">") {
                    idx = i + 2 
                    break
                }
            }
            
            # 2. Skip X, Y, and Z dynamically. 
            for (coord=1; coord<=3; coord++) {
                if ($(idx+1) ~ /^(m|cm|mm|um|nm|fm|pm)$/) {
                    idx += 2
                } else {
                    idx += 1
                }
            }
            
            # 3. idx now perfectly points to Kinetic Energy
            energy = $idx
            unit = $(idx+1)
            
            # Convert all standard Geant4 energy units to MeV
            if (unit == "eV") {
                energy = energy / 1000000.0
            } else if (unit == "keV") {
                energy = energy / 1000.0
            } else if (unit == "GeV") {
                energy = energy * 1000.0
            } else if (unit == "meV") {
                energy = energy / 1000000000.0
            } 
            
            # Print the converted energy and turn off the flag for THIS thread
            if (is_gamma[thread]) {
                print energy, "MeV" >> gamma_file
                is_gamma[thread] = 0
            } else if (is_neutron[thread]) {
                print energy, "MeV" >> neutron_file
                is_neutron[thread] = 0
            }
        }
    }
' "$log_file"