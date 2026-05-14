# Repository Guidelines

## Project Structure & Module Organization

This is a Geant4 C++17 simulation project. The executable entry point is
`main.cc`, with implementation files in `src/` and public project headers in
`include/`. Geant4 macro files live at the repository root, including
`init_vis.mac`, `vis_1.mac`, `vis_2.mac`, and `run_test.mac`; CMake copies the
main runtime macros into the build directory. Utility and batch scripts are in
`scripts/`. Third-party headers are expected under `submodules/`, including
Eigen and the generated header-only MCMC library.

## Build, Test, and Development Commands

Configure from a separate build directory:

```sh
cmake -S . -B build
```

Build the simulation executable:

```sh
cmake --build build
```

Run the default interactive visualization session from the build directory:

```sh
cd build && ./main
```

Run a batch macro:

```sh
cd build && ./main run_test.mac
```

Before configuring a fresh checkout, generate the MCMC header-only dependency:

```sh
cd submodules/mcmc && ./configure --header-only-version
```

## Coding Style & Naming Conventions

Use C++17 and keep project code inside the `QR` namespace. Follow the existing
style: `.cc` source files in `src/`, `.hh` headers in `include/`, Geant4-style
class names such as `DetectorConstruction` and `PrimaryGeneratorAction`, and
member names consistent with nearby code. Prefer clear Geant4 units and types
(`G4String`, `G4bool`, `G4int`) when interacting with Geant4 APIs. Keep comments
short and useful, especially around simulation assumptions, detector geometry,
and output metadata.

## Testing Guidelines

There is no dedicated unit test framework in this repository. Treat a clean
CMake configure/build plus a short macro run as the required smoke test before
submitting changes:

```sh
cmake -S . -B build
cmake --build build
cd build && ./main run_test.mac
```

For changes to geometry, physics lists, sensitive detectors, or output formats,
include the macro used for validation and summarize any generated CSV/JSON
outputs you inspected.

## Commit & Pull Request Guidelines

The current history uses short, imperative or descriptive commit subjects, for
example `Adds framework files` and `Initial commit`. Keep subjects concise and
specific to the changed behavior or module. Pull requests should include a brief
description, the commands/macros used to validate the change, any dependency or
environment assumptions, and screenshots only when visualization behavior
changes.

## Changelog & Versioning

For each version bump, always document the changes in the `CHANGELOG.md` file. Keep the entries clear and organized by version number to maintain an accurate history of the project's evolution.

## Security & Configuration Tips

Do not commit generated build products, simulation outputs, core dumps, or local
machine paths. Batch scripts may contain site-specific scheduler paths; keep
those configurable when adding new scripts. Special folders like `.cache`, `cache`, `data` (symlink or folder), and `output` (symlink or folder) are strictly ignored in `.gitignore`.
