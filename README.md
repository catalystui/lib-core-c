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

Core is a set of foundational libraries for CatalystUI, providing native C ABI contracts based on the **Foundational Definitions Specification (FDEFSPEC)**.

These libraries provide consistent, portable data types and shared utilities for representing digital data, reporting operation results, managing memory, and converting between text encodings, without tying applications to a particular programming language or platform.

Core is independent of any particular layer or sensory domain of the CatalystUI model. Its purpose is to provide a common foundation for other CatalystUI libraries.

## Development Status

> [!NOTE]
> Core is in early development (1.0.0-alpha.2). Core types, memory management, and text encoding are implemented. Memory management currently has backends for Windows, macOS, and Linux. API and ABI stability are not yet guaranteed.

The compile workflow checks C90 and C++98 on x86, and C99 and C++11 on x64, using GCC on Linux.

## Libraries

| Library | Windows | macOS | Linux | Description |
| --- | --- | --- | --- | --- |
| [CATMMLIB](include/catmmlib.h) | `catmmlib.dll`, `catmmlib.lib` | `libcatmm.dylib`, `libcatmm.a` | `libcatmm.so`, `libcatmm.a` | Memory allocation, reallocation, and deallocation. |
| [CATTELIB](include/cattelib.h) | `cattelib.dll`, `cattelib.lib` | `libcatte.dylib`, `libcatte.a` | `libcatte.so`, `libcatte.a` | Text encoding, decoding, length calculation, and conversion. |

The filenames follow the shared build configuration and platform toolchain conventions. Core types are provided through [`catalyst.h`](include/catalyst.h) and [`catalyst.hpp`](include/catalyst.hpp), without a separate binary library.

## Getting Started

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

The shared presets define target-specific compiler and platform configurations. For example, on an Apple Silicon Mac with the required Xcode toolchain:

```powershell
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
```

Preset availability depends on the host and installed toolchain; the presence of a preset does not imply verified runtime support.

## AI-Assisted Development

Core is developed with AI assistance under human direction. AI coding agents assist with implementation and repository maintenance, while the project owner defines the model, requirements, and design decisions.

AI-assisted contributions are subject to the same expectations for correctness, maintainability, attribution, and licensing as human-written contributions. Their presence does not imply independent line-by-line human review.

## License

The original source code and documentation in this repository are licensed under the [Apache License 2.0](LICENSE).
