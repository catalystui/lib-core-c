# CATTELIB

Provides encoding, decoding, length, and conversion operations for ASCII, Windows-1252, UTF-8, and UTF-16LE. See [Text Encoding](../../assumptions/index.md#text-encoding) for shared encoding contracts.

**C Header:** `cattelib.h`  
**C++ Header:** `cattelib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catte`  
**Dependencies:** [CATALYST](../catalyst/index.md)  
**Specification:** [FDEFSPEC Revision 1](https://catalystui.org/specifications/languages/fdefspec/)

## Macro Summary

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
| [`catte_utf8_encode`](#catte_utf8_encode) | Encodes a codepoint as one to four UTF-8 code units if it represents a Unicode scalar. |
| [`catte_utf8_decode`](#catte_utf8_decode) | Decodes one Unicode scalar from UTF-8 and reports the number of code units consumed (1–4). |
| [`catte_utf8_length`](#catte_utf8_length) | Counts Unicode scalars in a null-terminated UTF-8 string, excluding the terminator and any leading BOM. |

### UTF-16LE

| Function | Description |
| --- | --- |
| [`catte_utf16le_encode`](#catte_utf16le_encode) | Encodes a codepoint as one UTF-16LE code unit or a surrogate pair if it represents a Unicode scalar. |
| [`catte_utf16le_decode`](#catte_utf16le_decode) | Decodes one Unicode scalar from UTF-16LE and reports the number of code units consumed (1 or 2). |
| [`catte_utf16le_length`](#catte_utf16le_length) | Counts Unicode scalars in a null-terminated UTF-16LE string, excluding the terminator and any leading BOM. |

### String conversions

| Function | Description |
| --- | --- |
| [`catte_ascii_to_cp1252`](#catte_ascii_to_cp1252) | Converts a null-terminated ASCII string to Windows-1252. |
| [`catte_ascii_to_utf8`](#catte_ascii_to_utf8) | Converts a null-terminated ASCII string to UTF-8. |
| [`catte_ascii_to_utf16le`](#catte_ascii_to_utf16le) | Converts a null-terminated ASCII string to UTF-16LE. |
| [`catte_cp1252_to_ascii`](#catte_cp1252_to_ascii) | Converts a null-terminated Windows-1252 string to ASCII. |
| [`catte_cp1252_to_utf8`](#catte_cp1252_to_utf8) | Converts a null-terminated Windows-1252 string to UTF-8. |
| [`catte_cp1252_to_utf16le`](#catte_cp1252_to_utf16le) | Converts a null-terminated Windows-1252 string to UTF-16LE. |
| [`catte_utf8_to_ascii`](#catte_utf8_to_ascii) | Converts a null-terminated UTF-8 string to ASCII. |
| [`catte_utf8_to_cp1252`](#catte_utf8_to_cp1252) | Converts a null-terminated UTF-8 string to Windows-1252. |
| [`catte_utf8_to_utf16le`](#catte_utf8_to_utf16le) | Converts a null-terminated UTF-8 string to UTF-16LE. |
| [`catte_utf16le_to_ascii`](#catte_utf16le_to_ascii) | Converts a null-terminated UTF-16LE string to ASCII. |
| [`catte_utf16le_to_cp1252`](#catte_utf16le_to_cp1252) | Converts a null-terminated UTF-16LE string to Windows-1252. |
| [`catte_utf16le_to_utf8`](#catte_utf16le_to_utf8) | Converts a null-terminated UTF-16LE string to UTF-8. |

## Macro Details

### CATTELIB_API

On Windows/Cygwin, expands to `__declspec(dllexport)` with `CATTELIB_EXPORTS`, or `__declspec(dllimport)` with `CATTELIB_USE_DLL`. Otherwise empty.

## Function Details

### ASCII

#### catte_ascii_encode

```c
CATALYST_RESULT catte_ascii_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_ASCIIW asciiw
);
```

Encodes one codepoint as ASCII. Codepoints outside U+0000–U+007F are replaced with `?`.

**Parameters:**

`codepoint` — Codepoint to encode.  
`asciiw` — Writable buffer for one code unit.

**C++ equivalent:** `catalyst::ascii_encode`.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010300` | `SUCCESS` | Encode NUL | — |
| `0x00010400` | `SUCCESS` | Encode ASCII codepoint | — |
| `0x41010200` | `WARNING_PARTIAL` | Substitute unrepresentable codepoint | — |

#### catte_ascii_decode

```c
CATALYST_CODEPOINT catte_ascii_decode(CATALYST_ASCII ascii);
```

Decodes one readable ASCII code unit. Its byte becomes the least significant codepoint byte; the remaining bytes are zero. The ASCII range is not validated.

**Parameters:**

`ascii` — Valid non-null readable input containing one code unit.

**C++ equivalent:** `catalyst::ascii_decode`.

**Returns:** The decoded codepoint.

#### catte_ascii_length

```c
CATALYST_NUINT catte_ascii_length(CATALYST_ASCII ascii);
```

Counts the code units in a valid null-terminated ASCII string, excluding the terminator. ASCII values are not validated. The full string, including its terminator, must fit in addressable storage.

**Parameters:**

`ascii` — Valid non-null readable null-terminated input.

**C++ equivalent:** `catalyst::ascii_length`.

**Returns:** The number of code units before the terminator.

### Windows-1252

#### catte_cp1252_encode

```c
CATALYST_RESULT catte_cp1252_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_CP1252W cp1252w
);
```

Encodes one codepoint as Windows-1252. Unrepresentable codepoints are replaced with `?`.

**Parameters:**

`codepoint` — Codepoint to encode.  
`cp1252w` — Writable buffer for one code unit.

**C++ equivalent:** `catalyst::cp1252_encode`.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040200` | `SUCCESS` | Encode directly mapped codepoint | — |
| `0x00040300` | `SUCCESS` | Encode U+0152 as 0x8C | — |
| `0x00040400` | `SUCCESS` | Encode U+0153 as 0x9C | — |
| `0x00040500` | `SUCCESS` | Encode U+0160 as 0x8A | — |
| `0x00040600` | `SUCCESS` | Encode U+0161 as 0x9A | — |
| `0x00040700` | `SUCCESS` | Encode U+0178 as 0x9F | — |
| `0x00040800` | `SUCCESS` | Encode U+017D as 0x8E | — |
| `0x00040900` | `SUCCESS` | Encode U+017E as 0x9E | — |
| `0x00040A00` | `SUCCESS` | Encode U+0192 as 0x83 | — |
| `0x00040B00` | `SUCCESS` | Encode U+02C6 as 0x88 | — |
| `0x00040C00` | `SUCCESS` | Encode U+02DC as 0x98 | — |
| `0x00040D00` | `SUCCESS` | Encode U+2013 as 0x96 | — |
| `0x00040E00` | `SUCCESS` | Encode U+2014 as 0x97 | — |
| `0x00040F00` | `SUCCESS` | Encode U+2018 as 0x91 | — |
| `0x00041000` | `SUCCESS` | Encode U+2019 as 0x92 | — |
| `0x00041100` | `SUCCESS` | Encode U+201A as 0x82 | — |
| `0x00041200` | `SUCCESS` | Encode U+201C as 0x93 | — |
| `0x00041300` | `SUCCESS` | Encode U+201D as 0x94 | — |
| `0x00041400` | `SUCCESS` | Encode U+201E as 0x84 | — |
| `0x00041500` | `SUCCESS` | Encode U+2020 as 0x86 | — |
| `0x00041600` | `SUCCESS` | Encode U+2021 as 0x87 | — |
| `0x00041700` | `SUCCESS` | Encode U+2022 as 0x95 | — |
| `0x00041800` | `SUCCESS` | Encode U+2026 as 0x85 | — |
| `0x00041900` | `SUCCESS` | Encode U+2030 as 0x89 | — |
| `0x00041A00` | `SUCCESS` | Encode U+2039 as 0x8B | — |
| `0x00041B00` | `SUCCESS` | Encode U+203A as 0x9B | — |
| `0x00041C00` | `SUCCESS` | Encode U+20AC as 0x80 | — |
| `0x00041D00` | `SUCCESS` | Encode U+2122 as 0x99 | — |
| `0x41041E00` | `WARNING_PARTIAL` | Substitute unrepresentable codepoint | — |

#### catte_cp1252_decode

```c
CATALYST_RESULT catte_cp1252_decode(
    CATALYST_CP1252 cp1252,
    CATALYST_CODEPOINT* codepoint
);
```

Decodes one Windows-1252 code unit. Undefined bytes (`0x81`, `0x8D`, `0x8F`, `0x90`, `0x9D`) are replaced with U+003F (`?`).

**Parameters:**

`cp1252` — Readable input containing one code unit.  
`codepoint` — Receives the decoded codepoint.

**C++ equivalent:** `catalyst::cp1252_decode`.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050300` | `SUCCESS` | Decode directly mapped code unit | — |
| `0x00050400` | `SUCCESS` | Decode 0x80 as U+20AC | — |
| `0x00050500` | `SUCCESS` | Decode 0x82 as U+201A | — |
| `0x00050600` | `SUCCESS` | Decode 0x83 as U+0192 | — |
| `0x00050700` | `SUCCESS` | Decode 0x84 as U+201E | — |
| `0x00050800` | `SUCCESS` | Decode 0x85 as U+2026 | — |
| `0x00050900` | `SUCCESS` | Decode 0x86 as U+2020 | — |
| `0x00050A00` | `SUCCESS` | Decode 0x87 as U+2021 | — |
| `0x00050B00` | `SUCCESS` | Decode 0x88 as U+02C6 | — |
| `0x00050C00` | `SUCCESS` | Decode 0x89 as U+2030 | — |
| `0x00050D00` | `SUCCESS` | Decode 0x8A as U+0160 | — |
| `0x00050E00` | `SUCCESS` | Decode 0x8B as U+2039 | — |
| `0x00050F00` | `SUCCESS` | Decode 0x8C as U+0152 | — |
| `0x00051000` | `SUCCESS` | Decode 0x8E as U+017D | — |
| `0x00051100` | `SUCCESS` | Decode 0x91 as U+2018 | — |
| `0x00051200` | `SUCCESS` | Decode 0x92 as U+2019 | — |
| `0x00051300` | `SUCCESS` | Decode 0x93 as U+201C | — |
| `0x00051400` | `SUCCESS` | Decode 0x94 as U+201D | — |
| `0x00051500` | `SUCCESS` | Decode 0x95 as U+2022 | — |
| `0x00051600` | `SUCCESS` | Decode 0x96 as U+2013 | — |
| `0x00051700` | `SUCCESS` | Decode 0x97 as U+2014 | — |
| `0x00051800` | `SUCCESS` | Decode 0x98 as U+02DC | — |
| `0x00051900` | `SUCCESS` | Decode 0x99 as U+2122 | — |
| `0x00051A00` | `SUCCESS` | Decode 0x9A as U+0161 | — |
| `0x00051B00` | `SUCCESS` | Decode 0x9B as U+203A | — |
| `0x00051C00` | `SUCCESS` | Decode 0x9C as U+0153 | — |
| `0x00051D00` | `SUCCESS` | Decode 0x9E as U+017E | — |
| `0x00051E00` | `SUCCESS` | Decode 0x9F as U+0178 | — |
| `0x41051F00` | `WARNING_PARTIAL` | Substitute undefined code unit | — |

#### catte_cp1252_length

```c
CATALYST_NUINT catte_cp1252_length(CATALYST_CP1252 cp1252);
```

Counts the code units in a valid null-terminated Windows-1252 string, excluding the terminator. Undefined bytes are counted without warning. The full string, including its terminator, must fit in addressable storage.

**Parameters:**

`cp1252` — Valid non-null readable null-terminated input.

**C++ equivalent:** `catalyst::cp1252_length`.

**Returns:** The number of code units before the terminator.

### UTF-8

#### catte_utf8_encode

```c
CATALYST_RESULT catte_utf8_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT* produced
);
```

Encodes a Unicode scalar as one to four UTF-8 code units. `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

**Parameters:**

`codepoint` — Codepoint to encode.  
`utf8w` — Writable buffer of at least four code units.  
`produced` — Receives the number of code units written.

**C++ equivalent:** `catalyst::utf8_encode`.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00070300` | `SUCCESS` | Encode one-byte sequence | — |
| `0x00070500` | `SUCCESS` | Encode two-byte sequence | — |
| `0x00070800` | `SUCCESS` | Encode three-byte sequence | — |
| `0x00070900` | `SUCCESS` | Encode four-byte sequence | — |
| `0x81070700` | `ERROR_INVALID_ARGUMENT` | Reject surrogate codepoint | Argument 0 (`codepoint`) |
| `0x81070A00` | `ERROR_INVALID_ARGUMENT` | Reject codepoint outside Unicode range | Argument 0 (`codepoint`) |

#### catte_utf8_decode

```c
CATALYST_RESULT catte_utf8_decode(
    CATALYST_UTF8 utf8,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_NUINT* consumed
);
```

Decodes one Unicode scalar from UTF-8 and reports the number of code units consumed (1–4). Both `codepoint` and `consumed` are zero on decoding failure.

**Parameters:**

`utf8` — Readable input containing the indicated sequence or an accessible NUL terminator if truncated. No bytes beyond the terminator are read.  
`codepoint` — Receives the decoded codepoint.  
`consumed` — Receives the number of code units read.

**C++ equivalent:** `catalyst::utf8_decode`.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00080400` | `SUCCESS` | Decode NUL | — |
| `0x00080500` | `SUCCESS` | Decode one-byte sequence | — |
| `0x00080800` | `SUCCESS` | Decode two-byte sequence | — |
| `0x00080E00` | `SUCCESS` | Decode three-byte sequence | — |
| `0x00081500` | `SUCCESS` | Decode four-byte sequence | — |
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

#### catte_utf8_length

```c
CATALYST_RESULT catte_utf8_length(
    CATALYST_UTF8 utf8,
    CATALYST_NUINT* length
);
```

Counts Unicode scalars in a null-terminated UTF-8 string, excluding the terminator and any leading UTF-8 BOM (EF BB BF). Malformed terminated input requires no padding beyond its terminator. This is not a byte or grapheme count. `length` is zero on failure.

**Parameters:**

`utf8` — Readable null-terminated input.  
`length` — Receives the number of Unicode scalars.

**C++ equivalent:** `catalyst::utf8_length`.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00090500` | `SUCCESS` | Write length | — |
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

### UTF-16LE

#### catte_utf16le_encode

```c
CATALYST_RESULT catte_utf16le_encode(
    const CATALYST_CODEPOINT codepoint,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT* produced
);
```

Encodes a Unicode scalar as one UTF-16LE code unit or a surrogate pair. `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

**Parameters:**

`codepoint` — Codepoint to encode.  
`utf16lew` — Writable buffer of at least two UTF-16LE code units.  
`produced` — Receives the number of UTF-16LE code units written (not bytes).

**C++ equivalent:** `catalyst::utf16le_encode`.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000A0500` | `SUCCESS` | Encode one code unit | — |
| `0x000A0600` | `SUCCESS` | Encode surrogate pair | — |
| `0x810A0300` | `ERROR_INVALID_ARGUMENT` | Reject codepoint above U+10FFFF | Argument 0 (`codepoint`) |
| `0x810A0400` | `ERROR_INVALID_ARGUMENT` | Reject surrogate codepoint | Argument 0 (`codepoint`) |

#### catte_utf16le_decode

```c
CATALYST_RESULT catte_utf16le_decode(
    CATALYST_UTF16LE utf16le,
    CATALYST_CODEPOINT* codepoint,
    CATALYST_NUINT* consumed
);
```

Decodes one Unicode scalar from UTF-16LE and reports the number of code units consumed (1 or 2). Both `codepoint` and `consumed` are zero on decoding failure.

**Parameters:**

`utf16le` — Readable input containing the indicated sequence or an accessible NUL code unit if truncated.  
`codepoint` — Receives the decoded codepoint.  
`consumed` — Receives the number of UTF-16LE code units read (not bytes).

**C++ equivalent:** `catalyst::utf16le_decode`.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000B0400` | `SUCCESS` | Decode NUL | — |
| `0x000B0600` | `SUCCESS` | Decode one code unit | — |
| `0x000B0800` | `SUCCESS` | Decode surrogate pair | — |
| `0x830B0500` | `ERROR_MALFORMED_INPUT` | Reject isolated low surrogate | — |
| `0x830B0700` | `ERROR_MALFORMED_INPUT` | Reject missing low surrogate | — |

#### catte_utf16le_length

```c
CATALYST_RESULT catte_utf16le_length(
    CATALYST_UTF16LE utf16le,
    CATALYST_NUINT* length
);
```

Counts Unicode scalars in a null-terminated UTF-16LE string, excluding the terminator and any leading UTF-16LE BOM (FF FE). Malformed terminated input requires no padding beyond its terminator. A surrogate pair counts as one scalar. `length` is zero on failure.

**Parameters:**

`utf16le` — Readable null-terminated input.  
`length` — Receives the number of Unicode scalars.

**C++ equivalent:** `catalyst::utf16le_length`.

**Context:** `0x0C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000C0500` | `SUCCESS` | Write length | — |
| `0x830C0305` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x830C0307` | `ERROR_MALFORMED_INPUT` | Decode next codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |

### String conversions

#### catte_ascii_to_cp1252

```c
CATALYST_RESULT catte_ascii_to_cp1252(
    CATALYST_ASCII ascii,
    CATALYST_CP1252W cp1252w
);
```

Converts a null-terminated ASCII string to Windows-1252. Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.  
`cp1252w` — Writable destination buffer of at least the source byte count plus one terminator.

**C++ equivalent:** `catalyst::ascii_to_cp1252`.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000D0400` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410D0400` | `WARNING_PARTIAL` | Complete conversion with substitution | — |

#### catte_ascii_to_utf8

```c
CATALYST_RESULT catte_ascii_to_utf8(
    CATALYST_ASCII ascii,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated ASCII string to UTF-8. Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.  
`utf8w` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::ascii_to_utf8`.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000E0500` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410E0500` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x900E0300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x900E0400` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_ascii_to_utf16le

```c
CATALYST_RESULT catte_ascii_to_utf16le(
    CATALYST_ASCII ascii,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated ASCII string to UTF-16LE. Source bytes above 0x7F are replaced by '?'.

**Parameters:**

`ascii` — Readable null-terminated input.  
`utf16lew` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in two-byte UTF-16LE code units, including the terminator.

**C++ equivalent:** `catalyst::ascii_to_utf16le`.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000F0500` | `SUCCESS` | Complete conversion without substitution | — |
| `0x410F0500` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x900F0300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x900F0400` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_cp1252_to_ascii

```c
CATALYST_RESULT catte_cp1252_to_ascii(
    CATALYST_CP1252 cp1252,
    CATALYST_ASCIIW ascii
);
```

Converts a null-terminated Windows-1252 string to ASCII. Undefined source bytes and values not representable in ASCII are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.  
`ascii` — Writable destination buffer of at least the source byte count plus one terminator.

**C++ equivalent:** `catalyst::cp1252_to_ascii`.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00100600` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41100600` | `WARNING_PARTIAL` | Complete conversion with substitution | — |

#### catte_cp1252_to_utf8

```c
CATALYST_RESULT catte_cp1252_to_utf8(
    CATALYST_CP1252 cp1252,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated Windows-1252 string to UTF-8. Undefined source bytes are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.  
`utf8w` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::cp1252_to_utf8`.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00110700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41110700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x90110300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90110600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_cp1252_to_utf16le

```c
CATALYST_RESULT catte_cp1252_to_utf16le(
    CATALYST_CP1252 cp1252,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated Windows-1252 string to UTF-16LE. Undefined source bytes are replaced by '?'.

**Parameters:**

`cp1252` — Readable null-terminated input.  
`utf16lew` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in two-byte UTF-16LE code units, including the terminator.

**C++ equivalent:** `catalyst::cp1252_to_utf16le`.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00120700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41120700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x90120300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90120600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_utf8_to_ascii

```c
CATALYST_RESULT catte_utf8_to_ascii(
    CATALYST_UTF8 utf8,
    CATALYST_ASCIIW ascii,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-8 string to ASCII. Unrepresentable values are replaced by '?'.

**Parameters:**

`utf8` — Readable null-terminated input.  
`ascii` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::utf8_to_ascii`.

**Context:** `0x13`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00130700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41130700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
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

#### catte_utf8_to_cp1252

```c
CATALYST_RESULT catte_utf8_to_cp1252(
    CATALYST_UTF8 utf8,
    CATALYST_CP1252W cp1252w,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-8 string to Windows-1252. Unrepresentable values are replaced by '?'.

**Parameters:**

`utf8` — Readable null-terminated input.  
`cp1252w` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::utf8_to_cp1252`.

**Context:** `0x14`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00140700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41140700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
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

#### catte_utf8_to_utf16le

```c
CATALYST_RESULT catte_utf8_to_utf16le(
    CATALYST_UTF8 utf8,
    CATALYST_UTF16LEW utf16lew,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-8 string to UTF-16LE. All decoded Unicode scalars after an optional initial BOM are preserved.

**Parameters:**

`utf8` — Readable null-terminated input.  
`utf16lew` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in two-byte UTF-16LE code units, including the terminator.

**C++ equivalent:** `catalyst::utf8_to_utf16le`.

**Context:** `0x15`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00150700` | `SUCCESS` | Complete conversion without substitution | — |
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

#### catte_utf16le_to_ascii

```c
CATALYST_RESULT catte_utf16le_to_ascii(
    CATALYST_UTF16LE utf16le,
    CATALYST_ASCIIW ascii,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-16LE string to ASCII. Unrepresentable values are replaced by '?'.

**Parameters:**

`utf16le` — Readable null-terminated input.  
`ascii` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::utf16le_to_ascii`.

**Context:** `0x16`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00160700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41160700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x83160405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83160407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90160300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90160600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_utf16le_to_cp1252

```c
CATALYST_RESULT catte_utf16le_to_cp1252(
    CATALYST_UTF16LE utf16le,
    CATALYST_CP1252W cp1252w,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-16LE string to Windows-1252. Unrepresentable values are replaced by '?'.

**Parameters:**

`utf16le` — Readable null-terminated input.  
`cp1252w` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::utf16le_to_cp1252`.

**Context:** `0x17`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00170700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x41170700` | `WARNING_PARTIAL` | Complete conversion with substitution | — |
| `0x83170405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83170407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90170300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90170600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |

#### catte_utf16le_to_utf8

```c
CATALYST_RESULT catte_utf16le_to_utf8(
    CATALYST_UTF16LE utf16le,
    CATALYST_UTF8W utf8w,
    CATALYST_NUINT capacity
);
```

Converts a null-terminated UTF-16LE string to UTF-8. All decoded Unicode scalars after an optional initial BOM are preserved.

**Parameters:**

`utf16le` — Readable null-terminated input.  
`utf8w` — Writable destination buffer of at least `capacity` code units.  
`capacity` — Destination capacity in code units, including the terminator.

**C++ equivalent:** `catalyst::utf16le_to_utf8`.

**Context:** `0x18`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00180700` | `SUCCESS` | Complete conversion without substitution | — |
| `0x83180405` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x05`: Reject isolated low surrogate |
| `0x83180407` | `ERROR_MALFORMED_INPUT` | Decode source codepoint | `catte_utf16le_decode` operation `0x07`: Reject missing low surrogate |
| `0x90180300` | `ERROR_BUFFER_OVERFLOW` | Check destination capacity | — |
| `0x90180600` | `ERROR_BUFFER_OVERFLOW` | Check remaining destination capacity | — |
