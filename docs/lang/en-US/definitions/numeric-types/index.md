# Numeric types

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

## Integer limits

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

## Target configuration

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
