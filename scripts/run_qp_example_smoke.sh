#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
case "${1:-phonon}" in
  geometry) macro=qp_example_geometry_check.mac; mode=geometry ;;
  phonon) macro=qp_example_single_phonon.mac; mode=phonon ;;
  electron) macro=qp_example_10ev_electron.mac; mode=electron ;;
  electron-full) macro=qp_example_100mev_electron_full.mac; mode=electron-100mev-full ;;
  electron-10mev-full-al250)
    macro=qp_example_10mev_electron_full_al250.mac
    mode=electron-10mev-full-al250
    executable=qp_example_smoke_al250
    ;;
  electron-10mev-full-al250-capped)
    macro=qp_example_10mev_electron_full_al250_capped.mac
    mode=electron-10mev-full-al250-capped
    executable=qp_example_smoke_al250
    ;;
  electron-10mev-full-al250-cap1m)
    macro=qp_example_10mev_electron_full_al250_cap1m.mac
    mode=electron-10mev-full-al250-cap1m
    executable=qp_example_smoke_al250
    ;;
  phonon-1mev-full-al250)
    macro=qp_example_1mev_phonon_full_al250.mac
    mode=phonon-1mev-full-al250
    executable=qp_example_smoke_al250
    ;;
  geometry-dump-al250)
    macro=qp_example_geometry_dump_al250.mac
    mode=geometry-dump-al250
    executable=qp_example_smoke_al250
    ;;
  electron-100mev-full-al250)
    macro=qp_example_100mev_electron_full_al250.mac
    mode=electron-100mev-full-al250
    executable=qp_example_smoke_al250
    ;;
  electron-10ev-full-al250)
    macro=qp_example_10ev_electron_full_al250.mac
    mode=electron-10ev-full-al250
    executable=qp_example_smoke_al250
    ;;
  electron-1ev-full-al250)
    macro=qp_example_1ev_electron_full_al250.mac
    mode=electron-1ev-full-al250
    executable=qp_example_smoke_al250
    ;;
  electron-1ev-full-al250-x3y3)
    macro=qp_example_1ev_electron_full_al250_x3y3.mac
    mode=electron-1ev-full-al250-x3y3
    executable=qp_example_smoke_al250
    ;;
  electron-10ev-full-al250-x3y3)
    macro=qp_example_10ev_electron_full_al250_x3y3.mac
    mode=electron-10ev-full-al250-x3y3
    executable=qp_example_smoke_al250
    ;;
  *) echo "usage: $0 [geometry|phonon|electron|electron-full|electron-10mev-full-al250|electron-10mev-full-al250-capped|electron-10mev-full-al250-cap1m|phonon-1mev-full-al250|geometry-dump-al250|electron-100mev-full-al250|electron-10ev-full-al250|electron-1ev-full-al250|electron-1ev-full-al250-x3y3|electron-10ev-full-al250-x3y3]" >&2; exit 2 ;;
esac
deps_root=${QARRAY_DEPS_ROOT:-"$HOME/qarray-deps"}
build_dir=${QARRAY_QP_BUILD_DIR:-"$repo_root/build-qp-example-stock"}
export G4LATTICEDATA=${G4LATTICEDATA:-"$deps_root/g4cmp-11.4.1-install/share/G4CMP/CrystalMaps"}
executable=${executable:-qp_example_smoke}
if [[ ! -x "$build_dir/$executable" || ! -d "$G4LATTICEDATA" ]]; then
  echo "Build qp_example_smoke and set G4LATTICEDATA; see README.md." >&2
  exit 2
fi
out_dir="$repo_root/output/qp-example/$mode"
mkdir -p "$out_dir"
cd "$out_dir"
# Clear the prior summary so a failed run cannot leave a stale success report.
: > summary.json
if "$build_dir/$executable" "$repo_root/macros/$macro" > run.log 2>&1; then
  cat summary.json
else
  run_exit=$?
  tail -n 40 run.log >&2
  exit "$run_exit"
fi
