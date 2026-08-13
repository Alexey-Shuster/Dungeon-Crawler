cmake_minimum_required(VERSION 3.20)
project(Dungeons VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Для простого подключения intellisence
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
#   Global output directories
# -----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(TEST_SCRIPTS_DIR ${PROJECT_ROOT}/test_scripts)

# -----------------------------------------------------------------------------
#   Load Conan toolchain
#   CMAKE_BINARY_DIR is e.g. .../build/Debug or .../build/Release
# -----------------------------------------------------------------------------
include(${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake)

# -----------------------------------------------------------------------------
#   Include shared utilities
# -----------------------------------------------------------------------------
include(${CMAKE_SOURCE_DIR}/cmake/platform_deps.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/boost_deps.cmake)

# -----------------------------------------------------------------------------
#   Copy global config.json
# -----------------------------------------------------------------------------
file(GENERATE
        OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config.json"
        INPUT "${CMAKE_SOURCE_DIR}/config.json"
)

# -----------------------------------------------------------------------------
#   Add subprojects
# -----------------------------------------------------------------------------
#add_subdirectory(src/client)
add_subdirectory(src/common)
add_subdirectory(src/server)

option(BUILD_TESTS "Build tests" ON)
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(test_scripts)
endif ()
