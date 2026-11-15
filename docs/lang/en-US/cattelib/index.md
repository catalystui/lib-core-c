# Text encoding (CATTELIB)

Headers: `cattelib.h`, `cattelib.hpp`. Link CATTELIB. The codepoint, code-unit, encoding and string pointer types are defined in the [foundational definitions](../definitions/index.md).

## Conventions

- All functions return void; `result` may be null. All other pointer arguments require valid readable or writable storage.
- Encode/decode process one codepoint, including U+0000. They neither append a separate terminator nor skip a BOM.
- `produced` counts destination code units; `consumed` counts source code units. UTF-16LE counts two-byte code units, not bytes.
- Encoders require up to four UTF-8 units or two UTF-16LE units of writable storage. Decoders read the sequence width selected by the leading unit, even for malformed input. Supply that many readable units, including any padding needed beyond a truncated sequence. No input-size parameter or bounds discovery is provided.
- Length and conversion functions consume null-terminated input; lengths exclude the terminator. UTF-8 and UTF-16LE length/conversion functions skip an initial BOM (EF BB BF or FF FE), not internal U+FEFF.
- Use separate storage for input and output and for distinct output arguments. Null-argument errors occur before output writes.
- Result context is zero. Operation and detail are zero unless specified otherwise. ERROR_INVALID_ARGUMENT details are function-specific and are not always literal argument positions.


## ASCII

### catte_ascii_encode

```c
void catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw, RESULT* result);
```

Encodes one codepoint into one ASCII byte. Values outside U+0000–U+007F become '?'.

**Results:** SUCCESS for representable values; WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null asciiw.

### catte_ascii_decode

```c
void catte_ascii_decode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_decode(ASCII ascii, CODEPOINT* codepoint, RESULT* result);
```

Copies one ASCII byte into the least significant codepoint byte and zeroes the other three. The high bit is not validated.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null ascii, 1 = null codepoint.

### catte_ascii_length

```c
void catte_ascii_length(CATALYST_ASCII ascii, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_length(ASCII ascii, NUINT* length, RESULT* result);
```

Writes the number of nonzero bytes before the terminator to length, without validating ASCII values. Output is assigned only on success.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null ascii, 1 = null length; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.

## Windows-1252

### catte_cp1252_encode

```c
void catte_cp1252_encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_encode(const CODEPOINT codepoint, CP1252W cp1252w, RESULT* result);
```

Encodes one codepoint as a Windows-1252 byte. Unrepresentable values become '?'.

**Results:** SUCCESS for representable values; WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null cp1252w.

### catte_cp1252_decode

```c
void catte_cp1252_decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_decode(CP1252 cp1252, CODEPOINT* codepoint, RESULT* result);
```

Decodes one Windows-1252 byte into a codepoint. Undefined bytes 0x81, 0x8D, 0x8F, 0x90 and 0x9D become U+003F ('?').

**Results:** SUCCESS for defined bytes; WARNING_PARTIAL for undefined bytes; ERROR_INVALID_ARGUMENT detail 0 = null cp1252, 1 = null codepoint.

### catte_cp1252_length

```c
void catte_cp1252_length(CATALYST_CP1252 cp1252, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_length(CP1252 cp1252, NUINT* length, RESULT* result);
```

Writes the number of nonzero bytes before the terminator to length. Undefined CP1252 bytes are counted without warning. Output is assigned only on success.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null cp1252, 1 = null length; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.

## UTF-8

### catte_utf8_encode

```c
void catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into 1–4 UTF-8 bytes and writes their count to produced. Destination must have room for four units. After argument validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8w, 1 = null produced; ERROR operation 1 = overlong encoding branch (unreachable for valid input), 2 = surrogate, 3 = above U+10FFFF.

### catte_utf8_decode

```c
void catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-8 scalar and writes its width (1–4 units) to consumed. After argument validation, codepoint and consumed are zero on decoding failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8, 1 = null codepoint, 2 = null consumed; ERROR operation 1 = zero unit before sequence completion, 2 = invalid continuation, 3 = overlong sequence, 4 = surrogate, 5 = above U+10FFFF, 6 = invalid leading byte (including C0/C1).

