# Text representations

## Encoding identifiers

`CATALYST_TEXT_ENCODING` / `catalyst::TEXT_ENCODING` is an unsigned byte. C constants have the prefix `CATALYST_TEXT_ENCODING_`; C++ constants use `catalyst::TEXT_ENCODING_`.

| Suffix | Value | Encoding |
| --- | --- | --- |
| `UNKNOWN` | 0x00 | Unspecified |
| `ASCII` | 0x01 | ASCII |
| `CP1252` | 0x02 | Windows-1252 |
| `UTF8` | 0x03 | UTF-8 |
| `UTF16LE` | 0x04 | UTF-16 little-endian |

## Code units and strings

| C code-unit type | C read-only pointer | C writable pointer |
| --- | --- | --- |
| `CATALYST_ASCII_CODE_UNIT` | `CATALYST_ASCII` | `CATALYST_ASCIIW` |
| `CATALYST_CP1252_CODE_UNIT` | `CATALYST_CP1252` | `CATALYST_CP1252W` |
| `CATALYST_UTF8_CODE_UNIT` | `CATALYST_UTF8` | `CATALYST_UTF8W` |
| `CATALYST_UTF16LE_CODE_UNIT` | `CATALYST_UTF16LE` | `CATALYST_UTF16LEW` |

C++ names remove the `CATALYST_` prefix and reside in `catalyst`. ASCII, CP1252 and UTF-8 units are bytes. UTF-16LE units are structures with byte fields `lsb` followed by `msb`; the representation is independent of native byte order. String pointers do not carry length, capacity or ownership. The W suffix means writable, not wide.

## Codepoints

`CATALYST_CODEPOINT` is a four-byte structure. Fields `byte0`, `byte1`, `byte2`, `byte3` store the most significant through least significant byte. The representation does not validate Unicode values. UTF encoders accept Unicode scalar values (U+0000–U+10FFFF excluding U+D800–U+DFFF).

| C helper | Return value | Availability |
| --- | --- | --- |
| `cat_codepoint_from_uint(CATALYST_UINT value)` | `CATALYST_CODEPOINT` containing the four bytes of value | 32-bit integer support |
| `cat_codepoint_to_uint(CATALYST_CODEPOINT codepoint)` | `CATALYST_UINT` reconstructed from the four bytes | 32-bit integer support |
| `cat_utf16le_code_unit_from_ushort(CATALYST_USHORT value)` | `CATALYST_UTF16LE_CODE_UNIT` containing the low/high bytes | 16-bit integer support |
| `cat_utf16le_code_unit_to_ushort(CATALYST_UTF16LE_CODE_UNIT codeunit)` | `CATALYST_USHORT` reconstructed from the two bytes | 16-bit integer support |

These header-only functions do not validate the represented value or return a result status.

## C++ value types

`catalyst::CODEPOINT` derives from `CATALYST_CODEPOINT`; `catalyst::UTF16LE_CODE_UNIT` derives from `CATALYST_UTF16LE_CODE_UNIT`.

| Member | Behavior |
| --- | --- |
| `CODEPOINT()` | Zeroes all four bytes. |
| `CODEPOINT(BYTE byte0, BYTE byte1, BYTE byte2, BYTE byte3)` | Initializes the bytes in significance order. |
| `CODEPOINT(const CATALYST_CODEPOINT&)`, `operator=(const CATALYST_CODEPOINT&)` | Copies the C representation. |
| `explicit CODEPOINT(UINT)`, `operator=(UINT)`, `operator UINT() const` | Converts to/from the numeric representation when 32-bit integers are supported. |
| `UTF16LE_CODE_UNIT()` | Zeroes both bytes. |
| `UTF16LE_CODE_UNIT(BYTE msb, BYTE lsb)` | Initializes high then low byte; storage remains low then high. |
| `UTF16LE_CODE_UNIT(const CATALYST_UTF16LE_CODE_UNIT&)`, `operator=(const CATALYST_UTF16LE_CODE_UNIT&)` | Copies the C representation. |
| `explicit UTF16LE_CODE_UNIT(USHORT)`, `operator=(USHORT)`, `operator USHORT() const` | Converts to/from the numeric representation when 16-bit integers are supported. |

Assignments return a reference to the assigned object. None of these conversions validate Unicode.
