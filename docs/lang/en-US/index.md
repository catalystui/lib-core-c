# Core

**The universal translator between computers and people.**

*The native foundational C ABI libraries for CatalystUI.*

## About Core

Core is a set of foundational libraries for CatalystUI, providing native C ABI contracts based on the **[Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/)**.

These libraries provide consistent, portable data types and shared utilities for representing digital data, reporting operation results, managing memory, converting between text encodings, and working with relational collections, without tying applications to a particular programming language or platform.

Core is independent of any particular layer or sensory domain of the CatalystUI model. Its purpose is to provide a common foundation for other CatalystUI libraries. Its design requires that Core libraries be implementable and usable even on 8-bit systems.

The source code is available on GitHub: [catalystui/lib-core-c](https://github.com/catalystui/lib-core-c).

## Development Status

> [!NOTE]
> Core is in early development (1.0.0-alpha.2). Core types, memory management, text encoding, and relational collections are implemented. Memory management currently has backends for Windows, macOS, and Linux. API and ABI stability are not yet guaranteed.

The [compile workflow](https://github.com/catalystui/lib-core-c/actions/workflows/compile.yml) checks C90 and C++98 on x86, and C99 and C++11 on x64, using GCC on Linux.

## Libraries

| Library | Windows | macOS | Linux | Description |
| --- | --- | --- | --- | --- |
| [CATMMLIB](api-reference/catmmlib/index.md) | `catmm.lib`, `libcatmm.lib` | `libcatmm.dylib`, `libcatmm.a` | `libcatmm.so`, `libcatmm.a` | Memory allocation, release, resizing, and byte operations. |
| [CATTELIB](api-reference/cattelib/index.md) | `catte.lib`, `libcatte.lib` | `libcatte.dylib`, `libcatte.a` | `libcatte.so`, `libcatte.a` | Text encoding, decoding, length calculation, and conversion. |
| [CATRLLIB](api-reference/catrllib/index.md) | `catrl.lib`, `libcatrl.lib` | `libcatrl.dylib`, `libcatrl.a` | `libcatrl.so`, `libcatrl.a` | Arrays, sets, maps, files, and streams. |

These are development linker filenames. Runtime shared libraries use versioned filenames, and Unix shared-library linker aliases point to the corresponding runtime ABI aliases. The filenames follow the shared build configuration and platform toolchain conventions; the Windows names above describe the MSVC convention.

[Core types](api-reference/catalyst/index.md) are provided through `catalyst.h` and `catalyst.hpp`, without a separate binary library.

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

The [Assumptions](assumptions/index.md) page explains the principles governing function validation and operation results. The [API Reference](api-reference/index.md) documents the public contracts provided by Core.

## AI-Assisted Development

Core is developed with AI assistance under human direction. AI coding agents assist with implementation and repository maintenance, while the project owner defines the model, requirements, and design decisions.

AI-assisted contributions are subject to the same expectations for correctness, maintainability, attribution, and licensing as human-written contributions. Their presence does not imply independent line-by-line human review.

## License

The original source code and documentation in this repository are licensed under the [Apache License 2.0](https://github.com/catalystui/lib-core-c/blob/main/LICENSE).
