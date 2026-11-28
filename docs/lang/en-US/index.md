<div style="text-align:center;padding:1.4rem 0.65rem 1.15rem;margin-bottom:1.2rem;border-bottom:1px solid color-mix(in srgb, currentColor 15%, transparent);">

# Core

<p style="font-size:1.22em;font-weight:600;line-height:1.4;margin:0.75rem 0 0.35rem;">The universal translator between computers and people.</p>
<p style="font-style:italic;opacity:0.85;margin:0 0 1.2rem;">The native foundational C ABI libraries for CatalystUI.</p>

<div style="display:flex;flex-wrap:wrap;justify-content:center;align-items:center;gap:0.4rem;margin:0.4rem auto 1.2rem;">
<a href="https://github.com/catalystui/lib-core-c/blob/main/LICENSE"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/License-Apache--2.0-blue?style=flat-square&amp;logo=apache&amp;logoColor=white" alt="License: Apache-2.0"></a>
<img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/Interface-C%20ABI-00599C?style=flat-square&amp;logo=c&amp;logoColor=white" alt="Interface: C ABI">
<img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/badge/Build-CMake%204.3%2B-064F8C?style=flat-square&amp;logo=cmake&amp;logoColor=white" alt="Build: CMake 4.3+">
<a href="https://github.com/catalystui/lib-core-c/commits/main"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/github/last-commit/catalystui/lib-core-c/main?style=flat-square&amp;logo=github&amp;logoColor=white" alt="Last commit"></a>
<a href="https://github.com/catalystui/lib-core-c/actions/workflows/compile.yml"><img style="display:block;width:auto;height:20px;max-width:100% !important;padding:0 !important;margin:0;" src="https://img.shields.io/github/actions/workflow/status/catalystui/lib-core-c/compile.yml?branch=main&amp;style=flat-square&amp;logo=github&amp;logoColor=white" alt="Compile status"></a>
</div>

<div style="display:flex;flex-wrap:wrap;justify-content:center;align-items:center;gap:0.5rem 1.35rem;font-weight:600;margin:0.5rem 0 0.2rem;">

