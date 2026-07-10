#!/bin/bash

# Check if input file is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <input_csv_file>"
    exit 1
fi

INPUT_CSV="$1"

# Check if file exists
if [ ! -f "$INPUT_CSV" ]; then
    echo "Error: File '$INPUT_CSV' not found."
    exit 1
fi

# Step 1: Read the CSV line-by-line, skipping the header row
# Step 2: Clean up spaces, tabs, carriage returns, and commas
# Step 3: Format each valid numeric row into a /gps/hist/point command
tail -n +2 "$INPUT_CSV" | while read -r line || [ -n "$line" ]; do
    # Convert commas or tabs to spaces and squeeze multiple spaces into one
    cleaned_line=$(echo "$line" | tr ',' ' ' | tr -d '\r' | xargs)
    
    # Extract the two numeric columns (Energy and Weight)
    energy=$(echo "$cleaned_line" | awk '{print $1}')
    weight=$(echo "$cleaned_line" | awk '{print $2}')
    
    # Only print if both values are valid numbers and energy is strictly greater than 0
    # (This automatically drops any text residuals or 0.00E+00 baseline errors)
    # Update this conditional statement inside your convert_spectrum.sh script:
    if [[ ! -z "$energy" && ! -z "$weight" ]]; then
        # Ensure energy is positive AND weight is strictly greater than 0
        is_valid=$(awk -v eng="$energy" -v wgt="$weight" 'BEGIN {print (eng > 0 && wgt > 0 ? 1 : 0)}')
        if [ "$is_valid" -eq 1 ]; then
            echo "/gps/hist/point $energy $weight"
        fi
    fi
done