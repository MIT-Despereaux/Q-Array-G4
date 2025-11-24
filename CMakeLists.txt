# ----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 3.21)
project(QURAD_G4_SIM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
option(BUILD_STATIC_LIBS "Build static libraries" ON)

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
# Download and use the CRY cosmic ray generator library
#
# NOTE: CRY only support single threaded compilation.
include(ExternalProject)
ExternalProject_Add(
  cry
  PREFIX ${PROJECT_SOURCE_DIR}/ext
  URL https://nuclear.llnl.gov/simulation/cry_v1.7.tar.gz
  URL_HASH MD5=85f240bebe81fe0b257e92ef1d390a83
  UPDATE_DISCONNECTED TRUE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "make" "-j1"
  BUILD_IN_SOURCE TRUE
  INSTALL_COMMAND ""
)

# Gather properties
ExternalProject_Get_Property(cry SOURCE_DIR)
set(CRY_BASE ${SOURCE_DIR})
# message(STATUS "CRY base directory: ${CRY_BASE}")
set(CRY_LIB ${CRY_BASE}/lib/libCRY.a)

# Adds an interface library called CRY
add_library(CRY INTERFACE)
# Adds the built static link library, the include directory, etc. to the interface library
target_link_libraries(CRY INTERFACE ${CRY_LIB})
target_include_directories(CRY INTERFACE ${CRY_BASE}/src)
target_compile_definitions(CRY INTERFACE CRYDATA="${CRY_BASE}/data")

# ----------------------------------------------------------------------------
# Include Eigen3
include_directories(${PROJECT_SOURCE_DIR}/submodules/eigen)

# ----------------------------------------------------------------------------
# Include MCMC library
include_directories(${PROJECT_SOURCE_DIR}/submodules/mcmc/header_only_version)

# ----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

# ----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(main main.cc ${sources} ${headers})
target_link_libraries(main ${Geant4_LIBRARIES} CRY)

# ----------------------------------------------------------------------------
# Copy all scripts to the build directory.
#
set(EXTRA_SCRIPTS
  init_vis.mac
  vis_1.mac
  vis_2.mac
  run_test.mac
)

foreach(_script ${EXTRA_SCRIPTS})
  configure_file(
    ${PROJECT_SOURCE_DIR}/${_script}
    ${PROJECT_BINARY_DIR}/${_script}
    COPYONLY
  )
endforeach()

# ----------------------------------------------------------------------------
# Copy all scripts in the mac folder into the build directory.
#
# file(COPY ${PROJECT_SOURCE_DIR}/mac DESTINATION ${PROJECT_BINARY_DIR})
