#!/bin/bash

# Usage:
#   ./consolidate_csvs.sh [consolidate|leave]
#
# Modes:
#   consolidate (default) : Delete empty CSVs, merge data into RUN_NAME_MASTER.csv files, and delete raw chunks
#   leave                 : Delete empty CSVs & leave individual data files as-is

TARGET_DIR="../output/g4sim/q_array_cmp"

# Parse optional argument (defaults to 'consolidate' if not provided)
MODE=$(echo "${1:-consolidate}" | tr '[:upper:]' '[:lower:]')

case "$MODE" in
    consolidate|c|-c|--consolidate|a)
        DO_CONSOLIDATE=true
        echo "=== Mode: Remove empty files AND CONSOLIDATE into Run Masters ==="
        ;;
    leave|keep|l|-l|--leave|b)
        DO_CONSOLIDATE=false
        echo "=== Mode: Remove empty files AND LEAVE individual data files ==="
        ;;
    *)
        echo "Error: Invalid argument '$1'"
        echo "Usage: $0 [consolidate|leave]"
        echo "  consolidate : Merge data files into respective run masters and clean up"
        echo "  leave       : Delete empty CSVs & keep separate data files"
        exit 1
        ;;
esac

echo "Scanning CSV files in ${TARGET_DIR}..."
echo "------------------------------------------"

TOTAL_FILES=0
EMPTY_FILES=0
DATA_FILES=0
declare -A MASTER_CREATED # Array to track which master files we create

# Clean up any previous master files before consolidating to prevent duplicate appending
if [ "$DO_CONSOLIDATE" = true ]; then
    rm -f "${TARGET_DIR}"/*_MASTER.csv
fi

for file in "${TARGET_DIR}"/*.csv; do
    [ -e "$file" ] || continue
    # Skip if it is already a master file (just in case)
    if [[ "$file" == *"_MASTER.csv" ]]; then continue; fi
    
    ((TOTAL_FILES++))

    # Count how many lines DO NOT start with '#' (actual data rows)
    DATA_ROWS=$(grep -c -v '^#' "$file")

    if [ "$DATA_ROWS" -eq 0 ]; then
        # File only contains header lines -> DELETE IT
        rm "$file"
        ((EMPTY_FILES++))
    else
        # File has real data!
        ((DATA_FILES++))
        
        if [ "$DO_CONSOLIDATE" = true ]; then
            # Extract everything BEFORE "-csv_nt_" to get the base run name
            RUN_NAME=$(basename "$file" | sed -E 's/-csv_nt_.*//')
            MASTER_FILE="${TARGET_DIR}/${RUN_NAME}_MASTER.csv"

            # Save header from the first file of this run
            if [ ! -f "$MASTER_FILE" ]; then
                grep '^#' "$file" > "$MASTER_FILE"
                MASTER_CREATED["$RUN_NAME"]=1
                echo " [NEW MASTER] Created ${RUN_NAME}_MASTER.csv"
            fi
            
            # Append data rows to the run's master file
            grep -v '^#' "$file" >> "$MASTER_FILE"
            echo " [DATA -> ${RUN_NAME}_MASTER] $(basename "$file") ($DATA_ROWS data rows)"
            
            # Delete the raw chunk file so only the consolidated master remains
            rm "$file"
        else
            echo " [KEPT SEPARATE]  $(basename "$file") ($DATA_ROWS data rows)"
        fi
    fi
done

echo "=========================================="
echo "SUMMARY:"
echo "  Total raw files checked:      $TOTAL_FILES"
echo "  Empty files deleted:          $EMPTY_FILES"
if [ "$DO_CONSOLIDATE" = true ]; then
    echo "  Data files merged & deleted:  $DATA_FILES"
    echo "  Master files created:         ${#MASTER_CREATED[@]}"
else
    echo "  Files with data kept:         $DATA_FILES"
fi
echo "=========================================="