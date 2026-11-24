# Assumptions

**The caller is responsible for the validity of the invocation. The function is responsible for the outcome of the operation.**

These principles apply to public C functions, C++ wrappers, and internal helpers alike.

The C++ aliases of C types preserve their representations. The compiled libraries' inline C++ wrappers forward to the C API and preserve its ownership and failure guarantees.

[FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/) defines representations; [FRELSPEC Revision 2.1](https://catalystui.org/specifications/languages/frelspec/) defines relationships, including memory before collections. Core follows these conceptual dependencies. Individual module references document their specific ownership, storage, and operation contracts.

## Contents

- [Numerics and Arithmetic](#numerics-and-arithmetic)
  - [Numeric Representations](#numeric-representations)
  - [Arithmetic](#arithmetic)
  - [Consumer Requirements](#consumer-requirements)
- [Validation](#validation)
  - [Contract Validation](#contract-validation)
  - [Operational Validation](#operational-validation)
- [Results](#results)
  - [Determining the Return Type](#determining-the-return-type)
  - [Reporting Boundaries](#reporting-boundaries)

## Numerics and Arithmetic

**Eight-bit arithmetic is Core's minimum compatibility guarantee, not its performance target. Core uses Catalyst's numerical guarantees to remain portable while taking advantage of available representations.**

### Numeric Representations

The [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/) defines the exact composition and interpretation of numeric values. C and C++ types implement these definitions; they do not replace them.

<div style="box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);margin:1rem 0;">
<div style="text-align:center;font-weight:600;margin-bottom:0.9rem;">Fixed-width integer representations</div>
<div style="display:flex;flex-direction:column;gap:0.75rem;max-width:35rem;margin:auto;">
<div style="display:flex;flex-wrap:wrap;align-items:center;gap:0.4rem 0.85rem;">
<div style="flex:1 1 13rem;min-width:0;"><strong><code>BYTE / SBYTE</code></strong><br/><small style="overflow-wrap:anywhere;">Required</small></div>
<div style="flex:1 1 12rem;min-width:0;display:flex;align-items:center;gap:0.25rem;" aria-label="8 bits"><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 45%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 17%, transparent);"></span></div>
<small style="min-width:3.5rem;text-align:right;">8 bits</small>
</div>
<div style="display:flex;flex-wrap:wrap;align-items:center;gap:0.4rem 0.85rem;">
<div style="flex:1 1 13rem;min-width:0;"><strong><code>USHORT / SHORT</code></strong><br/><small style="overflow-wrap:anywhere;">CATALYST_SUPPORTS_16BIT</small></div>
<div style="flex:1 1 12rem;min-width:0;display:flex;align-items:center;gap:0.25rem;" aria-label="16 bits"><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span></div>
<small style="min-width:3.5rem;text-align:right;">16 bits</small>
</div>
<div style="display:flex;flex-wrap:wrap;align-items:center;gap:0.4rem 0.85rem;">
<div style="flex:1 1 13rem;min-width:0;"><strong><code>UINT / INT</code></strong><br/><small style="overflow-wrap:anywhere;">CATALYST_SUPPORTS_32BIT</small></div>
<div style="flex:1 1 12rem;min-width:0;display:flex;align-items:center;gap:0.25rem;" aria-label="32 bits"><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span></div>
<small style="min-width:3.5rem;text-align:right;">32 bits</small>
</div>
<div style="display:flex;flex-wrap:wrap;align-items:center;gap:0.4rem 0.85rem;">
<div style="flex:1 1 13rem;min-width:0;"><strong><code>ULONG / LONG</code></strong><br/><small style="overflow-wrap:anywhere;">CATALYST_SUPPORTS_64BIT</small></div>
<div style="flex:1 1 12rem;min-width:0;display:flex;align-items:center;gap:0.25rem;" aria-label="64 bits"><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span><span style="flex:0 1 calc((100% - 1.75rem) / 8);aspect-ratio:1;border:1px solid color-mix(in srgb, currentColor 24%, transparent);border-radius:3px;background:color-mix(in srgb, currentColor 7%, transparent);"></span></div>
<small style="min-width:3.5rem;text-align:right;">64 bits</small>
</div>
</div>
<div style="text-align:center;margin-top:0.9rem;opacity:0.8;"><small>Each square represents eight bits, not a native memory address or byte order.</small></div>
</div>

The `CATALYST_SUPPORTS_*` definitions indicate whether Catalyst provides an optional representation, not whether the processor executes arithmetic of that width directly. A conforming C implementation may lack an exact Catalyst representation.

Optional types require their corresponding `#if` guards. Defined representations may be converted to, packed into, and manipulated using supported Catalyst numeric types, preserving their meaning and logical order without assuming native memory layout. `NINT` and `NUINT` retain their target-specific pointer width.

### Arithmetic

All arithmetic, including intermediate calculations, uses Catalyst-defined numeric types. Fundamental operations remain available using bytes; compile-time-selected wider arithmetic should be used when it offers substantial performance or usability benefits.

<div style="box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);margin:1rem 0;text-align:center;">
<strong>Example · Adding two 64-bit unsigned values</strong>
<div style="max-width:33rem;margin:0.9rem auto 0;">
<div style="padding:0.65rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;">One mathematical operation</div>
<div style="opacity:0.75;padding:0.3rem 0;" aria-hidden="true">↓</div>
<div style="display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%, 11rem),1fr));gap:0.6rem;">
<div style="padding:0.75rem 0.4rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;display:flex;flex-direction:column;justify-content:center;gap:0.25rem;">
<small style="overflow-wrap:anywhere;opacity:0.8;"><code>#if SUPPORTS_64BIT</code></small>
<strong>1 × 64-bit</strong>
<code>ULONG</code>
<small>Wider arithmetic path</small>
</div>
<div style="padding:0.75rem 0.4rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;display:flex;flex-direction:column;justify-content:center;gap:0.25rem;">
<small style="overflow-wrap:anywhere;opacity:0.8;"><code>#elif SUPPORTS_32BIT</code></small>
<strong>2 × 32-bit</strong>
<code>UINT</code>
<small>Wider arithmetic path</small>
</div>
<div style="padding:0.75rem 0.4rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;display:flex;flex-direction:column;justify-content:center;gap:0.25rem;">
<small style="overflow-wrap:anywhere;opacity:0.8;"><code>#elif SUPPORTS_16BIT</code></small>
<strong>4 × 16-bit</strong>
<code>USHORT</code>
<small>Wider arithmetic path</small>
</div>
<div style="padding:0.75rem 0.4rem;border:1px solid color-mix(in srgb, currentColor 40%, transparent);border-radius:6px;display:flex;flex-direction:column;justify-content:center;gap:0.25rem;">
<small style="overflow-wrap:anywhere;opacity:0.8;"><code>#else</code></small>
<strong>8 × 8-bit</strong>
<code>BYTE</code>
<small>Minimum compatibility path</small>
</div>
</div>
<div style="opacity:0.75;padding:0.3rem 0;" aria-hidden="true">↓</div>
<div style="padding:0.65rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;">Identical sum and carry</div>
</div>
<div style="margin-top:0.65rem;opacity:0.8;"><small>Conceptual compile-time selection; SUPPORTS_* abbreviates CATALYST_SUPPORTS_*.</small></div>
</div>

All paths must produce equivalent mathematical results and observable behavior. Select beneficial optimizations using `CATALYST_SUPPORTS_*`, accounting for carry, overflow, truncation, signedness, and promotions. Representational availability alone does not guarantee a performance improvement.

**Why?** Compatibility defines the minimum capabilities required to use Core, not a limit on its performance.

### Consumer Requirements

Core supports optional interfaces that let consumers use wider numeric representations directly. A consumer may establish its minimum support requirements once for its codebase rather than guard every individual use.

```cpp
#include "catalyst.hpp"

#if !CATALYST_SUPPORTS_32BIT
    #error "This application requires 32-bit support."
#endif
```

On such a target, [`RESULT`](../api-reference/catalyst/index.md#result-members) can be constructed from, assigned from, or converted to `UINT` in `0xSSCCOODD` order. Its four fields and by-value C ABI remain unchanged. This permits efficient scalar operations without requiring them on targets lacking 32-bit support.

## Validation

**A function performs validation if, and only if, that validation is necessary to determine or safely produce the outcome of its operation when all caller preconditions are satisfied.**

<div style="display:flex;flex-wrap:wrap;gap:0.85rem;margin:0.8rem 0 1rem;">
<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.65rem;opacity:0.9;"><circle cx="12" cy="7" r="4"/><path d="M4 21v-2a8 8 0 0 1 16 0v2"/></svg>
<small style="display:block;text-align:center;opacity:0.8;">Caller responsibility</small>

<div style="text-align:center;">

### Contract Validation

</div>

<p style="text-align:center;"><strong>Can the function be invoked correctly?</strong></p>
<p>Establish valid pointers, accessible extents, initialization, lifetime, alignment, synchronization, and other documented preconditions (such as valid direct-access indices).</p>
<p style="margin-bottom:0;">Core assumes these conditions hold. Violating them is outside the function's guarantees, not a reportable outcome.</p>
</div>
<div style="flex:1 1 calc(50% - 0.5rem);min-width:min(100%, 15rem);box-sizing:border-box;padding:1.2rem;border:1px solid color-mix(in srgb, currentColor 19%, transparent);border-radius:12px;background:color-mix(in srgb, currentColor 3%, transparent);">
<svg aria-hidden="true" width="28" height="28" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round" style="display:block;width:32px;height:32px;margin:0.15rem auto 0.65rem;opacity:0.9;"><rect x="5" y="5" width="14" height="14" rx="2"/><path d="M8 12l3 3 5-6M9 2v3m6-3v3M9 19v3m6-3v3M2 9h3m-3 6h3m14-6h3m-3 6h3"/></svg>
<small style="display:block;text-align:center;opacity:0.8;">Function responsibility</small>

<div style="text-align:center;">

### Operational Validation

</div>

<p style="text-align:center;"><strong>Can the operation succeed?</strong></p>
<p>Detect malformed data, unrepresentable values, insufficient supplied capacity, extent overflow, allocation or I/O failure, and other reportable operational conditions.</p>
<p style="margin-bottom:0;">Check as part of the operation, before unsafe access or irreversible changes. Never read beyond the caller's accessible extent.</p>
</div>
</div>

**Why?** Establish preconditions once, then compose operations without repeatedly checking them. Valid internal state is established during creation and preserved through mutation. This avoids redundant work without hiding genuine operational failures as caller preconditions.

<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);margin:1rem 0;">
<div style="text-align:center;font-weight:600;">Example · Decoding UTF-8</div>
<div style="text-align:center;margin:0.9rem 0 0.4rem;">
<code>E2 82 AC</code> <span style="opacity:0.75;">or</span> <code>E2 00</code>
<div style="margin-top:0.3rem;">Caller provides accessible input and valid, separate output storage.</div>
<div style="text-align:center;line-height:1.3;padding:0.3rem 0;opacity:0.82;font-size:0.9em;" aria-hidden="true">↓</div>
<div style="padding:0.65rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;max-width:22rem;margin:auto;"><code>catte_utf8_decode</code><br/><small>Function determines encoding validity</small></div>
<div style="text-align:center;line-height:1.3;padding:0.3rem 0;opacity:0.82;font-size:0.9em;" aria-hidden="true">↓</div>
<div style="display:flex;flex-wrap:wrap;gap:0.65rem;justify-content:center;">
<div style="flex:1 1 10rem;min-width:0;padding:0.65rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;"><strong><code>SUCCESS</code></strong><br/>U+20AC (€)</div>
<div style="flex:1 1 10rem;min-width:0;padding:0.65rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:6px;overflow-wrap:anywhere;"><strong><code>ERROR_MALFORMED_INPUT</code></strong><br/>Truncated sequence</div>
</div>
</div>
</div>

## Results

**A function returns a Result if, and only if, its intended operation requires distinguishing a meaningful warning, error, or fatal outcome from ordinary successful behavior when all caller preconditions are satisfied.**

### Determining the Return Type

Define the function's purpose, preconditions, ordinary outputs, and reportable outcomes. Assume all caller preconditions hold, then follow the first applicable branch:

<div role="group" aria-label="Return type decision flowchart" style="margin:1rem auto;max-width:40rem;width:100%;">
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;max-width:24rem;margin:0 auto;">Start with the function's contract</div>
<div style="text-align:center;line-height:1.3;padding:0.3rem 0;opacity:0.82;font-size:0.9em;" aria-hidden="true">↓</div>
<div style="display:grid;grid-template-columns:minmax(0, 3fr) minmax(0, 2fr);gap:0.6rem;align-items:stretch;">
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;"><strong>Must the operation distinguish a warning, error, or fatal outcome?</strong></div>
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;border-color:color-mix(in srgb, currentColor 35%, transparent);"><small>Yes →</small><strong><code>RESULT</code></strong><small>Data via arguments</small></div>
<div style="text-align:center;line-height:1.3;padding:0.3rem 0;opacity:0.82;font-size:0.9em;">No ↓</div><div></div>
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;"><strong>Does it produce a single ordinary value?</strong></div>
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;border-color:color-mix(in srgb, currentColor 35%, transparent);"><small>Yes →</small><strong>Value</strong><small>Corresponding Catalyst type</small></div>
<div style="text-align:center;line-height:1.3;padding:0.3rem 0;opacity:0.82;font-size:0.9em;">No ↓</div><div></div>
<div style="box-sizing:border-box;min-width:0;padding:1rem;border:1px solid color-mix(in srgb, currentColor 22%, transparent);border-radius:10px;background:color-mix(in srgb, currentColor 4%, transparent);display:flex;flex-direction:column;justify-content:center;text-align:center;border-color:color-mix(in srgb, currentColor 35%, transparent);"><strong><code>void</code></strong><small>Outputs via arguments or mutation</small></div><div></div>
</div>
</div>

Return `CATALYST_RESULT` (`RESULT` in C++) by value on every exit, including success; return additional data through arguments. Multiple outputs alone do not require a Result.

| Function | Returns | Why? |
| --- | --- | --- |
| [`catte_utf8_decode`](../api-reference/cattelib/index.md#catte_utf8_decode) | `CATALYST_RESULT` | Encoding may be malformed. |
| [`catte_ascii_length`](../api-reference/cattelib/index.md#catte_ascii_length) | `CATALYST_NUINT` | Length is an ordinary value. |
| Non-reporting procedure | `void` | Writes outputs or mutates existing storage without a reportable outcome. |

Zero, false, equality, emptiness, absence, and no-ops are not inherently failures. For example, an empty ASCII string has length zero; this is an ordinary successful outcome. An operation that already requires a Result follows its documented no-op contract.

### Reporting Boundaries

Report at the outermost boundary responsible for operational validation and failure handling. Use non-reporting primitives once their preconditions have been established. An internal step that can still produce a reportable outcome must report it; performance is not a reason to suppress required diagnostics.

Document output validity for each outcome. Non-reporting functions do not construct or propagate Results.

Result tables use the logical `0xSSCCOODD` order defined by [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/). Dependency status and detail are retained where applicable, except as documented by the reporting operation. Undefined status codes remain reserved.
