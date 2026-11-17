# CATALYST

Provides the foundational numeric types, text representations, and operation result definitions used by CatalystUI Core.

**C Header:** `catalyst.h`  
**C++ Header:** `catalyst.hpp`  
**Namespace:** `catalyst`  
**Linking:** Header-only; no separate library is required.

The definitions correspond to the [Foundational Definitions Specification (FDEFSPEC)](https://catalystui.org/specifications/languages/fdefspec/).

## Contents

- [Type Summary](#type-summary)
- [Constant Summary](#constant-summary)
- [Function Summary](#function-summary)
- [Type Details](#type-details)
- [Constant Details](#constant-details)
- [Function Details](#function-details)
- [C++ Member Details](#c-member-details)
- [Target Configuration](#target-configuration)

## Type Summary

### Numerics

| C Type | C++ Type | Description | Availability |
| --- | --- | --- | --- |
| [`CATALYST_BIT`](#catalyst_bit) | `BIT` | Byte representing a bit (0 or 1). | Always |
| [`CATALYST_NIBBLE`](#catalyst_nibble) | `NIBBLE` | Byte representing four bits (0–15). | Always |
| [`CATALYST_BYTE`](#catalyst_byte) | `BYTE` | Unsigned eight-bit integer. | Always |
| [`CATALYST_SBYTE`](#catalyst_sbyte) | `SBYTE` | Signed eight-bit integer. | Always |
| [`CATALYST_SHORT`](#catalyst_short) | `SHORT` | Signed sixteen-bit integer. | 16-bit support |
| [`CATALYST_USHORT`](#catalyst_ushort) | `USHORT` | Unsigned sixteen-bit integer. | 16-bit support |
| [`CATALYST_INT`](#catalyst_int) | `INT` | Signed thirty-two-bit integer. | 32-bit support |
| [`CATALYST_UINT`](#catalyst_uint) | `UINT` | Unsigned thirty-two-bit integer. | 32-bit support |
| [`CATALYST_LONG`](#catalyst_long) | `LONG` | Signed sixty-four-bit integer. | 64-bit support |
| [`CATALYST_ULONG`](#catalyst_ulong) | `ULONG` | Unsigned sixty-four-bit integer. | 64-bit support |
| [`CATALYST_SINGLE`](#catalyst_single) | `SINGLE` | Thirty-two-bit floating-point value. | Single support |
| [`CATALYST_DOUBLE`](#catalyst_double) | `DOUBLE` | Sixty-four-bit floating-point value. | Double support |
| [`CATALYST_BOOL`](#catalyst_bool) | `BOOL` | Byte representing a Boolean value. | Always |
| [`CATALYST_NINT`](#catalyst_nint) | `NINT` | Signed pointer-width integer. | Always |
| [`CATALYST_NUINT`](#catalyst_nuint) | `NUINT` | Unsigned pointer-width integer. | Always |

### Text Encoding

| C Type | C++ Type | Description |
| --- | --- | --- |
| [`CATALYST_TEXT_ENCODING`](#catalyst_text_encoding) | `TEXT_ENCODING` | Identifies a text encoding. |
| [`CATALYST_CODEPOINT`](#catalyst_codepoint) | `CODEPOINT` | Four-byte Unicode codepoint representation. |
| [`CATALYST_ASCII_CODE_UNIT`](#catalyst_ascii_code_unit) | `ASCII_CODE_UNIT` | One ASCII code unit. |
| [`CATALYST_ASCII`](#catalyst_ascii) | `ASCII` | Read-only ASCII string pointer. |
| [`CATALYST_ASCIIW`](#catalyst_asciiw) | `ASCIIW` | Writable ASCII string pointer. |
| [`CATALYST_CP1252_CODE_UNIT`](#catalyst_cp1252_code_unit) | `CP1252_CODE_UNIT` | One Windows-1252 code unit. |
| [`CATALYST_CP1252`](#catalyst_cp1252) | `CP1252` | Read-only Windows-1252 string pointer. |
| [`CATALYST_CP1252W`](#catalyst_cp1252w) | `CP1252W` | Writable Windows-1252 string pointer. |
| [`CATALYST_UTF8_CODE_UNIT`](#catalyst_utf8_code_unit) | `UTF8_CODE_UNIT` | One UTF-8 code unit. |
| [`CATALYST_UTF8`](#catalyst_utf8) | `UTF8` | Read-only UTF-8 string pointer. |
| [`CATALYST_UTF8W`](#catalyst_utf8w) | `UTF8W` | Writable UTF-8 string pointer. |
| [`CATALYST_UTF16LE_CODE_UNIT`](#catalyst_utf16le_code_unit) | `UTF16LE_CODE_UNIT` | Two-byte UTF-16LE code unit. |
| [`CATALYST_UTF16LE`](#catalyst_utf16le) | `UTF16LE` | Read-only UTF-16LE string pointer. |
| [`CATALYST_UTF16LEW`](#catalyst_utf16lew) | `UTF16LEW` | Writable UTF-16LE string pointer. |

### Operation Status

| C Type | C++ Type | Description |
| --- | --- | --- |
| [`CATALYST_STATUS_CODE`](#catalyst_status_code) | `STATUS_CODE` | Identifies the outcome of an operation. |
| [`CATALYST_CONTEXT_CODE`](#catalyst_context_code) | `CONTEXT_CODE` | Identifies the context in which an operation was performed. |
| [`CATALYST_OPERATION_CODE`](#catalyst_operation_code) | `OPERATION_CODE` | Identifies a distinct operation within its execution context. |
| [`CATALYST_DETAIL_CODE`](#catalyst_detail_code) | `DETAIL_CODE` | Represents additional information about an operation. |
| [`CATALYST_RESULT`](#catalyst_result) | `RESULT` | Contains the four ordered result bytes. |

## Constant Summary

| Constants | Description |
| --- | --- |
| [Integer limits](#integer-limits) | Minimum and maximum values of numeric types. |
| [Boolean constants](#boolean-constants) | False and true. |
| [Text encoding constants](#text-encoding-constants) | The five encoding identifiers. |
| [Status code constants](#status-code-constants) | The universally defined operation outcomes. |
| [Status level mask](#status-level-mask) | Mask for the two most significant status bits. |
| [Result code constants](#result-code-constants) | Zero-valued C++ context, operation, and detail constants. |
| [Target configuration](#target-configuration) | Type-availability and language-version definitions. |

## Function Summary

### Status Code Predicates

| C Macro | C++ Function | Description |
| --- | --- | --- |
| [`CATALYST_STATUS_CODE_IS_SUCCESS`](#catalyst_status_code_is_success) | [`status_code_is_success`](#status_code_is_success) | Tests whether a status belongs to the Success level. |
| [`CATALYST_STATUS_CODE_IS_WARNING`](#catalyst_status_code_is_warning) | [`status_code_is_warning`](#status_code_is_warning) | Tests whether a status belongs to the Warning level. |
| [`CATALYST_STATUS_CODE_IS_ERROR`](#catalyst_status_code_is_error) | [`status_code_is_error`](#status_code_is_error) | Tests whether a status belongs to the Error level. |
| [`CATALYST_STATUS_CODE_IS_FATAL`](#catalyst_status_code_is_fatal) | [`status_code_is_fatal`](#status_code_is_fatal) | Tests whether a status belongs to the Fatal level. |

### C++ Value-Type Members

| Type | Member | Description |
| --- | --- | --- |
| [`CODEPOINT`](#codepoint-members) | [`CODEPOINT()`](#codepoint-default-constructor) | Initializes the default value. |
| [`CODEPOINT`](#codepoint-members) | [`CODEPOINT(BYTE, BYTE, BYTE, BYTE)`](#codepoint-field-constructor) | Initializes the individual fields. |
| [`CODEPOINT`](#codepoint-members) | [`CODEPOINT(const CATALYST_CODEPOINT&)`](#codepoint-copy-constructor) | Copies the C representation. |
| [`CODEPOINT`](#codepoint-members) | [`operator=(const CATALYST_CODEPOINT&)`](#codepoint-copy-assignment) | Assigns the C representation. |
| [`CODEPOINT`](#codepoint-members) | [`CODEPOINT(UINT)`](#codepoint-numeric-constructor) | Initializes from a native unsigned integer, when supported. |
| [`CODEPOINT`](#codepoint-members) | [`operator=(UINT)`](#codepoint-numeric-assignment) | Assigns from a native unsigned integer, when supported. |
| [`CODEPOINT`](#codepoint-members) | [`operator UINT() const`](#codepoint-numeric-conversion) | Converts to a native unsigned integer, when supported. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`UTF16LE_CODE_UNIT()`](#utf16le_code_unit-default-constructor) | Initializes the default value. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`UTF16LE_CODE_UNIT(BYTE, BYTE)`](#utf16le_code_unit-field-constructor) | Initializes the individual fields. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`UTF16LE_CODE_UNIT(const CATALYST_UTF16LE_CODE_UNIT&)`](#utf16le_code_unit-copy-constructor) | Copies the C representation. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`operator=(const CATALYST_UTF16LE_CODE_UNIT&)`](#utf16le_code_unit-copy-assignment) | Assigns the C representation. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`UTF16LE_CODE_UNIT(USHORT)`](#utf16le_code_unit-numeric-constructor) | Initializes from a native unsigned integer, when supported. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`operator=(USHORT)`](#utf16le_code_unit-numeric-assignment) | Assigns from a native unsigned integer, when supported. |
| [`UTF16LE_CODE_UNIT`](#utf16le_code_unit-members) | [`operator USHORT() const`](#utf16le_code_unit-numeric-conversion) | Converts to a native unsigned integer, when supported. |
| [`RESULT`](#result-members) | [`RESULT()`](#result-default-constructor) | Initializes the default value. |
| [`RESULT`](#result-members) | [`RESULT(STATUS_CODE, CONTEXT_CODE, OPERATION_CODE, DETAIL_CODE)`](#result-field-constructor) | Initializes the individual fields. |
| [`RESULT`](#result-members) | [`RESULT(const CATALYST_RESULT&)`](#result-copy-constructor) | Copies the C representation. |
| [`RESULT`](#result-members) | [`operator=(const CATALYST_RESULT&)`](#result-copy-assignment) | Assigns the C representation. |
| [`RESULT`](#result-members) | [`RESULT(UINT)`](#result-numeric-constructor) | Initializes from a native unsigned integer, when supported. |
| [`RESULT`](#result-members) | [`operator=(UINT)`](#result-numeric-assignment) | Assigns from a native unsigned integer, when supported. |
| [`RESULT`](#result-members) | [`operator UINT() const`](#result-numeric-conversion) | Converts to a native unsigned integer, when supported. |

## Type Details

### Numerics

All supported targets provide eight-bit byte types. Wider integer and floating-point types are conditional; use the corresponding availability macros. The native-width types are selected by the configured pointer size.

#### CATALYST_BIT

```c
typedef CATALYST_BYTE CATALYST_BIT;
```

Represents a bit with the values 0 and 1. The underlying byte does not enforce this range.

**C++:** `catalyst::BIT`.

**See Also:** [Integer limits](#integer-limits)

#### CATALYST_NIBBLE

```c
typedef CATALYST_BYTE CATALYST_NIBBLE;
```

Represents four bits with values from 0 through 15. The underlying byte does not enforce this range.

**C++:** `catalyst::NIBBLE`.

**See Also:** [Integer limits](#integer-limits)

#### CATALYST_BYTE

Unsigned eight-bit integer.

**C90:** Defined as `unsigned char` when its range matches.

**C99+:** Defined as `uint8_t`.

**C++:** `catalyst::BYTE`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_SBYTE

Signed eight-bit integer.

**C90:** Defined as `signed char` when its range matches.

**C99+:** Defined as `int8_t`.

**C++:** `catalyst::SBYTE`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_SHORT

Signed sixteen-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_16BIT` is 1. <span data-heading-badge="REQ. 16-BIT NUMERICS"></span>

**C90:** Defined as `signed short` when its range matches.

**C99+:** Defined as `int16_t`.

**C++:** `catalyst::SHORT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_USHORT

Unsigned sixteen-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_16BIT` is 1. <span data-heading-badge="REQ. 16-BIT NUMERICS"></span>

**C90:** Defined as `unsigned short` when its range matches.

**C99+:** Defined as `uint16_t`.

**C++:** `catalyst::USHORT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_INT

Signed thirty-two-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**C90:** Defined as `signed int` when its range matches.

**C99+:** Defined as `int32_t`.

**C++:** `catalyst::INT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_UINT

Unsigned thirty-two-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**C90:** Defined as `unsigned int` when its range matches.

**C99+:** Defined as `uint32_t`.

**C++:** `catalyst::UINT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_LONG

Signed sixty-four-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_64BIT` is 1. <span data-heading-badge="REQ. 64-BIT NUMERICS"></span>

**C90:** Not available.

**C99+:** Defined as `int64_t`.

**C++:** `catalyst::LONG`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_ULONG

Unsigned sixty-four-bit integer.

**REQUIRES:** `CATALYST_SUPPORTS_64BIT` is 1. <span data-heading-badge="REQ. 64-BIT NUMERICS"></span>

**C90:** Not available.

**C99+:** Defined as `uint64_t`.

**C++:** `catalyst::ULONG`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_SINGLE

```c
#if CATALYST_SUPPORTS_SINGLE
typedef float CATALYST_SINGLE;
#endif
```

A binary floating-point type with 24 bits of significand precision, exponent limits −125 through 128 as defined by the corresponding `<float.h>` macros, and storage of 4 bytes. Storage size is checked at compile time.

**REQUIRES:** `CATALYST_SUPPORTS_SINGLE` is 1. <span data-heading-badge="REQ. SINGLE PRECISION"></span>

**C++:** `catalyst::SINGLE`.

#### CATALYST_DOUBLE

```c
#if CATALYST_SUPPORTS_DOUBLE
typedef double CATALYST_DOUBLE;
#endif
```

A binary floating-point type with 53 bits of significand precision, exponent limits −1021 through 1024 as defined by the corresponding `<float.h>` macros, and storage of 8 bytes. Storage size is checked at compile time.

**REQUIRES:** `CATALYST_SUPPORTS_DOUBLE` is 1. <span data-heading-badge="REQ. DOUBLE PRECISION"></span>

**C++:** `catalyst::DOUBLE`.

#### CATALYST_BOOL

```c
typedef CATALYST_BYTE CATALYST_BOOL;
```

Represents a Boolean value. `CATALYST_FALSE` is 0 and `CATALYST_TRUE` is 1. The underlying byte does not enforce these two values.

**C++:** `catalyst::BOOL`.

**See Also:** [Boolean constants](#boolean-constants)

#### CATALYST_NINT

A signed integer matching the configured pointer width. Its underlying type is selected as follows:

| Pointer Size (bytes) | Underlying Type |
| --- | --- |
| 1 | `CATALYST_SBYTE` |
| 2 | `CATALYST_SHORT` |
| 4 | `CATALYST_INT` |
| 8 | `CATALYST_LONG` |

Compilation fails when the configured pointer size is unsupported or its corresponding integer type is unavailable. No pointer-conversion contract is defined.

**C++:** `catalyst::NINT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

#### CATALYST_NUINT

An unsigned integer matching the configured pointer width. Its underlying type is selected as follows:

| Pointer Size (bytes) | Underlying Type |
| --- | --- |
| 1 | `CATALYST_BYTE` |
| 2 | `CATALYST_USHORT` |
| 4 | `CATALYST_UINT` |
| 8 | `CATALYST_ULONG` |

Compilation fails when the configured pointer size is unsupported or its corresponding integer type is unavailable. No pointer-conversion contract is defined.

**C++:** `catalyst::NUINT`.

**See Also:** [Integer limits](#integer-limits), [Target configuration](#target-configuration).

### Text Encoding

String pointer types do not carry a length, capacity, or ownership information. The unsuffixed types provide read-only access to code units; the `W` suffix denotes writable access, not a wide-character representation. A null-terminated string ends with a zero code unit, not necessarily a zero byte.

#### CATALYST_TEXT_ENCODING

```c
typedef CATALYST_BYTE CATALYST_TEXT_ENCODING;
```

Identifies the encoding of text data. The defined values are listed under [Text encoding constants](#text-encoding-constants).

**C++:** `catalyst::TEXT_ENCODING`.

#### CATALYST_CODEPOINT

```c
typedef struct CATALYST_CODEPOINT {
    CATALYST_BYTE byte0;
    CATALYST_BYTE byte1;
    CATALYST_BYTE byte2;
    CATALYST_BYTE byte3;
} CATALYST_CODEPOINT;
```

Represents a Unicode codepoint using four ordered bytes, independent of native integer byte order. Unicode codepoints range from U+0000 through U+10FFFF; Unicode scalar values exclude the surrogate range U+D800–U+DFFF. The structure itself does not validate these ranges.

**Fields:**

| Field | Description |
| --- | --- |
| `byte0` | Bits 31–24 (most significant byte). |
| `byte1` | Bits 23–16. |
| `byte2` | Bits 15–8. |
| `byte3` | Bits 7–0 (least significant byte). |

**C++:** `catalyst::CODEPOINT` publicly derives from this structure. See [CODEPOINT members](#codepoint-members).

#### CATALYST_ASCII_CODE_UNIT

```c
typedef CATALYST_BYTE CATALYST_ASCII_CODE_UNIT;
```

Represents a seven-bit ASCII code unit in a byte. Valid ASCII values are 0x00–0x7F; the byte type itself does not enforce this range.

**C++:** `catalyst::ASCII_CODE_UNIT`.

#### CATALYST_ASCII

```c
typedef const CATALYST_ASCII_CODE_UNIT* CATALYST_ASCII;
```

Pointer to read-only ASCII code units. The pointer itself does not guarantee null termination or a valid encoding.

**C++:** `catalyst::ASCII`.

#### CATALYST_ASCIIW

```c
typedef CATALYST_ASCII_CODE_UNIT* CATALYST_ASCIIW;
```

Pointer to writable ASCII code units. Buffer capacity and termination requirements are specified by the consuming operation.

**C++:** `catalyst::ASCIIW`.

#### CATALYST_CP1252_CODE_UNIT

```c
typedef CATALYST_BYTE CATALYST_CP1252_CODE_UNIT;
```

Represents one Windows-1252 code unit as a byte. The type does not distinguish assigned from unassigned encoding values.

**C++:** `catalyst::CP1252_CODE_UNIT`.

#### CATALYST_CP1252

```c
typedef const CATALYST_CP1252_CODE_UNIT* CATALYST_CP1252;
```

Pointer to read-only Windows-1252 code units. The pointer itself does not guarantee null termination or a valid encoding.

**C++:** `catalyst::CP1252`.

#### CATALYST_CP1252W

```c
typedef CATALYST_CP1252_CODE_UNIT* CATALYST_CP1252W;
```

Pointer to writable Windows-1252 code units. Buffer capacity and termination requirements are specified by the consuming operation.

**C++:** `catalyst::CP1252W`.

#### CATALYST_UTF8_CODE_UNIT

```c
typedef CATALYST_BYTE CATALYST_UTF8_CODE_UNIT;
```

Represents one UTF-8 code unit as a byte. A Unicode scalar may occupy one to four code units; an individual unit is not necessarily a complete scalar.

**C++:** `catalyst::UTF8_CODE_UNIT`.

#### CATALYST_UTF8

```c
typedef const CATALYST_UTF8_CODE_UNIT* CATALYST_UTF8;
```

Pointer to read-only UTF-8 code units. The pointer itself does not guarantee null termination or a valid encoding.

**C++:** `catalyst::UTF8`.

#### CATALYST_UTF8W

```c
typedef CATALYST_UTF8_CODE_UNIT* CATALYST_UTF8W;
```

Pointer to writable UTF-8 code units. Buffer capacity and termination requirements are specified by the consuming operation.

**C++:** `catalyst::UTF8W`.

#### CATALYST_UTF16LE_CODE_UNIT

```c
typedef struct CATALYST_UTF16LE_CODE_UNIT {
    CATALYST_BYTE lsb;
    CATALYST_BYTE msb;
} CATALYST_UTF16LE_CODE_UNIT;
```

Represents one sixteen-bit UTF-16LE code unit as two bytes in little-endian significance order. A supplementary Unicode scalar requires a pair of code units.

**Fields:**

| Field | Description |
| --- | --- |
| `lsb` | Least significant byte, stored first. |
| `msb` | Most significant byte, stored second. |

**C++:** `catalyst::UTF16LE_CODE_UNIT` publicly derives from this structure. See [UTF16LE_CODE_UNIT members](#utf16le_code_unit-members).

#### CATALYST_UTF16LE

```c
typedef const CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LE;
```

Pointer to read-only UTF-16LE code units. The pointer itself does not guarantee null termination or a valid encoding.

**C++:** `catalyst::UTF16LE`.

#### CATALYST_UTF16LEW

```c
typedef CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LEW;
```

Pointer to writable UTF-16LE code units. Buffer capacity and termination requirements are specified by the consuming operation.

**C++:** `catalyst::UTF16LEW`.

### Operation Status

A result describes the outcome and origin of an operation. Status meanings are universal; context and operation meanings are defined by the reporting implementation. Detail meanings are resolved in order of precedence: status, context, operation.

#### CATALYST_STATUS_CODE

```c
typedef CATALYST_BYTE CATALYST_STATUS_CODE;
```

Represents the outcome of a completed operation. Its meaning is independent of the context, operation, and detail values. The status code definitions and severity ranges are listed under [Status code constants](#status-code-constants) and [Status level mask](#status-level-mask).

**C++:** `catalyst::STATUS_CODE`.

#### CATALYST_CONTEXT_CODE

```c
typedef CATALYST_BYTE CATALYST_CONTEXT_CODE;
```

Identifies the context in which an operation was performed. Its meaning is unique within the associated implementation’s scope. Within Core, a context normally identifies a method within its module. The value must be zero for Success-level statuses, including `SUCCESS_NOOP`. Otherwise, its meaning is implementation-defined.

**C++:** `catalyst::CONTEXT_CODE`.

#### CATALYST_OPERATION_CODE

```c
typedef CATALYST_BYTE CATALYST_OPERATION_CODE;
```

Identifies a distinct operation within an execution context. Within a Core method, each distinct result-reporting site has its own operation code, including successful sites. Meanings are local to that context; consult the reporting method’s reference. Undefined values are unknown. On success, the caller supplies the method context because the context byte is zero.

**C++:** `catalyst::OPERATION_CODE`.

#### CATALYST_DETAIL_CODE

```c
typedef CATALYST_BYTE CATALYST_DETAIL_CODE;
```

Represents additional information about a completed operation. Its meaning is specific to the associated operation unless a status or context definition takes precedence (status first, then context). It may be a symbolic value or actual operation data. Zero may be a meaningful value, or indicate no additional detail; undefined values are unknown.

**C++:** `catalyst::DETAIL_CODE`.

#### CATALYST_RESULT

```c
typedef struct CATALYST_RESULT {
    CATALYST_STATUS_CODE status;
    CATALYST_CONTEXT_CODE context;
    CATALYST_OPERATION_CODE operation;
    CATALYST_DETAIL_CODE detail;
} CATALYST_RESULT;
```

Represents a result as four ordered byte fields.

**Fields:**

| Field | Type | Description |
| --- | --- | --- |
| `status` | [`CATALYST_STATUS_CODE`](#catalyst_status_code) | Outcome of the operation. |
| `context` | [`CATALYST_CONTEXT_CODE`](#catalyst_context_code) | Execution context. |
| `operation` | [`CATALYST_OPERATION_CODE`](#catalyst_operation_code) | Distinct operation within that context. |
| `detail` | [`CATALYST_DETAIL_CODE`](#catalyst_detail_code) | Additional information, when defined. |

Results are written in this reference as `0xSSCCOODD`, where `SS`, `CC`, `OO`, and `DD` are the status, context, operation, and detail bytes, respectively. The notation expresses field significance, not a requirement to pack the fields into a native integer.

For example, `0x81080100` denotes an invalid-argument error (`0x81`), context `0x08`, operation `0x01`, and argument index 0 (`0x00`). Context and operation assignments in this example are illustrative; their actual meaning depends on the reporting module and method.

Core functions that accept a final `CATALYST_RESULT* result` parameter write their result there when non-null. Passing null discards the result. A C aggregate has no automatic default initialization unless initialized by its declaration or storage duration.

**C++:** `catalyst::RESULT` publicly derives from this structure. See [RESULT members](#result-members).

## Constant Details

### Integer limits

The following macros define the bounds of the corresponding types. They remain available when including `catalyst.hpp`; optional integer types and their bounds exist only when supported.

| Minimum Constant | Value | Maximum Constant | Value | Requires |
| --- | --- | --- | --- | --- |
| `CATALYST_BIT_MIN` | 0 | `CATALYST_BIT_MAX` | 1 | — |
| `CATALYST_NIBBLE_MIN` | 0 | `CATALYST_NIBBLE_MAX` | 15 | — |
| `CATALYST_BYTE_MIN` | 0 | `CATALYST_BYTE_MAX` | 255 | — |
| `CATALYST_SBYTE_MIN` | Native minimum | `CATALYST_SBYTE_MAX` | 127 | — |
| `CATALYST_SHORT_MIN` | Native minimum | `CATALYST_SHORT_MAX` | 32,767 | `CATALYST_SUPPORTS_16BIT` is 1 |
| `CATALYST_USHORT_MIN` | 0 | `CATALYST_USHORT_MAX` | 65,535 | `CATALYST_SUPPORTS_16BIT` is 1 |
| `CATALYST_INT_MIN` | Native minimum | `CATALYST_INT_MAX` | 2,147,483,647 | `CATALYST_SUPPORTS_32BIT` is 1 |
| `CATALYST_UINT_MIN` | 0 | `CATALYST_UINT_MAX` | 4,294,967,295 | `CATALYST_SUPPORTS_32BIT` is 1 |
| `CATALYST_LONG_MIN` | −9,223,372,036,854,775,808 | `CATALYST_LONG_MAX` | 9,223,372,036,854,775,807 | `CATALYST_SUPPORTS_64BIT` is 1 |
| `CATALYST_ULONG_MIN` | 0 | `CATALYST_ULONG_MAX` | 18,446,744,073,709,551,615 | `CATALYST_SUPPORTS_64BIT` is 1 |
| `CATALYST_NINT_MIN` | Selected signed minimum | `CATALYST_NINT_MAX` | Selected signed maximum | — |
| `CATALYST_NUINT_MIN` | 0 | `CATALYST_NUINT_MAX` | Selected unsigned maximum | — |

For `SBYTE`, `SHORT`, and `INT`, the C90 header uses `SCHAR_MIN`, `SHRT_MIN`, and `INT_MIN`, respectively; the C99 header uses the corresponding exact-width minima (−128, −32,768, and −2,147,483,648). Native-width limits alias the selected type’s limits. No floating-point limit macros are provided by CATALYST.

### Boolean constants

| C Constant | C++ Constant | Value | Description |
| --- | --- | --- | --- |
| `CATALYST_FALSE` | `FALSE` | `0` | False. |
| `CATALYST_TRUE` | `TRUE` | `1` | True. |

### Text encoding constants

| C Constant | C++ Constant | Value | Description |
| --- | --- | --- | --- |
| `CATALYST_TEXT_ENCODING_UNKNOWN` | `TEXT_ENCODING_UNKNOWN` | `0x00` | Unknown or unspecified encoding. |
| `CATALYST_TEXT_ENCODING_ASCII` | `TEXT_ENCODING_ASCII` | `0x01` | ASCII. |
| `CATALYST_TEXT_ENCODING_CP1252` | `TEXT_ENCODING_CP1252` | `0x02` | Windows-1252. |
| `CATALYST_TEXT_ENCODING_UTF8` | `TEXT_ENCODING_UTF8` | `0x03` | UTF-8. |
| `CATALYST_TEXT_ENCODING_UTF16LE` | `TEXT_ENCODING_UTF16LE` | `0x04` | UTF-16 little-endian. |

### Status code constants

The following values are defined by FDEFSPEC and exposed by CATALYST. All other status values are reserved and their meanings are undefined. An em dash in the status-defined detail column does not prohibit an implementation from defining detail at the context or operation level.

C constants use the prefix `CATALYST_STATUS_CODE_`; corresponding C++ constants use `catalyst::STATUS_CODE_`.

#### Success

| C Constant | Value | Description | Status-defined Detail |
| --- | --- | --- | --- |
| `CATALYST_STATUS_CODE_SUCCESS` | `0x00` | Operation completed successfully. | — |
| `CATALYST_STATUS_CODE_SUCCESS_NOOP` | `0x01` | Operation completed successfully without performing an action. | — |

#### Warning

| C Constant | Value | Description | Status-defined Detail |
| --- | --- | --- | --- |
| `CATALYST_STATUS_CODE_WARNING` | `0x40` | Operation completed, but the result may be unexpected or undesirable. | — |
| `CATALYST_STATUS_CODE_WARNING_PARTIAL` | `0x41` | Operation completed only partially; output may be incomplete. | — |
| `CATALYST_STATUS_CODE_WARNING_DEPRECATED` | `0x42` | Operation used deprecated behavior that may no longer work in the future. | — |

#### Error

| C Constant | Value | Description | Status-defined Detail |
| --- | --- | --- | --- |
| `CATALYST_STATUS_CODE_ERROR` | `0x80` | Operation failed due to a recoverable error. | — |
| `CATALYST_STATUS_CODE_ERROR_INVALID_ARGUMENT` | `0x81` | An argument was invalid or out of range. | Zero-based index in the complete function signature, from left to right. |
| `CATALYST_STATUS_CODE_ERROR_INVALID_STATE` | `0x82` | The system is inconsistent, corrupted, or otherwise invalid. | — |
| `CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT` | `0x83` | Required input was malformed or contained invalid data. | — |
| `CATALYST_STATUS_CODE_ERROR_ACCESS_DENIED` | `0x84` | A required resource denied access. | — |
| `CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED` | `0x85` | The requested feature or behavior is not implemented. | — |
| `CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED` | `0x86` | The target system or configuration does not support the requested feature. | — |
| `CATALYST_STATUS_CODE_ERROR_TIMEOUT` | `0x87` | A required timeout occurred before completion. | — |
| `CATALYST_STATUS_CODE_ERROR_NOT_FOUND` | `0x88` | A required resource does not exist. | — |
| `CATALYST_STATUS_CODE_ERROR_INTERRUPTED` | `0x89` | An external event interrupted the operation. | — |
| `CATALYST_STATUS_CODE_ERROR_DEPENDENCY_FAILURE` | `0x8A` | A required dependency failed. | The dependency’s status code value. |
| `CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW` | `0x90` | A buffer was too small to hold the required data. | — |
| `CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED` | `0x91` | A memory allocation request was unsuccessful. | — |
| `CATALYST_STATUS_CODE_ERROR_IO_ERROR` | `0xA0` | An input/output error occurred. | — |

#### Fatal

| C Constant | Value | Description | Status-defined Detail |
| --- | --- | --- | --- |
| `CATALYST_STATUS_CODE_FATAL` | `0xC0` | Operation failed due to an unrecoverable error. | — |
| `CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION` | `0xC1` | A fundamental invariant was violated, indicating critical logic error or data corruption. | — |

For `ERROR_DEPENDENCY_FAILURE`, the operation code identifies the reporting operation, which may also identify the failing dependency. The reporting method’s documentation defines that code. The detail byte remains the dependency’s status, not a context or operation code.

### Status level mask

```c
#define CATALYST_STATUS_CODE_LEVEL_MASK ((CATALYST_STATUS_CODE) 0xC0)
```

Selects the two most significant bits of a status code. Every byte belongs to exactly one status level, even if the particular code is reserved.

| Level | Inclusive Range | Masked Value |
| --- | --- | --- |
| Success | `0x00`–`0x3F` | `0x00` |
| Warning | `0x40`–`0x7F` | `0x40` |
| Error | `0x80`–`0xBF` | `0x80` |
| Fatal | `0xC0`–`0xFF` | `0xC0` |

**C++:** `catalyst::STATUS_CODE_LEVEL_MASK`.

### Result code constants

These C++ constants have no corresponding named C constants.

| C++ Constant | Type | Value | Description |
| --- | --- | --- | --- |
| `CONTEXT_CODE_NONE` | `CONTEXT_CODE` | `0x00` | No context supplied; required for Success-level results. |
| `OPERATION_CODE_NONE` | `OPERATION_CODE` | `0x00` | Default operation value; consult the reporting method for any defined meaning. |
| `DETAIL_CODE_NONE` | `DETAIL_CODE` | `0x00` | Default detail value; zero may have a defined meaning for a particular result. |

## Function Details

The status predicates classify status levels, not individual named status values. They do not produce an operation result. The C forms are comparison-expression macros; the C++ functions return a `catalyst::BOOL`.

### CATALYST_STATUS_CODE_IS_SUCCESS

```c
#define CATALYST_STATUS_CODE_IS_SUCCESS(status) \
    ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_SUCCESS)
```

Tests whether the given status belongs to the Success level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

A true comparison expression for values 0x00–0x3F, inclusive; otherwise false. This includes reserved values within the range.

**See Also:** [`status_code_is_success`](#status_code_is_success), [Status level mask](#status-level-mask).

### status_code_is_success

```cpp
inline BOOL status_code_is_success(STATUS_CODE status);
```

Tests whether the given status belongs to the Success level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

`TRUE` for values 0x00–0x3F, inclusive; otherwise `FALSE`.

**See Also:** [`CATALYST_STATUS_CODE_IS_SUCCESS`](#catalyst_status_code_is_success).

### CATALYST_STATUS_CODE_IS_WARNING

```c
#define CATALYST_STATUS_CODE_IS_WARNING(status) \
    ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_WARNING)
```

Tests whether the given status belongs to the Warning level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

A true comparison expression for values 0x40–0x7F, inclusive; otherwise false. This includes reserved values within the range.

**See Also:** [`status_code_is_warning`](#status_code_is_warning), [Status level mask](#status-level-mask).

### status_code_is_warning

```cpp
inline BOOL status_code_is_warning(STATUS_CODE status);
```

Tests whether the given status belongs to the Warning level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

`TRUE` for values 0x40–0x7F, inclusive; otherwise `FALSE`.

**See Also:** [`CATALYST_STATUS_CODE_IS_WARNING`](#catalyst_status_code_is_warning).

### CATALYST_STATUS_CODE_IS_ERROR

```c
#define CATALYST_STATUS_CODE_IS_ERROR(status) \
    ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_ERROR)
```

Tests whether the given status belongs to the Error level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

A true comparison expression for values 0x80–0xBF, inclusive; otherwise false. This includes reserved values within the range.

**See Also:** [`status_code_is_error`](#status_code_is_error), [Status level mask](#status-level-mask).

### status_code_is_error

```cpp
inline BOOL status_code_is_error(STATUS_CODE status);
```

Tests whether the given status belongs to the Error level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

`TRUE` for values 0x80–0xBF, inclusive; otherwise `FALSE`.

**See Also:** [`CATALYST_STATUS_CODE_IS_ERROR`](#catalyst_status_code_is_error).

### CATALYST_STATUS_CODE_IS_FATAL

```c
#define CATALYST_STATUS_CODE_IS_FATAL(status) \
    ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_FATAL)
```

Tests whether the given status belongs to the Fatal level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

A true comparison expression for values 0xC0–0xFF, inclusive; otherwise false. This includes reserved values within the range.

**See Also:** [`status_code_is_fatal`](#status_code_is_fatal), [Status level mask](#status-level-mask).

### status_code_is_fatal

```cpp
inline BOOL status_code_is_fatal(STATUS_CODE status);
```

Tests whether the given status belongs to the Fatal level.

**Parameters:**

`status` — Status code to classify.

**Returns:**

`TRUE` for values 0xC0–0xFF, inclusive; otherwise `FALSE`.

**See Also:** [`CATALYST_STATUS_CODE_IS_FATAL`](#catalyst_status_code_is_fatal).

## C++ Member Details

All members below are defined inline in `catalyst.hpp` in namespace `catalyst`. Constructors and assignments copy or initialize representations; they do not validate codepoint, code-unit, or result semantics and do not emit `CATALYST_RESULT` values. Native integer conveniences are conditional and are not prerequisites for using the byte representations.

### CODEPOINT members

**Base type:** [`CATALYST_CODEPOINT`](#catalyst_codepoint).

#### CODEPOINT default constructor

```cpp
CODEPOINT();
```

Initializes the default value. All four bytes are zero (U+0000).

#### CODEPOINT field constructor

```cpp
CODEPOINT(BYTE byte0, BYTE byte1, BYTE byte2, BYTE byte3);
```

Initializes each field with its corresponding argument.

**Parameters:**

`byte0` — Most significant byte (bits 31–24).

`byte1` — Bits 23–16.

`byte2` — Bits 15–8.

`byte3` — Least significant byte (bits 7–0).

#### CODEPOINT copy constructor

```cpp
CODEPOINT(const ::CATALYST_CODEPOINT& codepoint);
```

Copies all fields from the C representation.

**Parameters:**

`codepoint` — Value to copy.

#### CODEPOINT copy assignment

```cpp
CODEPOINT& operator=(const ::CATALYST_CODEPOINT& codepoint);
```

Copies all fields from the C representation into this object.

**Parameters:**

`codepoint` — Value to assign.

**Returns:**

A reference to this object.

#### CODEPOINT numeric constructor

```cpp
explicit CODEPOINT(UINT value);
```

Initializes the fields from the unsigned integer. The four bytes correspond to the integer from most significant to least significant.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to initialize from.

#### CODEPOINT numeric assignment

```cpp
CODEPOINT& operator=(UINT value);
```

Assigns the fields from the unsigned integer using the same significance order as the numeric constructor.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to assign.

**Returns:**

A reference to this object.

#### CODEPOINT numeric conversion

```cpp
operator UINT() const;
```

Converts the stored fields to an unsigned integer.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Returns:**

The integer formed from `byte0` through `byte3`, in decreasing significance.

### UTF16LE_CODE_UNIT members

**Base type:** [`CATALYST_UTF16LE_CODE_UNIT`](#catalyst_utf16le_code_unit).

#### UTF16LE_CODE_UNIT default constructor

```cpp
UTF16LE_CODE_UNIT();
```

Initializes the default value. Both bytes are zero.

#### UTF16LE_CODE_UNIT field constructor

```cpp
UTF16LE_CODE_UNIT(BYTE msb, BYTE lsb);
```

Initializes each field with its corresponding argument. The argument order is high byte, then low byte; the stored field order remains low byte, then high byte.

**Parameters:**

`msb` — Most significant byte of the code unit.

`lsb` — Least significant byte of the code unit.

#### UTF16LE_CODE_UNIT copy constructor

```cpp
UTF16LE_CODE_UNIT(const ::CATALYST_UTF16LE_CODE_UNIT& codeunit);
```

Copies all fields from the C representation.

**Parameters:**

`codeunit` — Value to copy.

#### UTF16LE_CODE_UNIT copy assignment

```cpp
UTF16LE_CODE_UNIT& operator=(const ::CATALYST_UTF16LE_CODE_UNIT& codeunit);
```

Copies all fields from the C representation into this object.

**Parameters:**

`codeunit` — Value to assign.

**Returns:**

A reference to this object.

#### UTF16LE_CODE_UNIT numeric constructor

```cpp
explicit UTF16LE_CODE_UNIT(USHORT value);
```

Initializes the fields from the unsigned integer. The high eight bits become `msb`; the low eight bits become `lsb`. Storage order is unchanged.

**REQUIRES:** `CATALYST_SUPPORTS_16BIT` is 1. <span data-heading-badge="REQ. 16-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to initialize from.

#### UTF16LE_CODE_UNIT numeric assignment

```cpp
UTF16LE_CODE_UNIT& operator=(USHORT value);
```

Assigns the fields from the unsigned integer using the same significance order as the numeric constructor.

**REQUIRES:** `CATALYST_SUPPORTS_16BIT` is 1. <span data-heading-badge="REQ. 16-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to assign.

**Returns:**

A reference to this object.

#### UTF16LE_CODE_UNIT numeric conversion

```cpp
operator USHORT() const;
```

Converts the stored fields to an unsigned integer.

**REQUIRES:** `CATALYST_SUPPORTS_16BIT` is 1. <span data-heading-badge="REQ. 16-BIT NUMERICS"></span>

**Returns:**

The integer with `msb` in its high eight bits and `lsb` in its low eight bits.

### RESULT members

**Base type:** [`CATALYST_RESULT`](#catalyst_result).

#### RESULT default constructor

```cpp
RESULT();
```

Initializes the default value. `status` is `STATUS_CODE_SUCCESS`; the other three fields are zero.

#### RESULT field constructor

```cpp
RESULT(STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation, DETAIL_CODE detail);
```

Initializes each field with its corresponding argument.

**Parameters:**

`status` — Status code.

`context` — Context code.

`operation` — Operation code.

`detail` — Detail code.

#### RESULT copy constructor

```cpp
RESULT(const ::CATALYST_RESULT& result);
```

Copies all fields from the C representation.

**Parameters:**

`result` — Value to copy.

#### RESULT copy assignment

```cpp
RESULT& operator=(const ::CATALYST_RESULT& result);
```

Copies all fields from the C representation into this object.

**Parameters:**

`result` — Value to assign.

**Returns:**

A reference to this object.

#### RESULT numeric constructor

```cpp
explicit RESULT(UINT value);
```

Initializes the fields from the unsigned integer. The integer is interpreted in `0xSSCCOODD` significance order, independent of host byte order.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to initialize from.

#### RESULT numeric assignment

```cpp
RESULT& operator=(UINT value);
```

Assigns the fields from the unsigned integer using the same significance order as the numeric constructor.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Parameters:**

`value` — Numeric representation to assign.

**Returns:**

A reference to this object.

#### RESULT numeric conversion

```cpp
operator UINT() const;
```

Converts the stored fields to an unsigned integer.

**REQUIRES:** `CATALYST_SUPPORTS_32BIT` is 1. <span data-heading-badge="REQ. 32-BIT NUMERICS"></span>

**Returns:**

The integer in `0xSSCCOODD` significance order.

## Target Configuration

The public C header includes the generated `cmakecfg.h` and selects a numeric representation using the configured target C standard. The same selection applies when the C++ header is included.

### Configuration definitions

| Definition | Meaning |
| --- | --- |
| `TARGET_STDC_VERSION` | Selects `catalyst.c99.h` if defined and at least `199901L`; otherwise selects `catalyst.c90.h`. |
| `TARGET_SIZEOF_VOID_P` | Required pointer size in bytes: 1, 2, 4, or 8. |
| `CATALYST_CONSTEXPR` | Defined only in C++; expands to `constexpr` for C++11 or later, otherwise empty. |

### Availability definitions

| Macro | Meaning when 1 |
| --- | --- |
| `CATALYST_SUPPORTS_16BIT` | Sixteen-bit signed and unsigned integer types are available. |
| `CATALYST_SUPPORTS_32BIT` | Thirty-two-bit signed and unsigned integer types are available. |
| `CATALYST_SUPPORTS_64BIT` | Sixty-four-bit signed and unsigned integer types are available. |
| `CATALYST_SUPPORTS_SINGLE` | The required single-precision floating-point format is available. |
| `CATALYST_SUPPORTS_DOUBLE` | The required double-precision floating-point format is available. |

Each availability definition is 0 when its corresponding representation is unavailable. Byte types are required rather than optional.

### Numeric headers

| Header | Description |
| --- | --- |
| `catalyst.c90.h` | Uses `<limits.h>` to test native types. Requires eight-bit bytes, tests `short` for sixteen-bit support and `int` for thirty-two-bit support, and does not provide sixty-four-bit types. |
| `catalyst.c99.h` | Uses `<stdint.h>` exact-width types when the corresponding range macros are available. Requires the eight-bit pair. |

These headers are selected through `catalyst.h`; they are not separate modules. The compile-time size-check typedefs `CATALYST_requires_float_4_bytes` and `CATALYST_requires_double_8_bytes` are implementation checks, not application data types.

## See Also

- [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/)
- [C header source](https://github.com/catalystui/lib-core-c/blob/main/include/catalyst.h)
- [C++ header source](https://github.com/catalystui/lib-core-c/blob/main/include/catalyst.hpp)
- [C90 numeric header](https://github.com/catalystui/lib-core-c/blob/main/include/catalyst.c90.h)
- [C99 numeric header](https://github.com/catalystui/lib-core-c/blob/main/include/catalyst.c99.h)
