# Changelog
All notable changes to this project will be documented in this file.
This file is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [QArray-G4_v0.2.0] TBD
### Added
- Pins Eigen and MCMC as git submodules under `submodules/`.
- Adds CTest smoke macros for particle-gun runs and CRY-disabled validation.
- Adds a `QARRAY_DETECTOR_GEOMETRY` CMake selector with `LEIDEN_II` as the
  current supported detector geometry.
- Documents the cached simulation geometry structure and DSPX cryostat merge
  plan under `.agents/`.

### Changed
- Makes CRY support optional via `-DWITH_CRY=ON`; default builds use particle gun/GPS without downloading CRY.
- Lets CMake generate the MCMC header-only include directory when needed.
- Improves compatibility with Geant4 10.7 headers used by the local smoke-test environment.
- Renames the internal C++ namespace from `QR` to `QArray` while retaining the
  `/QR/...` Geant4 UI command prefix for compatibility.

## [Qurad-G4-Simulation_v0.1.3] 2023-12-13
### Added
- Adds `mcmc` and `eigen` dependencies as git submodules in the `submodules/` folder for sampling from multi-dimension distributions.
- Muon sampling now uses an MCMC multi-dimensional $\theta$, $E$ sampler.

### Changed
- Adds `CRY` dependency automatically via cmake in the `ext/` folder.
- G4 now by default is assumed to be built with static library.

### Fixed
- Not really a bug, but particles will no longer spawn outside of the world volume. 
- Should be good for production runs on the cloud. 

## [Qurad-G4-Simulation_v0.1.2] 2023-06-29
### Added
- Adds primary particle vertex information in the output.
- Adds Al foil wrap (1 mm) on the outside of detectors.

## [Qurad-G4-Simulation_v0.1.1] 2023-06-15
### Added
- Adds a python sync script for data synchronization.
- Adds detector scintillator type III to geometry.

### Changed
- Modifies geometry in `DetectorConstruction`.

## [Qurad-G4-Simulation_v0.1.0] 2023-05-29
### Added
- Initial release, following the B1/B2 examples for G4 simulation on a detector.
