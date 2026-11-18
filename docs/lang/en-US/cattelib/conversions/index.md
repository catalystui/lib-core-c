# String conversions

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Every outcome carries the reporting method's context. See the consolidated API reference for exact result codes; the topic tables below may describe earlier operation and detail assignments.

Conversions write a destination terminator on success or warning; no destination BOM is added. On error a written prefix may remain unterminated; there is no rollback or produced-length output. A warning means substituted output, not a failed conversion.

When a conversion reports ERROR_DEPENDENCY_FAILURE, operation 1 denotes decoding and operation 2 denotes encoding; detail is the dependency's status byte. The dependency's context, operation and detail are not propagated.

## catte_ascii_to_cp1252

Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Source bytes above 0x7F are replaced by '?'.

```c
CATALYST_RESULT catte_ascii_to_cp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w);
```

| Parameter | Description |
| --- | --- |
| `ascii` | Readable null-terminated input. |
| `cp1252w` | Writable destination buffer. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | The index reaches `CATALYST_NUINT_MAX`. |

## catte_ascii_to_utf8

`capacity` specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

```c
CATALYST_RESULT catte_ascii_to_utf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `ascii` | Readable null-terminated input. |
| `utf8w` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |

## catte_ascii_to_utf16le

`capacity` specifies writable destination code units, including the terminator. Source bytes above 0x7F are replaced by '?'.

```c
CATALYST_RESULT catte_ascii_to_utf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `ascii` | Readable null-terminated input. |
| `utf16lew` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |

## catte_cp1252_to_ascii

Destination must hold at least the source byte count plus one terminator; no capacity parameter is provided. Undefined source bytes and values not representable in ASCII are replaced by '?'.

```c
CATALYST_RESULT catte_cp1252_to_ascii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW ascii);
```

| Parameter | Description |
| --- | --- |
| `cp1252` | Readable null-terminated input. |
| `ascii` | Writable destination buffer. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | The index reaches `CATALYST_NUINT_MAX`. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_cp1252_to_utf8

`capacity` specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

```c
CATALYST_RESULT catte_cp1252_to_utf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `cp1252` | Readable null-terminated input. |
| `utf8w` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_cp1252_to_utf16le

`capacity` specifies writable destination code units, including the terminator. Undefined source bytes are replaced by '?'.

```c
CATALYST_RESULT catte_cp1252_to_utf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `cp1252` | Readable null-terminated input. |
| `utf16lew` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf8_to_ascii

`capacity` specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

```c
CATALYST_RESULT catte_utf8_to_ascii(CATALYST_UTF8 utf8, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf8` | Readable null-terminated input. |
| `ascii` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf8_to_cp1252

`capacity` specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

```c
CATALYST_RESULT catte_utf8_to_cp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf8` | Readable null-terminated input. |
| `cp1252w` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf8_to_utf16le

`capacity` specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

```c
CATALYST_RESULT catte_utf8_to_utf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf8` | Readable null-terminated input. |
| `utf16lew` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Completed. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf16le_to_ascii

`capacity` specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

```c
CATALYST_RESULT catte_utf16le_to_ascii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf16le` | Readable null-terminated input. |
| `ascii` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf16le_to_cp1252

`capacity` specifies writable destination code units, including the terminator. Unrepresentable values are replaced by '?'.

```c
CATALYST_RESULT catte_utf16le_to_cp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf16le` | Readable null-terminated input. |
| `cp1252w` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Conversion completed without substitution. |
| `WARNING_PARTIAL` | Conversion completed with substitution. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |

## catte_utf16le_to_utf8

`capacity` specifies writable destination code units, including the terminator. All valid Unicode scalars are preserved.

```c
CATALYST_RESULT catte_utf16le_to_utf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);
```

| Parameter | Description |
| --- | --- |
| `utf16le` | Readable null-terminated input. |
| `utf8w` | Writable destination buffer. |
| `capacity` | Destination capacity in code units, including the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Completed. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `source`.<br>**Detail `1`:** Null `destination`. |
| `ERROR_BUFFER_OVERFLOW` | `capacity` is zero or insufficient for the next encoded value and terminator. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Decoder failed.<br>**Operation `2`:** Encoder failed.<br>Detail: dependency status byte. |
