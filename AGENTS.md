# AGENTS.md

These instructions apply to the entire repository and establish the architectural requirements for CatalystUI Core. Follow them for all development, including tests, documentation, and build changes.

## Authority and scope

- CatalystUI Core provides foundational, language-independent C ABI libraries based on the Foundational Definitions Specification (FDEFSPEC). It is not tied to any particular CatalystUI layer, domain, operating system, or programming language.
- The project owner defines the architecture, specifications, and requirements. Implement them faithfully; do not invent new contracts or reinterpret existing ones for convenience.
- Before modifying any existing foundational implementation, API, data representation, or behavior, explain the proposed change, its necessity, and its consequences to the owner and obtain explicit approval. This includes bug fixes, refactors, signature changes, and adding utility or conversion functions to existing code. An approved task authorizes only the changes explicitly covered by that task.
- In particular, do not change the numeric definitions, Codepoint, Text Encoding, or Operation Status systems, or add helpers to them, merely to make a new implementation easier. Stop and ask first. Do not silently bypass this rule with a different wrapper or helper.
- Prefer simple, direct, explicit implementations. Avoid speculative abstractions, unnecessary dependencies, convenience APIs, duplicated algorithms, or unrelated cleanup. Keep changes narrowly scoped.
- `README.md` is for human readers only. Do not use it as development context or modify it unless explicitly requested. Use the specifications, headers, implementation, API reference, and tests instead.

## Mandatory 8-bit compatibility

Every Core library MUST be implementable and usable on an 8-bit system. This is a foundational invariant, not an optimization preference.

- No operation may require a native integer wider than eight bits. Perform all arithmetic exclusively using bytes; this applies to intermediate values as well as inputs and outputs.
- Use `CATALYST_BYTE` / `BYTE` (or `CATALYST_SBYTE` / `SBYTE` where signed byte semantics are required). Represent wider logical values using explicitly ordered bytes. Implement carry, borrow, shifts, comparisons, multiplication, division, and overflow detection byte-by-byte as needed. Mask before shifting and do not depend on a widened intermediate to preserve a carry.
- Never pack bytes into a native 16-, 32-, or 64-bit integer to calculate, compare, shift, encode, decode, or otherwise manipulate a value. Do not hide wider arithmetic behind casts, unions, pointer reinterpretation, intrinsic functions, library calls, or conditional fast paths. No native floating-point arithmetic is required for a Core algorithm.
- C and C++ integer promotions are a language detail, not permission to rely on a wider mathematical range. Write byte operations whose correctness requires no arithmetic capability beyond eight bits.
- Preserve explicit byte order. Do not assume host endianness, alignment, pointer width, or the layout of native integers. Do not serialize a native struct or integer as a substitute for a specified byte representation.
- The byte-oriented Codepoint and Text Encoding algorithms in `src/cattelib.cpp` are the reference: `CATALYST_CODEPOINT` has four most-significant-byte-first fields (`byte0` through `byte3`), and `CATALYST_UTF16LE_CODE_UNIT` has `lsb` followed by `msb`. A four-byte codepoint does not require 32-bit arithmetic.
- Existing native-width types `CATALYST_NINT` and `CATALYST_NUINT` follow configured pointer width (including one-byte pointers). Preserve their existing ABI for addresses, addressable sizes, counts, and indices; never assume a minimum width greater than eight bits or use them to circumvent byte-only arithmetic. Where existing size/address calculations use native arithmetic, flag the conflict and seek approval before changing the established contract or implementation.
- Optional wider numeric representations already defined by the specification are not prerequisites for Core operations. Respect the existing `CATALYST_SUPPORTS_*` guards; do not expand their use or introduce new native-width conversion utilities.
- The original guarded C++ native-integer constructors, assignments, and conversions are optional language conveniences, not precedent for new Core algorithms. Do not use them in Core implementations or expand them without approval.
- Native platform API types may be unavoidable at a platform boundary; keep such types confined to the platform-specific backend and do not make them a requirement of portable Core code. Ask the owner before introducing a new exception or resolving a conflict with byte-only arithmetic.

## Language and ABI

- Maintain C90-compatible public C headers and C++98-compatible implementations and wrappers. Newer standards may be supported through the existing feature guards; do not require them.
- Preserve the C ABI: public declarations in `include/<library>.h`, matching implementations in `src/<library>.cpp`, and thin inline C++ wrappers in `include/<library>.hpp`. Use the existing include guards, `extern "C"` guards, and library-specific export macros.
- Use Catalyst's defined types in public signatures, not language-specific containers, exceptions, references, native booleans, or unguarded fixed-width types. Use `CATALYST_BOOL` / `BOOL`, `TRUE` and `FALSE` for Catalyst boolean values.
- Public library operations return `void`. Return operation data through explicit output parameters and status through the final `CATALYST_RESULT* result` parameter. C++ wrappers use `RESULT* result` and forward to the C function without duplicating logic.
- Preserve the distinction between readable and writable pointers; use `const` for read-only input. Follow the existing text pointer naming convention (e.g. `CATALYST_UTF8` versus `CATALYST_UTF8W`).
- Keep opaque handles opaque. Define private storage in implementation files. Preserve ownership, lifetime, allocation, alignment, and buffer contracts.
- Use CATMMLIB for shared allocation and byte operations where appropriate. Keep operating-system calls in platform-specific source files (`*.win32.cpp`, `*.linux.cpp`, `*.darwin.mm`). Do not introduce STL containers, heap abstractions, or external dependencies in portable Core implementations without approval.

