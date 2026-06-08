#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <executable> <macro>" >&2
  exit 2
fi

executable=$1
macro=$2
workdir="${PWD}/dspx-sn-only-scoring"

rm -rf "${workdir}"
mkdir -p "${workdir}"

shopt -s nullglob
mesh_files=("$(dirname "${executable}")"/*.stl)
shopt -u nullglob
if [[ ${#mesh_files[@]} -gt 0 ]]; then
  cp "${mesh_files[@]}" "${workdir}/"
fi

(
  cd "${workdir}"
  "${executable}" "${macro}" > run.log 2>&1
)

active_logicals=$(
  awk '
    /^DSPX_SCORING_ACTIVE / {
      logical = active = ""
      for (i = 2; i <= NF; ++i) {
        split($i, field, "=")
        if (field[1] == "logical") logical = field[2]
        else if (field[1] == "active") active = field[2]
      }
      if (active == "true") print logical
    }
  ' "${workdir}/run.log" | sort
)

expected="DetectorSnCubeLogical"
if [[ "${active_logicals}" != "${expected}" ]]; then
  echo "Expected only ${expected} to be active in production scoring mode" >&2
  echo "Actual active logical volumes:" >&2
  printf '%s\n' "${active_logicals}" >&2
  exit 1
fi

shopt -s nullglob
csv_files=("${workdir}"/dspx_sn_only_scoring*.csv)
shopt -u nullglob
if [[ ${#csv_files[@]} -gt 1 ]]; then
  echo "Expected at most one Sn scoring CSV, found ${#csv_files[@]}" >&2
  printf '%s\n' "${csv_files[@]}" >&2
  exit 1
fi

if [[ ${#csv_files[@]} -eq 1 && ${csv_files[0]} != *DetectorSnCubeLogical* ]]; then
  echo "Unexpected production scoring CSV: ${csv_files[0]}" >&2
  exit 1
fi

echo "DSPX production scoring activates only DetectorSnCubeLogical"
