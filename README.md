# Qurad-G4-sim

This repository is intended as detailed GEANT4 simulations for experiments related to investigating radiation's impact on qubits.

# Installing GEANT4

For the laptop chip transport runs, use Geant4 11.4.1 and G4CMP
`g4cmp-V10-05-00`. The following macOS Apple Silicon setup installs both
outside this repository:

Tested build tools: CMake 4.4.4, uv 0.12.23, and nlohmann-json 3.12.0.
Pinned source commits: Geant4 `fea0b19d064da3d85e1e7529445fa5af531f2962`
and G4CMP `1bd1e8aa1b799abc99f1994f2e2b8da4c62c76c8`.

```sh
brew install cmake uv nlohmann-json
mkdir -p "$HOME/qarray-deps"
SDK_CXX_HEADERS="$(xcrun --show-sdk-path)/usr/include/c++/v1"
git clone --branch v11.4.1 --depth 1 https://github.com/Geant4/geant4.git "$HOME/qarray-deps/geant4-11.4.1-src"
git clone --branch g4cmp-V10-05-00 --depth 1 https://github.com/G4CMP/G4CMP.git "$HOME/qarray-deps/g4cmp-src"
cmake -S "$HOME/qarray-deps/geant4-11.4.1-src" -B "$HOME/qarray-deps/geant4-11.4.1-build" \
  -DCMAKE_INSTALL_PREFIX="$HOME/qarray-deps/geant4-11.4.1-install" \
  -DCMAKE_BUILD_TYPE=Release -DGEANT4_INSTALL_DATA=ON \
  -DGEANT4_INSTALL_EXAMPLES=OFF -DGEANT4_USE_QT=OFF \
  -DGEANT4_USE_OPENGL_X11=OFF -DGEANT4_BUILD_MULTITHREADED=ON \
  -DCMAKE_CXX_FLAGS="-I$SDK_CXX_HEADERS"
cmake --build "$HOME/qarray-deps/geant4-11.4.1-build" --parallel 6
cmake --install "$HOME/qarray-deps/geant4-11.4.1-build"
cmake -S "$HOME/qarray-deps/g4cmp-src" -B "$HOME/qarray-deps/g4cmp-11.4.1-build" \
  -DCMAKE_INSTALL_PREFIX="$HOME/qarray-deps/g4cmp-11.4.1-install" \
  -DCMAKE_PREFIX_PATH="$HOME/qarray-deps/geant4-11.4.1-install" \
  -DCMAKE_CXX_FLAGS="-I$SDK_CXX_HEADERS" \
  -DWITH_GEANT4_UIVIS=OFF
cmake --build "$HOME/qarray-deps/g4cmp-11.4.1-build" --parallel 6
cmake --install "$HOME/qarray-deps/g4cmp-11.4.1-build"
```

The explicit SDK include path works around this laptop's incomplete default
Command Line Tools C++ header search path. On a machine where a C++17
`#include <filesystem>` test already compiles, omit `CMAKE_CXX_FLAGS`.

The installed 11.4.1 build reuses the previously downloaded Geant4 datasets.
Its configure command used `-DGEANT4_INSTALL_DATA=OFF` and
`-DGEANT4_INSTALL_DATADIR="$HOME/qarray-deps/geant4-install/share/Geant4/data"`.
The dataset definitions are identical between these two Geant4 versions.
For a fresh installation, use `GEANT4_INSTALL_DATA=ON` as above.
Both Geant4 and G4CMP are built from unchanged source.

## Stock G4CMP quasiparticle geometry

The laptop workflow uses the unchanged detector and geometry classes from
the G4CMP `examples/quasiparticle` directory. It requires no GDS conversion.
A separate serial runner in `qp_example/` provides a GPS source, ordinary electron
physics (`QBBC_EMV`), G4CMP physics, compact scoring, and explicit failure
reporting. It compiles the geometry directly from the pinned local checkout;
no dependency source is copied, patched, or downloaded during configuration.