### catte_utf8_length

```c
void catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_length(UTF8 utf8, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. This is not a byte or grapheme count. After pointer validation, length is zero on failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8, 1 = null length; ERROR operation 1 = decode failure; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.

## UTF-16LE

### catte_utf16le_encode

```c
void catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into one code unit or a surrogate pair and writes the count to produced. Destination must have room for two units. After pointer validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16lew, 1 = null produced; ERROR operation 1 = above U+10FFFF, 2 = surrogate codepoint.

### catte_utf16le_decode

```c
void catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-16LE unit or surrogate pair; consumed receives 1 or 2. After argument validation, codepoint and consumed are zero on decoding failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16le, 1 = null codepoint, 2 = null consumed; ERROR operation 1 = isolated low surrogate, 2 = high surrogate not followed by low surrogate.

### catte_utf16le_length

```c
void catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_length(UTF16LE utf16le, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. A surrogate pair counts as one. After pointer validation, length is zero on failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16le, 1 = null length; ERROR operation 1 = decode failure; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.

## String conversions

Conversions write a destination terminator on success or warning; no destination BOM is added. On error a written prefix may remain unterminated; there is no rollback or produced-length output. A warning means substituted output, not a failed conversion.

When a conversion reports ERROR_DEPENDENCY_FAILURE, operation 1 denotes decoding and operation 2 denotes encoding; detail is the dependency's status byte. The dependency's context, operation and detail are not propagated.


### catte_ascii_to_cp1252

```c
void catte_ascii_to_cp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_cp1252(ASCII ascii, CP1252W cp1252w, RESULT* result);
```

Converts a null-terminated ASCII string to Windows-1252. Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if the index reaches CATALYST_NUINT_MAX

### catte_ascii_to_utf8

```c
void catte_ascii_to_utf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_utf8(ASCII ascii, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated ASCII string to UTF-8. capacity specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator

### catte_ascii_to_utf16le

```c
void catte_ascii_to_utf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_utf16le(ASCII ascii, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated ASCII string to UTF-16LE. capacity specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator

### catte_cp1252_to_ascii

```c
void catte_cp1252_to_ascii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW ascii, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_ascii(CP1252 cp1252, ASCIIW ascii, RESULT* result);
```

Converts a null-terminated Windows-1252 string to ASCII. Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Undefined source bytes and values not representable in ASCII are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if the index reaches CATALYST_NUINT_MAX; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_cp1252_to_utf8

```c
void catte_cp1252_to_utf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_utf8(CP1252 cp1252, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated Windows-1252 string to UTF-8. capacity specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_cp1252_to_utf16le

```c
void catte_cp1252_to_utf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_utf16le(CP1252 cp1252, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated Windows-1252 string to UTF-16LE. capacity specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf8_to_ascii

```c
void catte_utf8_to_ascii(CATALYST_UTF8 utf8, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_ascii(UTF8 utf8, ASCIIW ascii, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to ASCII. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf8_to_cp1252

```c
void catte_utf8_to_cp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_cp1252(UTF8 utf8, CP1252W cp1252w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to Windows-1252. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf8_to_utf16le

```c
void catte_utf8_to_utf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_utf16le(UTF8 utf8, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to UTF-16LE. capacity specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf16le_to_ascii

```c
void catte_utf16le_to_ascii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_ascii(UTF16LE utf16le, ASCIIW ascii, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to ASCII. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf16le_to_cp1252

```c
void catte_utf16le_to_cp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_cp1252(UTF16LE utf16le, CP1252W cp1252w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to Windows-1252. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

### catte_utf16le_to_utf8

```c
void catte_utf16le_to_utf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_utf8(UTF16LE utf16le, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to UTF-8. capacity specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## API export definitions

`CATTELIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATTELIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATTELIB_USE_DLL` is defined. Otherwise it is empty.
