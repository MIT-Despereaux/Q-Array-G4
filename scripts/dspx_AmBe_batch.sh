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


mkdir -p "${staging_dir}"

# The Geant4 CSV backend refuses to overwrite per-ntuple files. Remove only
# this GPS job's staging outputs so repeated runs remain deterministic.
find "${staging_dir}" -maxdepth 1 -type f -name "${output_prefix}*" -delete

cmake -S "${repo_root}" -B "${build_dir}" \
  -DQARRAY_DETECTOR_GEOMETRY=DSPX \
  -DWITH_CRY=OFF
cmake --build "${build_dir}"

if [[ "${run_multi_setup}" == true ]]; then
    "${repo_root}/scripts/multi_source_setup.sh" "batch"
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

# "${repo_root}/scripts/batch_mode.sh" "${macro}" 
# Normal version
#./main "${build_macro}"

# #Keep this, its for troubleshooting purposes:
./main "${build_macro}" | awk '
# 1. Capture the particle type from the track info header line
/G4Track Information:/ {
    # Extract the thread ID (e.g., "G4WT3")
    thread = $1
    
    # Safely isolate the particle name by searching for the token "Particle ="
    if (match($0, /Particle = [a-zA-Z0-9_-]+/)) {
        part_string = substr($0, RSTART, RLENGTH)
        split(part_string, parts, "= ")
        particle[thread] = parts[2]
    }
}

# 2. When the initial step is found, grab the energy and write it out
/initStep$/ {
    thread = $1
    p_type = particle[thread]
    
    # In a standard tracking line, KineE is the 5th column after the thread ID prefix
    # ($2=Step#, $3=X, $4=Y, $5=Z, $6=KineE)
    # If the units are split, we can grab both the value ($6) and unit ($7)
    energy = $10 
    unit = $11

    if (p_type == "neutron") {
        print energy "," unit >> "../output/initial_data/audited_neutrons.csv"
    } else if (p_type == "gamma") {
        print energy "," unit >> "../output/initial_data/audited_gammas.csv"
    }
    
    # Clean up state for this thread
    delete particle[thread]
}'
echo "Added to CSV file!"



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

python3 "${repo_root}/scripts/sort_sim_data.py" "${dist_type}" "${macro}" "${staging_dir}" "${output_dir}"

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
