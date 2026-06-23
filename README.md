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

# Building

Configure and build with CRY disabled by default:

```sh
cmake -S . -B build
cmake --build build
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
cd build && ./main leiden_cosmic_batch.mac
```

For running single/multiple particle sources instead return to root folder and run. test_hist, ISO_spectrum, and mono are solely used to label your output folders for ease of working with. single creates one particle source, dual two, and multi N particle sources, however it requires a CSV to be fed in. The CSV format must have type,number,intensity,spectrum as its header column. number must be an integer, spectrum must be relative path to spectrum.mac files to be read for that source.

```sh
cd .. && ./Start_Point.sh <test_hist | ISO_spectrum | mono> <single | dual | multi> <int: number of events> "Path_to_CSV"
```

# Testing

Build and run the smoke tests:

```sh
cmake --build build
ctest --test-dir build --output-on-failure
```

The default Leiden II test suite runs a short cosmic batch macro, a
particle-gun event, and verifies that selecting CRY fails clearly when
`WITH_CRY=OFF`. DSPX builds use the DSPX-specific scoring checks.
