#!/bin/bash

# Exit immediately if a command fails
set -e

# Number of times you want to run the simulation
RUNS=50

echo "=========================================="
echo " Starting batch execution: $RUNS total runs"
echo "=========================================="

for ((i=1; i<=RUNS; i++))
do
    echo ""
    echo "------------------------------------------"
    echo " Executing Run $i of $RUNS"
    echo "------------------------------------------"
    
    # Call your original runtime script here.
    # Pass whatever arguments it usually expects (like your macro name)
    ./scripts/run_dspx_start_point.sh ISO_spectrum multi 100 "/Users/tclassen23/q-array-g4/scripts/Multi_Source_Spectrums.csv" batch
    
done

echo "=========================================="
echo " All $RUNS runs completed successfully!"
echo " Check audited_neutrons.csv and audited_gammas.csv"
echo "=========================================="