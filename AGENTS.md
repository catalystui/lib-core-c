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

`CATALYST_RESULT` / `RESULT` is four ordered bytes: status, context, operation, detail. Each has a distinct scope and purpose; never pack into a native integer or substitute exceptions/ad hoc conventions.

- **Status (universal):** Outcome, analogous to HTTP status. Use the most specific applicable FDEFSPEC code. Undefined codes are reserved; never add or redefine them without approval.
- **Context (unique per module):** Execution context, normally a stable ID identifying the method within its library/module. Locates the result's origin, not the internal failure stage. MUST be zero for success-level statuses (including no-op); otherwise use the applicable context ID.
- **Operation (unique per method):** Identifies the precise internal operation/result-reporting site, not the public method or a generic error category. Assign a different opcode to every distinct result assignment site within a method, including success paths; never reuse an opcode across sites even when their statuses or failure reasons match. A single site may report multiple detail values. Keep IDs stable and documented.
- **Detail (scoped to operation):** Additional useful information for the status/context/operation combination. May be symbolic, a literal byte value, an index, a count, or zero when nothing further is needed. Meaning precedence: status > context > operation; never override a higher-priority definition. Do not duplicate information already conveyed by other bytes or force runtime data into detail.
- `ERROR_INVALID_ARGUMENT` detail MUST be the zero-based argument index from the full public function signature (including by-value arguments). Zero is meaningful here.
- Define/document context IDs, opcodes, and applicable detail meanings consistently within their scopes before implementation. Example: UTF-8 decoder context identifies the method; each continuation check has its own opcode; detail may be zero or the offending byte if useful. Distinct missing-unit cases at one unexpected-end check may use distinct symbolic details.
- Dependency failures: retain the reporting method's context and exact call-site opcode; report the actual applicable status and useful detail without blindly copying dependency fields or defaulting to `ERROR_DEPENDENCY_FAILURE`. Preserve meaningful diagnostics and fatal severity when applicable; respect status-defined detail. Changes to existing result contracts require approval.
- Null `result` discards status. Otherwise assign all four fields on every path using `if (result != 0) *result = RESULT(...);`, defined status constants and severity helpers. Never invent information solely to avoid zero.

## Correctness

- Validate required pointers/parameters before access or output writes. Preserve documented validation order, failure-output behavior, and other per-function guarantees.
- Check overflow, capacity, bounds, allocation failures, malformed input, and dependency results. Preserve logical state on recoverable mutation failure where promised; clean up resources and report cleanup failures accurately.
- Distinguish bytes, code units, codepoints, terminators, and BOMs; preserve text buffer, substitution, and invalid-input contracts.
- Report existing implementation/reference conflicts with these requirements; do not treat them as precedent or fix them outside an approved task.

## Documentation

- Specifications define concepts; explanatory docs teach them; API references describe precise contracts for lookup. Keep headers, wrappers, implementations, references, and tests consistent with approved contracts.
- Each public declaration gets a short `/** ... */` comment describing behavior and essential preconditions/output behavior, not a duplicate reference.
- API Reference: `docs/lang/en-US/api-reference/index.md` and one comprehensive `api-reference/<module>/index.md` per module (CATALYST, CATMMLIB, CATTELIB, CATRLLIB); update `docs/manifest.json` for navigation changes. Module pages cover headers, linking, types, constants, conventions, exports, and all functions. Preserve explanatory docs separately. Define shared behavior once and link to it.
- Javadoc-inspired module structure: introduction with relevant header/type; **Function Summary** table of linked exact C function names and brief descriptions; **Function Details** with each exact function name as a heading, exact C signature in a fenced `c` block (multiline when long), natural description, **Parameters:** with individual names/descriptions (no parameter table), **Results:** table, and **See Also:** links when useful. Add Type/Constant Summary and Details when applicable; use **Returns:** for directly returned values, **Results:** for `CATALYST_RESULT` outputs. Reference current implementation and approved contracts; never document proposed result assignments as current behavior. For conditional definitions, use a standalone `<span data-heading-badge="MACRO" data-heading-badge-background="#b4232c" data-heading-badge-color="#ffffff" title="Requires MACRO to be 1"></span>` after the heading or its description; no separate REQUIRES paragraph. The renderer moves markers into the preceding heading without changing anchors. Other badges may specify their own background, text color, border (`data-heading-badge-border`), radius (`data-heading-badge-radius`), and tooltip (`title`). Where types differ by C version, use **C90:** / **C99+:** qualifiers. Link specifications to the live CatalystUI website.
- Results tables have exactly five columns: `Result | Status | Context | Operation | Detail`. One row per distinct result, including each fixed detail value. First column: complete hexadecimal `0xSSCCOODD` (uppercase digits, status–context–operation–detail order regardless of host endianness). Remaining columns explain each byte. Use an em dash only for truly unused fields, not meaningful zero values. Never group distinct fixed results in one row/cell.
- For genuinely variable fields, document exact byte ranges/patterns and meanings instead of inventing fixed codes or omitting values. Hex notation does not imply native integer packing.
- Explicitly document units, capacities, nullability, ownership, failure outputs, edge cases, and observable guarantees. Use exact Catalyst terms/identifiers, concise readable prose, and tables for lookup or relationships—not implementation narration. No unsupported promises, FDEFSPEC changes, or silent reconciliation of discrepancies.

## Build and verification

- Preserve shared CMake infrastructure, `cmakecfg.h`, and static/shared targets. Initialize `cmake` submodule recursively; use existing presets. Do not change submodule, build configuration, or CI to work around code issues without approval.
- Runtime binaries (executables/tests, Windows DLLs, Unix versioned shared libraries and ABI aliases) go directly in `debug/` or `release/`; development linker inputs (static archives, Windows import libraries and unversioned Unix linker aliases) go in its flat `lib/` directory. Unix runtime dependencies resolve relative to the binary; development aliases point to the runtime ABI aliases. No separate test/import-library directory. Preserve formal library names, headers, and CMake targets; artifact bases: `catmm`, `catte`, `catrl`. Shared `build-cmake` owns naming/versioning, derived from project metadata.
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
