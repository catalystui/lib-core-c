cmake_minimum_required(VERSION 4.3.0)
project(catalyst C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
include_directories("${CMAKE_SOURCE_DIR}/include")
include(${CMAKE_SOURCE_DIR}/cmake/CMakeInit.cmake REQUIRED)
metadata("catalyst" 1 0 0 "alpha.2" "Foundational C ABI libraries for CatalystUI core types, memory management, text encoding, and relational collections.")
configure("${TARGET_VENDOR}" "${TARGET_SYSTEM}" "${TARGET_ARCHITECTURE}")
define("cmake/CMakeConfig.h" "cmakecfg.h")

# DARWIN platform specifics
if(TARGET_PLATFORM_DARWIN)
    # Enable Objective-C and Objective-C++
    enable_language(OBJC OBJCXX)
endif()

# catmmlib
add_library(catmmlib_shared SHARED)
add_library(catmmlib_static STATIC)
configure_target(
    NAME catmmlib
    TARGETS catmmlib_shared catmmlib_static
    SOURCES
        src/catmmlib.cpp
)
if (TARGET_PLATFORM_DARWIN)
    configure_target(
        TARGETS catmmlib_shared catmmlib_static
        SOURCES
            src/catmmlib.darwin.mm
    )
elseif(TARGET_PLATFORM_WIN32)
    configure_target(
        TARGETS catmmlib_shared catmmlib_static
        SOURCES
            src/catmmlib.win32.cpp
    )
elseif(TARGET_PLATFORM_LINUX)
    configure_target(
        TARGETS catmmlib_shared catmmlib_static
        SOURCES
            src/catmmlib.linux.cpp
    )
else()
    message(WARNING "No platform-specific source files configured for CATMMLIB!")
endif()

# cattelib
add_library(cattelib_shared SHARED)
add_library(cattelib_static STATIC)
configure_target(
    NAME cattelib
    TARGETS cattelib_shared cattelib_static
    SOURCES
        src/cattelib.cpp
)

# catrllib
add_library(catrllib_shared SHARED)
add_library(catrllib_static STATIC)
configure_target(
    NAME catrllib
    TARGETS catrllib_shared catrllib_static
    SOURCES
        src/catrllib.cpp
)
target_link_libraries(catrllib_shared PRIVATE catmmlib_shared)
target_link_libraries(catrllib_static PRIVATE catmmlib_static)

# Export C ABI symbols and separate Windows import libraries from static archives.
target_compile_definitions(catmmlib_shared PRIVATE CATMMLIB_EXPORTS INTERFACE CATMMLIB_USE_DLL)
target_compile_definitions(catrllib_shared PRIVATE CATRLLIB_EXPORTS INTERFACE CATRLLIB_USE_DLL)
if(TARGET_PLATFORM_WIN32)
    set_target_properties(catmmlib_shared catrllib_shared PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/import"
    )
endif()

# Define test executables
define_test_executables(
    STATIC_DEPENDS
        catmmlib_static
        cattelib_static
        catrllib_static
    SHARED_DEPENDS
        catmmlib_shared
        cattelib_shared
        catrllib_shared
)