## Naming and code style

- C types and public constants: uppercase snake case with `CATALYST_` (e.g. `CATALYST_CODEPOINT`, `CATALYST_STATUS_CODE_ERROR_INVALID_ARGUMENT`).
- C functions: lowercase snake case with the owning library's prefix, then the subject and operation (e.g. `catmm_copy`, `catte_utf8_encode`, `catrl_array_create`). Use `cat_` for existing foundational naming patterns only; new foundational functions require approval.
- C++ types and constants: the corresponding uppercase names without `CATALYST_`, in `namespace catalyst` (e.g. `CODEPOINT`, `STATUS_CODE_SUCCESS`). C++ function names omit the C library prefix (e.g. `utf8_encode`, `array_create`).
- Use lowercase snake case for public parameters and structured fields (e.g. `element_size`, `requested_size`). Private functions generally use lowercase snake case; local variables may use the existing descriptive lower-camel-case style (e.g. `copySize`, `codepoint0`). Match the surrounding code; do not rename unrelated symbols.
- Use four spaces for indentation, opening braces on the same line, and the existing `if (...) {` / `} else {` style. Use `0` for null pointers, `typedef` for C-compatible declarations, and the existing explicit cast style (e.g. `(BYTE) 0x80`).
- Use early returns for validation and failure. Prefer direct control flow, named byte values and straightforward loops over clever expressions or generic abstractions. Use `static` for implementation-only helpers when needed; do not export them accidentally.
- Follow existing section-divider comments and concise `//` implementation comments. Explain intent, bit layouts, nonobvious conditions, invariants, or failure stages rather than narrating obvious assignments.
- Follow `.editorconfig` and `.gitattributes`: UTF-8, CRLF working-tree line endings, final newline, no trailing whitespace.

## Operation results and correctness

`CATALYST_RESULT` / `RESULT` is a structured representation of an operation's outcome, not a status byte with three optional or interchangeable metadata bytes. Its four ordered bytes are status, context, operation, and detail, each with a distinct responsibility. Do not replace it with a native packed integer, exception, or ad hoc error convention.

- **Status:** What happened? Use the most specific applicable FDEFSPEC status. Distinguish success, no-op, warning, recoverable error, and fatal invariant violation. Do not invent new status meanings or codes without approval.
- **Context:** Under what circumstances did it happen? Identify the meaningful context of the outcome, such as argument validation, storage-size calculation, handle allocation, element storage allocation, or resource cleanup. Populate it meaningfully for warnings, errors, and fatal results; do not routinely default it to zero. FDEFSPEC requires context to be zero for all success-level statuses (including no-op).
- **Operation:** What operation was being performed? Identify the associated public operation with a stable, documented operation code on every result path, including success and no-op. The operation code identifies the operation itself, not its internal execution stage or failure location; those belong in context. Do not default operation to zero or reuse one operation code for unrelated operations within its documented scope.
- **Detail:** What additional information is needed? The meaning is defined by status first, then context, then operation, in that order. A lower-priority definition must never override a higher-priority definition. For `ERROR_INVALID_ARGUMENT`, detail MUST be the zero-based argument index in the associated public function's signature, including arguments that precede output pointers. Do not use it for an unrelated native error number, dependency status, or internal argument index. Where no higher-priority meaning is defined, use context- or operation-defined detail if it conveys useful additional information; otherwise zero is appropriate.

Define and document the operation and context code assignments and applicable detail meanings consistently within their intended scope before implementing them. Reuse consistent meanings where appropriate. New foundational definitions and changes to existing result contracts require owner approval. Zero is not inherently wrong: success context is necessarily zero, argument zero is meaningful detail, and detail need not duplicate information already conveyed by the other bytes.

- Report dependency failures using the actual applicable status rather than introducing or defaulting to a generic dependency-failure status. Use context to describe the affected stage and retain the identity of the public operation in the operation byte. Preserve status-defined detail semantics for the public operation; do not blindly copy a dependency result or its detail. Where a dependency status does not accurately describe the public operation's outcome, select an appropriate existing status, including fatal invariant violation when warranted. Existing `ERROR_DEPENDENCY_FAILURE` behavior is not precedent for new code; changes to existing behavior remain subject to owner approval.
- A null result pointer is valid and discards status information. When it is non-null, populate all four fields on every return path. Use the established `if (result != 0) *result = RESULT(...);` pattern and the defined status constants and severity helpers as appropriate.
- Validate required pointers and parameters before accessing them or writing outputs. Honor the documented order of validation and the specific output-state contract on failure. Do not assume all APIs have identical failure-output behavior.
- Check overflow, capacity, bounds, allocation failures, malformed input, and dependency results. Preserve logical state on recoverable mutation failures when promised by the API. Clean up owned resources on failure, and accurately report the outcome of the public operation, including cleanup failures, according to the four-byte semantics above.
- Text APIs distinguish bytes, code units, codepoints, terminators, and BOMs. Preserve these distinctions and documented buffer requirements, substitution behavior, and invalid-input handling.

