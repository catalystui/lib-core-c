cmake_minimum_required(VERSION 4.3.0)
project(catalyst C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories("${CMAKE_SOURCE_DIR}/include")
include(${CMAKE_SOURCE_DIR}/cmake/CMakeInit.cmake REQUIRED)
metadata("catalyst" 1 0 0 "alpha.2" "Foundational C ABI libraries for CatalystUI core types and text encoding.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# Establish exact scalar widths and IEEE 754 format support without executing target code.
include(CheckTypeSize)
foreach(type SHORT INT LONG FLOAT DOUBLE)
    string(TOLOWER "${type}" name)
    check_type_size("${name}" CATALYST_TARGET_SIZEOF_${type} LANGUAGE C)
endforeach()
include(CheckCXXSourceCompiles)
if(CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID AND
   CMAKE_C_COMPILER_VERSION STREQUAL CMAKE_CXX_COMPILER_VERSION)
    check_cxx_source_compiles("#include <limits>
typedef char check[(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559) ? 1 : -1];
int main() { return 0; }" CATALYST_TARGET_IEC559_SINGLE)
    check_cxx_source_compiles("#include <limits>
typedef char check[(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559) ? 1 : -1];
int main() { return 0; }" CATALYST_TARGET_IEC559_DOUBLE)
else()
    set(CATALYST_TARGET_IEC559_SINGLE 0)
    set(CATALYST_TARGET_IEC559_DOUBLE 0)
endif()
define("config/catalystcfg.h.in" "catalystcfg.h")

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
