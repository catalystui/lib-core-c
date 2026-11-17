# CATTELIB

Provides encoding, decoding, length, and conversion operations for ASCII, Windows-1252, UTF-8, and UTF-16LE.

**C Header:** `cattelib.h`  
**C++ Header:** `cattelib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catte`

## Conventions

- All functions return void; `result` may be null. All other pointer arguments require valid readable or writable storage.
- Encode/decode process one codepoint, including U+0000. They neither append a separate terminator nor skip a BOM.
- `produced` counts destination code units; `consumed` counts source code units. UTF-16LE counts two-byte code units, not bytes.
- Encoders require up to four UTF-8 units or two UTF-16LE units of writable storage. Decoders read the sequence width selected by the leading unit, even for malformed input. Supply that many readable units, including any padding needed beyond a truncated sequence. No input-size parameter or bounds discovery is provided.
- Length and conversion functions consume null-terminated input; lengths exclude the terminator. UTF-8 and UTF-16LE length/conversion functions skip an initial BOM (EF BB BF or FF FE), not internal U+FEFF.
- Use separate storage for input and output and for distinct output arguments. Null-argument errors occur before output writes.

Defensive failure paths are included even when unreachable with valid input and intact invariants. When an internal call fails, detail identifies its operation code unless a status-specific detail takes precedence.

## Constant Summary

