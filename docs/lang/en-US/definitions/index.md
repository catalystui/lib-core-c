# Foundational definitions

Headers: `catalyst.h` (C), `catalyst.hpp` (C++). Include these rather than the internal numeric headers directly. The C++ names are in the `catalyst` namespace.

## Numeric types

| C type | C++ type | Representation | Availability |
| --- | --- | --- | --- |
| `CATALYST_BIT` | `BIT` | Byte, values 0–1 | Always |
| `CATALYST_NIBBLE` | `NIBBLE` | Byte, values 0–15 | Always |
| `CATALYST_BYTE` | `BYTE` | Unsigned 8-bit integer | Always |
| `CATALYST_SBYTE` | `SBYTE` | Signed 8-bit integer | Always |
| `CATALYST_SHORT` | `SHORT` | Signed 16-bit integer | `CATALYST_SUPPORTS_16BIT` |
| `CATALYST_USHORT` | `USHORT` | Unsigned 16-bit integer | `CATALYST_SUPPORTS_16BIT` |
| `CATALYST_INT` | `INT` | Signed 32-bit integer | `CATALYST_SUPPORTS_32BIT` |
| `CATALYST_UINT` | `UINT` | Unsigned 32-bit integer | `CATALYST_SUPPORTS_32BIT` |
| `CATALYST_LONG` | `LONG` | Signed 64-bit integer | `CATALYST_SUPPORTS_64BIT` |
| `CATALYST_ULONG` | `ULONG` | Unsigned 64-bit integer | `CATALYST_SUPPORTS_64BIT` |
| `CATALYST_SINGLE` | `SINGLE` | 32-bit floating point, binary radix, 24-bit precision | `CATALYST_SUPPORTS_SINGLE` |
| `CATALYST_DOUBLE` | `DOUBLE` | 64-bit floating point, binary radix, 53-bit precision | `CATALYST_SUPPORTS_DOUBLE` |
| `CATALYST_BOOL` | `BOOL` | Byte, 0 = false, 1 = true | Always |
| `CATALYST_NINT` | `NINT` | Signed native-width integer | Always |
| `CATALYST_NUINT` | `NUINT` | Unsigned native-width integer | Always |

The bit, nibble and boolean aliases do not enforce their value ranges. `CATALYST_TRUE` / `CATALYST_FALSE` and `catalyst::TRUE` / `catalyst::FALSE` are 1 and 0 respectively.

### Integer limits

Each available integer representation exposes its lower and upper limits through the following macros. The macros are also available when including the C++ header.

| Minimum | Maximum |
| --- | --- |
| `CATALYST_BIT_MIN` | `CATALYST_BIT_MAX` |
| `CATALYST_NIBBLE_MIN` | `CATALYST_NIBBLE_MAX` |
| `CATALYST_BYTE_MIN` | `CATALYST_BYTE_MAX` |
| `CATALYST_SBYTE_MIN` | `CATALYST_SBYTE_MAX` |
| `CATALYST_SHORT_MIN` | `CATALYST_SHORT_MAX` |
| `CATALYST_USHORT_MIN` | `CATALYST_USHORT_MAX` |
| `CATALYST_INT_MIN` | `CATALYST_INT_MAX` |
| `CATALYST_UINT_MIN` | `CATALYST_UINT_MAX` |
| `CATALYST_LONG_MIN` | `CATALYST_LONG_MAX` |
| `CATALYST_ULONG_MIN` | `CATALYST_ULONG_MAX` |
| `CATALYST_NINT_MIN` | `CATALYST_NINT_MAX` |
| `CATALYST_NUINT_MIN` | `CATALYST_NUINT_MAX` |

Unsigned minima are zero. Integer limits come from the selected numeric header; native limits follow the selected pointer width.

### Target configuration

| Definition | Meaning |
| --- | --- |
| `TARGET_STDC_VERSION` | Configured C standard; values at least 199901L select `catalyst.c99.h`, otherwise `catalyst.c90.h`. |
| `TARGET_SIZEOF_VOID_P` | Configured pointer size in bytes (1, 2, 4 or 8). Selects the matching signed/unsigned native-width types; compilation fails if unavailable. |
| `CATALYST_SUPPORTS_16BIT`, `CATALYST_SUPPORTS_32BIT`, `CATALYST_SUPPORTS_64BIT` | 1 if that integer width is available, 0 otherwise. |
| `CATALYST_SUPPORTS_SINGLE`, `CATALYST_SUPPORTS_DOUBLE` | 1 if the floating-point format is supported, 0 otherwise. Size is checked at compile time. |
| `CATALYST_CONSTEXPR` | In C++, expands to `constexpr` for C++11 or later, otherwise empty. |

