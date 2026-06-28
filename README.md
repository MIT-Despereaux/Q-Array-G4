# Qurad-G4-sim

This repository is intended as detailed GEANT4 simulations for experiments related to investigating radiation's impact on qubits.

# Installing GEANT4

- TBD.

# Initializing dependencies

Initialize pinned Eigen and MCMC dependencies after cloning:

```sh
git submodule update --init --recursive
```

# Linking Examples Folder

```sh
ln -s $G4INSTALL/share/Geant4/examples examples
```

# Conventions

All scripts for running the program and for submitting slurm jobs should go into the `scripts` folder,
and they should be assumed to be run under the repo root directory.

All macros should go under the `macros` folder, including source spectrum used for the general particle source (GPS)
generator. These files will be copied into the `build` (`build-dspx`) folder during cmake configuration and build phase. 

For visualization tests, run `./main` (which loads the `init_vis.mac` automatically) and then `/control/execute <macro_name>.mac` under the `build` folder.

For batch tests, run `./main <macro_name>.mac` under the `build` folder to execute the script. 

# Building

Configure and build with the DSPX detector geometry selected and CRY disabled
by default:

```sh
cmake -S . -B build
cmake --build build
```

To build with the Leiden II detector geometry instead:

```sh
cmake -S . -B build-leiden -DQARRAY_DETECTOR_GEOMETRY=LEIDEN_II
cmake --build build-leiden
```

To include the CRY cosmic ray generator, opt in at configure time:

```sh
cmake -S . -B build-cry -DWITH_CRY=ON
cmake --build build-cry
```

CMake generates the MCMC header-only include directory from the submodule when
needed. CRY is downloaded and built under `ext/` only when `WITH_CRY=ON`.

# Running

Run the interactive visualization session:

```sh
cd build && ./main
```

Note, you might need to use `export QT_QPA_PLATFORM=xcb` to force an X11 session if you're using Wayland.

Run a batch macro:

```sh
cd build && ./main macros/dspx_cosmic_batch.mac
```

GPS test macros are self-contained and live in `macros/`. Run them directly:

```sh
mkdir -p output/Data/_gps_sources
cd build-dspx && ./main macros/gps_single_neutron_test.mac
cd build-dspx && ./main macros/gps_double_neutron_gamma_test.mac
cd build-dspx && ./main macros/gps_multi_demo_test.mac
```

Visualization equivalents use the same GPS source definitions and color
trajectories by particle type:

```sh
cd build-dspx && ./main macros/gps_single_neutron_visual.mac
cd build-dspx && ./main macros/gps_double_neutron_gamma_visual.mac
cd build-dspx && ./main macros/gps_multi_demo_visual.mac
```

For running the same GPS test macros and sorting the generated `test*.csv`/`test*.json` outputs, use `scripts/run_dspx_start_point.sh`. The script configures and builds `build-dspx` with DSPX geometry, runs from that build directory so copied macros resolve correctly, and stages raw outputs under `output/Data/_gps_sources` before sorting them into a numbered run folder. `test_hist`, `ISO_spectrum`, and `mono` only label the sorted output folder.

```sh
./scripts/run_dspx_start_point.sh <test_hist | ISO_spectrum | mono> <single | double | multi>
```

# Testing

Build and run the smoke tests:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

The default DSPX test suite runs the DSPX-specific scoring checks, a
particle-gun event, and verifies that selecting CRY fails clearly when
`WITH_CRY=OFF`. Leiden II builds use the Leiden cosmic and energy-deposition
checks.