Existing implementations and references may conflict with these requirements. Identify and report conflicts; do not silently preserve them as precedent or modify existing behavior outside an explicitly approved fix.

## Documentation

- Keep public headers, C++ wrappers, implementation, reference documentation, and tests consistent with the approved contract.
- Distinguish specifications (what a concept is), explanatory documentation (how concepts work and are used), and API reference (the precise contract of each operation). Keep the reference focused on lookup rather than tutorials or implementation narration.
- Add a short `/** ... */` documentation comment above each public declaration, matching the existing header style. State what the operation does and essential preconditions or output behavior. Do not duplicate the entire reference page in the header.
- Detailed API reference lives in `docs/lang/en-US/<library>/<topic>/index.md`. Keep the existing nested navigation structure, and update `docs/manifest.json` when adding or moving pages.
- Library overview pages describe headers, linking, types, common conventions, exports, and links to reference topics. Document shared behavior once and link to it.
- Use a Javadoc-inspired reference structure. Introduce the topic and its relevant header/type, then provide a **Function Summary** table linking each exact C function name to its entry with a short description. Follow with **Function Details**.
- For each operation, use a heading with its exact C function name, its exact C declaration in a fenced `c` block (format long declarations across multiple lines), a natural description of its behavior, and a **Parameters:** section with individual parameter names and descriptions rather than a parameter table. Use **See Also:** links where useful.
- Each operation has a **Results:** table with exactly five columns: `Result | Status | Context | Operation | Detail`. Use one row for each distinct possible result, including each distinct fixed detail value. The first column displays the complete four-byte hexadecimal code in uppercase digits as `0xSSCCOODD`, in status–context–operation–detail order, independent of native endianness. The remaining columns explain the meaning of each corresponding byte in that result. Use an em dash only for genuinely unused fields, not for a meaningful zero value such as argument index zero. Do not combine multiple fixed results in one cell or row.
- If a field is genuinely variable (rather than a finite set of defined outcomes), document its byte range or pattern and exact meaning explicitly; do not invent a fixed hexadecimal result or omit possible values. The reference notation does not imply that the implementation packs a result into a native integer.
- Explicitly document units, required buffer capacity, nullability, ownership, failure-output behavior, and other observable guarantees wherever applicable. Do not promise behavior not supported by the implementation or omit a documented edge case.
- Use exact Catalyst terminology and identifiers. Keep documentation concise, factual, readable, and focused on the API contract. Use tables where they aid lookup or express relationships, not as the default for all content. Do not change FDEFSPEC or silently reconcile specification discrepancies.

## Build and verification

- Preserve the existing shared CMake infrastructure, `cmakecfg.h` target configuration, and static/shared build targets. Initialize the `cmake` submodule recursively and use existing presets where available; do not modify the submodule, build configuration, or CI to work around a code issue without approval.
- Executables (including tests) are placed directly in the configuration output directory (`debug/` or `release/`); all library artifacts, including Windows DLLs, import libraries, static libraries, Unix shared libraries, and symbolic links, share its flat `lib/` subdirectory. Do not create a separate test output directory. Keep formal library names, headers, and CMake target identifiers unchanged; artifact base names are `catmm`, `catte`, and `catrl`. The shared `build-cmake` module owns naming and versioning. For release 1.5.2, Windows (MSVC) produces `libcatmm.lib`, `catmm.lib`, and `catmm15.dll` (full version in the DLL resource); Linux produces `libcatmm.a`, `libcatmm.so`, `libcatmm.so.1.5`, and `libcatmm.so.1.5.2`; macOS produces `libcatmm.a`, `libcatmm.dylib`, `libcatmm.1.5.dylib`, and `libcatmm.1.5.2.dylib`. Unix linker and ABI aliases are symbolic links. Derive versions from project metadata; do not add a separate import-library directory.
- Follow the existing standalone C++ test programs in `test/test_<library>.cpp`. Exercise the C++ wrapper and verify operation output and all four result fields. Cover normal cases, boundary values, no-ops, invalid arguments, overflow, and failure-state behavior as applicable. Use simple checks and nonzero exit codes; no additional testing framework is needed.
- Verify public C and C++ headers and library builds under the existing C90/C++98 and C99/C++11 configurations when possible. The current x86/x64 compile matrix does not prove 8-bit compatibility; independently review every operation for byte-only arithmetic and width assumptions. Do not claim 8-bit target support has been tested unless it actually has.
- Report what was tested, what could not be tested, and any discovered invariant conflicts. Do not silently fix pre-existing issues outside the approved scope.

## Commits

- Keep commits focused on the authorized task. Use concise imperative commit subjects, matching repository history.
- Include the existing AI co-author trailer on AI-assisted commits:

  `Co-authored-by: Codex <noreply@openai.com>`
