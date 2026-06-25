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
display_mode=$5
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
    echo "/control/execute $macro" >> "init_vis.mac"
    ./main
    ;;
    batch)
    #file here to strip visual from macro
    ./main "${macro}"
    ;;
    *)

    ;;
esac


echo "=========================================="
echo "Simulation finished. Starting file sorter."
echo "=========================================="

python3 "${repo_root}/scripts/sort_sim_data.py" "${dist_type}" "${macro}" "${staging_dir}" "${output_dir}"

echo "=========================================="
echo "Pipeline complete."
echo "=========================================="
