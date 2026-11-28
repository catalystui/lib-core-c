<!-- AI AGENTS: DO NOT USE README.md as development context or modify it unless explicitly instructed by the user. This file is intended for human readers only. -->

<h1 align="center">CatalystUI – Core</h1>
<p align="center"><strong>The universal translator between computers and people.</strong></p>
<p align="center">
  <em>The native foundational C ABI libraries for CatalystUI.</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-Apache--2.0-blue?style=flat-square&amp;logo=apache&amp;logoColor=white" alt="License: Apache-2.0"></a>
  <img src="https://img.shields.io/badge/Interface-C%20ABI-00599C?style=flat-square&amp;logo=c&amp;logoColor=white" alt="Interface: C ABI">
  <img src="https://img.shields.io/badge/Build-CMake%204.3%2B-064F8C?style=flat-square&amp;logo=cmake&amp;logoColor=white" alt="Build: CMake 4.3+">
  <a href="https://github.com/catalystui/lib-core-c/commits/main"><img src="https://img.shields.io/github/last-commit/catalystui/lib-core-c/main?style=flat-square&amp;logo=github&amp;logoColor=white" alt="Last commit"></a>
  <a href="https://github.com/catalystui/lib-core-c/actions/workflows/compile.yml"><img src="https://img.shields.io/github/actions/workflow/status/catalystui/lib-core-c/compile.yml?branch=main&amp;style=flat-square&amp;logo=github&amp;logoColor=white" alt="Compile status"></a>
</p>

## About Core