```sh
SDK_CXX_HEADERS="$(xcrun --show-sdk-path)/usr/include/c++/v1"
cmake -S qp_example -B build-qp-example-stock \
  -DG4CMP_SOURCE_DIR="$HOME/qarray-deps/g4cmp-src" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/qarray-deps/geant4-11.4.1-install;$HOME/qarray-deps/g4cmp-11.4.1-install;$(brew --prefix nlohmann-json)" \
  -DCMAKE_CXX_FLAGS="-I$SDK_CXX_HEADERS"
cmake --build build-qp-example-stock --parallel 6
scripts/run_qp_example_smoke.sh geometry
scripts/run_qp_example_smoke.sh phonon
scripts/run_qp_example_smoke.sh electron
```

Run these commands from the repository root. The wrapper executes existing
macros and writes `run.log`, `steps.csv`, and `summary.json` under ignored
`output/qp-example/<mode>/`. Rerunning a mode replaces its outputs.
`QARRAY_DEPS_ROOT`, `QARRAY_QP_BUILD_DIR`, and `G4LATTICEDATA` can override
local dependency, build, and crystal-map paths.

The phonon macro starts one 1 meV phonon inside the example's aluminum
`ResonatorAssembly_0` film, near its midplane, directed along the film.
The electron macro starts one 10 eV electron at `(0, 0, 5.0) mm`, the lateral
center and 100 nm below the silicon's patterned face, directed toward the film.
The stock chip is 8 mm by 8 mm with a 90 nm patterned film and retains the
example's niobium ground plane and aluminum resonators.

All three macros set `/g4cmp/recordMinETracks false` before
`/run/initialize`. The run verifies the live setting and fails if it is true.
Particle runs use fixed seeds, 10 phonon reflections, 20 QP reflections,
and a maximum of 1,000 steps per QP. The electron macro additionally uses
`/g4cmp/samplingEnergy 1 eV` to reduce secondary tracking while keeping the
initial electron energy at 10 eV. These are bounded transport checks;
`limited_qp_tracks` reports truncation. Set `/smoke/maxQPSteps 0` to remove
that step limit for a longer run, and review reflection and sampling settings
before quantitative work. Step rows retain the Geant4 track weight.

`summary.json` records the actual starting particle, energy, material and
volume; primary deposited energy; phonon and QP counts; moving QPs; limits;
runtime; dependency versions; and event completion. A particle run passes
only if its starting material matches the macro, QPs are created and move
beyond their first step, and no event is aborted. The CSV saves first steps,
second QP steps and final steps in one file. Counts are not calibrated
junction-area densities; density analysis remains a separate phase.

Measured fixed-seed runs on this laptop:

| Source | Runtime | Phonon tracks | QP tracks | QPs moving beyond step 1 | QPs reaching step limit |
| --- | --- | --- | --- | --- | --- |
| 1 meV phonon in aluminum | 10.7 s | 6 | 6 | 6 | 0 |
| 10 eV electron in silicon | 548.4 s | 42,522 | 32,620 | 31,428 | 1 |

Both events completed without aborting and verified `recordMinETracks=false`.
The electron deposited 10 eV in silicon. The phonon event created four QPs
in the aluminum resonator and two in the ground plane, with no geometry or
boundary warnings. The electron log contained nine warnings: five `Limit001`
escaped-phonon warnings, two `Boundary011` transmission warnings, and two
`QPDiffusion025` triple-junction safety warnings. Thus the stock example avoids
the GDS conversion problems but still has transport warnings to investigate
before quantitative analysis. Track counts include descendants and weights;
they are not a physical QP yield or a density estimate.

Each mode produced three files. The electron CSV contains 154,847 step rows
and occupies about 29 MiB. Its measured run used the same geometry, physics
and macro before source-provenance fields were added to the runner's summary;
subsequent runs also record the pinned G4CMP tag and commit. The GDS attempts
encountered `Boundary012` and `QPDiffusion010`; that conversion work is deferred.

### Quasiparticle density visualization

The saved electron result can be visualized without rerunning the simulation:

```sh
MPLCONFIGDIR=/tmp/matplotlib-codex .venv/bin/python \
  scripts/analyze_qp_density.py output/qp-example/electron/steps.csv
```

