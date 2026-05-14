# Repository Guidelines

## Project Structure & Module Organization

This is a Geant4 C++17 simulation. `main.cc` is the executable entry point,
with implementation files in `src/` and public headers in `include/`. Runtime
macros such as `init_vis.mac`, `vis_1.mac`, `vis_2.mac`, and `run_test.mac`
live at the repo root and are copied into the build directory. Utility and
batch scripts are in `scripts/`; pinned Eigen and MCMC submodules are under
`submodules/`.

## Build, Test, and Development Commands

To ensure Geant4 is available in the agent terminal environment, run the following to source G4 v10.7.4 before configuring or building:

```sh
### GEANT4 v10.7.4
if [ -f "$HOME/.local/GEANT4-10.7.4/bin/geant4.sh" ]; then
    source "$HOME/.local/GEANT4-10.7.4/bin/geant4.sh"
fi
```

Configure from a separate build directory:

```sh
git submodule update --init --recursive
cmake -S . -B build
```

Build the simulation executable:

```sh
cmake --build build
```

Run the interactive visualization session:

```sh
cd build && ./main
```

Run a batch macro:

```sh
cd build && ./main run_test.mac
```

Run smoke tests after building:

```sh
ctest --test-dir build --output-on-failure
```

CRY support is disabled by default. Opt in when configuring if you need
`/QR/generator/mode cry`:

```sh
cmake -S . -B build-cry -DWITH_CRY=ON
```

## Coding Style & Naming Conventions

Use C++17 and keep project code inside the `QR` namespace. Follow the existing
style: `.cc` sources in `src/`, `.hh` headers in `include/`, Geant4-style class
names such as `DetectorConstruction`, and member names consistent with nearby
code. Prefer Geant4 units and types (`G4String`, `G4bool`, `G4int`) when using
Geant4 APIs. Keep comments short and focused on simulation assumptions,
geometry, or output metadata.

## Testing Guidelines

There is no dedicated unit test framework. Treat a clean CMake configure/build
plus a short macro run as the required smoke test before submitting changes:

```sh
cmake -S . -B build
cmake --build build
cd build && ./main run_test.mac
```

For changes to geometry, physics lists, sensitive detectors, or output formats,
include the macro used for validation and summarize any generated CSV/JSON
outputs you inspected.

## Commit & Pull Request Guidelines

History uses short, imperative or descriptive commit subjects, for example
`Adds framework files` and `Initial commit`. Keep subjects concise and specific.
Pull requests should include a brief description, validation commands/macros,
dependency or environment assumptions, and screenshots only for visualization
changes.

## Changelog & Versioning

For each version bump, document changes in `CHANGELOG.md`. Keep entries clear
and organized by version number.

## Security & Configuration Tips

Do not commit generated build products, simulation outputs, core dumps, or local
machine paths. Batch scripts may contain site-specific scheduler paths; keep
those configurable. `.cache`, `cache`, `data`, and `output` are ignored.
