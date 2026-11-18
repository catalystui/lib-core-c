# Text encoding (CATTELIB)

Headers: `cattelib.h`, `cattelib.hpp`. Link CATTELIB. The codepoint, code-unit, encoding and string pointer types are defined in the [foundational definitions](../definitions/index.md).

## Conventions

- All functions return `CATALYST_RESULT` by value. All pointer arguments require valid readable or writable storage.
- Encode/decode process one codepoint, including U+0000. They neither append a separate terminator nor skip a BOM.
- `produced` counts destination code units; `consumed` counts source code units. UTF-16LE counts two-byte code units, not bytes.
- Encoders require up to four UTF-8 units or two UTF-16LE units of writable storage. Decoders read the sequence width selected by the leading unit, even for malformed input. Supply that many readable units, including any padding needed beyond a truncated sequence. No input-size parameter or bounds discovery is provided.
- Length and conversion functions consume null-terminated input; lengths exclude the terminator. UTF-8 and UTF-16LE length/conversion functions skip an initial BOM (EF BB BF or FF FE), not internal U+FEFF.
- Use separate storage for input and output and for distinct output arguments. Null-argument errors occur before output writes.
- Result context is zero. Operation and detail are zero unless specified otherwise. ERROR_INVALID_ARGUMENT details are function-specific and are not always literal argument positions.


## API export definitions

`CATTELIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATTELIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATTELIB_USE_DLL` is defined. Otherwise it is empty.

## Reference

- [ASCII](ascii/index.md)
- [Windows-1252](cp1252/index.md)
- [UTF-8](utf8/index.md)
- [UTF-16LE](utf16le/index.md)
- [String conversions](conversions/index.md)
