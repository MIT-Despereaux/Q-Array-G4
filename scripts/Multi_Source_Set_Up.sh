#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

DIST_TYPE=${1:-test_hist}
MACRO="macros/gps_multi_demo_test.mac"

echo "=========================================="
echo "Running Geant4 macro: $MACRO"
echo "=========================================="

./build/main "$MACRO"

echo "=========================================="
echo "Simulation finished. Starting file sorter."
echo "=========================================="

python3 scripts/sort_sim_data.py "$DIST_TYPE" "$MACRO"

echo "=========================================="
echo "Pipeline complete."
echo "=========================================="
