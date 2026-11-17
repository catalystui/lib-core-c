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

- `CATALYST_RESULT` / `RESULT` is four ordered bytes: status, context, operation, detail. Do not replace it with a native packed integer, exception, or ad hoc error convention.
- A null result pointer is valid and discards status information. When it is non-null, populate all four fields on every return path. Use the established `if (result != 0) *result = RESULT(...);` pattern.
- Use defined status constants and the severity helpers when appropriate. Distinguish success, no-op, warning, recoverable error, and fatal invariant violation. Do not invent new status meanings or codes without approval.
- Success has zero context; use zero operation/detail unless the contract specifies otherwise. Invalid-argument detail normally identifies the zero-based argument position. Preserve documented function-specific exceptions rather than silently normalizing them.
- Validate required pointers and parameters before accessing them or writing outputs. Honor the documented order of validation and the specific output-state contract on failure. Do not assume all APIs have identical failure-output behavior.
- Check overflow, capacity, bounds, allocation failures, malformed input, and dependency results. Preserve logical state on recoverable mutation failures when promised by the API. Clean up owned resources on failure, and propagate dependency failures according to the documented status, operation, and detail conventions.
- Text APIs distinguish bytes, code units, codepoints, terminators, and BOMs. Preserve these distinctions and documented buffer requirements, substitution behavior, and invalid-input handling.

## Documentation

- Keep public headers, C++ wrappers, implementation, reference documentation, and tests consistent with the approved contract.
- Add a short `/** ... */` documentation comment above each public declaration, matching the existing header style. State what the operation does and essential preconditions or output behavior. Do not duplicate the entire reference page in the header.
- Detailed API reference lives in `docs/lang/en-US/<library>/<topic>/index.md`. Keep the existing nested navigation structure, and update `docs/manifest.json` when adding or moving pages.
- Library overview pages describe headers, linking, types, common conventions, exports, and links to reference topics. Document shared behavior once and link to it.
- For each operation, use a heading with its exact C function name, a concise description, its exact C declaration in a fenced `c` block, a `Parameter | Description` table, and a `Status | Meaning` table. Document all applicable result codes and nonzero context, operation, and detail values, including argument indices and dependency stages. Use the existing concise notation (e.g. **Detail `0`:** Null output).
- Explicitly document units, required buffer capacity, nullability, ownership, failure-output behavior, and other observable guarantees wherever applicable. Do not promise behavior not supported by the implementation or omit a documented edge case.
- Use exact Catalyst terminology and identifiers. Keep documentation concise, factual, and focused on the API contract, not implementation narration. Do not change FDEFSPEC or silently reconcile specification discrepancies.

## Build and verification

- Preserve the existing shared CMake infrastructure, `cmakecfg.h` target configuration, and static/shared build targets. Initialize the `cmake` submodule recursively and use existing presets where available; do not modify the submodule, build configuration, or CI to work around a code issue without approval.
- Follow the existing standalone C++ test programs in `test/test_<library>.cpp`. Exercise the C++ wrapper and verify operation output and all four result fields. Cover normal cases, boundary values, no-ops, invalid arguments, overflow, and failure-state behavior as applicable. Use simple checks and nonzero exit codes; no additional testing framework is needed.
- Verify public C and C++ headers and library builds under the existing C90/C++98 and C99/C++11 configurations when possible. The current x86/x64 compile matrix does not prove 8-bit compatibility; independently review every operation for byte-only arithmetic and width assumptions. Do not claim 8-bit target support has been tested unless it actually has.
- Report what was tested, what could not be tested, and any discovered invariant conflicts. Do not silently fix pre-existing issues outside the approved scope.

## Commits

- Keep commits focused on the authorized task. Use concise imperative commit subjects, matching repository history.
- Include the existing AI co-author trailer on AI-assisted commits:

  `Co-authored-by: Codex <noreply@openai.com>`
