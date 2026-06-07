#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "${script_dir}/.." && pwd)
build_dir="${repo_root}/build-dspx"
output_dir="${repo_root}/output/g4sim/v02x"
output_prefix="dspx_cosmic_batch"

mkdir -p "${output_dir}"

# The Geant4 CSV backend refuses to overwrite per-ntuple files. Remove only
# this batch job's previous outputs so repeated runs remain deterministic.
find "${output_dir}" -maxdepth 1 -type f -name "${output_prefix}*" -delete

cmake -S "${repo_root}" -B "${build_dir}" \
  -DQARRAY_DETECTOR_GEOMETRY=DSPX \
  -DWITH_CRY=OFF
cmake --build "${build_dir}"

cd "${build_dir}"
./main dspx_cosmic_batch.mac
