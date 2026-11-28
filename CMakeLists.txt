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

# catoolib
add_library(catoolib_shared SHARED)
add_library(catoolib_static STATIC)
configure_target(
    NAME catoo
    TARGETS catoolib_shared catoolib_static
    SOURCES
        src/catoolib.cpp
)
target_link_libraries(catoolib_shared PUBLIC catrllib_shared)
target_link_libraries(catoolib_static PUBLIC catrllib_static)

# Export each shared library's C ABI and propagate its import definition.
target_compile_definitions(cattelib_shared PRIVATE CATTELIB_EXPORTS INTERFACE CATTELIB_USE_DLL)
target_compile_definitions(catmmlib_shared PRIVATE CATMMLIB_EXPORTS INTERFACE CATMMLIB_USE_DLL)
target_compile_definitions(catrllib_shared PRIVATE CATRLLIB_EXPORTS INTERFACE CATRLLIB_USE_DLL)
target_compile_definitions(catoolib_shared PRIVATE CATOOLIB_EXPORTS INTERFACE CATOOLIB_USE_DLL)

# Define test executables
define_test_executables(
    STATIC_DEPENDS
        cattelib_static
    SHARED_DEPENDS
        cattelib_shared
)

# Test sources are discovered above; add the corresponding modules for their companion tests.
target_link_libraries(test_catmmlib PRIVATE catmmlib_static)
target_link_libraries(perf_catmmlib PRIVATE catmmlib_static)
target_link_libraries(test_catrllib PRIVATE catrllib_static)
target_link_libraries(perf_catrllib PRIVATE catrllib_static)
target_link_libraries(test_catoolib PRIVATE catoolib_static)
target_link_libraries(perf_catoolib PRIVATE catoolib_static)

# Release optimization is local to Core; preserve Debug and the selected target ISA.
set(CATALYST_RELEASE_LIBRARIES
    cattelib_shared cattelib_static
    catmmlib_shared catmmlib_static
    catrllib_shared catrllib_static
    catoolib_shared catoolib_static
)
set(CATALYST_RELEASE_EXECUTABLES test_cattelib test_catmmlib test_catrllib test_catoolib perf_cattelib perf_catmmlib perf_catrllib perf_catoolib)

foreach(TARGET_NAME IN LISTS CATALYST_RELEASE_LIBRARIES CATALYST_RELEASE_EXECUTABLES)
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            "$<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/O2;/Gy;/Gw>"
        )
        if(NOT TARGET_TYPE STREQUAL "STATIC_LIBRARY")
            target_link_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:/OPT:REF>")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        target_compile_options(${TARGET_NAME} PRIVATE
            "$<$<AND:$<CONFIG:Release>,$<OR:$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang>,$<COMPILE_LANG_AND_ID:OBJCXX,GNU,Clang,AppleClang>>>:-O2>"
        )
        if(TARGET_PLATFORM_LINUX)
            target_compile_options(${TARGET_NAME} PRIVATE
                "$<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-ffunction-sections;-fdata-sections>"
            )
        endif()
        if(NOT TARGET_TYPE STREQUAL "STATIC_LIBRARY")
            target_link_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:-O2>")
            if(TARGET_PLATFORM_LINUX)
                target_link_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:LINKER:--gc-sections>")
            elseif(TARGET_PLATFORM_DARWIN)
                target_link_options(${TARGET_NAME} PRIVATE "$<$<CONFIG:Release>:LINKER:-dead_strip>")
            endif()
        endif()
    endif()
endforeach()

# Enable static IPO by default; consumers distributing native machine-code
# archives can opt out to avoid compiler/linker-specific LTO requirements.
option(CATALYST_RELEASE_STATIC_IPO "Enable Release IPO for static libraries (requires a compatible consumer toolchain)" ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang|MSVC)$")
    include(CheckIPOSupported)
    function(catalyst_configure_release_ipo)
        check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT LANGUAGES CXX)
        if(NOT IPO_SUPPORTED)
            message(STATUS "Core Release IPO unavailable: ${IPO_OUTPUT}")
            return()
        endif()
        foreach(TARGET_NAME IN LISTS CATALYST_RELEASE_LIBRARIES CATALYST_RELEASE_EXECUTABLES)
            get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
            if(TARGET_TYPE STREQUAL "STATIC_LIBRARY" AND NOT CATALYST_RELEASE_STATIC_IPO)
                continue()
            endif()
            # CheckIPOSupported does not probe Objective-C++; require the same
            # toolchain and CMake's Objective-C++ IPO support for CATMMLIB.
            if(TARGET_PLATFORM_DARWIN AND TARGET_NAME MATCHES "^catmmlib_" AND
               (NOT CMAKE_OBJCXX_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID OR
                NOT CMAKE_OBJCXX_COMPILER_VERSION STREQUAL CMAKE_CXX_COMPILER_VERSION OR
                NOT CMAKE_OBJCXX_COMPILE_OPTIONS_IPO))
                continue()
            endif()
            set_property(TARGET ${TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        endforeach()
    endfunction()
    catalyst_configure_release_ipo()
endif()
