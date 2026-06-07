#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 4 || $# -gt 5 ]]; then
  echo "usage: $0 <executable> <macro> <output-prefix> <detector-name|manifest> [manifest]" >&2
  exit 2
fi

executable=$1
macro=$2
output_prefix=$3
check_target=$4
mode=${5:-single}
workdir="${PWD}/energy-deposition-$(basename "${check_target}")"

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
  "${executable}" "${macro}" 2>&1 | tee run.log
)

has_positive_edep() {
  local detector_name=$1
  local csv_files
  shopt -s nullglob
  csv_files=("${workdir}/${output_prefix}"*"${detector_name}"*.csv)
  shopt -u nullglob
  if [[ ${#csv_files[@]} -eq 0 ]]; then
    echo "No CSV output found for detector ${detector_name}" >&2
    return 1
  fi

  local csv_file
  for csv_file in "${csv_files[@]}"; do
    if awk -F, '
      BEGIN { column_count = 0; edep_column = 0 }
      /^#column / {
        ++column_count
        if ($0 ~ / edep$/) edep_column = column_count
        next
      }
      /^#/ { next }
      edep_column > 0 && ($edep_column + 0) > 0 { found = 1 }
      END { exit found ? 0 : 1 }
    ' "${csv_file}"; then
      return 0
    fi
  done
  echo "No positive edep value found for detector ${detector_name}" >&2
  return 1
}

if [[ ${mode} == single ]]; then
  has_positive_edep "${check_target}"
  echo "Positive energy deposition found for detector ${check_target}"
  exit 0
fi

manifest=${check_target}
actual="${workdir}/actual-inventory.txt"
awk '
  /^DSPX_SCORING / {
    logical = detector = material = requirement = ""
    for (i = 2; i <= NF; ++i) {
      split($i, field, "=")
      if (field[1] == "logical") logical = field[2]
      else if (field[1] == "detector") detector = field[2]
      else if (field[1] == "material") material = field[2]
      else if (field[1] == "requirement") requirement = field[2]
    }
    if (logical != "") print requirement, logical, material, detector
  }
' "${workdir}/run.log" | sort -u > "${actual}"

expected="${workdir}/expected-inventory.txt"
awk 'NF && $1 !~ /^#/ { print $1, $2, $3, "dspx_" $2 }' "${manifest}" | sort > "${expected}"

if ! diff -u "${expected}" "${actual}"; then
  echo "DSPX scoring inventory does not match ${manifest}" >&2
  exit 1
fi

while read -r requirement logical material detector; do
  if [[ ${requirement} == positive ]]; then
    has_positive_edep "${detector}"
  fi
done < "${actual}"

echo "DSPX scoring inventory and positive energy deposition checks passed"
