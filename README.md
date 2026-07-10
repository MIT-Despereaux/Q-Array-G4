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

A generalized particle source exists. `dspx_AmBe_visual.sh` and `dspx_AmBe_batch.sh`create a neutron and gamma source for 241-AmBe. However this script is highly generalizable to other arbitrary combination of particle types with different energy spectrum. Altering `./scripts/Multi_Source_Spectrums.csv` allows new combinations of general particle sources to be specified. A CSV file must be used for the spectrums.

These spectrum can be tested by uncommenting the portion between `# <<< DEBUGGING SPECTRUM: START` and `# >>> DEBUGGING SPECTRUM: END` while commenting out `./main`. This saves the initialized energies in `./output/initial_data`.

Feeding the path into the python files allow for graphing, gamma rays are naturally created by Geant4 so using the noisy grapher works better for retrieving the actual one. Run once with particle of choice replaced with geantino and another time with it remaining. The program takes the two and subtract them to create a new histogram. I plan to implement an automated version of this.

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

Further testing:

To validate that the source you are creating is accurate do the following from repo root:

1. `./scripts/multi_source_setup.sh batch 10000 ./scripts/Multi_Source_Spectrums.csv`
   (you can swap run number and path to your version of `./scripts/Multi_Source_Spectrums.csv` above)

2. `cd build-dspx`
   Now within run your test source in batch mode and put output into log file:

3. `./main </path/to/temporary/file/here> > </path/to/log/file/here.log>`
   Extract the initial data from your path

4. `cd ..` and then `./scripts/initial_energy_from_log.sh </path/to/log/file/here.log>`
   Finally we can plot them, edit the following file to include your `.csv` used in your spectrum and your `.csv` just created in 4:

5. `python ./notebook/initial_energy_comparison_generator.py`
