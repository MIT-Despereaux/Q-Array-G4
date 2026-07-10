#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "${script_dir}/.." && pwd)
build_dir="${repo_root}/build-dspx"
output_dir="${repo_root}/output/Data"
staging_dir="${output_dir}/_gps_sources"
output_prefix="test"
generated_template_dir="$repo_root/build-dspx"
macro="$generated_template_dir/temporary_multi_source.mac"

dist_type="ISO_spectrum"
run_multi_setup=true

Event_Number=$1
CSV="$2"

if [[ ! "$Event_Number" =~ ^[0-9]+$ ]]; then
    echo "Error: Event_Number '$Event_Number' must be a valid integer." >&2
    exit 1
fi

# 2. Validate that csv_file ends with .csv
if [[ "$CSV" != *.csv ]]; then
    echo "Error: csv_file '$CSV' must be a string ending in .csv" >&2
    exit 1
fi

mkdir -p "${staging_dir}"

# The Geant4 CSV backend refuses to overwrite per-ntuple files. Remove only
# this GPS job's staging outputs so repeated runs remain deterministic.
find "${staging_dir}" -maxdepth 1 -type f -name "${output_prefix}*" -delete

cmake -S "${repo_root}" -B "${build_dir}" \
  -DQARRAY_DETECTOR_GEOMETRY=DSPX \
  -DWITH_CRY=OFF
cmake --build "${build_dir}"

if [[ "${run_multi_setup}" == true ]]; then
    "${repo_root}/scripts/multi_source_setup.sh" "batch" "$Event_Number" "$CSV"
fi

echo "=========================================="
echo "Running Geant4 macro: ${macro}"
echo "=========================================="

cd "${build_dir}"

repo_macro="../macros/blank_template.mac"
build_macro="./macros/blank_template.mac"  # Points to build-dspx/macros/ version


#Batch might need some help now that
#essentially I need to change this portion, it scrubs temporary file of visual components, but really I want it to do all above just no visual

# CREATE CLEANUP FUNCTION: This runs no matter what (even on crash/exit)
cleanup() {
    echo "--> Reverting macros back to pristine state..."
    cat << 'EOF' > "$repo_macro"
# Batch macro template

/control/verbose 2
/run/verbose 2
EOF
    # Sync the pristine version back to the build directory too
    cp "$repo_macro" "$build_macro"
}
# Register the cleanup function to fire when the script exits or is interrupted
trap cleanup EXIT INT TERM

# 1. Pull the base clean template from the repo into the build directory
tr -d '\r' < "$repo_macro" > "$build_macro"

echo "" >> "$build_macro"

# 2. Run your source macro setup
echo "/control/execute ${macro}" >> "$build_macro"

# 3. Copy the modified macro to the repo if Geant4 needs to look there too
cp "$build_macro" "$repo_macro"

# Normal version
./main "${build_macro}"

echo "=========================================="
    echo "EXPLICIT GEANT4 OUTPUT AUDIT:"
    echo "=========================================="
    # Hunt down any test*.csv or test*.json files modified in the last 1 minute inside the build directory
    found_files=$(find "${build_dir}" -maxdepth 2 -type f \( -name "test*.csv" -o -name "test*.json" \) -mmin -1)
    
    if [[ -z "${found_files}" ]]; then
        echo "VERDICT: No output files were written by the binary to ${build_dir}!"
    else
        echo "VERDICT: Files found! Printing absolute locations and sizes:"
        echo "${found_files}" | while read -r file; do
            ls -lh "$file" | awk '{print $9 " (" $5 ")"}'
        done
    fi
    echo "=========================================="



# Unified Staging Move: Target the files exactly where they are built
mv "${build_dir}"/test*.csv "${staging_dir}/" 2>/dev/null || true
mv "${build_dir}"/test*.json "${staging_dir}/" 2>/dev/null || true

echo "=========================================="
echo "Simulation finished. Starting file sorter."
echo "=========================================="

# python3 "${repo_root}/scripts/sort_sim_data.py" "${dist_type}" "${macro}" "${staging_dir}" "${output_dir}"
# Not needed anymore as output added with macros.

echo "=========================================="
echo "Cleaning up temporary macros..."
echo "=========================================="
# Safely wipe out the dynamically generated macro files from this run
rm -f "${macro}"
rm -f "${build_dir}"/template_source_[0-9]*.mac

# Reset init_vis.mac back to its clean repository state so git changes stay clear
if [[ "${display_mode}" == "visual" ]]; then
    cp "${repo_root}/macros/init_vis.mac" "${build_dir}/init_vis.mac"
fi

echo "=========================================="
echo "Pipeline complete."
echo "=========================================="