| Constant | Description |
| --- | --- |
| [`CATTELIB_API`](#cattelib_api) | Export/import annotation. |

## Function Summary

### ASCII

| Function | Description |
| --- | --- |
| [`catte_ascii_encode`](#catte_ascii_encode) | Encodes one codepoint as ASCII. |
| [`catte_ascii_decode`](#catte_ascii_decode) | Decodes a single ASCII code unit into a codepoint. |
| [`catte_ascii_length`](#catte_ascii_length) | Counts the code units in a null-terminated ASCII string, excluding the terminator. |

### Windows-1252

| Function | Description |
| --- | --- |
| [`catte_cp1252_encode`](#catte_cp1252_encode) | Encodes one codepoint as Windows-1252. |
| [`catte_cp1252_decode`](#catte_cp1252_decode) | Decodes one Windows-1252 code unit. |
| [`catte_cp1252_length`](#catte_cp1252_length) | Counts the code units in a null-terminated Windows-1252 string, excluding the terminator. |

### UTF-8

| Function | Description |
| --- | --- |
| [`catte_utf8_encode`](#catte_utf8_encode) | Encodes a Unicode scalar as one to four UTF-8 code units. |
| [`catte_utf8_decode`](#catte_utf8_decode) | Decodes one Unicode scalar from UTF-8 and reports the number of code units consumed (1–4). |
| [`catte_utf8_length`](#catte_utf8_length) | Counts Unicode scalars in a null-terminated UTF-8 string, excluding the terminator and any leading BOM. |

### UTF-16LE

| Function | Description |
| --- | --- |
| [`catte_utf16le_encode`](#catte_utf16le_encode) | Encodes a Unicode scalar as one UTF-16LE code unit or a surrogate pair. |
| [`catte_utf16le_decode`](#catte_utf16le_decode) | Decodes one Unicode scalar from UTF-16LE and reports the number of code units consumed (1 or 2). |
| [`catte_utf16le_length`](#catte_utf16le_length) | Counts Unicode scalars in a null-terminated UTF-16LE string, excluding the terminator and any leading BOM. |

### String conversions

| Function | Description |
| --- | --- |
| [`catte_ascii_to_cp1252`](#catte_ascii_to_cp1252) | Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. |
| [`catte_ascii_to_utf8`](#catte_ascii_to_utf8) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_ascii_to_utf16le`](#catte_ascii_to_utf16le) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_cp1252_to_ascii`](#catte_cp1252_to_ascii) | Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. |
| [`catte_cp1252_to_utf8`](#catte_cp1252_to_utf8) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_cp1252_to_utf16le`](#catte_cp1252_to_utf16le) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf8_to_ascii`](#catte_utf8_to_ascii) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf8_to_cp1252`](#catte_utf8_to_cp1252) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf8_to_utf16le`](#catte_utf8_to_utf16le) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf16le_to_ascii`](#catte_utf16le_to_ascii) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf16le_to_cp1252`](#catte_utf16le_to_cp1252) | `capacity` specifies writable destination code units, including the terminator. |
| [`catte_utf16le_to_utf8`](#catte_utf16le_to_utf8) | `capacity` specifies writable destination code units, including the terminator. |

## Constant Details

### CATTELIB_API

On Windows/Cygwin, expands to `__declspec(dllexport)` with `CATTELIB_EXPORTS`, or `__declspec(dllimport)` with `CATTELIB_USE_DLL`. Otherwise empty.

## Function Details

### ASCII

#### catte_ascii_encode

```c
void catte_ascii_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_ASCIIW asciiw,
    CATALYST_RESULT* result
);
```

Encodes one codepoint as ASCII. Codepoints outside U+0000–U+007F are replaced with `?`.

**Parameters:**

`codepoint` — Codepoint to encode.

`asciiw` — Writable buffer for one code unit.

`result` — Optional result output.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | Encode NUL | — |
| `0x00000400` | `SUCCESS` | Encode ASCII codepoint | — |
| `0x41010200` | `WARNING_PARTIAL` | Substitute unrepresentable codepoint | — |
| `0x81010101` | `ERROR_INVALID_ARGUMENT` | Validate `asciiw` | Argument 1 (`asciiw`) |

**C++:** `catalyst::ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw, RESULT* result)`.


#### catte_ascii_decode

```c
void catte_ascii_decode(
    CATALYST_ASCII ascii,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_RESULT* result
);
```

Decodes a single ASCII code unit into a codepoint. The input byte becomes the least significant codepoint byte; the remaining bytes are zero. The ASCII range is not validated.

**Parameters:**

`ascii` — Readable input containing one code unit.

`codepoint` — Receives the decoded codepoint.

`result` — Optional result output.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | Decode code unit | — |
| `0x81020100` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 0 (`ascii`) |
| `0x81020201` | `ERROR_INVALID_ARGUMENT` | Validate `codepoint` | Argument 1 (`codepoint`) |

**C++:** `catalyst::ascii_decode(ASCII ascii, CODEPOINT* codepoint, RESULT* result)`.


#### catte_ascii_length

```c
void catte_ascii_length(
    CATALYST_ASCII ascii,
    CATALYST_NUINT* length,
    CATALYST_RESULT* result
);
```

Counts the code units in a null-terminated ASCII string, excluding the terminator. ASCII values are not validated. `length` is written only on success.

**Parameters:**

`ascii` — Readable null-terminated input.

`length` — Receives the number of code units before the terminator.

`result` — Optional result output.

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | Write length | — |
| `0x81030100` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 0 (`ascii`) |
| `0x81030201` | `ERROR_INVALID_ARGUMENT` | Validate `length` | Argument 1 (`length`) |
| `0x90030300` | `ERROR_BUFFER_OVERFLOW` | Check count limit | — |

**C++:** `catalyst::ascii_length(ASCII ascii, NUINT* length, RESULT* result)`.


### Windows-1252

#### catte_cp1252_encode

```c
void catte_cp1252_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_CP1252W cp1252w,
    CATALYST_RESULT* result
);
```

Encodes one codepoint as Windows-1252. Unrepresentable codepoints are replaced with `?`.

**Parameters:**

`codepoint` — Codepoint to encode.

`cp1252w` — Writable buffer for one code unit.

`result` — Optional result output.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | Encode directly mapped codepoint | — |
| `0x00000300` | `SUCCESS` | Encode U+0152 as 0x8C | — |
| `0x00000400` | `SUCCESS` | Encode U+0153 as 0x9C | — |
| `0x00000500` | `SUCCESS` | Encode U+0160 as 0x8A | — |
| `0x00000600` | `SUCCESS` | Encode U+0161 as 0x9A | — |
| `0x00000700` | `SUCCESS` | Encode U+0178 as 0x9F | — |
| `0x00000800` | `SUCCESS` | Encode U+017D as 0x8E | — |
| `0x00000900` | `SUCCESS` | Encode U+017E as 0x9E | — |
| `0x00000A00` | `SUCCESS` | Encode U+0192 as 0x83 | — |
| `0x00000B00` | `SUCCESS` | Encode U+02C6 as 0x88 | — |
| `0x00000C00` | `SUCCESS` | Encode U+02DC as 0x98 | — |
| `0x00000D00` | `SUCCESS` | Encode U+2013 as 0x96 | — |
| `0x00000E00` | `SUCCESS` | Encode U+2014 as 0x97 | — |
| `0x00000F00` | `SUCCESS` | Encode U+2018 as 0x91 | — |
| `0x00001000` | `SUCCESS` | Encode U+2019 as 0x92 | — |
| `0x00001100` | `SUCCESS` | Encode U+201A as 0x82 | — |
| `0x00001200` | `SUCCESS` | Encode U+201C as 0x93 | — |
| `0x00001300` | `SUCCESS` | Encode U+201D as 0x94 | — |
| `0x00001400` | `SUCCESS` | Encode U+201E as 0x84 | — |
| `0x00001500` | `SUCCESS` | Encode U+2020 as 0x86 | — |
| `0x00001600` | `SUCCESS` | Encode U+2021 as 0x87 | — |
| `0x00001700` | `SUCCESS` | Encode U+2022 as 0x95 | — |
| `0x00001800` | `SUCCESS` | Encode U+2026 as 0x85 | — |
| `0x00001900` | `SUCCESS` | Encode U+2030 as 0x89 | — |
| `0x00001A00` | `SUCCESS` | Encode U+2039 as 0x8B | — |
| `0x00001B00` | `SUCCESS` | Encode U+203A as 0x9B | — |
| `0x00001C00` | `SUCCESS` | Encode U+20AC as 0x80 | — |
| `0x00001D00` | `SUCCESS` | Encode U+2122 as 0x99 | — |
| `0x41041E00` | `WARNING_PARTIAL` | Substitute unrepresentable codepoint | — |
| `0x81040101` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252w` | Argument 1 (`cp1252w`) |

**C++:** `catalyst::cp1252_encode(const CODEPOINT codepoint, CP1252W cp1252w, RESULT* result)`.


#### catte_cp1252_decode

```c
void catte_cp1252_decode(
    CATALYST_CP1252 cp1252,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_RESULT* result
);
```

Decodes one Windows-1252 code unit. Undefined bytes (`0x81`, `0x8D`, `0x8F`, `0x90`, `0x9D`) are replaced with U+003F (`?`).

**Parameters:**

`cp1252` — Readable input containing one code unit.

`codepoint` — Receives the decoded codepoint.

`result` — Optional result output.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | Decode directly mapped code unit | — |
| `0x00000400` | `SUCCESS` | Decode 0x80 as U+20AC | — |
| `0x00000500` | `SUCCESS` | Decode 0x82 as U+201A | — |
| `0x00000600` | `SUCCESS` | Decode 0x83 as U+0192 | — |
| `0x00000700` | `SUCCESS` | Decode 0x84 as U+201E | — |
| `0x00000800` | `SUCCESS` | Decode 0x85 as U+2026 | — |
| `0x00000900` | `SUCCESS` | Decode 0x86 as U+2020 | — |
| `0x00000A00` | `SUCCESS` | Decode 0x87 as U+2021 | — |
| `0x00000B00` | `SUCCESS` | Decode 0x88 as U+02C6 | — |
| `0x00000C00` | `SUCCESS` | Decode 0x89 as U+2030 | — |
| `0x00000D00` | `SUCCESS` | Decode 0x8A as U+0160 | — |
| `0x00000E00` | `SUCCESS` | Decode 0x8B as U+2039 | — |
| `0x00000F00` | `SUCCESS` | Decode 0x8C as U+0152 | — |
| `0x00001000` | `SUCCESS` | Decode 0x8E as U+017D | — |
| `0x00001100` | `SUCCESS` | Decode 0x91 as U+2018 | — |
| `0x00001200` | `SUCCESS` | Decode 0x92 as U+2019 | — |
| `0x00001300` | `SUCCESS` | Decode 0x93 as U+201C | — |
| `0x00001400` | `SUCCESS` | Decode 0x94 as U+201D | — |
| `0x00001500` | `SUCCESS` | Decode 0x95 as U+2022 | — |
| `0x00001600` | `SUCCESS` | Decode 0x96 as U+2013 | — |
| `0x00001700` | `SUCCESS` | Decode 0x97 as U+2014 | — |
| `0x00001800` | `SUCCESS` | Decode 0x98 as U+02DC | — |
| `0x00001900` | `SUCCESS` | Decode 0x99 as U+2122 | — |
| `0x00001A00` | `SUCCESS` | Decode 0x9A as U+0161 | — |
| `0x00001B00` | `SUCCESS` | Decode 0x9B as U+203A | — |
| `0x00001C00` | `SUCCESS` | Decode 0x9C as U+0153 | — |
| `0x00001D00` | `SUCCESS` | Decode 0x9E as U+017E | — |
| `0x00001E00` | `SUCCESS` | Decode 0x9F as U+0178 | — |
| `0x41051F00` | `WARNING_PARTIAL` | Substitute undefined code unit | — |
| `0x81050100` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252` | Argument 0 (`cp1252`) |
| `0x81050201` | `ERROR_INVALID_ARGUMENT` | Validate `codepoint` | Argument 1 (`codepoint`) |

**C++:** `catalyst::cp1252_decode(CP1252 cp1252, CODEPOINT* codepoint, RESULT* result)`.


#### catte_cp1252_length

```c
void catte_cp1252_length(
    CATALYST_CP1252 cp1252,
    CATALYST_NUINT* length,
    CATALYST_RESULT* result
);
```

Counts the code units in a null-terminated Windows-1252 string, excluding the terminator. Undefined bytes are counted without warning. `length` is written only on success.

**Parameters:**

`cp1252` — Readable null-terminated input.

`length` — Receives the number of code units before the terminator.

`result` — Optional result output.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | Write length | — |
| `0x81060100` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252` | Argument 0 (`cp1252`) |
| `0x81060201` | `ERROR_INVALID_ARGUMENT` | Validate `length` | Argument 1 (`length`) |
| `0x90060300` | `ERROR_BUFFER_OVERFLOW` | Check count limit | — |

**C++:** `catalyst::cp1252_length(CP1252 cp1252, NUINT* length, RESULT* result)`.


### UTF-8

#### catte_utf8_encode

```c
void catte_utf8_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT* produced,
    CATALYST_RESULT* result
);
```

Encodes a Unicode scalar as one to four UTF-8 code units. The destination must accommodate four units. After argument validation, `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

**Parameters:**

`codepoint` — Codepoint to encode.

`utf8w` — Writable buffer for up to four code units.

`produced` — Receives the number of code units written.

`result` — Optional result output.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | Encode one-byte sequence | — |
| `0x00000500` | `SUCCESS` | Encode two-byte sequence | — |
| `0x00000800` | `SUCCESS` | Encode three-byte sequence | — |
| `0x00000900` | `SUCCESS` | Encode four-byte sequence | — |
| `0x81070101` | `ERROR_INVALID_ARGUMENT` | Validate `utf8w` | Argument 1 (`utf8w`) |
| `0x81070202` | `ERROR_INVALID_ARGUMENT` | Validate `produced` | Argument 2 (`produced`) |
| `0x81070700` | `ERROR_INVALID_ARGUMENT` | Reject surrogate codepoint | Argument 0 (`codepoint`) |
| `0x81070A00` | `ERROR_INVALID_ARGUMENT` | Reject codepoint outside Unicode range | Argument 0 (`codepoint`) |
| `0xC1070400` | `FATAL_INVARIANT_VIOLATION` | Check two-byte encoding invariant (defensive) | — |
| `0xC1070600` | `FATAL_INVARIANT_VIOLATION` | Check three-byte encoding invariant (defensive) | — |

**C++:** `catalyst::utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced, RESULT* result)`.


#### catte_utf8_decode

```c
void catte_utf8_decode(
    CATALYST_UTF8 utf8,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_NUINT* consumed,
    CATALYST_RESULT* result
);
```

Decodes one Unicode scalar from UTF-8 and reports the number of code units consumed (1–4). After argument validation, both `codepoint` and `consumed` are zero on decoding failure.

**Parameters:**

`utf8` — Readable input containing the complete encoded sequence.

`codepoint` — Receives the decoded codepoint.

`consumed` — Receives the number of code units read.

`result` — Optional result output.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | Decode NUL | — |
| `0x00000500` | `SUCCESS` | Decode one-byte sequence | — |
| `0x00000800` | `SUCCESS` | Decode two-byte sequence | — |
| `0x00000E00` | `SUCCESS` | Decode three-byte sequence | — |
| `0x00001500` | `SUCCESS` | Decode four-byte sequence | — |
| `0x81080100` | `ERROR_INVALID_ARGUMENT` | Validate `utf8` | Argument 0 (`utf8`) |
| `0x81080201` | `ERROR_INVALID_ARGUMENT` | Validate `codepoint` | Argument 1 (`codepoint`) |
| `0x81080302` | `ERROR_INVALID_ARGUMENT` | Validate `consumed` | Argument 2 (`consumed`) |
| `0x83080600` | `ERROR_MALFORMED_INPUT` | Two-byte sequence: unexpected end | — |
| `0x83080700` | `ERROR_MALFORMED_INPUT` | Two-byte sequence: invalid continuation | — |
| `0x83080901` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: unexpected end | First zero continuation at index 1 |
| `0x83080902` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: unexpected end | First zero continuation at index 2 |
| `0x83080A00` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: invalid first continuation | — |
| `0x83080B00` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: invalid second continuation | — |
| `0x83080C00` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: overlong encoding | — |
| `0x83080D00` | `ERROR_MALFORMED_INPUT` | Three-byte sequence: surrogate codepoint | — |
| `0x83080F01` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: unexpected end | First zero continuation at index 1 |
| `0x83080F02` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: unexpected end | First zero continuation at index 2 |
| `0x83080F03` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: unexpected end | First zero continuation at index 3 |
| `0x83081000` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: invalid first continuation | — |
| `0x83081100` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: invalid second continuation | — |
| `0x83081200` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: invalid third continuation | — |
| `0x83081300` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: overlong encoding | — |
| `0x83081400` | `ERROR_MALFORMED_INPUT` | Four-byte sequence: above U+10FFFF | — |
| `0x83081600` | `ERROR_MALFORMED_INPUT` | Invalid leading code unit | — |

**C++:** `catalyst::utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed, RESULT* result)`.


#### catte_utf8_length

```c
void catte_utf8_length(
    CATALYST_UTF8 utf8,
    CATALYST_NUINT* length,
    CATALYST_RESULT* result
);
```

Counts Unicode scalars in a null-terminated UTF-8 string, excluding the terminator and any leading BOM. This is not a byte or grapheme count. After argument validation, `length` is zero on failure.

**Parameters:**

`utf8` — Readable null-terminated input.

`length` — Receives the number of Unicode scalars.

`result` — Optional result output.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | Write length | — |
| `0x81090100` | `ERROR_INVALID_ARGUMENT` | Validate `utf8` | Argument 0 (`utf8`) |
| `0x81090201` | `ERROR_INVALID_ARGUMENT` | Validate `length` | Argument 1 (`length`) |
| `0x83090306` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x06`: Two-byte sequence: unexpected end |
| `0x83090307` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x07`: Two-byte sequence: invalid continuation |
| `0x83090309` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x09`: Three-byte sequence: unexpected end |
| `0x8309030A` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x0A`: Three-byte sequence: invalid first continuation |
| `0x8309030B` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x0B`: Three-byte sequence: invalid second continuation |
| `0x8309030C` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x0C`: Three-byte sequence: overlong encoding |
| `0x8309030D` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x0D`: Three-byte sequence: surrogate codepoint |
| `0x8309030F` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x0F`: Four-byte sequence: unexpected end |
| `0x83090310` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x10`: Four-byte sequence: invalid first continuation |
| `0x83090311` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x11`: Four-byte sequence: invalid second continuation |
| `0x83090312` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x12`: Four-byte sequence: invalid third continuation |
| `0x83090313` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x13`: Four-byte sequence: overlong encoding |
| `0x83090314` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x14`: Four-byte sequence: above U+10FFFF |
| `0x83090316` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf8_decode` operation `0x16`: Invalid leading code unit |
| `0x90090400` | `ERROR_BUFFER_OVERFLOW` | Check count limit | — |
| `0xC1090301` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf8_decode` operation `0x01`: Validate `utf8` (internal argument failure) |
| `0xC1090302` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf8_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1090303` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf8_decode` operation `0x03`: Validate `consumed` (internal argument failure) |

**C++:** `catalyst::utf8_length(UTF8 utf8, NUINT* length, RESULT* result)`.


### UTF-16LE

#### catte_utf16le_encode

```c
void catte_utf16le_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT* produced,
    CATALYST_RESULT* result
);
```

Encodes a Unicode scalar as one UTF-16LE code unit or a surrogate pair. The destination must accommodate two units. After argument validation, `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

**Parameters:**

`codepoint` — Codepoint to encode.

`utf16lew` — Writable buffer for up to two code units.

`produced` — Receives the number of code units written.

`result` — Optional result output.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | Encode one code unit | — |
| `0x00000600` | `SUCCESS` | Encode surrogate pair | — |
| `0x810A0101` | `ERROR_INVALID_ARGUMENT` | Validate `utf16lew` | Argument 1 (`utf16lew`) |
| `0x810A0202` | `ERROR_INVALID_ARGUMENT` | Validate `produced` | Argument 2 (`produced`) |
| `0x810A0300` | `ERROR_INVALID_ARGUMENT` | Reject codepoint above U+10FFFF | Argument 0 (`codepoint`) |
| `0x810A0400` | `ERROR_INVALID_ARGUMENT` | Reject surrogate codepoint | Argument 0 (`codepoint`) |

**C++:** `catalyst::utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced, RESULT* result)`.


#### catte_utf16le_decode

```c
void catte_utf16le_decode(
    CATALYST_UTF16LE utf16le,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_NUINT* consumed,
    CATALYST_RESULT* result
);
```

Decodes one Unicode scalar from UTF-16LE and reports the number of code units consumed (1 or 2). After argument validation, both `codepoint` and `consumed` are zero on decoding failure.

**Parameters:**

`utf16le` — Readable input containing the complete encoded sequence.

`codepoint` — Receives the decoded codepoint.

`consumed` — Receives the number of code units read.

`result` — Optional result output.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | Decode NUL | — |
| `0x00000600` | `SUCCESS` | Decode one code unit | — |
| `0x00000800` | `SUCCESS` | Decode surrogate pair | — |
| `0x810B0100` | `ERROR_INVALID_ARGUMENT` | Validate `utf16le` | Argument 0 (`utf16le`) |
| `0x810B0201` | `ERROR_INVALID_ARGUMENT` | Validate `codepoint` | Argument 1 (`codepoint`) |
| `0x810B0302` | `ERROR_INVALID_ARGUMENT` | Validate `consumed` | Argument 2 (`consumed`) |
| `0x830B0500` | `ERROR_MALFORMED_INPUT` | Reject isolated low surrogate | — |
| `0x830B0700` | `ERROR_MALFORMED_INPUT` | Reject missing low surrogate | — |

**C++:** `catalyst::utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed, RESULT* result)`.


#### catte_utf16le_length

```c
void catte_utf16le_length(
    CATALYST_UTF16LE utf16le,
    CATALYST_NUINT* length,
    CATALYST_RESULT* result
);
```

Counts Unicode scalars in a null-terminated UTF-16LE string, excluding the terminator and any leading BOM. A surrogate pair counts as one scalar. After argument validation, `length` is zero on failure.

**Parameters:**

`utf16le` — Readable null-terminated input.

`length` — Receives the number of Unicode scalars.

`result` — Optional result output.

**Context:** `0x0C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | Write length | — |
| `0x810C0100` | `ERROR_INVALID_ARGUMENT` | Validate `utf16le` | Argument 0 (`utf16le`) |
| `0x810C0201` | `ERROR_INVALID_ARGUMENT` | Validate `length` | Argument 1 (`length`) |
| `0x830C0305` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x830C0307` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x900C0400` | `ERROR_BUFFER_OVERFLOW` | Check count limit | — |
| `0xC10C0301` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf16le_decode` operation `0x01`: Validate `utf16le` (internal argument failure) |
| `0xC10C0302` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf16le_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC10C0303` | `FATAL_INVARIANT_VIOLATION` | Decode next codepoint | `catte_utf16le_decode` operation `0x03`: Validate `consumed` (internal argument failure) |

**C++:** `catalyst::utf16le_length(UTF16LE utf16le, NUINT* length, RESULT* result)`.


### String conversions

Conversions write a destination terminator on success or warning; no destination BOM is added. On failure a written prefix may remain unterminated. There is no rollback or produced-length output.

#### catte_ascii_to_cp1252

```c
void catte_ascii_to_cp1252(
    CATALYST_ASCII ascii,
    CATALYST_CP1252W cp1252w,
    CATALYST_RESULT* result
);
```

Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.

`cp1252w` — Writable destination buffer.

`result` — Optional result output.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410D0400` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x810D0100` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 0 (`ascii`) |
| `0x810D0201` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252w` | Argument 1 (`cp1252w`) |
| `0x900D0300` | `ERROR_BUFFER_OVERFLOW` | Check index limit | — |

**C++:** `catalyst::ascii_to_cp1252(ASCII ascii, CP1252W cp1252w, RESULT* result)`.


#### catte_ascii_to_utf8

```c
void catte_ascii_to_utf8(
    CATALYST_ASCII ascii,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.

`utf8w` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410E0500` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x810E0100` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 0 (`ascii`) |
| `0x810E0201` | `ERROR_INVALID_ARGUMENT` | Validate `utf8w` | Argument 1 (`utf8w`) |
| `0x900E0300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x900E0400` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

**C++:** `catalyst::ascii_to_utf8(ASCII ascii, UTF8W utf8w, NUINT capacity, RESULT* result)`.


#### catte_ascii_to_utf16le

```c
void catte_ascii_to_utf16le(
    CATALYST_ASCII ascii,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.

`utf16lew` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410F0500` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x810F0100` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 0 (`ascii`) |
| `0x810F0201` | `ERROR_INVALID_ARGUMENT` | Validate `utf16lew` | Argument 1 (`utf16lew`) |
| `0x900F0300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x900F0400` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

**C++:** `catalyst::ascii_to_utf16le(ASCII ascii, UTF16LEW utf16lew, NUINT capacity, RESULT* result)`.


#### catte_cp1252_to_ascii

```c
void catte_cp1252_to_ascii(
    CATALYST_CP1252 cp1252,
    CATALYST_ASCIIW ascii,
    CATALYST_RESULT* result
);
```

Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Undefined source bytes and values not representable in ASCII are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.

`ascii` — Writable destination buffer.

`result` — Optional result output.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000600` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41100600` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81100100` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252` | Argument 0 (`cp1252`) |
| `0x81100201` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 1 (`ascii`) |
| `0x90100500` | `ERROR_BUFFER_OVERFLOW` | Check index limit | — |
| `0xC1100301` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x01`: Validate `cp1252` (internal argument failure) |
| `0xC1100302` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1100401` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_ascii_encode` operation `0x01`: Validate `asciiw` (internal argument failure) |

**C++:** `catalyst::cp1252_to_ascii(CP1252 cp1252, ASCIIW ascii, RESULT* result)`.


#### catte_cp1252_to_utf8

```c
void catte_cp1252_to_utf8(
    CATALYST_CP1252 cp1252,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Undefined source bytes are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.

`utf8w` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41110700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81110100` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252` | Argument 0 (`cp1252`) |
| `0x81110201` | `ERROR_INVALID_ARGUMENT` | Validate `utf8w` | Argument 1 (`utf8w`) |
| `0x90110300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90110600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1110401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x01`: Validate `cp1252` (internal argument failure) |
| `0xC1110402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1110501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x01`: Validate `utf8w` (internal argument failure) |
| `0xC1110502` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x02`: Validate `produced` (internal argument failure) |
| `0xC1110504` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x04`: Check two-byte encoding invariant (defensive) |
| `0xC1110506` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x06`: Check three-byte encoding invariant (defensive) |
| `0xC1110507` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x07`: Reject surrogate codepoint (internal argument failure) |
| `0xC111050A` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x0A`: Reject codepoint outside Unicode range (internal argument failure) |

**C++:** `catalyst::cp1252_to_utf8(CP1252 cp1252, UTF8W utf8w, NUINT capacity, RESULT* result)`.


#### catte_cp1252_to_utf16le

```c
void catte_cp1252_to_utf16le(
    CATALYST_CP1252 cp1252,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Undefined source bytes are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.

`utf16lew` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41120700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81120100` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252` | Argument 0 (`cp1252`) |
| `0x81120201` | `ERROR_INVALID_ARGUMENT` | Validate `utf16lew` | Argument 1 (`utf16lew`) |
| `0x90120300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90120600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1120401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x01`: Validate `cp1252` (internal argument failure) |
| `0xC1120402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_cp1252_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1120501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x01`: Validate `utf16lew` (internal argument failure) |
| `0xC1120502` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x02`: Validate `produced` (internal argument failure) |
| `0xC1120503` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x03`: Reject codepoint above U+10FFFF (internal argument failure) |
| `0xC1120504` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x04`: Reject surrogate codepoint (internal argument failure) |

**C++:** `catalyst::cp1252_to_utf16le(CP1252 cp1252, UTF16LEW utf16lew, NUINT capacity, RESULT* result)`.


#### catte_utf8_to_ascii

```c
void catte_utf8_to_ascii(
    CATALYST_UTF8 utf8,
    CATALYST_ASCIIW ascii,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Unrepresentable values are replaced by '?'.

**Parameters:**

`utf8` — Readable null-terminated input.

`ascii` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x13`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41130700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81130100` | `ERROR_INVALID_ARGUMENT` | Validate `utf8` | Argument 0 (`utf8`) |
| `0x81130201` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 1 (`ascii`) |
| `0x83130406` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x06`: Two-byte sequence: unexpected end |
| `0x83130407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x07`: Two-byte sequence: invalid continuation |
| `0x83130409` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x09`: Three-byte sequence: unexpected end |
| `0x8313040A` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0A`: Three-byte sequence: invalid first continuation |
| `0x8313040B` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0B`: Three-byte sequence: invalid second continuation |
| `0x8313040C` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0C`: Three-byte sequence: overlong encoding |
| `0x8313040D` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0D`: Three-byte sequence: surrogate codepoint |
| `0x8313040F` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0F`: Four-byte sequence: unexpected end |
| `0x83130410` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x10`: Four-byte sequence: invalid first continuation |
| `0x83130411` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x11`: Four-byte sequence: invalid second continuation |
| `0x83130412` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x12`: Four-byte sequence: invalid third continuation |
| `0x83130413` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x13`: Four-byte sequence: overlong encoding |
| `0x83130414` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x14`: Four-byte sequence: above U+10FFFF |
| `0x83130416` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x16`: Invalid leading code unit |
| `0x90130300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90130600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1130401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x01`: Validate `utf8` (internal argument failure) |
| `0xC1130402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1130403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1130501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_ascii_encode` operation `0x01`: Validate `asciiw` (internal argument failure) |

**C++:** `catalyst::utf8_to_ascii(UTF8 utf8, ASCIIW ascii, NUINT capacity, RESULT* result)`.


#### catte_utf8_to_cp1252

```c
void catte_utf8_to_cp1252(
    CATALYST_UTF8 utf8,
    CATALYST_CP1252W cp1252w,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Unrepresentable values are replaced by '?'.

**Parameters:**

`utf8` — Readable null-terminated input.

`cp1252w` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x14`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41140700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81140100` | `ERROR_INVALID_ARGUMENT` | Validate `utf8` | Argument 0 (`utf8`) |
| `0x81140201` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252w` | Argument 1 (`cp1252w`) |
| `0x83140406` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x06`: Two-byte sequence: unexpected end |
| `0x83140407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x07`: Two-byte sequence: invalid continuation |
| `0x83140409` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x09`: Three-byte sequence: unexpected end |
| `0x8314040A` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0A`: Three-byte sequence: invalid first continuation |
| `0x8314040B` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0B`: Three-byte sequence: invalid second continuation |
| `0x8314040C` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0C`: Three-byte sequence: overlong encoding |
| `0x8314040D` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0D`: Three-byte sequence: surrogate codepoint |
| `0x8314040F` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0F`: Four-byte sequence: unexpected end |
| `0x83140410` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x10`: Four-byte sequence: invalid first continuation |
| `0x83140411` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x11`: Four-byte sequence: invalid second continuation |
| `0x83140412` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x12`: Four-byte sequence: invalid third continuation |
| `0x83140413` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x13`: Four-byte sequence: overlong encoding |
| `0x83140414` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x14`: Four-byte sequence: above U+10FFFF |
| `0x83140416` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x16`: Invalid leading code unit |
| `0x90140300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90140600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1140401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x01`: Validate `utf8` (internal argument failure) |
| `0xC1140402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1140403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1140501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_cp1252_encode` operation `0x01`: Validate `cp1252w` (internal argument failure) |

**C++:** `catalyst::utf8_to_cp1252(UTF8 utf8, CP1252W cp1252w, NUINT capacity, RESULT* result)`.


#### catte_utf8_to_utf16le

```c
void catte_utf8_to_utf16le(
    CATALYST_UTF8 utf8,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

All valid Unicode scalars are preserved.

**Parameters:**

`utf8` — Readable null-terminated input.

`utf16lew` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x15`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x81150100` | `ERROR_INVALID_ARGUMENT` | Validate `utf8` | Argument 0 (`utf8`) |
| `0x81150201` | `ERROR_INVALID_ARGUMENT` | Validate `utf16lew` | Argument 1 (`utf16lew`) |
| `0x83150406` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x06`: Two-byte sequence: unexpected end |
| `0x83150407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x07`: Two-byte sequence: invalid continuation |
| `0x83150409` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x09`: Three-byte sequence: unexpected end |
| `0x8315040A` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0A`: Three-byte sequence: invalid first continuation |
| `0x8315040B` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0B`: Three-byte sequence: invalid second continuation |
| `0x8315040C` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0C`: Three-byte sequence: overlong encoding |
| `0x8315040D` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0D`: Three-byte sequence: surrogate codepoint |
| `0x8315040F` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x0F`: Four-byte sequence: unexpected end |
| `0x83150410` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x10`: Four-byte sequence: invalid first continuation |
| `0x83150411` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x11`: Four-byte sequence: invalid second continuation |
| `0x83150412` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x12`: Four-byte sequence: invalid third continuation |
| `0x83150413` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x13`: Four-byte sequence: overlong encoding |
| `0x83150414` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x14`: Four-byte sequence: above U+10FFFF |
| `0x83150416` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf8_decode` operation `0x16`: Invalid leading code unit |
| `0x90150300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90150600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1150401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x01`: Validate `utf8` (internal argument failure) |
| `0xC1150402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1150403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf8_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1150501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x01`: Validate `utf16lew` (internal argument failure) |
| `0xC1150502` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x02`: Validate `produced` (internal argument failure) |
| `0xC1150503` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x03`: Reject codepoint above U+10FFFF (internal argument failure) |
| `0xC1150504` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf16le_encode` operation `0x04`: Reject surrogate codepoint (internal argument failure) |

**C++:** `catalyst::utf8_to_utf16le(UTF8 utf8, UTF16LEW utf16lew, NUINT capacity, RESULT* result)`.


#### catte_utf16le_to_ascii

```c
void catte_utf16le_to_ascii(
    CATALYST_UTF16LE utf16le,
    CATALYST_ASCIIW ascii,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Unrepresentable values are replaced by '?'.

**Parameters:**

`utf16le` — Readable null-terminated input.

`ascii` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x16`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41160700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81160100` | `ERROR_INVALID_ARGUMENT` | Validate `utf16le` | Argument 0 (`utf16le`) |
| `0x81160201` | `ERROR_INVALID_ARGUMENT` | Validate `ascii` | Argument 1 (`ascii`) |
| `0x83160405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83160407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90160300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90160600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1160401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x01`: Validate `utf16le` (internal argument failure) |
| `0xC1160402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1160403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1160501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_ascii_encode` operation `0x01`: Validate `asciiw` (internal argument failure) |

**C++:** `catalyst::utf16le_to_ascii(UTF16LE utf16le, ASCIIW ascii, NUINT capacity, RESULT* result)`.


#### catte_utf16le_to_cp1252

```c
void catte_utf16le_to_cp1252(
    CATALYST_UTF16LE utf16le,
    CATALYST_CP1252W cp1252w,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

Unrepresentable values are replaced by '?'.

**Parameters:**

`utf16le` — Readable null-terminated input.

`cp1252w` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x17`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41170700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x81170100` | `ERROR_INVALID_ARGUMENT` | Validate `utf16le` | Argument 0 (`utf16le`) |
| `0x81170201` | `ERROR_INVALID_ARGUMENT` | Validate `cp1252w` | Argument 1 (`cp1252w`) |
| `0x83170405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83170407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90170300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90170600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1170401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x01`: Validate `utf16le` (internal argument failure) |
| `0xC1170402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1170403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1170501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_cp1252_encode` operation `0x01`: Validate `cp1252w` (internal argument failure) |

**C++:** `catalyst::utf16le_to_cp1252(UTF16LE utf16le, CP1252W cp1252w, NUINT capacity, RESULT* result)`.


#### catte_utf16le_to_utf8

```c
void catte_utf16le_to_utf8(
    CATALYST_UTF16LE utf16le,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity,
    CATALYST_RESULT* result
);
```

All valid Unicode scalars are preserved.

**Parameters:**

`utf16le` — Readable null-terminated input.

`utf8w` — Writable destination buffer.

`capacity` — Destination capacity in code units, including the terminator.

`result` — Optional result output.

**Context:** `0x18`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x81180100` | `ERROR_INVALID_ARGUMENT` | Validate `utf16le` | Argument 0 (`utf16le`) |
| `0x81180201` | `ERROR_INVALID_ARGUMENT` | Validate `utf8w` | Argument 1 (`utf8w`) |
| `0x83180405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83180407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90180300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90180600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
| `0xC1180401` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x01`: Validate `utf16le` (internal argument failure) |
| `0xC1180402` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x02`: Validate `codepoint` (internal argument failure) |
| `0xC1180403` | `FATAL_INVARIANT_VIOLATION` | Decode source codepoint | `catte_utf16le_decode` operation `0x03`: Validate `consumed` (internal argument failure) |
| `0xC1180501` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x01`: Validate `utf8w` (internal argument failure) |
| `0xC1180502` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x02`: Validate `produced` (internal argument failure) |
| `0xC1180504` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x04`: Check two-byte encoding invariant (defensive) |
| `0xC1180506` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x06`: Check three-byte encoding invariant (defensive) |
| `0xC1180507` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x07`: Reject surrogate codepoint (internal argument failure) |
| `0xC118050A` | `FATAL_INVARIANT_VIOLATION` | Encode destination codepoint | `catte_utf8_encode` operation `0x0A`: Reject codepoint outside Unicode range (internal argument failure) |

**C++:** `catalyst::utf16le_to_utf8(UTF16LE utf16le, UTF8W utf8w, NUINT capacity, RESULT* result)`.
