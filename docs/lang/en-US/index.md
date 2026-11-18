# Core

**The universal translator between computers and people.**

*The native foundational C ABI libraries for CatalystUI.*

Core provides portable building blocks based on the [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/) and [Foundational Relations Specification (FRELSPEC)](https://catalystui.org/specifications/languages/frelspec/). Its native C ABI allows libraries to share data representations and operations independently of programming language, operating system, or hardware.

Core is designed to be implementable even on 8-bit systems. It is independent of every layer and sensory domain of the CatalystUI model, and exists to support them all.

[View source on GitHub](https://github.com/catalystui/lib-core-c) · [Understand the design](assumptions/index.md) · [API Reference](api-reference/index.md)

> [!NOTE]
> Core is in early development (1.0.0-alpha.2). Memory allocation has Windows, macOS, and Linux backends. API and ABI stability are not yet guaranteed.

## Libraries

<div style="display:flex;flex-wrap:wrap;gap:0.85rem;margin:0.8rem 0 1rem;">
<div style="flex:1 1 38%;min-width:min(100%, 14rem);box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;margin-bottom:0.55rem;opacity:0.85;"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/></svg>

**[CATALYST](api-reference/catalyst/index.md)**

<p style="margin:0 0 0.6rem;">Foundational types and operation results.</p>
<small style="opacity:0.8;">Header-only</small>
</div>
<div style="flex:1 1 38%;min-width:min(100%, 14rem);box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;margin-bottom:0.55rem;opacity:0.85;"><rect x="5" y="5" width="14" height="14" rx="2"/><rect x="9" y="9" width="6" height="6" rx="1"/><path d="M8 2v3m4-3v3m4-3v3M8 19v3m4-3v3m4-3v3M2 8h3m-3 4h3m-3 4h3m14-8h3m-3 4h3m-3 4h3"/></svg>

**[CATMMLIB](api-reference/catmmlib/index.md)**

<p style="margin:0 0 0.6rem;">Allocate, release, resize, and manipulate memory.</p>
<small style="opacity:0.8;">Link: <code>catmm</code></small>
</div>
<div style="flex:1 1 38%;min-width:min(100%, 14rem);box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;margin-bottom:0.55rem;opacity:0.85;"><path d="M3 19L9 5l6 14M5 14h8M17 7h4m-2 0v12m-2 0h4"/></svg>

**[CATTELIB](api-reference/cattelib/index.md)**

<p style="margin:0 0 0.6rem;">Encode, decode, measure, and convert text.</p>
<small style="opacity:0.8;">Link: <code>catte</code></small>
</div>
<div style="flex:1 1 38%;min-width:min(100%, 14rem);box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;margin-bottom:0.55rem;opacity:0.85;"><rect x="9" y="2" width="6" height="6" rx="1"/><rect x="2" y="16" width="6" height="6" rx="1"/><rect x="16" y="16" width="6" height="6" rx="1"/><path d="M12 8v4m-7 4v-4h14v4"/></svg>

**[CATRLLIB](api-reference/catrllib/index.md)**

<p style="margin:0 0 0.6rem;">Arrays, sets, maps, files, and streams.</p>
<small style="opacity:0.8;">Link: <code>catrl</code></small>
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

## AI-Assisted Development

Core is developed with AI assistance under human direction. The owner defines the model, requirements, and design decisions. All contributions follow the same correctness, maintainability, attribution, and licensing expectations; AI-assisted code is not necessarily reviewed independently line by line.

## License

Core is licensed under the [Apache License 2.0](https://github.com/catalystui/lib-core-c/blob/main/LICENSE).
