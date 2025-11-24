# Qurad-G4-sim
This repository is intended as detailed GEANT4 simulations for experiments related to investigating radiation's impact on qubits. 

# Installing GEANT4

- TBD.

# Installing CRY

- `CRY` is configured to be donwloaded in CMake. 

# Linking Examples Folder
```sh
ln -s $G4INSTALL/share/Geant4/examples examples
```

# Dependencies
Need to run the following to create the header-only library for libcmcmc:
```sh
cd submodules/mcmc && ./configure --header-only-version
```
