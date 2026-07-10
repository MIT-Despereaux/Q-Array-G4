#!/bin/bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
echo "Script directory: $script_dir"
echo "Repository root: $repo_root"
template_dir="$repo_root/macros/templates"
generated_template_dir="$repo_root/build-dspx"

cleanup_after_run=false
output_dir="../output"
mode=""

usage() {
    echo "Usage: $0 <batch|visual> <event-count> <sources.csv> <output-file> [--output-dir <relative-dir>] [--cleanup]" >&2
}

# Parse incoming arguments, filtering out our runtime behavior flags
args=()
while [[ $# -gt 0 ]]; do
    arg="$1"
    case "$arg" in
        --cleanup)
            cleanup_after_run=true
            ;;
        --output-dir)
            shift
            if [[ $# -eq 0 ]]; then
                echo "Error: --output-dir requires a relative directory." >&2
                usage
                exit 1
            fi
            output_dir="$1"
            ;;
        --output-dir=*)
            output_dir="${arg#--output-dir=}"
            ;;
        --visual)
            mode="visual"
            ;;
        --batch)
            mode="batch"
            ;;
        *)
            args+=("$arg")
            ;;
    esac
    shift
done
set -- "${args[@]}"

if [[ -z "$mode" ]]; then
    mode="$1"
    shift
fi

Event_Number="$1"
csv_file="$2"
output_file="$3"

if [[ "$mode" != "batch" && "$mode" != "visual" ]]; then
    echo "ERROR: Invalid mode specified: '$mode'" >&2
    usage
    exit 1
fi

