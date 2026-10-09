#!/usr/bin/env bash
# Run one uncapped qp_example macro for several seeds in parallel.
# usage: run_qp_example_seeds.sh MODE N_SEEDS [FIRST_SEED] [MAX_PARALLEL]
# Results: output/qp-example/MODE-seeds/seed-<S>/{run.log,summary.json,*.csv.gz}
# and output/qp-example/MODE-seeds/index.csv with one row per seed.
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
deps_root=${QARRAY_DEPS_ROOT:-"$HOME/qarray-deps"}
build_dir=${QARRAY_QP_BUILD_DIR:-"$repo_root/build-qp-example-stock"}
export G4LATTICEDATA=${G4LATTICEDATA:-"$deps_root/g4cmp-11.4.1-install/share/G4CMP/CrystalMaps"}
executable=qp_example_smoke_al250

if [[ "${1:-}" == "--one" ]]; then  # internal: run a single seed
  macro=$2; seed_dir=$3; seed=$4
  mkdir -p "$seed_dir"; cd "$seed_dir"
  : > summary.json
  "$build_dir/$executable" "$repo_root/macros/$macro" "$seed" 4824 > run.log 2>&1 || true
  exit 0
fi

mode=${1:-}; n=${2:-}; first=${3:-1}; jobs=${4:-4}
case "$mode" in
  electron-10mev-full-al250) macro=qp_example_10mev_electron_full_al250.mac ;;
  phonon-1mev-full-al250) macro=qp_example_1mev_phonon_full_al250.mac ;;
  electron-1ev-full-al250) macro=qp_example_1ev_electron_full_al250.mac ;;
  electron-1ev-full-al250-x3y3) macro=qp_example_1ev_electron_full_al250_x3y3.mac ;;
  electron-10ev-full-al250-x3y3) macro=qp_example_10ev_electron_full_al250_x3y3.mac ;;
  electron-10ev-full-al250) macro=qp_example_10ev_electron_full_al250.mac ;;
  electron-100mev-full-al250) macro=qp_example_100mev_electron_full_al250.mac ;;
  *) echo "usage: $0 {electron-10mev-full-al250|electron-100mev-full-al250|electron-10ev-full-al250|electron-1ev-full-al250|electron-1ev-full-al250-x3y3|electron-10ev-full-al250-x3y3|phonon-1mev-full-al250} N_SEEDS [FIRST_SEED] [MAX_PARALLEL]" >&2; exit 2 ;;
esac
[[ "$n" =~ ^[0-9]+$ && "$n" -ge 1 ]] || { echo "N_SEEDS must be a positive integer" >&2; exit 2; }
if [[ ! -x "$build_dir/$executable" || ! -d "$G4LATTICEDATA" ]]; then
  echo "Build $executable and set G4LATTICEDATA; see README.md." >&2
  exit 2
fi
out_root="$repo_root/output/qp-example/$mode-seeds"
mkdir -p "$out_root"

seq "$first" $((first+n-1)) | xargs -P "$jobs" -I{} \
  "$0" --one "$macro" "$out_root/seed-{}" {}

python3 - "$out_root" <<'PY'
import csv, glob, json, os, sys
root = sys.argv[1]
rows = []
for path in sorted(glob.glob(os.path.join(root, "seed-*")), key=lambda p: int(p.rsplit("-", 1)[1])):
    seed = path.rsplit("-", 1)[1]
    try:
        s = json.load(open(os.path.join(path, "summary.json")))
    except Exception:
        rows.append({"seed": seed, "status": "failed_or_running"}); continue
    rows.append({"seed": seed, "status": "passed" if s["passed"] else "failed",
                 "qp_tracks": s["qp_tracks"], "phonon_tracks": s["phonon_tracks"],
                 "qp_sample_rows": s["qp_sample_rows"], "elapsed_seconds": round(s["elapsed_seconds"], 1),
                 "dir": os.path.relpath(path, root)})
cols = ["seed", "status", "qp_tracks", "phonon_tracks", "qp_sample_rows", "elapsed_seconds", "dir"]
with open(os.path.join(root, "index.csv"), "w", newline="") as f:
    w = csv.DictWriter(f, cols); w.writeheader(); w.writerows(rows)
print(open(os.path.join(root, "index.csv")).read())
PY
