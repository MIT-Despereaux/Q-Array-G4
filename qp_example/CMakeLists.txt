cmake_minimum_required(VERSION 3.16)
project(QArrayQPExampleSmoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Geant4 REQUIRED)
find_package(G4CMP REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ZLIB REQUIRED)
include(${Geant4_USE_FILE})
set(G4CMP_SOURCE_DIR "" CACHE PATH "Local checkout of the pinned G4CMP source")
set(QP_EXAMPLE "${G4CMP_SOURCE_DIR}/examples/quasiparticle")
if(NOT EXISTS "${QP_EXAMPLE}/src/QuasiparticleDetectorConstruction.cc")
  message(FATAL_ERROR "Set G4CMP_SOURCE_DIR to the local G4CMP checkout")
endif()
execute_process(COMMAND git -C "${G4CMP_SOURCE_DIR}" rev-parse HEAD
  OUTPUT_VARIABLE G4CMP_SOURCE_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE G4CMP_GIT_STATUS)
if(NOT G4CMP_GIT_STATUS EQUAL 0)
  message(FATAL_ERROR "Cannot identify the local G4CMP source commit")
endif()
execute_process(COMMAND git -C "${G4CMP_SOURCE_DIR}" describe --tags --exact-match
  OUTPUT_VARIABLE G4CMP_SOURCE_TAG OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
set(QP_COMMON_SOURCES)
foreach(name ConfigManager ConfigMessenger Sensitivity QubitHousing Pad TransmissionLine ResonatorAssembly)
  list(APPEND QP_COMMON_SOURCES "${QP_EXAMPLE}/src/Quasiparticle${name}.cc")
endforeach()

# Generate a local build overlay for the requested 250 nm aluminum ground plane.
# The pinned G4CMP checkout remains unchanged and the exact expected source text
# is checked so an upstream change cannot silently produce the wrong geometry.
set(AL250_DIR "${CMAKE_CURRENT_BINARY_DIR}/al250_geometry")
file(MAKE_DIRECTORY "${AL250_DIR}")
file(READ "${QP_EXAMPLE}/include/QuasiparticleDetectorParameters.hh" AL250_PARAMETERS)
set(OLD_THICKNESS "constexpr double dp_groundPlaneDimZ = 90 * CLHEP::nm;")
set(NEW_THICKNESS "constexpr double dp_groundPlaneDimZ = 250 * CLHEP::nm;")
string(FIND "${AL250_PARAMETERS}" "${OLD_THICKNESS}" THICKNESS_POSITION)
if(THICKNESS_POSITION EQUAL -1)
  message(FATAL_ERROR "Cannot locate the stock 90 nm ground-plane thickness")
endif()
string(REPLACE "${OLD_THICKNESS}" "${NEW_THICKNESS}" AL250_PARAMETERS "${AL250_PARAMETERS}")
file(WRITE "${AL250_DIR}/QuasiparticleDetectorParameters.hh" "${AL250_PARAMETERS}")

file(READ "${QP_EXAMPLE}/src/QuasiparticleDetectorConstruction.cc" AL250_DETECTOR)
set(OLD_MATERIAL "new G4LogicalVolume(solid_groundPlane,fNiobium,\"GroundPlane_log\")")
set(NEW_MATERIAL "new G4LogicalVolume(solid_groundPlane,fAluminum,\"GroundPlane_log\")")
string(FIND "${AL250_DETECTOR}" "${OLD_MATERIAL}" MATERIAL_POSITION)
if(MATERIAL_POSITION EQUAL -1)
  message(FATAL_ERROR "Cannot locate the stock niobium ground-plane material")
endif()
string(REPLACE "${OLD_MATERIAL}" "${NEW_MATERIAL}" AL250_DETECTOR "${AL250_DETECTOR}")
file(WRITE "${AL250_DIR}/QuasiparticleDetectorConstruction.cc" "${AL250_DETECTOR}")

function(add_qp_example_runner target detector_source)
  add_executable(${target} smoke_main.cc "${detector_source}" ${QP_COMMON_SOURCES})
  target_compile_definitions(${target} PRIVATE
    QARRAY_G4CMP_COMMIT="${G4CMP_SOURCE_COMMIT}"
    QARRAY_G4CMP_TAG="${G4CMP_SOURCE_TAG}")
  target_include_directories(${target} PRIVATE ${ARGN} "${QP_EXAMPLE}/include" ${G4CMP_INCLUDE_DIRS})
  target_link_libraries(${target} PRIVATE
    ${G4CMP_LIBRARIES} ${Geant4_LIBRARIES} nlohmann_json::nlohmann_json ZLIB::ZLIB)
endfunction()

add_qp_example_runner(qp_example_smoke
  "${QP_EXAMPLE}/src/QuasiparticleDetectorConstruction.cc")
add_qp_example_runner(qp_example_smoke_al250
  "${AL250_DIR}/QuasiparticleDetectorConstruction.cc" "${AL250_DIR}")
target_compile_definitions(qp_example_smoke_al250 PRIVATE QARRAY_COMPRESS_STEPS)
