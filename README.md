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

The cylindrical source location is hardcoded in `DetectorConstruction_DSPX` and is there purely for visual effect.

I've implemented a new bool macro to toggle between Sn cube and Qubit array without needing to recompile. Default is Sn Cube, they spawn in the same location use `/QR/geom/useQubitArray true` to swap to Qubit array. Both Sn Cube and Qubit Array are registered as lattice (Currently Sn uses Al lattice stats as tin isn't finished being implemented) and registered as senstivie detectors.

Additionally a protoype of GDS to GEANT4 has been implemented so far using `gds_flatten.py` along with the source files to parse the JSON into extruded solids. From there the solids are unioned together via a binary tree. This is currently in development as there is a geometry normal vector error that kills `BogoliobovQP` along with the stack. MultiUnion hasn't worked to solve this, a potential avenue for inquiry is using an STL but it might slow compuation down significantly. Extrusion can't extrude whole

`gds_flatten.py` must be used to create the JSON before any simulations of Qubit array can be done.

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

# Analysis

We've implemented several analysis scripts that can be used for quasiparticles, below is detailed the script, its input type, and function.

`qpDensityPerTime.py` - .log file tracking verbosity 2, along with hardcoded position of JJ centers on chip. This will give QP density over time histogram for each JJ specified.

`AmBe_Exp_Sim_Grapher.py` - .csv file of SpectrumEnergySummary.csv. I implemented a change to the hit recording to count how much energy each event deposits and saves it here with form: particle type, energy, total energy deposited by this event. Use this to recreate graph comparing experimental and

`qpDistributionBirthDeath.py` - .csv file, This will take all birth and death locations of QP along with their energy to create 4 subplots: birth position of QPs (color is power scale of energy), death positions, Energy distribution of QPs at birth and death.

Less useful we have the following:

`qpHeatmap.py` - .log file, useful it troubleshooting qp diffusion. If using `Immortal_QP.mac` where the quasiparticle can't lose energy we can see how that particle will trace out and diffuse within the space. potential avenue for use in showing ergodicity of stadium geometry (?)

`ISO_neutron_spectrum_plotter.py` - .csv file, this is just used for validity, showing ISO standard for neutrons from 241-AmBe versus our simulated version.
