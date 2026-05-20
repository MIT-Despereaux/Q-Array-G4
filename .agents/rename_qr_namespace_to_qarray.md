# Implementation Plan: Rename `QR` Namespace to `QArray`

## Overview

Rename the project C++ namespace from `QR` to `QArray` across headers,
implementation files, and entry-point usage while keeping the Geant4 simulation
behavior unchanged. Treat Geant4 UI command paths such as `/QR/output/...` as a
separate user-facing command namespace decision, not as part of the mechanical
C++ namespace rename unless explicitly approved.

## Implementation Status

Implemented. The internal C++ namespace is now `QArray`, and the Geant4 UI
command prefix remains `/QR/...` for compatibility with existing macros and
user scripts.

## Pre-Implementation Scope

Before implementation, the C++ namespace appeared in:

- `main.cc`: `using namespace QR;`
- `include/*.hh`: class and helper declarations under `namespace QR`
- `src/*.cc`: implementation blocks under `namespace QR`
- `src/DataWriter.cc`: qualified reference `QR::ListReducers`

The string command surface also contains many `/QR/...` paths in source,
macros, and documentation. These are Geant4 UI command names and metadata keys,
not C++ namespaces.

## Architecture Decisions

- Rename only the C++ namespace in the first implementation pass:
  `namespace QR` -> `namespace QArray`, `QR::` -> `QArray::`, and
  `using namespace QR` -> `using namespace QArray`.
- Do not rename `/QR/...` Geant4 UI commands in the same pass unless a
  backwards-compatibility policy is chosen first.
- Keep file names, class names, target names, executable name, and macro names
  unchanged unless a later plan broadens the rename.
- Update repository guidance so future code is placed in `QArray`, not `QR`.

## Task List

### Task 1: Baseline Inventory

**Description:** Capture every C++ namespace reference and every user-facing
`/QR/...` command path before editing so the rename does not accidentally mix
internal and external interfaces.

**Acceptance criteria:**

- [x] `rg -n '\bQR\b|namespace QR|QR::|using namespace QR' main.cc include src tests CMakeLists.txt README.md AGENTS.md CHANGELOG.md` has been reviewed.
- [x] `/QR/...` command paths are listed separately from C++ namespace symbols.
- [x] Any generated or archived paths, including `cache/` and zip contents, are
      excluded from implementation edits.

**Verification:**

- [x] Inventory command output is saved in the implementation notes or final
      change summary.

**Dependencies:** None

**Files likely touched:** None

**Estimated scope:** Small

### Task 2: Rename C++ Namespace Declarations and References

**Description:** Update project source so all current project classes and
helpers live under `QArray` instead of `QR`.

**Acceptance criteria:**

- [x] All `namespace QR` declarations in `include/` and `src/` become
      `namespace QArray`.
- [x] All qualified `QR::` references become `QArray::`.
- [x] `main.cc` uses `QArray` instead of `QR`.
- [x] No C++ namespace references to `QR` remain outside compatibility shims,
      if any are deliberately added.

**Verification:**

- [x] `rg -n 'namespace QR|QR::|using namespace QR' main.cc include src tests`
      returns no matches.
- [x] `rg -n 'namespace QArray|QArray::|using namespace QArray' main.cc include src`
      shows the expected declarations and references.

**Dependencies:** Task 1

**Files likely touched:**

- `main.cc`
- `include/*.hh`
- `src/*.cc`

**Estimated scope:** Medium

### Task 3: Update Repository Guidance and Changelog

**Description:** Align contributor-facing documentation with the new namespace
so future code does not reintroduce `QR`.

**Acceptance criteria:**

- [x] `AGENTS.md` coding style section says project code belongs in
      `QArray`.
- [x] `CHANGELOG.md` records the namespace rename under the appropriate
      version or an Unreleased section.
- [x] Any README text that describes the C++ namespace is updated if present.

**Verification:**

- [x] `rg -n 'namespace|QR|QArray' AGENTS.md README.md CHANGELOG.md` has been
      checked for stale namespace guidance.

**Dependencies:** Task 2

**Files likely touched:**

- `AGENTS.md`
- `CHANGELOG.md`
- `README.md`, only if it contains namespace guidance

**Estimated scope:** Small

### Task 4: Build and Smoke Test

**Description:** Configure and build the project with the existing agent shell
environment, then run the standard smoke macro.

**Acceptance criteria:**

- [x] No local `geant4.sh` file is sourced before configuring or building.
- [x] CMake configure completes.
- [x] Build completes without namespace-related compiler errors.
- [x] `run_test.mac` runs successfully.

**Verification:**

```sh
cmake -S . -B build
cmake --build build
cd build && ./main run_test.mac
```

**Dependencies:** Tasks 2 and 3

**Files likely touched:** None

**Estimated scope:** Small

### Task 5: Decide Geant4 Command Path Compatibility

**Description:** Decide whether runtime UI and metadata paths should remain
rooted at `/QR/...` for compatibility or migrate to `/QArray/...`.

**Acceptance criteria:**

- [x] A compatibility decision is recorded before changing command paths.
- [x] If paths remain `/QR/...`, documentation states that the runtime command
      prefix is intentionally stable despite the C++ namespace rename.
- [x] `/QArray/...` command paths were not introduced; migration planning is
      deferred until a future command-prefix rename is requested.

**Verification:**

- [x] Existing macros continue to run if `/QR/...` is retained.
- [x] `/QArray/...` was not introduced, so no new command path testing is
      required.

**Dependencies:** Task 4

**Files likely touched if command paths are migrated:**

- `src/*.cc`
- `*.mac`
- `AGENTS.md`
- `README.md`
- `CHANGELOG.md`

**Estimated scope:** Medium if retaining `/QR/...`; Large if migrating command
paths with compatibility aliases.

## Checkpoints

### Checkpoint: After Tasks 1-2

- [x] C++ namespace references are consistently `QArray`.
- [x] No `/QR/...` command strings were changed accidentally.
- [x] The source still configures far enough for CMake to generate build files.

### Checkpoint: After Tasks 3-4

- [x] Documentation and changelog match the implementation.
- [x] Build succeeds.
- [x] `run_test.mac` succeeds.

### Checkpoint: Before Task 5 Implementation

- [x] `/QR/...` command paths are retained for this implementation.

## Risks and Mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| Confusing C++ namespace with Geant4 UI paths | High | Keep `/QR/...` string commands out of the mechanical namespace rename. |
| Hidden `QR::` references in tests or scripts | Medium | Search `tests/`, `scripts/`, and docs before and after edits. |
| External macros rely on `/QR/...` commands | High | Retain paths by default, or add compatibility aliases before changing them. |
| Generated/cache files are edited accidentally | Medium | Limit edits to repo source, docs, and root macros; exclude `cache/` and archives. |
| Changelog version placement is unclear | Low | Use an `Unreleased` section if no active version is obvious. |

## Future Questions

- Should the user-facing Geant4 command prefix `/QR/...` remain stable, or
  should it eventually become `/QArray/...`?
- Is a backwards-compatible alias namespace, for example
  `namespace QR = QArray;`, desired temporarily for downstream code, or should
  this be a clean internal rename?
- Should CMake target names, executable names, output metadata labels, or macro
  filenames be considered for a later branding rename?
