# AGENTS.md

Repository-wide requirements for CatalystUI Core, including implementation, documentation, tests, and builds.

## Authority and scope

- Core provides foundational, language-independent C ABI libraries based on FDEFSPEC (Foundational Definitions Specification), independent of CatalystUI layers, domains, operating systems, and languages.
- The owner defines architecture and contracts. Do not invent or reinterpret them. Before changing existing foundational code, APIs, representations, or behavior (including fixes, refactors, signatures, and new helpers), explain the change, necessity, and consequences and obtain explicit approval. Approval covers only the requested scope.
- Never modify or bypass Numeric Definitions, Codepoint, Text Encoding, or Operation Status to simplify another implementation without approval.
- Prefer simple, direct, explicit code; avoid speculative abstractions, unnecessary dependencies, convenience APIs, duplicated algorithms, and unrelated cleanup.
- `README.md` is for humans only; neither use it as development context nor edit it without explicit request. Consult specifications, headers, implementations, references, and tests.

## Mandatory 8-bit compatibility

Every Core library MUST be implementable and usable on an 8-bit system.

- All arithmetic, including intermediates, must require at most eight bits. Use `CATALYST_BYTE` / `BYTE` or `CATALYST_SBYTE` / `SBYTE`; represent larger logical values as ordered bytes. Implement carry, borrow, shifts, comparisons, multiplication, division, and overflow checks byte-by-byte; mask before shifting. Never rely on widened intermediates, including implicit C/C++ integer promotions.
- Never pack bytes into native wider integers for computation, comparison, shifts, encoding, or decoding, even through casts, unions, reinterpretation, intrinsics, library calls, or conditional fast paths. Core algorithms must not require native floating-point arithmetic.
- Preserve explicit byte order; assume no host endianness, alignment, pointer width, or native integer layout. Never serialize native structs/integers in place of specified byte representations.
- Reference: `src/cattelib.cpp` implements Codepoint and Text Encoding byte-wise. `CATALYST_CODEPOINT` uses MSB-first `byte0`–`byte3`; `CATALYST_UTF16LE_CODE_UNIT` uses `lsb`, then `msb`.
- Preserve the pointer-width ABI of `CATALYST_NINT` / `CATALYST_NUINT` for addresses, addressable sizes, counts, and indices, including one-byte pointers. Never use them to bypass byte arithmetic or assume a wider minimum. Report existing native-width arithmetic conflicts and obtain approval before changing them.
- Wider numeric types and existing guarded C++ native-integer constructors, assignments, and conversions are optional conveniences, not algorithm prerequisites. Preserve `CATALYST_SUPPORTS_*` guards; do not use these conveniences in Core algorithms, extend them, or add native conversion helpers without approval.
- Confine unavoidable native platform API types to platform backends. Obtain approval for new exceptions or resolutions of byte-arithmetic conflicts.

## Language and ABI

- Public C headers: C90. Implementations/wrappers: C++98. Newer standards are optional through existing guards.
- Preserve C ABI: `include/<library>.h`, `src/<library>.cpp`, thin inline `include/<library>.hpp` wrappers; retain include guards, `extern "C"`, and export macros.
- Use Catalyst types, not language-specific containers, exceptions, references, native booleans, or unguarded fixed-width types. Use `CATALYST_BOOL` / `BOOL` and `TRUE` / `FALSE`.
- Public operations return `void`, data via output parameters, status via final `CATALYST_RESULT* result`. C++ wrappers use `RESULT* result` and forward without duplicating logic.
- Preserve read/write pointer distinctions (`const`, e.g. `CATALYST_UTF8` vs. `CATALYST_UTF8W`), opaque handles with private implementation storage, ownership, lifetime, alignment, allocation, and buffer contracts.
- Use CATMMLIB for shared allocation/byte operations where appropriate. OS calls belong in `*.win32.cpp`, `*.linux.cpp`, or `*.darwin.mm`. No STL containers, heap abstractions, or external dependencies in portable Core without approval.

## Naming and style

- C types/constants: uppercase snake case, `CATALYST_` prefix. C functions: lowercase snake case, library prefix + subject + operation (e.g. `catmm_copy`, `catte_utf8_encode`, `catrl_array_create`). Reserve `cat_` for existing foundational patterns; new foundational functions require approval.
- C++ types/constants omit `CATALYST_`; functions omit library prefix, all in `namespace catalyst`. Public parameters/fields and generally private functions use lowercase snake case; existing descriptive lower-camel-case locals are acceptable. Match surrounding code; avoid unrelated renames.
- Four-space indent, same-line braces, `if (...) {` / `} else {`, null pointer `0`, C-compatible `typedef`, existing explicit casts (e.g. `(BYTE) 0x80`).
- Use early returns, direct control flow, named byte values, simple loops, and `static` for private helpers. Follow section dividers and concise `//` comments explaining intent, bit layouts, nonobvious conditions, invariants, and failure stages—not obvious assignments.
- Follow `.editorconfig` and `.gitattributes`: UTF-8, CRLF working-tree endings, final newline, no trailing whitespace.

## Operation results

`CATALYST_RESULT` / `RESULT` is four ordered bytes with distinct responsibilities, not a status plus interchangeable metadata. Never replace it with a packed native integer, exception, or ad hoc convention.