The analysis uses each QP's statistical track weight and produces a figure,
a machine-readable summary, and the 100 µm binned density matrix under
`output/qp-example/electron/density/`. The figure shows first-step surface
density, radial density from the electron impact point, first-step timing, and
weighted track counts by film component.

This is a creation-location proxy: the compact smoke CSV stores the position
at the end of each QP's first transport step, rather than the exact creation
vertex, and omits intermediate steps. It therefore cannot reconstruct an
instantaneous QP population density or a junction-area density over time.

### Multi-seed runs

The uncapped 250 nm aluminum macros take their seeds from `{seed1}` and
`{seed2}`, which `qp_example_smoke_al250` sets from its optional
`macro seed1 seed2` arguments (default `10854 4824`). Run several seeds in
parallel with:

```sh
scripts/run_qp_example_seeds.sh phonon-1mev-full-al250 8 1 4   # 8 seeds, first=1, 4 at once
scripts/run_qp_example_seeds.sh electron-10mev-full-al250 8
```

Each seed writes `output/qp-example/<mode>-seeds/seed-<N>/` (`run.log`,
`summary.json`, `steps.csv.gz`, `qp_samples.csv.gz`; `seed2` is fixed at 4824).
`index.csv` in the parent directory lists seed, status, track counts, sample
rows and elapsed time. A seed whose `summary.json` is empty is still running or
failed; its `run.log` ends with `SMOKE_PROGRESS` lines while it runs.

### Fully tracked QP density animation

The lower-energy full-trajectory run uses one 100 meV electron at the same
chip-center position:

```sh
scripts/run_qp_example_smoke.sh electron-full
MPLCONFIGDIR=/tmp/matplotlib-codex .venv/bin/python \
  scripts/animate_qp_density.py \
  output/qp-example/electron-100mev-full/steps.csv
```

Its macro sets `recordMinETracks=false` before initialization, records every
QP step, removes the analysis runner's artificial QP step cap, and restores
G4CMP's standard 100-reflection QP limit. The phonon reflection limit remains
10 to bound the laptop workload. The 1 eV sampling threshold is above the
entire 100 meV event, so the event is not downsampled. Each QP step row includes
pre- and post-step time, position, direction, energy, material and volume, plus
energy deposition, process, status, statistical weight, and step length.

The fixed-seed run completed in 62.4 seconds and deposited the full 100 meV in
silicon. It produced 447 phonon tracks, 320 QP tracks, and 234,030 QP step rows
in an 89 MiB CSV. All QP step sequences were contiguous, all 320 tracks reached
a terminal status, no track reached an artificial step limit, and the log had
no G4CMP or geometry warnings. The maximum simultaneous population in the
sampled animation frames was 214 QPs at about 699 ns.

The GIF uses 72 logarithmically spaced times from 25 ps to 1.43 ms. At each
frame, the analyzer linearly interpolates every active QP within its recorded
Geant4 step and bins the statistical track weights into 100 µm square cells.
The generated GIF, per-frame statistics, and JSON summary are stored under
`output/qp-example/electron-100mev-full/density-animation/`.

Existing DSPX regressions are run separately with
`ctest --test-dir build-qp-11.4.1 --output-on-failure`. Every test macro used
here sets `recordMinETracks false` before initialization. The inventory and
energy-deposition test disables condensed-matter secondary production so
that it tests deposited energy without generating a large QP shower.
All five existing smoke tests passed (37.2 seconds total).

### Geant4 11.4.2 / G4CMP compatibility blocker

With Geant4 11.4.2 and the pinned G4CMP source, the event aborts at its first
energy-deposition step with `TRACK0982: Process/model ID <40000> is invalid`.
G4CMP registers ID 40000 as a custom physics model, while Geant4 11.4.2's
`G4Track::SetAuxiliaryTrackInformation` rejects that ID. Geant4 11.4.1
works with G4CMP's existing compatibility implementation and avoids this
exception without changing either dependency's source.
The same `TRACK0982` exception was reproduced with one 4 meV phonon in the
unmodified G4CMP quasiparticle example geometry, using
`/g4cmp/recordMinETracks false`. This confirms the blocker is independent of
the GDS chip geometry.

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
