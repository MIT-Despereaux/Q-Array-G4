#!/bin/bash

# Usage:
#   ./consolidate_csvs.sh [consolidate|leave]
#
# Modes:
#   consolidate (default) : Delete empty CSVs & merge data files into MASTER_CONSOLIDATED.csv
#   leave                 : Delete empty CSVs & leave individual data files as-is

TARGET_DIR="../output/g4sim/q_array_cmp"
CONSOLIDATED_FILE="${TARGET_DIR}/MASTER_CONSOLIDATED.csv"

# Parse optional argument (defaults to 'consolidate' if not provided)
MODE=$(echo "${1:-consolidate}" | tr '[:upper:]' '[:lower:]')

case "$MODE" in
    consolidate|c|-c|--consolidate|a)
        DO_CONSOLIDATE=true
        echo "=== Mode: Remove empty files AND CONSOLIDATE data files ==="
        ;;
    leave|keep|l|-l|--leave|b)
        DO_CONSOLIDATE=false
        echo "=== Mode: Remove empty files AND LEAVE individual data files ==="
        ;;
    *)
        echo "Error: Invalid argument '$1'"
        echo "Usage: $0 [consolidate|leave]"
        echo "  consolidate : Delete empty CSVs & merge data files into MASTER_CONSOLIDATED.csv"
        echo "  leave       : Delete empty CSVs & keep separate data files"
        exit 1
        ;;
esac

echo "Scanning CSV files in ${TARGET_DIR}..."
echo "------------------------------------------"

TOTAL_FILES=0
EMPTY_FILES=0
DATA_FILES=0
HEADER_SAVED=0

# Clean up any previous master file before consolidating
if [ "$DO_CONSOLIDATE" = true ]; then
    rm -f "$CONSOLIDATED_FILE"
fi

for file in "${TARGET_DIR}"/*.csv; do
    # Skip if no CSV files exist or if it's the master file
    [ -e "$file" ] || continue
    [ "$file" == "$CONSOLIDATED_FILE" ] && continue
    
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
            echo " [DATA -> MASTER] $(basename "$file") ($DATA_ROWS data rows)"
            # Save header from the first non-empty file
            if [ $HEADER_SAVED -eq 0 ]; then
                grep '^#' "$file" > "$CONSOLIDATED_FILE"
                HEADER_SAVED=1
            fi
            # Append data rows to master file
            grep -v '^#' "$file" >> "$CONSOLIDATED_FILE"
        else
            echo " [KEPT SEPARATE]  $(basename "$file") ($DATA_ROWS data rows)"
        fi
    fi
done

echo "=========================================="
echo "SUMMARY:"
echo "  Total CSV files checked:  $TOTAL_FILES"
echo "  Empty files deleted:      $EMPTY_FILES"
echo "  Files with data kept:     $DATA_FILES"
if [ "$DO_CONSOLIDATE" = true ]; then
    if [ $HEADER_SAVED -eq 1 ]; then
        echo "  Consolidated file:        $CONSOLIDATED_FILE"
    else
        echo "  Result: No data rows found across any files."
    fi
fi
echo "=========================================="
