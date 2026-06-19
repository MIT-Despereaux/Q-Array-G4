# Repository Guidelines

## Project Structure & Module Organization

This is a Geant4 C++17 simulation. `main.cc` is the executable entry point,
with implementation files in `src/` and public headers in `include/`. Runtime
macros such as `init_vis.mac`, `leiden_cosmic_batch.mac`,
`leiden_cosmic_visual.mac`, `dspx_cosmic_batch.mac`, and
`dspx_cosmic_visual.mac` live at the repo root and are copied into the build
directory. Utility and batch scripts are in `scripts/`; pinned Eigen and MCMC
submodules are under `submodules/`.

## Build, Test, and Development Commands

Geant4 is already available in the agent terminal environment; do not source a
local `geant4.sh` file before configuring or building.

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
cd build && ./main leiden_cosmic_batch.mac
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

Use C++17 and keep project code inside the `QArray` namespace. The Geant4 UI
command prefix remains `/QR/...` for compatibility with existing macros and
user scripts. Follow the existing style: `.cc` sources in `src/`, `.hh` headers
in `include/`, Geant4-style class names such as `DetectorConstruction`, and
member names consistent with nearby code. Prefer Geant4 units and types
(`G4String`, `G4bool`, `G4int`) when using Geant4 APIs. Keep comments short and
focused on simulation assumptions, geometry, or output metadata.

## Testing Guidelines

There is no dedicated unit test framework. Treat a clean CMake configure/build
plus a short macro run as the required smoke test before submitting changes:

```sh
cmake -S . -B build
cmake --build build
cd build && ./main leiden_cosmic_batch.mac
```

For changes to geometry, physics lists, sensitive detectors, or output formats,
include the macro used for validation and summarize any generated CSV/JSON
outputs you inspected.

## Python Analysis & Marimo

The root `.venv` is the repository's dedicated Python analysis environment and
is managed with uv. Keep this environment local and do not commit it. Create
and synchronize it from the repository root:

```sh
uv venv .venv
uv sync --all-groups
source .venv/bin/activate
```

References to the activated `pycrp` environment in older marimo workflow
notes mean this root analysis environment for this repository.

Pass `--no-sandbox` to marimo subcommands that support it (e.g. `marimo export`). The notebook
dependencies are already installed in `.venv`, and notebooks with inline
dependencies may otherwise prompt interactively to create another sandboxed
virtual environment. That prompt can appear to hang during unattended runs.
For example, force an HTML export without sandboxing with:

```sh
MPLCONFIGDIR=/tmp/matplotlib-codex marimo export html <notebook.py> --no-sandbox -o <output.html> -f
```

Verified with marimo 0.23.9: `marimo check` does not expose
`--no-sandbox` or the short `-f` flag. Its automatic rewrite option is the
long `--fix` flag. Disable marimo's network update check during unattended
validation:

```sh
MARIMO_SKIP_UPDATE_CHECK=1 marimo check <notebook.py>
```

The equivalent command required by the `marimo-notebook` skill is:

```sh
MARIMO_SKIP_UPDATE_CHECK=1 uvx marimo check <notebook.py>
```

Using `uvx` creates or reuses a separate uv tool environment and may require
network access even when the project `.venv` is synchronized. Prefer the
activated environment's `marimo` executable for repeat checks. Use
`--fix` only when file rewrites are intended:

```sh
MARIMO_SKIP_UPDATE_CHECK=1 marimo check <notebook.py> --fix
```

In restricted agent command sandboxes, marimo 0.23.9 may hang in
`marimo check` because its checker uses `asyncio.to_thread` for filesystem
operations. Export may fail because marimo opens a local kernel socket. If
either operation stalls or raises `PermissionError: [Errno 1] Operation not
permitted`, rerun that exact command with the sandbox restriction lifted; do
not interpret it as a notebook failure.

If uv cannot write to its default user cache or tool directory in a restricted
environment, redirect only those transient locations:

```sh
UV_CACHE_DIR=/tmp/uv-cache UV_TOOL_DIR=/tmp/uv-tools uvx marimo check <notebook.py>
```

The checked HTML export form supports both `--no-sandbox` and `-f`.
`MPLCONFIGDIR=/tmp/matplotlib-codex` avoids matplotlib cache writes to a
restricted home directory.

Temporary marimo workflow note: prefer the `marimo-notebook` skill for
inspecting, editing, checking, and testing marimo notebook files and their
outputs. Do not use `marimo-pair` unless the user explicitly says they are
working in a browser-based marimo session; VS Code marimo notebooks use a
separate kernel and document lifecycle. Browser or Chrome DevTools testing is
optional and should only be used when browser-level interaction or visual
verification is necessary.

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
