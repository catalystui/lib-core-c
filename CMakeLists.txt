cmake_minimum_required(VERSION 4.3.0)
project(catalyst C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories("${CMAKE_SOURCE_DIR}/include")
include(${CMAKE_SOURCE_DIR}/cmake/CMakeInit.cmake REQUIRED)
metadata("catalyst" 1 0 0 "alpha.2" "Foundational C ABI libraries for CatalystUI core types and text encoding.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# cattelib
add_library(cattelib_shared SHARED)
add_library(cattelib_static STATIC)
configure_target(
    NAME catte
    TARGETS cattelib_shared cattelib_static
    SOURCES
        src/cattelib.cpp
)

# Export each shared library's C ABI and propagate its import definition.
target_compile_definitions(cattelib_shared PRIVATE CATTELIB_EXPORTS INTERFACE CATTELIB_USE_DLL)

# Define test executables
define_test_executables(
    STATIC_DEPENDS
        cattelib_static
    SHARED_DEPENDS
        cattelib_shared
)