| Byte | Responsibility | Rule |
| --- | --- | --- |
| Status | What happened? | Most specific applicable FDEFSPEC status: success, no-op, warning, recoverable error, or fatal invariant violation. No new meanings/codes without approval. |
| Context | Under what circumstances? | Meaningful for warnings/errors/fatal results (validation, size calculation, handle/element allocation, cleanup, etc.). Do not routinely use zero. MUST be zero for success-level statuses, including no-op. |
| Operation | What public operation? | Stable, documented, nonzero operation identity on every path, including success/no-op. Never an internal stage or failure location; that belongs in context. No reuse for unrelated operations within the documented scope. |
| Detail | What additional information? | Meaning precedence: status > context > operation. Never override a higher-priority definition. Use meaningful extra information, otherwise zero; do not duplicate other bytes. |

- `ERROR_INVALID_ARGUMENT` detail MUST identify the zero-based parameter index in the public signature, including preceding input parameters. Never substitute native error numbers, dependency status, or internal argument indices. Zero can be meaningful.
- Define/document consistent scoped operation/context assignments and detail meanings before implementation; reuse meanings where appropriate. New foundational definitions and changes to existing contracts require approval.
- Dependency failures: report the actual applicable status, context of the failed stage, public operation identity, and public status-defined detail. Do not blindly copy dependency results or default to `ERROR_DEPENDENCY_FAILURE` (legacy behavior is not precedent). If the dependency status misrepresents the public outcome, use an appropriate existing status, including fatal invariant violation when warranted.
- Null `result` discards status. Otherwise write all four fields on every path using `if (result != 0) *result = RESULT(...);`, defined constants, and severity helpers.

## Correctness

- Validate required pointers/parameters before access or output writes. Preserve documented validation order, failure-output behavior, and other per-function guarantees.
- Check overflow, capacity, bounds, allocation failures, malformed input, and dependency results. Preserve logical state on recoverable mutation failure where promised; clean up resources and report cleanup failures accurately.
- Distinguish bytes, code units, codepoints, terminators, and BOMs; preserve text buffer, substitution, and invalid-input contracts.
- Report existing implementation/reference conflicts with these requirements; do not treat them as precedent or fix them outside an approved task.

## Documentation

- Specifications define concepts; explanatory docs teach them; API references describe precise contracts for lookup. Keep headers, wrappers, implementations, references, and tests consistent with approved contracts.
- Each public declaration gets a short `/** ... */` comment describing behavior and essential preconditions/output behavior, not a duplicate reference.
- Reference pages: `docs/lang/en-US/<library>/<topic>/index.md`; preserve nested navigation and update `docs/manifest.json` for additions/moves. Library overviews cover headers, linking, types, conventions, exports, and topic links. Define shared behavior once and link to it.
- Javadoc-inspired topic structure: introduction with relevant header/type; **Function Summary** table of linked exact C function names and brief descriptions; **Function Details** with each exact function name as a heading, exact C signature in a fenced `c` block (multiline when long), natural description, **Parameters:** with individual names/descriptions (no parameter table), **Results:** table, and **See Also:** links when useful.
- Results tables have exactly five columns: `Result | Status | Context | Operation | Detail`. One row per distinct result, including each fixed detail value. First column: complete hexadecimal `0xSSCCOODD` (uppercase digits, status–context–operation–detail order regardless of host endianness). Remaining columns explain each byte. Use an em dash only for truly unused fields, not meaningful zero values. Never group distinct fixed results in one row/cell.
- For genuinely variable fields, document exact byte ranges/patterns and meanings instead of inventing fixed codes or omitting values. Hex notation does not imply native integer packing.
- Explicitly document units, capacities, nullability, ownership, failure outputs, edge cases, and observable guarantees. Use exact Catalyst terms/identifiers, concise readable prose, and tables for lookup or relationships—not implementation narration. No unsupported promises, FDEFSPEC changes, or silent reconciliation of discrepancies.

## Build and verification

- Preserve shared CMake infrastructure, `cmakecfg.h`, and static/shared targets. Initialize `cmake` submodule recursively; use existing presets. Do not change submodule, build configuration, or CI to work around code issues without approval.
- Executables/tests go directly in `debug/` or `release/`; all library artifacts and symlinks go in its flat `lib/` directory. No separate test/import-library directory. Preserve formal library names, headers, and CMake targets; artifact bases: `catmm`, `catte`, `catrl`. Shared `build-cmake` owns naming/versioning, derived from project metadata.
- Example v1.5.2 (`catmm`):
  - MSVC: `libcatmm.lib`, `catmm.lib`, `catmm15.dll` (full version in DLL resource).
  - Linux: `libcatmm.a`, `libcatmm.so`, `libcatmm.so.1.5`, `libcatmm.so.1.5.2`.
  - macOS: `libcatmm.a`, `libcatmm.dylib`, `libcatmm.1.5.dylib`, `libcatmm.1.5.2.dylib`.
  - Unix linker/ABI aliases are symlinks.
- Use existing standalone `test/test_<library>.cpp` programs, exercising C++ wrappers and checking outputs and all four result bytes. Cover normal, boundary, no-op, invalid, overflow, and failure-state cases as applicable; simple checks/nonzero exits, no new test framework.
- Verify public headers/builds with C90/C++98 and C99/C++11 when possible. Independently review byte arithmetic and width assumptions; x86/x64 builds do not establish 8-bit compatibility. Never claim actual 8-bit testing unless performed.
- Report tests performed, limitations, and invariant conflicts.

## Commits

Keep commits focused on authorized changes, with concise imperative subjects matching history. AI-assisted commits include:

`Co-authored-by: Codex <noreply@openai.com>`