if [[ $# -ne 3 ]]; then
    usage
    exit 1
fi

if [[ ! "$Event_Number" =~ ^[0-9]+$ ]]; then
    echo "Error: Event_Number '$Event_Number' must be a valid integer." >&2
    exit 1
fi

if [[ "$csv_file" != *.csv ]]; then
    echo "Error: csv_file '$csv_file' must be a string ending in .csv" >&2
    exit 1
fi

if [[ -z "$output_file" || "$output_file" == */* || "$output_file" == "." || "$output_file" == ".." ]]; then
    echo "Error: output-file must be a file name without a directory component." >&2
    exit 1
fi

if [[ -z "$output_dir" || "$output_dir" == /* || "$output_dir" == ~* ]]; then
    echo "Error: --output-dir must be a relative directory." >&2
    exit 1
fi
output_dir="${output_dir%/}"
[[ -n "$output_dir" ]] || output_dir="."

# Dynamically construct the specific macro template path based on the parsed mode
template_multi_source="$template_dir/template_multi_source_${mode}.mac"
template_source="$template_dir/template_source.mac"
temporary_multi_source="$generated_template_dir/temporary_multi_source.mac"
output_path="${output_dir}/${output_file}"

echo "Selected configuration mode: $mode"
echo "Using macro template: $template_multi_source"
echo "Output filename: $output_path"

# --- sanity checks ---
if [ ! -f "$csv_file" ]; then
    echo "Error: The CSV file could not be found at: $csv_file"
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
mkdir -p "$generated_template_dir/$output_dir"

# Clean old generated source outputs
rm -f "$temporary_multi_source" "$generated_template_dir"/template_source_[0-9]*.mac

# Copy the baseline chosen master template file directly to our destination target
cp "$template_multi_source" "$temporary_multi_source"

escaped_output_path=$(printf '%s' "$output_path" | sed 's/[&|\\]/\\&/g')
if [[ "$mode" == "batch" ]]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sed -i '' "s|<OUTPUT_FILE>|$escaped_output_path|g" "$temporary_multi_source"
    else
        sed -i "s|<OUTPUT_FILE>|$escaped_output_path|g" "$temporary_multi_source"
    fi
fi

# Create a clean scratch file to pool all dynamic source configurations during execution
snippet_file="$generated_template_dir/dynamic_sources.tmp"
rm -f "$snippet_file"

first_source=true
source_counter=0   

SEED1=$RANDOM
SEED2=$RANDOM

# Write initial headers to the dynamic snippet cache
echo "/QR/generator/mode gps" >> "$snippet_file"
echo "/random/setSeeds $SEED1 $SEED2" >> "$snippet_file"
echo "" >> "$snippet_file"

while IFS=',' read -r col1 col2 col3 col4 || [ -n "$col1" ]; do
    
    col1=$(echo "$col1" | tr -d '\r\n[:space:]')
    col2=$(echo "$col2" | tr -d '\r\n[:space:]')
    col3=$(echo "$col3" | tr -d '\r\n[:space:]')
    col4=$(echo "$col4" | tr -d '\r\n[:space:]')

    if [[ "$col1" == "type" || -z "$col1" ]]; then
        echo "DEBUG: Skipping this line because it is a header or empty."
        continue
    fi

    particletype="$col1"
    declare -i num="$col2" 
    intensity="$col3"
    spectrum="$col4"

    echo "Processing CSV Row -> Type: $particletype, Count: $num, Intensity: $intensity"

    for (( i=1; i<=num; i++ )); do
        ((source_counter++))
        current_macro="template_source_${source_counter}.mac"
        current_macro_path="$generated_template_dir/$current_macro"

        cp "$template_source" "$current_macro_path"
        gps_index=$((source_counter - 1))

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

        clean_spectrum=$(echo "$spectrum" | tr -d '\r\n[:space:]')

        if [[ "$clean_spectrum" == "../"* ]]; then
            pure_path=${clean_spectrum#../}
            full_spectrum_path="${repo_root}/${pure_path}"
        else
            full_spectrum_path="${clean_spectrum}"
        fi

        echo "# --- Appended Spectrum Points via convert_spectrum.sh ---" >> "$current_macro_path"
        
        if ! "$script_dir/convert_spectrum.sh" "$full_spectrum_path" >> "$current_macro_path"; then
            echo "ERROR: convert_spectrum.sh crashed or could not process path: $full_spectrum_path"
            exit 1
        fi
        
        echo "/gps/hist/inter Lin" >> "$current_macro_path"

        # Redirect the orchestration command routing to our scratch workspace block
        if [ "$first_source" = true ]; then
            echo "/gps/source/intensity $intensity" >> "$snippet_file"
            echo "/control/execute ./$current_macro" >> "$snippet_file"
            echo "" >> "$snippet_file"
            first_source=false
        else
            echo "/gps/source/add $intensity" >> "$snippet_file"
            echo "/control/execute ./$current_macro" >> "$snippet_file"
            echo "" >> "$snippet_file"
        fi
    done

done < <(sed -e '1s/^\xef\xbb\xbf//' -e 's/\r//g' "$csv_file")

# Append the trailing target macro definitions to the snippet file
#echo "" >> "$snippet_file"

if [ "$mode" = "batch" ]; then
    echo "/tracking/verbose 1" >> "$snippet_file"
fi

echo "/gps/source/multiplevertex false" >> "$snippet_file"
echo "/run/beamOn $Event_Number" >> "$snippet_file"
echo "" >> "$snippet_file"

# --- SURGICAL IN-PLACE INJECTION STEP ---
# This awk process scans the copied template, finds the # >>> marker, dumps the dynamic source profile,
# skips the stale inner lines, and resumes printing safely from # <<< downwards.
awk -v snippet="$snippet_file" '
    /^# >>>/ {
        print
        while ((getline line < snippet) > 0) {
            print line
        }
        close(snippet)
        skip = 1
        next
    }
    /^# <<</ {
        skip = 0
    }
    !skip {
        print
    }
' "$temporary_multi_source" > "${temporary_multi_source}.tmp" && mv "${temporary_multi_source}.tmp" "$temporary_multi_source"

# Evacuate the temporary scratch accumulator file
rm -f "$snippet_file"

# if [ "$cleanup_after_run" = true ]; then
#     rm -f "$temporary_multi_source" "$generated_template_dir"/template_source_[0-9]*.mac
# fi

echo "Finished script orchestration successfully!"
echo "Your script can be executed with /control/execute $temporary_multi_source"