Core is a set of foundational libraries for CatalystUI, providing native C ABI contracts based on the [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/) and [Foundational Relations Specification (FRELSPEC)](https://catalystui.org/specifications/languages/frelspec/).

These libraries provide consistent, portable data representations and operations independently of programming language, operating system, or hardware. Core is independent of every layer and sensory domain of the CatalystUI model and exists to support them all.

Core is designed around 8-bit arithmetic as its minimum compatibility requirement, taking advantage of wider numeric representations where beneficial without changing observable behavior. Its public interfaces target C90 and provide thin C++98-compatible wrappers. Platform-specific services require an appropriate native backend.

## Development Status

> [!NOTE]
> Core is in beta development (1.0.0-beta.1). Foundational types, text encoding, memory, collections, threading (on supported platforms), and object-oriented composites are implemented. API and ABI stability are not yet guaranteed.

The [compile workflow](.github/workflows/compile.yml) checks C90/C++98 on x86 and C99/C++11 on x64 with GCC on Linux. It also verifies optional-width text encoding fallbacks and cross-compiles portable modules for 8-bit AVR targets. Cross-compilation does not establish runtime support on physical hardware.

## Libraries

| Module | Linker name | Description |
| --- | --- | --- |
| [CATALYST](include/catalyst.h) | Header-only | Foundational numeric types, text representations, and operation results. |
| [CATTELIB](include/cattelib.h) | `catte` | ASCII, CP1252, UTF-8, and UTF-16LE encoding, decoding, length calculation, and conversion. |
| [CATMMLIB](include/catmmlib.h) | `catmm` | Memory representations, native allocation, managed heap allocation, resizing, release, and byte operations. |
| [CATRLLIB](include/catrllib.h) | `catrl` | Set, Map, Array, File, Stream, Queue, Stack, Pool, HashSet, and HashMap. |
| [CATTRLIB](include/cattrlib.h) | `cattr` | Process and thread operations, bounded dispatch, and work selection. |
| [CATOOLIB](include/catoolib.h) | `catoo` | Members, objects, fields, methods, properties, structures, classes, and interfaces. |

Each module provides a C header (`.h`) and a C++ wrapper (`.hpp`) in [include/](include/). The five compiled libraries are available as static and shared targets. CATTRLIB is built on Windows, macOS, and Linux; CATMMLIB provides native allocation backends for the same platforms.

CATTELIB and CATMMLIB depend on the foundational CATALYST headers. CATRLLIB depends on CATMMLIB, while CATTRLIB and CATOOLIB depend on CATRLLIB. CATTRLIB additionally uses native threading facilities.

For more information, see the [Core documentation](docs/lang/en-US/index.md), [design assumptions](docs/lang/en-US/assumptions/index.md), and [API reference](docs/lang/en-US/api-reference/index.md).

## Getting Started

### Prebuilt Packages

Download a target-specific package from [GitHub Releases](https://github.com/catalystui/lib-core-c/releases).

| Platform | Architectures |
| --- | --- |
| Windows | x86, x64, ARM64 |
| macOS | x64, ARM64 |
| Linux | x86, x64, ARM64 |

Packages include the public headers, generated target configuration, static and shared libraries, CMake package files, and verification executables. Published static archives contain native machine code rather than compiler-specific IPO data.

### Building from Source

The repository uses **CMake 4.3 or later**, Git, and a C/C++ toolchain appropriate for the selected build preset.

Clone the repository along with its shared build infrastructure:

```powershell
git clone --recurse-submodules https://github.com/catalystui/lib-core-c.git
Set-Location lib-core-c
cmake --list-presets
```

If the repository has already been cloned, initialize its submodule with:

```powershell
git submodule update --init --recursive
```

For example, on an Apple Silicon Mac with the required Xcode toolchain:

```powershell
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
cmake --install target/aclang17-darwin24.6-arm64 --config Release --prefix ./package
```

Preset availability depends on the host and installed toolchain; the presence of a preset does not imply verified runtime support.

Standalone verification programs use the `test_<library>` and `perf_<library>` naming conventions. Build options include `CATALYST_CORE_BUILD_TESTS`, `CATALYST_CORE_INSTALL`, and `CATALYST_RELEASE_STATIC_IPO`. Tests and installation are enabled by default in standalone builds and disabled in nested builds. Release static IPO is enabled by default where supported; disable it for native machine-code archives intended for distribution.

## Consuming Core

Core provides the same namespaced CMake targets when built from source or installed as a package.

| Module | Static target | Shared target |
| --- | --- | --- |
| CATALYST | `Catalyst::catalyst` (header-only) | — |
| CATTELIB | `Catalyst::catte_static` | `Catalyst::catte_shared` |
| CATMMLIB | `Catalyst::catmm_static` | `Catalyst::catmm_shared` |
| CATRLLIB | `Catalyst::catrl_static` | `Catalyst::catrl_shared` |
| CATTRLIB | `Catalyst::cattr_static` | `Catalyst::cattr_shared` |
| CATOOLIB | `Catalyst::catoo_static` | `Catalyst::catoo_shared` |

Targets propagate public include paths, the generated configuration, shared-library import definitions, and required library dependencies. CATTRLIB targets are available on supported platforms only. Use a consistent static or shared dependency chain rather than embedding separate copies of stateful Core libraries into multiple shared libraries.

### Installed Dependency

Extract a prebuilt package or install a source build into a dedicated prefix for its target and configuration. Set `CMAKE_PREFIX_PATH` to that prefix when configuring the consuming project.

```cmake
find_package(CatalystCore CONFIG REQUIRED)

target_link_libraries(my_application PRIVATE Catalyst::catte_static)
```

Installed packages place runtime libraries and ABI aliases at the package root, development archives and linker aliases under `lib/`, and public headers under `include/`. Shared-library consumers must deploy the required runtime libraries and ABI aliases where the native loader can find them.

Use a package compatible with the consuming project's target ABI and toolchain.

### Source Dependency

Alternatively, use CMake's FetchContent with the consuming project's target toolchain and Catalyst target settings:

```cmake
include(FetchContent)

if(NOT TARGET Catalyst::catalyst)
    FetchContent_Declare(
        CatalystCore
        GIT_REPOSITORY https://github.com/catalystui/lib-core-c.git
        GIT_TAG "${CATALYST_CORE_REVISION}"
        GIT_SUBMODULES cmake
    )
    FetchContent_MakeAvailable(CatalystCore)
endif()

target_link_libraries(my_application PRIVATE Catalyst::catte_static)
```

Set `CATALYST_CORE_REVISION` to the intended immutable commit or release tag before declaring the dependency. Use the same dependency name, `CatalystCore`, throughout the build. Prerelease versions do not guarantee compatibility.

## AI-Assisted Development

Core is developed with AI assistance under human direction. AI coding agents assist with implementation and repository maintenance, while the project owner defines the model, requirements, and design decisions.

AI-assisted contributions are subject to the same expectations for correctness, maintainability, attribution, and licensing as human-written contributions. Their presence does not imply independent line-by-line human review.

## License

The original source code and documentation in this repository are licensed under the [Apache License 2.0](LICENSE).