[View source on GitHub](https://github.com/catalystui/lib-core-c)

[Understand the design](assumptions/index.md)

[API Reference](api-reference/index.md)

</div>
</div>

Core provides portable building blocks based on the [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/). Its native C ABI allows libraries to share data representations and operations independently of programming language, operating system, or hardware.

Core is designed to be implementable even on 8-bit systems while taking advantage of wider numeric representations where beneficial. It is independent of every layer and sensory domain of the CatalystUI model, and exists to support them all.

> [!NOTE]
> Core is in early development (1.0.0-alpha.2). Core types, native memory operations, text encoding, and collections are implemented. API and ABI stability are not yet guaranteed.

## Libraries

<div style="display:flex;flex-wrap:wrap;gap:0.85rem;margin:0.8rem 0 1rem;">
<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/></svg>

**[CATALYST](api-reference/catalyst/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Foundational types and operation results.</p>
<small style="opacity:0.8;margin-top:auto;">Header-only</small>
</div>

<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><path d="M3 19L9 5l6 14M5 14h8M17 7h4m-2 0v12m-2 0h4"/></svg>

**[CATTELIB](api-reference/cattelib/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Encode, decode, measure, and convert text.</p>
<small style="opacity:0.8;margin-top:auto;">Link: <code>catte</code></small>
</div>

<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><rect x="5" y="5" width="14" height="14" rx="2"/><rect x="9" y="9" width="6" height="6" rx="1"/><path d="M9 2v3m6-3v3M9 19v3m6-3v3M2 9h3m-3 6h3m14-6h3m-3 6h3"/></svg>

**[CATMMLIB](api-reference/catmmlib/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Allocate, resize, and operate on memory.</p>
<small style="opacity:0.8;margin-top:auto;">Link: <code>catmm</code></small>
</div>

<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><rect x="3" y="4" width="18" height="16" rx="2"/><path d="M3 9h18M9 9v11m6-11v11"/></svg>

**[CATRLLIB](api-reference/catrllib/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Store collections, resolve Files, and produce Streams.</p>
<small style="opacity:0.8;margin-top:auto;">Link: <code>catrl</code></small>
</div>

<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;align-items:center;text-align:center;">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.8rem;opacity:0.9;"><rect x="9" y="3" width="6" height="6" rx="1"/><path d="M12 9v4M5 13h14M5 13v3m14-3v3"/><rect x="2" y="16" width="6" height="5" rx="1"/><rect x="16" y="16" width="6" height="5" rx="1"/></svg>

**[CATTRLIB](api-reference/cattrlib/index.md)**

<p style="margin:0 0 0.85rem;max-width:20rem;">Represent processes, execute threads, and dispatch work.</p>
<small style="opacity:0.8;margin-top:auto;">Link: <code>cattr</code></small>
</div>

</div>

## Getting Started

You'll need Git, **CMake 4.3 or later**, and a C/C++ toolchain for your target.

Clone Core and list the available build presets:

```powershell
git clone --recurse-submodules https://github.com/catalystui/lib-core-c.git
Set-Location lib-core-c
cmake --list-presets
```

For example, on an Apple Silicon Mac with the required Xcode toolchain:

```powershell
cmake --preset aclang17-darwin24.6-arm64
cmake --build --preset aclang17-darwin24.6-arm64-release
```

Already cloned? Run `git submodule update --init --recursive` to initialize the shared build infrastructure. Presets depend on the installed toolchain and do not guarantee runtime support.

The [compile workflow](https://github.com/catalystui/lib-core-c/actions/workflows/compile.yml) checks C90/C++98 on x86 and C99/C++11 on x64 with GCC on Linux.

### Consuming Core

Core provides the same namespaced CMake targets when built from source or installed as a package.

| Module | Static | Shared |
| --- | --- | --- |
| CATALYST (headers only) | `Catalyst::catalyst` | `Catalyst::catalyst` |
| CATTELIB | `Catalyst::catte_static` | `Catalyst::catte_shared` |
| CATMMLIB | `Catalyst::catmm_static` | `Catalyst::catmm_shared` |
| CATRLLIB | `Catalyst::catrl_static` | `Catalyst::catrl_shared` |
| CATTRLIB (where implemented) | `Catalyst::cattr_static` | `Catalyst::cattr_shared` |

Targets propagate public include directories, the generated target configuration, shared-library import definitions, and required library dependencies. Choose a consistent static or shared dependency chain; do not embed independent copies of stateful Core libraries into multiple shared libraries.

#### Source dependency

Use the same dependency name, `CatalystCore`, throughout a CMake build. Select an immutable Core revision via `CATALYST_CORE_REVISION`, and configure the consuming project using its target toolchain and the existing `TARGET_VENDOR`, `TARGET_SYSTEM`, and `TARGET_ARCHITECTURE` settings.

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

A parent project can declare the revision first or set `FETCHCONTENT_SOURCE_DIR_CATALYSTCORE` to an existing local checkout. Core's standalone verification programs and install rules default to disabled in nested builds; enable them explicitly with `CATALYST_CORE_BUILD_TESTS` and `CATALYST_CORE_INSTALL` when needed.

#### Installed dependency

After building Core, install into a dedicated prefix for the chosen target and configuration. For the Apple Silicon preset above:

```powershell
cmake --install target/aclang17-darwin24.6-arm64 --config Release --prefix ./package
```

Use one prefix per target and configuration. The package preserves the runtime libraries and ABI aliases at its root, development archives/import libraries and linker aliases under `lib/`, and public headers under `include/`. Set `CMAKE_PREFIX_PATH` to this prefix when configuring a consuming project.

```cmake
find_package(CatalystCore CONFIG REQUIRED)

target_link_libraries(my_application PRIVATE Catalyst::catte_static)
```

Installed packages require a compatible target ABI and toolchain. The package checks the operating system, pointer width when available, and explicitly declared Catalyst target identifiers; these checks do not establish every aspect of binary compatibility. Release static archives use IPO by default; configure Core with `CATALYST_RELEASE_STATIC_IPO=OFF` for native machine-code archives intended for distribution. Alpha versions do not guarantee compatibility; pin the intended source revision or select the corresponding binary package rather than relying on the numeric version alone. Shared-library consumers must deploy the corresponding runtime libraries and ABI aliases where the native loader can find them.

## AI-Assisted Development

Core is developed with AI assistance under human direction. The owner defines the model, requirements, and design decisions. All contributions follow the same correctness, maintainability, attribution, and licensing expectations; AI-assisted code is not necessarily reviewed independently line by line.

## License

Core is licensed under the [Apache License 2.0](https://github.com/catalystui/lib-core-c/blob/main/LICENSE).