`cmakecfg.h` supplies target configuration. The selected numeric representation is shared by C and C++.

| Internal header | Type selection |
| --- | --- |
| `catalyst.c90.h` | Requires eight-bit bytes; uses signed/unsigned char, short and int when their ranges match the required widths. Does not expose 64-bit integers or search long for 32-bit integers. |
| `catalyst.c99.h` | Uses exact-width `stdint.h` types; requires 8-bit types and exposes 16-, 32- and 64-bit pairs only when their range macros exist. |

`CATALYST_requires_float_4_bytes` and `CATALYST_requires_double_8_bytes` are internal compile-time size checks, not application data types. Native-width integers represent sizes and counts; no pointer conversion or normalization contract is defined.

## Text representations

### Encoding identifiers

`CATALYST_TEXT_ENCODING` / `catalyst::TEXT_ENCODING` is an unsigned byte. C constants have the prefix `CATALYST_TEXT_ENCODING_`; C++ constants use `catalyst::TEXT_ENCODING_`.

| Suffix | Value | Encoding |
| --- | --- | --- |
| `UNKNOWN` | 0x00 | Unspecified |
| `ASCII` | 0x01 | ASCII |
| `CP1252` | 0x02 | Windows-1252 |
| `UTF8` | 0x03 | UTF-8 |
| `UTF16LE` | 0x04 | UTF-16 little-endian |

### Code units and strings

| C code-unit type | C read-only pointer | C writable pointer |
| --- | --- | --- |
| `CATALYST_ASCII_CODE_UNIT` | `CATALYST_ASCII` | `CATALYST_ASCIIW` |
| `CATALYST_CP1252_CODE_UNIT` | `CATALYST_CP1252` | `CATALYST_CP1252W` |
| `CATALYST_UTF8_CODE_UNIT` | `CATALYST_UTF8` | `CATALYST_UTF8W` |
| `CATALYST_UTF16LE_CODE_UNIT` | `CATALYST_UTF16LE` | `CATALYST_UTF16LEW` |

C++ names remove the `CATALYST_` prefix and reside in `catalyst`. ASCII, CP1252 and UTF-8 units are bytes. UTF-16LE units are structures with byte fields `lsb` followed by `msb`; the representation is independent of native byte order. String pointers do not carry length, capacity or ownership. The W suffix means writable, not wide.

### Codepoints

`CATALYST_CODEPOINT` is a four-byte structure. Fields `byte0`, `byte1`, `byte2`, `byte3` store the most significant through least significant byte. The representation does not validate Unicode values. UTF encoders accept Unicode scalar values (U+0000–U+10FFFF excluding U+D800–U+DFFF).

| C helper | Return value | Availability |
| --- | --- | --- |
| `cat_codepoint_from_uint(CATALYST_UINT value)` | `CATALYST_CODEPOINT` containing the four bytes of value | 32-bit integer support |
| `cat_codepoint_to_uint(CATALYST_CODEPOINT codepoint)` | `CATALYST_UINT` reconstructed from the four bytes | 32-bit integer support |
| `cat_utf16le_code_unit_from_ushort(CATALYST_USHORT value)` | `CATALYST_UTF16LE_CODE_UNIT` containing the low/high bytes | 16-bit integer support |
| `cat_utf16le_code_unit_to_ushort(CATALYST_UTF16LE_CODE_UNIT codeunit)` | `CATALYST_USHORT` reconstructed from the two bytes | 16-bit integer support |

These header-only functions do not validate the represented value or return a result status.

### C++ value types

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

## Operation results

`CATALYST_RESULT` contains four byte fields in declaration order:

| Field | C type | C++ type | Meaning |
| --- | --- | --- | --- |
| `status` | `CATALYST_STATUS_CODE` | `STATUS_CODE` | Operation outcome |
| `context` | `CATALYST_CONTEXT_CODE` | `CONTEXT_CODE` | Operation-defined context |
| `operation` | `CATALYST_OPERATION_CODE` | `OPERATION_CODE` | Function-specific stage |
| `detail` | `CATALYST_DETAIL_CODE` | `DETAIL_CODE` | Status-specific detail |

C++ constants `CONTEXT_CODE_NONE`, `OPERATION_CODE_NONE` and `DETAIL_CODE_NONE` are zero. Each library documents its own metadata. A null result pointer discards status information; a non-null pointer receives all four fields. Library functions return `void`; other output parameters carry operation data.

### Status codes

The C prefix is `CATALYST_STATUS_CODE_`; the C++ prefix is `catalyst::STATUS_CODE_`. Other values are not assigned a named status by these headers.

