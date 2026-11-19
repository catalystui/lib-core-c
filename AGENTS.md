# AGENTS.md

Requirements for CatalystUI Core.

## Authority and architecture

- Implement FDEFSPEC and FRELSPEC, not conventional library abstractions. Core is independent of CatalystUI layers, sensory domains, operating systems, and programming languages.
- Follow the specifications' conceptual dependencies: definitions before relations, collections before memory. More fundamental concepts must not depend on derived concepts. Collections do not inherently require allocation or ownership; compose resource management with fundamental operations separately. CATTELIB's caller-provided storage illustrates this approach.
- The owner defines architecture and contracts. Explain the necessity and consequences of foundational changes (including fixes, APIs, representations, helpers, and behavior) and obtain explicit approval first. Never modify or bypass Numeric Definitions, Codepoint, Text Encoding, or Operation Status to simplify another implementation. Report conflicts or missing concepts rather than inventing solutions or restoring removed designs.
- Prefer simple, direct, explicit implementations. No speculative abstractions, duplicated algorithms, unnecessary dependencies, or unrelated cleanup.
- Before stability, legacy API/ABI compatibility is not required; no shims or parallel APIs merely to preserve old behavior. Update affected code, callers, tests, and references together.
- README.md is for humans only; do not use it as development context or edit it without explicit request.

## Numerics and arithmetic

8-bit arithmetic is Core's minimum compatibility guarantee, not its performance target.

- Catalyst Numeric Definitions guarantee exact representations. C/C++ compliance does not guarantee Catalyst compatibility. Native types implement Catalyst types; they are not substitutes for them.
- Use only Catalyst-defined numeric types for arithmetic, including intermediate values. Never bypass a missing Catalyst representation with native language types or rely on wider implicit promotions.
- `CATALYST_SUPPORTS_*` indicates representational availability, not processor width or arithmetic performance. Guard access to and arithmetic on optional types with `#if CATALYST_SUPPORTS_*`, not `#ifdef`.
- Fundamental operations must remain available using `CATALYST_BYTE`/`CATALYST_SBYTE`. Use compile-time-selected wider Catalyst arithmetic when it offers substantial performance or usability benefits. Guard algorithms only by the representations they use, not unrelated unavailable widths. All paths must preserve mathematical behavior and observable guarantees.
- Optional interfaces may require supported representations; consumers may establish their own minimum support requirements once for their entire codebase.
- Common arithmetic primitives may be centralized in CATALYST with compiler-gated implementations after owner approval of their contracts and interfaces. Do not scatter equivalent arithmetic or representation conversions across libraries.
- Preserve defined byte order; do not assume native endianness, alignment, padding, or integer layout. Convert, pack, and operate on defined representations using supported Catalyst scalars; preserve their meaning and logical order. Never assume native object layout matches specified byte representations. Codepoint remains MSB-first (`byte0`–`byte3`); UTF-16LE code units remain `lsb`, then `msb`.
- Preserve the pointer-width scalar ABI of `CATALYST_NINT`/`CATALYST_NUINT` for addresses, sizes, counts, and indices, including one-byte pointers. Do not assume a wider minimum or use these types to bypass numeric support guarantees. Obtain approval before defining new ABI conversion helpers.
- Account for promotions, overflow, truncation, signedness, and shift behavior. Never require native floating-point arithmetic or optional wider instructions. Confine unavoidable native platform API types to platform boundaries.

## Language, ABI, and style

