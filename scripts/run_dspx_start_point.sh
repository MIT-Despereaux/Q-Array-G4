#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "${script_dir}/.." && pwd)
build_dir="${repo_root}/build-dspx"
output_dir="${repo_root}/output/Data"
staging_dir="${output_dir}/_gps_sources"
output_prefix="test"
generated_template_dir="$repo_root/build-dspx"
temporary_multi_source="$generated_template_dir/temporary_multi_source.mac"


if [[ $# -lt 2 ]]; then
    echo "Usage: ./scripts/run_dspx_start_point.sh <test_hist | ISO_spectrum | mono> <single | double | multi> [multi_events] [multi_csv]"
    exit 1
fi

dist_type=$1
source_mode=$2
multi_events=${3:-100}
multi_csv=${4:-"${repo_root}/scripts/example_multi_source.csv"}
display_mode=${5:-"batch"}
#we choose if visual or batch here as I want to append to 
run_multi_setup=false

case "${source_mode}" in
    single)
        macro="macros/gps_single_neutron_test.mac"
        ;;
    double)
        macro="macros/gps_double_neutron_gamma_test.mac"
        ;;
    multi)
        run_multi_setup=true
        macro=$temporary_multi_source
        ;;
    *)
        echo "Invalid mode selection: '${source_mode}'"
        echo "Usage: ./scripts/run_dspx_start_point.sh <test_hist | ISO_spectrum | mono> <single | double | multi> [multi_events] [multi_csv]"
        exit 1
        ;;
esac

mkdir -p "${staging_dir}"

# The Geant4 CSV backend refuses to overwrite per-ntuple files. Remove only
# this GPS job's staging outputs so repeated runs remain deterministic.
find "${staging_dir}" -maxdepth 1 -type f -name "${output_prefix}*" -delete

cmake -S "${repo_root}" -B "${build_dir}" \
  -DQARRAY_DETECTOR_GEOMETRY=DSPX \
  -DWITH_CRY=OFF
cmake --build "${build_dir}"

if [[ "${run_multi_setup}" == true ]]; then
    "${repo_root}/scripts/multi_source_setup.sh" "${multi_events}" "${multi_csv}"
fi

echo "=========================================="
echo "Running Geant4 macro: ${macro}"
echo "=========================================="

cd "${build_dir}"
case "${display_mode}" in
    visual)
    local_macro="./init_vis.mac"
    repo_macro="/Users/tclassen23/q-array-g4/macros/init_vis.mac"

    # 1. Pull the base clean template from the repo
    tr -d '\r' < "$repo_macro" > "$local_macro"
    
    # 2. Add safe Unix line spacing
    echo "" >> "$local_macro"
    echo "" >> "$local_macro"
    
    # 3. Append the baseline visual controls that worked originally
    echo "/vis/drawVolume" >> "$local_macro"
    echo "/vis/scene/add/trajectories smooth" >> "$local_macro"
    echo "/vis/scene/endOfEventAction accumulate -1" >> "$local_macro"
    
    # --- CUSTOM PARTICLE COLORING ---
    echo "/vis/modeling/trajectories/create/drawByParticleID customDrawByParticle" >> "$local_macro"
    echo "/vis/modeling/trajectories/customDrawByParticle/set neutron yellow" >> "$local_macro"
    echo "/vis/modeling/trajectories/customDrawByParticle/set gamma cyan" >> "$local_macro"
    echo "/vis/modeling/trajectories/customDrawByParticle/set e- red" >> "$local_macro"
    echo "/vis/modeling/trajectories/customDrawByParticle/set geantino magenta" >> "$local_macro"
    echo "/vis/modeling/trajectories/customDrawByParticle/set proton blue" >> "$local_macro"
    echo "/vis/modeling/trajectories/select customDrawByParticle" >> "$local_macro"
    
    # Run your source macro setup
    echo "/control/execute ${macro}" >> "$local_macro"
    
    # 4. Overwrite the repository file path so Geant4 reads it
    cp "$local_macro" "$repo_macro"
    
    # 5. Launch Geant4 natively
    ./main
    
    # 6. Revert back to the clean baseline state using bulletproof echos
    echo "/control/verbose 2" > "$repo_macro"
    echo "/run/verbose 2" >> "$repo_macro"
    echo "" >> "$repo_macro"
    echo "# 1. Open the visual window canvas" >> "$repo_macro"
    echo "/vis/open OGL 1200x1200-0+0" >> "$repo_macro"
    echo "" >> "$repo_macro"
    echo "# 2. Tell Geant4 to look at your generated source macro" >> "$repo_macro"
    echo "# (The shell script will append this exact line to the bottom)" >> "$repo_macro"
    
    cp "$repo_macro" "$local_macro"
    ;;
    cp "$repo_macro" "$local_macro"
    ;;
    batch)
    "${repo_root}/scripts/batch_mode.sh" "${macro}" 
    ./main "${macro}"
    # sleep 9
    "${repo_root}/scripts/batch_mode.sh" "${macro}" 
    ;;
    
    *)
    echo "Invalid argument: <visual | batch>"
    exit 1
    ;;
esac

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
rm -f "${temporary_multi_source}"
rm -f "${build_dir}"/template_source_[0-9]*.mac

# Reset init_vis.mac back to its clean repository state so git changes stay clear
if [[ "${display_mode}" == "visual" ]]; then
    cp "${repo_root}/macros/init_vis.mac" "${build_dir}/init_vis.mac"
fi

echo "=========================================="
echo "Pipeline complete."
echo "=========================================="