| Suffix | Value | Meaning |
| --- | --- | --- |
| `SUCCESS` | 0x00 | Operation completed. |
| `SUCCESS_NOOP` | 0x01 | Succeeded without an action. |
| `WARNING` | 0x40 | Completed with an unexpected or undesirable result. |
| `WARNING_PARTIAL` | 0x41 | Completed with incomplete or substituted output. |
| `WARNING_DEPRECATED` | 0x42 | Used behavior that may be removed. |
| `ERROR` | 0x80 | Recoverable failure. |
| `ERROR_INVALID_ARGUMENT` | 0x81 | Invalid argument; detail identifies the argument according to the API. |
| `ERROR_INVALID_STATE` | 0x82 | Invalid, corrupted or inconsistent state. |
| `ERROR_MALFORMED_INPUT` | 0x83 | Malformed required input. |
| `ERROR_ACCESS_DENIED` | 0x84 | Access denied. |
| `ERROR_NOT_IMPLEMENTED` | 0x85 | Requested behavior not implemented. |
| `ERROR_SYSTEM_NOT_SUPPORTED` | 0x86 | Unsupported target system or configuration. |
| `ERROR_TIMEOUT` | 0x87 | Required timeout occurred. |
| `ERROR_NOT_FOUND` | 0x88 | Required resource does not exist. |
| `ERROR_INTERRUPTED` | 0x89 | External interruption. |
| `ERROR_DEPENDENCY_FAILURE` | 0x8A | Dependency failed; detail contains its status byte. |
| `ERROR_BUFFER_OVERFLOW` | 0x90 | Data exceeds available buffer or representable extent. |
| `ERROR_ALLOCATION_FAILED` | 0x91 | Memory allocation failed. |
| `ERROR_IO_ERROR` | 0xA0 | Input/output failure. |
| `FATAL` | 0xC0 | Unrecoverable failure. |
| `FATAL_INVARIANT_VIOLATION` | 0xC1 | Fundamental invariant violated. |

### Severity

`CATALYST_STATUS_CODE_LEVEL` / `STATUS_CODE_LEVEL` is an unsigned byte. The mask `CATALYST_STATUS_CODE_LEVEL_MASK` / `STATUS_CODE_LEVEL_MASK` is 0xC0.

| C constant suffix (after CATALYST_STATUS_CODE_LEVEL_) | C++ constant suffix (after STATUS_CODE_LEVEL_) | Value |
| --- | --- | --- |
| SUCCESS | SUCCESS | 0x00 |
| WARNING | WARNING | 0x40 |
| ERROR | ERROR | 0x80 |
| FATAL | FATAL | 0xC0 |

| C macro | C++ function | Return value |
| --- | --- | --- |
| `CATALYST_STATUS_CODE_GET_LEVEL(status)` | `status_code_get_level(status)` | Masked severity |
| `CATALYST_STATUS_CODE_IS_SUCCESS(status)` | `status_code_is_success(status)` | True for success severity (including NOOP) |
| `CATALYST_STATUS_CODE_IS_WARNING(status)` | `status_code_is_warning(status)` | True for warning severity |
| `CATALYST_STATUS_CODE_IS_ERROR(status)` | `status_code_is_error(status)` | True for recoverable error severity |
| `CATALYST_STATUS_CODE_IS_FATAL(status)` | `status_code_is_fatal(status)` | True for fatal severity |

The C predicates are comparison expressions; C++ predicates return `BOOL`. Severity classification also applies to unnamed status values.

### Result conversions

With 32-bit integer support:

| C helper | Return value |
| --- | --- |
| `cat_result_from_uint(CATALYST_UINT value)` | `CATALYST_RESULT` unpacked from numeric 0xSSCCOODD |
| `cat_result_to_uint(CATALYST_RESULT result)` | `CATALYST_UINT` packed as numeric 0xSSCCOODD |

SS = status, CC = context, OO = operation, DD = detail. The helpers are endian-independent numeric conversions, not memory reinterpretation.

`catalyst::RESULT` derives from `CATALYST_RESULT`.

| Member | Behavior |
| --- | --- |
| `RESULT()` | SUCCESS, all other fields zero. |
| `RESULT(STATUS_CODE, CONTEXT_CODE, OPERATION_CODE, DETAIL_CODE)` | Initializes the four fields. |
| `RESULT(const CATALYST_RESULT&)`, `operator=(const CATALYST_RESULT&)` | Copies the C representation. |
| `explicit RESULT(UINT)`, `operator=(UINT)`, `operator UINT() const` | Packs or unpacks the four fields when 32-bit integers are supported. |

C aggregate results must be initialized explicitly when needed. C++ assignments return `RESULT&`.
