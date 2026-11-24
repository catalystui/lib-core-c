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

# catmmlib
if(TARGET_PLATFORM_DARWIN)
    enable_language(OBJCXX)
    set(CATMMLIB_PLATFORM_SOURCE src/catmmlib.darwin.mm)
elseif(TARGET_PLATFORM_WIN32)
    set(CATMMLIB_PLATFORM_SOURCE src/catmmlib.win32.cpp)
elseif(TARGET_PLATFORM_LINUX)
    set(CATMMLIB_PLATFORM_SOURCE src/catmmlib.linux.cpp)
endif()
add_library(catmmlib_shared SHARED)
add_library(catmmlib_static STATIC)
configure_target(
    NAME catmm
    TARGETS catmmlib_shared catmmlib_static
    SOURCES
        src/catmmlib.cpp
        src/catmmlib.internal.hpp
        ${CATMMLIB_PLATFORM_SOURCE}
)

# catrllib
add_library(catrllib_shared SHARED)
add_library(catrllib_static STATIC)
configure_target(
    NAME catrl
    TARGETS catrllib_shared catrllib_static
    SOURCES
        src/catrllib.cpp
)
target_link_libraries(catrllib_shared PUBLIC catmmlib_shared)
target_link_libraries(catrllib_static PUBLIC catmmlib_static)

# Export each shared library's C ABI and propagate its import definition.
target_compile_definitions(cattelib_shared PRIVATE CATTELIB_EXPORTS INTERFACE CATTELIB_USE_DLL)
target_compile_definitions(catmmlib_shared PRIVATE CATMMLIB_EXPORTS INTERFACE CATMMLIB_USE_DLL)
target_compile_definitions(catrllib_shared PRIVATE CATRLLIB_EXPORTS INTERFACE CATRLLIB_USE_DLL)

# Define test executables
define_test_executables(
    STATIC_DEPENDS
        cattelib_static
    SHARED_DEPENDS
        cattelib_shared
)

# Test sources are discovered above; add the memory module for its companion test.
target_link_libraries(test_catmmlib PRIVATE catmmlib_static)
