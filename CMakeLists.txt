cmake_minimum_required(VERSION 4.3.0)
project(catalyst C CXX)
if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()
include("${PROJECT_SOURCE_DIR}/cmake/CMakeInit.cmake" REQUIRED)
metadata("catalyst" 1 0 0 "beta.1" "Foundational C ABI libraries for CatalystUI.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# Public headers and target configuration are usage requirements, not global
# include paths. Keep the generated configuration separate from other projects.
add_library(catalyst_headers INTERFACE)
add_library(Catalyst::catalyst ALIAS catalyst_headers)
set_target_properties(catalyst_headers PROPERTIES EXPORT_NAME catalyst)
target_include_directories(catalyst_headers INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    "$<INSTALL_INTERFACE:include>"
    "$<INSTALL_INTERFACE:include/catalyst-core-config>"
)

option(CATALYST_CORE_BUILD_TESTS "Build Core's standalone verification programs" ${PROJECT_IS_TOP_LEVEL})
option(CATALYST_CORE_INSTALL "Install Core libraries and CMake package" ${PROJECT_IS_TOP_LEVEL})

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
        src/catrllib.internal.hpp
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

# cattrlib: the POSIX backend is shared by Linux and Darwin.
if(TARGET_PLATFORM_WIN32 OR TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN)
    if(TARGET_PLATFORM_WIN32)
        set(CATTRLIB_PLATFORM_SOURCE src/cattrlib.win32.cpp)
    else()
        set(CATTRLIB_PLATFORM_SOURCE src/cattrlib.posix.cpp)
        set(THREADS_PREFER_PTHREAD_FLAG TRUE)
        find_package(Threads REQUIRED)
    endif()
    add_library(cattrlib_shared SHARED)
    add_library(cattrlib_static STATIC)
    configure_target(
        NAME cattr
        TARGETS cattrlib_shared cattrlib_static
        SOURCES
            src/cattrlib.cpp
            src/cattrlib.internal.hpp
            ${CATTRLIB_PLATFORM_SOURCE}
    )
    target_link_libraries(cattrlib_shared PUBLIC catrllib_shared)
    target_link_libraries(cattrlib_static PUBLIC catrllib_static)
    if(NOT TARGET_PLATFORM_WIN32)
        target_link_libraries(cattrlib_shared PRIVATE Threads::Threads)
        target_link_libraries(cattrlib_static PRIVATE Threads::Threads)
    endif()
    target_compile_definitions(cattrlib_shared PRIVATE CATTRLIB_EXPORTS INTERFACE CATTRLIB_USE_DLL)
endif()

# Export each shared library's C ABI and propagate its import definition.
target_compile_definitions(cattelib_shared PRIVATE CATTELIB_EXPORTS INTERFACE CATTELIB_USE_DLL)
target_compile_definitions(catmmlib_shared PRIVATE CATMMLIB_EXPORTS INTERFACE CATMMLIB_USE_DLL)
target_compile_definitions(catrllib_shared PRIVATE CATRLLIB_EXPORTS INTERFACE CATRLLIB_USE_DLL)
target_compile_definitions(catoolib_shared PRIVATE CATOOLIB_EXPORTS INTERFACE CATOOLIB_USE_DLL)

if(CATALYST_CORE_BUILD_TESTS)
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
    if(TARGET cattrlib_static)
        target_link_libraries(test_cattrlib PRIVATE cattrlib_static)
        target_link_libraries(perf_cattrlib PRIVATE cattrlib_static)
    endif()
endif()

# Release optimization is local to Core; preserve Debug and the selected target ISA.
set(CATALYST_RELEASE_LIBRARIES
    cattelib_shared cattelib_static
    catmmlib_shared catmmlib_static
    catrllib_shared catrllib_static
    catoolib_shared catoolib_static
)
set(CATALYST_RELEASE_EXECUTABLES)
if(CATALYST_CORE_BUILD_TESTS)
    set(CATALYST_RELEASE_EXECUTABLES test_cattelib test_catmmlib test_catrllib perf_cattelib perf_catmmlib perf_catrllib test_catoolib perf_catoolib)
endif()

if(TARGET cattrlib_static)
    list(APPEND CATALYST_RELEASE_LIBRARIES cattrlib_shared cattrlib_static)
    if(CATALYST_CORE_BUILD_TESTS)
        list(APPEND CATALYST_RELEASE_EXECUTABLES test_cattrlib perf_cattrlib)
    endif()
endif()

# Export the same stable target names for source and installed consumption.
foreach(TARGET_NAME IN LISTS CATALYST_RELEASE_LIBRARIES)
    string(REPLACE "lib_" "_" EXPORT_TARGET_NAME "${TARGET_NAME}")
    set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME "${EXPORT_TARGET_NAME}")
    add_library(Catalyst::${EXPORT_TARGET_NAME} ALIAS ${TARGET_NAME})
    target_link_libraries(${TARGET_NAME} PUBLIC catalyst_headers)
endforeach()

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

# Preserve the existing runtime/development artifact layout in each install prefix.
if(CATALYST_CORE_INSTALL)
    include(CMakePackageConfigHelpers)
    set(CATALYST_CORE_PACKAGE_DIR "lib/cmake/CatalystCore")
    install(TARGETS catalyst_headers ${CATALYST_RELEASE_LIBRARIES}
        EXPORT CatalystCoreTargets
        RUNTIME DESTINATION .
        LIBRARY DESTINATION .
        ARCHIVE DESTINATION lib
    )
    if(TARGET_PLATFORM_LINUX OR TARGET_PLATFORM_DARWIN)
        foreach(TARGET_NAME IN LISTS CATALYST_RELEASE_LIBRARIES)
            get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
            if(TARGET_TYPE STREQUAL "SHARED_LIBRARY")
                get_target_property(ABI_NAME ${TARGET_NAME} CATALYST_ABI_NAME)
                get_target_property(LINKER_NAME ${TARGET_NAME} CATALYST_LINKER_NAME)
                set_property(TARGET ${TARGET_NAME} APPEND PROPERTY EXPORT_PROPERTIES CATALYST_ABI_NAME)
                install(FILES "$<TARGET_FILE_DIR:${TARGET_NAME}>/${ABI_NAME}" DESTINATION .)
                install(FILES "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${LINKER_NAME}" DESTINATION lib)
            endif()
        endforeach()
    endif()
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION include
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/cmakecfg.h"
        DESTINATION include/catalyst-core-config)
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/NOTICE"
        DESTINATION share/CatalystCore)

    set(CATALYST_CORE_NEEDS_THREADS FALSE)
    if(TARGET cattrlib_static AND NOT TARGET_PLATFORM_WIN32)
        set(CATALYST_CORE_NEEDS_THREADS TRUE)
    endif()
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/packaging/CatalystCoreConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CatalystCoreConfig.cmake"
        INSTALL_DESTINATION "${CATALYST_CORE_PACKAGE_DIR}"
    )
    # Prerelease versions do not promise compatibility with other numeric versions.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/CatalystCoreConfigVersion.cmake"
        VERSION "${METADATA_VERSION_MAJOR}.${METADATA_VERSION_MINOR}.${METADATA_VERSION_PATCH}"
        COMPATIBILITY ExactVersion
    )
    install(EXPORT CatalystCoreTargets
        NAMESPACE Catalyst::
        DESTINATION "${CATALYST_CORE_PACKAGE_DIR}")
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/CatalystCoreConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/CatalystCoreConfigVersion.cmake"
        DESTINATION "${CATALYST_CORE_PACKAGE_DIR}")
endif()
