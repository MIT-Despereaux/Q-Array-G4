# Implementation Plan: Rename `QR` Namespace to `QArray` — COMPLETED

## Summary

Renamed the C++ namespace from `QR` to `QArray` across all headers and
implementation files while keeping the Geant4 UI command prefix `/QR/...`
stable for compatibility with existing macros and user scripts.

## What Happened

### Decisions
- C++ namespace: `namespace QArray` (was `namespace QR`)
- Geant4 UI commands: retained `/QR/...` (stable prefix, intentionally not migrated)
- Architecture: mechanical rename only; no file renames and no command path changes

### Files Touched
- `main.cc`: `using namespace QArray;`
- `include/*.hh`: headers updated
- `src/*.cc`: implementations updated
- `AGENTS.md` and `CHANGELOG.md`: updated guidance and changelog entry

### Verification
```sh
rg -n 'namespace QR|QR::|using namespace QR' main.cc include src tests
# (returns no matches)

rg -n 'namespace QArray|QArray::' main.cc include src
# (shows expected declarations)

cmake -S . -B build && cmake --build build && cd build && ./main run_test.mac
# (passes)
```

## Future Questions (deferred)

- Should the user-facing Geant4 command prefix `/QR/...` eventually become `/QArray/...`?
- Is a backwards-compatible alias `namespace QR = QArray;` desired temporarily?
- Should CMake target names, executable names, or macro filenames be rebranded later?
