#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
echo "Script directory: $script_dir"
echo "Repository root: $repo_root"
template_dir="$repo_root/macros/templates"
generated_template_dir="$repo_root/build-dspx"
template_multi_source="$template_dir/template_multi_source.mac"
template_source="$template_dir/template_source.mac"
temporary_multi_source="$generated_template_dir/temporary_multi_source.mac"
echo "Template directory: $template_dir"
echo "Generated template directory: $generated_template_dir"
cleanup_after_run=false

#ensure correct input: csv file and a number of events needed, add proper bool here
# We can put csvs in the scripts folder.
args=()
for arg in "$@"; do
    case "$arg" in
        --cleanup)
            cleanup_after_run=true
            ;;
        *)
            args+=("$arg")
            ;;
    esac
done
set -- "${args[@]}"

if [[ -z "$1" || -z "$2" ]]; then
    echo "Error: You must supply number of events (int) and CSV file."
    echo "Usage: ./multi_source_setup.sh [--cleanup] <int> <.csv>"
    exit 1
fi

Event_Number=$1
csv_file=$2

# --- ADD THIS CHECK HERE ---
if [ ! -f "$csv_file" ]; then
    echo "Error: The CSV file could not be found at: $csv_file"
    echo "Please double-check the path and try again."
    exit 1
fi

if [ ! -f "$template_multi_source" ]; then
    echo "Error: The multi-source template could not be found at: $template_multi_source"
    exit 1
fi

if [ ! -f "$template_source" ]; then
    echo "Error: The source template could not be found at: $template_source"
    exit 1
fi

mkdir -p "$generated_template_dir"

# Remove stale generated files so the new run starts from a clean template folder.
rm -f "$temporary_multi_source" "$generated_template_dir"/template_source_[0-9]*.mac

# Rebuild the temporary macro from the static template header every run.
awk '
    { print }
    /^# \[Your multiple sources appended here.*$/ { exit }
' "$template_multi_source" > "$temporary_multi_source"
echo "" >> "$temporary_multi_source"

first_source=true
source_counter=0   # NEW: Track a unique ID for every source file created

while IFS=',' read -r col1 col2 col3 col4 || [ -n "$col1" ]; do
    
    # Strip hidsden Windows/Excel carriage returns (\r) and trailing spaces instantly
    col1=$(echo "$col1" | tr -d '\r\n[:space:]')
    col2=$(echo "$col2" | tr -d '\r\n[:space:]')
    col3=$(echo "$col3" | tr -d '\r\n[:space:]')
    col4=$(echo "$col4" | tr -d '\r\n[:space:]')

    # Ignore header to csv
    if [[ "$col1" == "type" || -z "$col1" ]]; then
        echo "DEBUG: Skipping this line because it is a header or empty."
        continue
    fi

    particletype="$col1"
    declare -i num="$col2" 
    intensity="$col3"
    spectrum="$col4"

    echo "Processing CSV Row -> Type: $particletype, Count: $num, Intensity: $intensity"

    # Loop for the total number of sources specified in this row
# Loop for the total number of sources specified in this row
    for (( i=1; i<=num; i++ )); do
        # Increment global counter to give every single source its own macro file
        ((source_counter++))
        current_macro="template_source_${source_counter}.mac"
        current_macro_path="$generated_template_dir/$current_macro"

        # Copy the clean template to our new uniquely named target file
        cp "$template_source" "$current_macro_path"

        # Calculate the exact 0-indexed GPS ID for this source block
        gps_index=$((source_counter - 1))

        # --- UNIFIED SED AND SPECTRUM APPNEDING LAYER ---
        if [[ "$OSTYPE" == "darwin"* ]]; then
            sed -i '' \
              -e "s|/gps/source/set.*|/gps/source/set $gps_index|" \
              -e "s|__SOURCE_INDEX__|$gps_index|" \
              -e "s|/gps/particle.*|/gps/particle $particletype|" \
              "$current_macro_path"
        else
            sed -i \
              -e "s|/gps/source/set.*|/gps/source/set $gps_index|" \
              -e "s|__SOURCE_INDEX__|$gps_index|" \
              -e "s|/gps/particle.*|/gps/particle $particletype|" \
              "$current_macro_path"
        fi

        # Pass the spectrum path to your converter relative to the repo root execution 
        # (Assuming $spectrum from the CSV holds the relative path like 'macros/sources/ISO_Neutron_Spectrum.txt')
        #full_spectrum_path="$repo_root/$spectrum"

        # Append the converted raw points and cap it with interpolation
        echo "# --- Appended Spectrum Points via convert_spectrum.sh ---" >> "$current_macro_path"
        "$script_dir/convert_spectrum.sh" "$spectrum" >> "$current_macro_path"
        echo "/gps/hist/inter Lin" >> "$current_macro_path"

        # Write the orchestration commands to temporary_multi_source.mac
        if [ "$first_source" = true ]; then
            echo "/gps/source/intensity $intensity" >> "$temporary_multi_source"
            echo "/control/execute ./$current_macro" >> "$temporary_multi_source"
            echo "" >> "$temporary_multi_source"
            first_source=false
        else
            echo "/gps/source/add $intensity" >> "$temporary_multi_source"
            echo "/control/execute ./$current_macro" >> "$temporary_multi_source"
            echo "" >> "$temporary_multi_source"
        fi
    done

done < <(sed -e '1s/^\xef\xbb\xbf//' -e 's/\r//g' "$csv_file")

# Guarantee a clean newline layout for the tail commands
echo "" >> "$temporary_multi_source"
echo "/gps/source/multiplevertex false" >> "$temporary_multi_source"
echo "/run/beamOn $Event_Number" >> "$temporary_multi_source"
echo "" >> "$temporary_multi_source"

if [ "$cleanup_after_run" = true ]; then
    rm -f "$temporary_multi_source" "$generated_template_dir"/template_source_[0-9]*.mac
fi

#now slide it into init_vis.mac in similiar manner.
# Don't do this. For visual testing, please put this somewhere else to avoid overwriting the original init_vis.mac.
# echo "/control/execute ./template_multi_source.mac" >> init_vis.mac

echo "Finished subscript!"
