#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

if [[ -z "$1" || -z "$2" ]]; then
    echo "Usage: ./scripts/Start_Point.sh <test_hist | ISO_spectrum | mono> <single | double | multi>"
    exit 1
fi

DIST_TYPE=$1
SOURCE_MODE=$2

case "$SOURCE_MODE" in
    single)
        MACRO="macros/gps_single_neutron_test.mac"
        ;;
    double)
        MACRO="macros/gps_double_neutron_gamma_test.mac"
        ;;
    multi)
        MACRO="macros/gps_multi_demo_test.mac"
        ;;
    *)
        echo "Invalid mode selection: '$SOURCE_MODE'"
        echo "Usage: ./scripts/Start_Point.sh <test_hist | ISO_spectrum | mono> <single | double | multi>"
        exit 1
        ;;
esac

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
