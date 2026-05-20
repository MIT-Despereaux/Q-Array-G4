# ----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.20)
project(QURAD_G4_SIM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

# Include Dlib as a dependency
# include(FetchContent)
# FetchContent_Declare(dlib
#      GIT_REPOSITORY https://github.com/davisking/dlib.git
#      GIT_TAG        v19.24
# )
# FetchContent_MakeAvailable(dlib)

# ----------------------------------------------------------------------------
# Find Geant4 package, activating all available UI and Vis drivers by default
# You can set WITH_GEANT4_UIVIS to OFF via the command line or ccmake/cmake-gui
# to build a batch mode only executable
#
option(WITH_GEANT4_UIVIS "Build with Geant4 UI and Vis drivers" ON)
option(BUILD_STATIC_LIBS "Build static libraries" OFF)
option(WITH_CRY "Build with CRY cosmic ray generator support" OFF)
set(QARRAY_DETECTOR_GEOMETRY "LEIDEN_II" CACHE STRING "Detector geometry implementation: CURIE, LEIDEN_II, or DSPX")
set_property(CACHE QARRAY_DETECTOR_GEOMETRY PROPERTY STRINGS CURIE LEIDEN_II DSPX)

# Determine required components for Geant4 based on the given options.
set(GEANT4_COMPONENTS)
if(WITH_GEANT4_UIVIS)
  list(APPEND GEANT4_COMPONENTS ui_all vis_all)
endif()

if(BUILD_STATIC_LIBS)
  list(APPEND GEANT4_COMPONENTS static)
endif()

# Now find the Geant4 package with the determined components
find_package(Geant4 REQUIRED ${GEANT4_COMPONENTS})

# ----------------------------------------------------------------------------
# Setup Geant4 include directories and compile definitions
# Setup include directory for this project
#
include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
# Optionally download and use the CRY cosmic ray generator library
#
if(WITH_CRY)
  # NOTE: CRY only supports single threaded compilation.
  include(ExternalProject)
  ExternalProject_Add(
    cry
    PREFIX ${PROJECT_SOURCE_DIR}/ext
    URL https://nuclear.llnl.gov/simulation/cry_v1.7.tar.gz
    URL_HASH MD5=85f240bebe81fe0b257e92ef1d390a83
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    UPDATE_DISCONNECTED TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "make" "-j1"
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
  )

  ExternalProject_Get_Property(cry SOURCE_DIR)
  set(CRY_BASE ${SOURCE_DIR})
  set(CRY_LIB ${CRY_BASE}/lib/libCRY.a)

  add_library(CRY INTERFACE)
  add_dependencies(CRY cry)
  target_link_libraries(CRY INTERFACE ${CRY_LIB})
  target_include_directories(CRY INTERFACE ${CRY_BASE}/src)
  target_compile_definitions(CRY INTERFACE CRYDATA="${CRY_BASE}/data")
endif()

# ----------------------------------------------------------------------------
# Include Eigen3
set(EIGEN_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/submodules/eigen)
if(NOT EXISTS ${EIGEN_INCLUDE_DIR}/Eigen)
  message(FATAL_ERROR "Eigen submodule is missing. Run: git submodule update --init --recursive")
endif()
include_directories(${EIGEN_INCLUDE_DIR})

# ----------------------------------------------------------------------------
# Include MCMC library
set(MCMC_SOURCE_DIR ${PROJECT_SOURCE_DIR}/submodules/mcmc)
set(MCMC_HEADER_ONLY_DIR ${MCMC_SOURCE_DIR}/header_only_version)
if(NOT EXISTS ${MCMC_SOURCE_DIR}/configure)
  message(FATAL_ERROR "MCMC submodule is missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS ${MCMC_SOURCE_DIR}/include/BaseMatrixOps/include/BaseMatrixOps.hpp)
  message(FATAL_ERROR "MCMC nested submodules are missing. Run: git submodule update --init --recursive")
endif()
if(NOT EXISTS ${MCMC_HEADER_ONLY_DIR}/mcmc.hpp)
  message(STATUS "Generating MCMC header-only dependency")
  execute_process(
    COMMAND ${MCMC_SOURCE_DIR}/configure --header-only-version
    WORKING_DIRECTORY ${MCMC_SOURCE_DIR}
    RESULT_VARIABLE MCMC_CONFIGURE_RESULT
  )
  if(NOT MCMC_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate MCMC header-only dependency")
  endif()
  if(NOT EXISTS ${MCMC_HEADER_ONLY_DIR}/mcmc.hpp)
    message(FATAL_ERROR "MCMC header-only dependency was not generated")
  endif()
endif()
include_directories(${MCMC_HEADER_ONLY_DIR})

# ----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)
list(REMOVE_ITEM sources
  ${PROJECT_SOURCE_DIR}/src/DetectorConstruction.cc
  ${PROJECT_SOURCE_DIR}/src/DetectorConstruction_CURIE.cc
)

if(QARRAY_DETECTOR_GEOMETRY STREQUAL "CURIE")
  message(FATAL_ERROR "QARRAY_DETECTOR_GEOMETRY=CURIE is reserved for src/DetectorConstruction_CURIE.cc, but CURIE is not wired into the current QArray application build yet")
elseif(QARRAY_DETECTOR_GEOMETRY STREQUAL "LEIDEN_II")
  list(APPEND sources ${PROJECT_SOURCE_DIR}/src/DetectorConstruction.cc)
elseif(QARRAY_DETECTOR_GEOMETRY STREQUAL "DSPX")
  message(FATAL_ERROR "QARRAY_DETECTOR_GEOMETRY=DSPX is reserved for the translated cache geometry, but DSPX detector construction is not implemented yet")
else()
  message(FATAL_ERROR "Unsupported QARRAY_DETECTOR_GEOMETRY=${QARRAY_DETECTOR_GEOMETRY}; expected CURIE, LEIDEN_II, or DSPX")
endif()

# ----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(main main.cc ${sources} ${headers})
target_link_libraries(main ${Geant4_LIBRARIES})
target_compile_definitions(main PRIVATE QARRAY_DETECTOR_GEOMETRY_${QARRAY_DETECTOR_GEOMETRY})
if(WITH_CRY)
  add_dependencies(main cry)
  target_link_libraries(main CRY)
  target_compile_definitions(main PRIVATE QR_WITH_CRY)
endif()

# ----------------------------------------------------------------------------
# Copy all scripts to the build directory.
#
set(EXTRA_SCRIPTS
  init_vis.mac
  vis_1.mac
  vis_2.mac
  run_test.mac
  tests/particle_gun_smoke.mac
  tests/mt_single_thread_no_vis.mac
  tests/cry_disabled.mac
)

foreach(_script ${EXTRA_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
  )
endforeach()

add_test(
  NAME particle_gun_smoke
  COMMAND main tests/particle_gun_smoke.mac
)
add_test(
  NAME mt_single_thread_no_vis
  COMMAND main tests/mt_single_thread_no_vis.mac
)
set(SMOKE_TESTS particle_gun_smoke mt_single_thread_no_vis)

if(NOT WITH_CRY)
  add_test(
    NAME cry_disabled_fails
    COMMAND main tests/cry_disabled.mac
  )
  list(APPEND SMOKE_TESTS cry_disabled_fails)
  set_tests_properties(
    cry_disabled_fails
    PROPERTIES
      WILL_FAIL TRUE
      PASS_REGULAR_EXPRESSION "CRY support was not compiled"
  )
endif()

function(_append_target_library_dir target out_var)
  if(TARGET ${target})
    get_target_property(_target_lib ${target} IMPORTED_LOCATION_RELEASE)
    if(NOT _target_lib)
      get_target_property(_target_lib ${target} IMPORTED_LOCATION_NOCONFIG)
    endif()
    if(NOT _target_lib)
      get_target_property(_target_lib ${target} IMPORTED_LOCATION)
    endif()
    if(_target_lib)
      get_filename_component(_target_lib_dir ${_target_lib} DIRECTORY)
      set(${out_var} ${${out_var}} ${_target_lib_dir} PARENT_SCOPE)
    endif()
  endif()
endfunction()

set(TEST_RUNTIME_LIBRARY_DIRS)
_append_target_library_dir(Geant4::G4processes TEST_RUNTIME_LIBRARY_DIRS)
_append_target_library_dir(CLHEP::CLHEP TEST_RUNTIME_LIBRARY_DIRS)
if(TEST_RUNTIME_LIBRARY_DIRS)
  list(REMOVE_DUPLICATES TEST_RUNTIME_LIBRARY_DIRS)
  list(JOIN TEST_RUNTIME_LIBRARY_DIRS ":" TEST_RUNTIME_LIBRARY_PATH)
  list(APPEND TEST_ENVIRONMENT "LD_LIBRARY_PATH=${TEST_RUNTIME_LIBRARY_PATH}:$ENV{LD_LIBRARY_PATH}")
endif()
foreach(_g4dataset ${Geant4_DATASETS})
  list(APPEND TEST_ENVIRONMENT
    "${Geant4_DATASET_${_g4dataset}_ENVVAR}=${Geant4_DATASET_${_g4dataset}_PATH}"
  )
endforeach()
if(TEST_ENVIRONMENT)
  set_tests_properties(
    ${SMOKE_TESTS}
    PROPERTIES ENVIRONMENT "${TEST_ENVIRONMENT}"
  )
endif()

# ----------------------------------------------------------------------------
# Copy all scripts in the mac folder into the build directory.
#
# file(COPY ${PROJECT_SOURCE_DIR}/mac DESTINATION ${PROJECT_BINARY_DIR})
