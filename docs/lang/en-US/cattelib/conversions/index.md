# String conversions

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

Conversions write a destination terminator on success or warning; no destination BOM is added. On error a written prefix may remain unterminated; there is no rollback or produced-length output. A warning means substituted output, not a failed conversion.

When a conversion reports ERROR_DEPENDENCY_FAILURE, operation 1 denotes decoding and operation 2 denotes encoding; detail is the dependency's status byte. The dependency's context, operation and detail are not propagated.


## catte_ascii_to_cp1252

```c
void catte_ascii_to_cp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_cp1252(ASCII ascii, CP1252W cp1252w, RESULT* result);
```

Converts a null-terminated ASCII string to Windows-1252. Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if the index reaches CATALYST_NUINT_MAX

## catte_ascii_to_utf8

```c
void catte_ascii_to_utf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_utf8(ASCII ascii, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated ASCII string to UTF-8. capacity specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator

## catte_ascii_to_utf16le

```c
void catte_ascii_to_utf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_to_utf16le(ASCII ascii, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated ASCII string to UTF-16LE. capacity specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator

## catte_cp1252_to_ascii

```c
void catte_cp1252_to_ascii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW ascii, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_ascii(CP1252 cp1252, ASCIIW ascii, RESULT* result);
```

Converts a null-terminated Windows-1252 string to ASCII. Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Undefined source bytes and values not representable in ASCII are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if the index reaches CATALYST_NUINT_MAX; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_cp1252_to_utf8

```c
void catte_cp1252_to_utf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_utf8(CP1252 cp1252, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated Windows-1252 string to UTF-8. capacity specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_cp1252_to_utf16le

```c
void catte_cp1252_to_utf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_to_utf16le(CP1252 cp1252, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated Windows-1252 string to UTF-16LE. capacity specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf8_to_ascii

```c
void catte_utf8_to_ascii(CATALYST_UTF8 utf8, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_ascii(UTF8 utf8, ASCIIW ascii, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to ASCII. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf8_to_cp1252

```c
void catte_utf8_to_cp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_cp1252(UTF8 utf8, CP1252W cp1252w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to Windows-1252. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf8_to_utf16le

```c
void catte_utf8_to_utf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_to_utf16le(UTF8 utf8, UTF16LEW utf16lew, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-8 string to UTF-16LE. capacity specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf16le_to_ascii

```c
void catte_utf16le_to_ascii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_ascii(UTF16LE utf16le, ASCIIW ascii, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to ASCII. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf16le_to_cp1252

```c
void catte_utf16le_to_cp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_cp1252(UTF16LE utf16le, CP1252W cp1252w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to Windows-1252. capacity specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

**Results:** SUCCESS or WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).

## catte_utf16le_to_utf8

```c
void catte_utf16le_to_utf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_to_utf8(UTF16LE utf16le, UTF8W utf8w, NUINT capacity, RESULT* result);
```

Converts a null-terminated UTF-16LE string to UTF-8. capacity specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null source, 1 = null destination; ERROR_BUFFER_OVERFLOW if capacity is zero or insufficient for the next encoded value and terminator; ERROR_DEPENDENCY_FAILURE for a failed decoder or encoder (see conversion conventions).