- Public headers: C90; implementations/wrappers: C++98. Newer standards only through existing guards. Maintain the C ABI: `include/<library>.h`, `src/<library>.cpp`, thin inline `include/<library>.hpp` wrappers that forward without duplicating logic. Keep C++ definitions in `.hpp`; the sole approved exception is UTF-16LE code-unit members guarded by `__cplusplus`, preserving a shared C/C++ array element type. Arrays crossing the C ABI use actual C element types. Preserve include guards, `extern "C"`, exports, const/read-write distinctions, ownership, lifetimes, alignment, and buffer contracts.
- Use Catalyst types and `BOOL`/`TRUE`/`FALSE` (C: `CATALYST_BOOL`/`CATALYST_TRUE`/`CATALYST_FALSE`). No native numeric substitutes, native booleans, exceptions, references, STL containers, heap abstractions, or external dependencies in portable Core without approval. OS calls belong in platform-specific files (`*.win32.cpp`, `*.linux.cpp`, `*.darwin.mm`).
- C types/constants: uppercase snake case with `CATALYST_`; functions: lowercase snake case, library prefix + subject + operation (e.g. `catte_utf8_encode`). Reserve `cat_` for approved foundational functions. C++ equivalents omit prefixes in `namespace catalyst`. Parameters/fields/private functions use lowercase snake case; descriptive lower-camel-case locals are acceptable.
- Four spaces, same-line braces, null pointer `0`, C-compatible typedefs, explicit casts. Prefer early returns, simple loops, static private helpers, section dividers, concise comments explaining intent, not obvious code. Follow .editorconfig/.gitattributes (UTF-8, CRLF working tree, final newline, no trailing whitespace).

## Validation and function contracts

- **Caller:** Establish documented preconditions (valid initialized objects/handles, pointers, extents, termination, direct-access indices, alignment, lifetime, non-overlap, synchronization). Core assumes these hold; violations are outside its guarantees. Document preconditions for public and private functions; do not recheck them or provide parallel checked variants.
- **Function:** Validate operational conditions even with valid inputs: malformed data, unrepresentable values, insufficient capacity, overflow, allocation/I/O/dependency/cleanup failures, failed retrieval. Validate during the operation, before unsafe access or irreversible changes; never exceed accessible extents. Do not disguise operational failures as caller preconditions.
- Establish valid internal state on creation and preserve it through mutation. Trust established guarantees; do not repeatedly inspect metadata to detect misuse or arbitrary corruption. Preserve promised failure state, clean up resources, and report genuine operational failures.
- Determine the operation's approved contract before selecting the first applicable return convention:
  1. Warning/error/fatal outcome must be distinguished: return `CATALYST_RESULT` (`RESULT` in C++) by value; additional data via output arguments.
  2. Single ordinary value: return its Catalyst type directly.
  3. Otherwise: return `void`; mutate storage or write output arguments.
- Ordinary zero, false, equality, absence, emptiness, carry/borrow, and no-op are not inherently failures. Multiple outputs alone do not require a Result. Do not invent sentinel failures or success-only Results. Reporting operations retain their documented no-op contracts.
- Report at the outermost boundary owning operational validation and failure handling; use non-reporting primitives once preconditions are established. Internal steps still report when their caller must distinguish a warning or failure. Never suppress required diagnostics for performance.
- Apply equally to exports, wrappers, and private helpers. Removing redundant caller-contract checks and Results is approved; other changes to intended behavior require approval. Document output validity for every outcome. Preserve text encoding units, BOM, terminator, substitution, and buffer contracts.
- Reporting overhead target: <10%; >=20% triggers review, not removal of diagnostics. Benchmark equivalent work including output storage and status consumption; report absolute time, frequency, and relative overhead. Instruction counts are not timings. Obtain approval for boundary/contract changes.

## Operation results

`RESULT` represents four ordered bytes: status, context, operation, detail. Guarded `UINT` packing, conversion, and operations in `0xSSCCOODD` order are valid; preserve the four-field C ABI and by-value return. No exceptions, result output pointers, last-result state, or larger combined return structures.

- **Status:** Use the most specific FDEFSPEC code. Undefined codes are reserved.
- **Context:** Unique per module; fixed method ID on every return, including success and no-op. Zero means unspecified, not success.
- **Operation:** Unique per method and result-reporting site, including success. Distinct sites never share an opcode; one site may have multiple details. No need to retain obsolete IDs.
- **Detail:** Meaning precedence: status > context > operation. Use useful additional information without duplication; zero when unused. `ERROR_INVALID_ARGUMENT` uses the zero-based index in the full signature (including by-value arguments).
- Dependency failures: preserve applicable status, fatal severity, and useful detail, retaining the reporting method's context and call-site opcode. Do not blindly copy fields or default to `ERROR_DEPENDENCY_FAILURE`.
- Define assignments before implementation; use `return RESULT(...);` with defined status constants/severity helpers on every exit. Non-reporting functions do not construct, propagate, or document Results.

## Documentation

- Specifications define concepts; explanatory docs teach them; references describe actual contracts. Keep source, wrappers, tests, and docs consistent; report discrepancies instead of silently reconciling them. Use concise exact terminology, no redundant prose.
- Preserve Overview, Assumptions, and API Reference (last), plus `docs/manifest.json` navigation with `"alignment": "block"` per page. Overview aligns with README; Assumptions explains architecture, arithmetic, validation, and reporting. New guides require approval; no duplicate contracts or module-level Conventions sections.
- Public declarations: short `/** ... */` behavior/precondition comments. API reference: one comprehensive `docs/lang/en-US/api-reference/<module>/index.md` per module, linked from the reference index. Include headers, namespace, **Library:** actual linker name or Header-only, **Dependencies:** if applicable, and linked type/constant/function summary and details.
- Type details: description before declaration/representation, fields, C++ equivalent. Show concrete, typed **C90:**/**C99+:** examples when different; explain target-dependent native alternatives briefly; show optional types with their guards and state unavailable variants. Availability badge: `<span data-heading-badge="MACRO" data-heading-badge-background="#b4232c" data-heading-badge-color="#ffffff" title="Requires MACRO to be 1"></span>` after heading/description; no separate REQUIRES paragraph.
- Function details: exact C name heading, fenced C signature, description, **Parameters:** individual entries (not a table), **Returns:** for ordinary values or **Results:** for Results, targeted **See Also:** when useful. Use Markdown hard line breaks for consecutive metadata/parameter/field entries; separate See Also. Document units, capacities, nullability, preconditions, ownership, failure outputs, and edge cases.
- Reporting methods: **Context:** `0xCC` immediately before **Results:** table with exactly `Result | Status | Operation | Detail`. One row per distinct result, including each fixed detail. First column `0xSSCCOODD` (uppercase, logical byte order regardless of endianness); other columns describe meaning, not redundant bytes. Document genuine variable ranges/patterns; em dash only for unused fields, not meaningful zero. Link specifications to the live website.

## Build and verification

- Preserve shared CMake infrastructure, cmakecfg.h, existing presets, and static/shared targets. Target capabilities belong in build-cmake and cmakecfg.h; do not introduce parallel configuration headers. Initialize cmake submodule recursively; no build/CI/submodule workarounds without approval.
- Runtime executables, tests, DLLs, versioned shared libraries and ABI aliases belong directly in debug/ or release/. Development archives/import libraries/unversioned linker aliases belong in flat lib/. Unix runtime dependencies resolve relative to binaries; linker aliases point to runtime ABI aliases. Shared build-cmake derives names/versions from metadata; preserve library names and targets (catte).
- Example v1.5.2: MSVC `libcatte.lib`, `catte.lib`, `catte15.dll` (full resource version); Linux `libcatte.a`, `libcatte.so`, `libcatte.so.1.5`, `libcatte.so.1.5.2`; macOS `libcatte.a`, `libcatte.dylib`, `libcatte.1.5.dylib`, `libcatte.1.5.2.dylib`. Unix aliases are symlinks.
- Use existing standalone test/test_<library>.cpp programs, no new framework. Test wrappers, normal/boundary/no-op/failure behavior and all four Result bytes; never violate caller preconditions. Test byte and wider paths for equivalence; verify claimed optimization benefits.
- Verify C90/C++98 and C99/C++11 builds/headers where possible; review arithmetic and representation assumptions independently. Test optional-width fallbacks and AVR compilation; cross-compilation does not establish hardware execution. Report tests performed, limitations, and conflicts.

## Commits

Focused authorized changes, concise imperative subjects. AI-assisted commits include:

`Co-authored-by: Codex <noreply@openai.com>`
