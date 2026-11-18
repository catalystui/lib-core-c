# Windows-1252

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Result context, operation and detail default to zero unless indicated.

## catte_cp1252_encode

Encodes one codepoint as Windows-1252. Unrepresentable codepoints are replaced with `?`.

```c
CATALYST_RESULT catte_cp1252_encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w);
```

| Parameter | Description |
| --- | --- |
| `codepoint` | Codepoint to encode. |
| `cp1252w` | Writable buffer for one code unit. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint encoded. |
| `WARNING_PARTIAL` | Unrepresentable codepoint replaced with '?'. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `cp1252w`. |

## catte_cp1252_decode

Decodes one Windows-1252 code unit. Undefined bytes (`0x81`, `0x8D`, `0x8F`, `0x90`, `0x9D`) are replaced with U+003F (`?`).

```c
CATALYST_RESULT catte_cp1252_decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint);
```

| Parameter | Description |
| --- | --- |
| `cp1252` | Readable input containing one code unit. |
| `codepoint` | Receives the decoded codepoint. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint decoded. |
| `WARNING_PARTIAL` | Undefined byte replaced with '?'. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `cp1252`.<br>**Detail `1`:** Null `codepoint`. |

## catte_cp1252_length

Counts the code units in a null-terminated Windows-1252 string, excluding the terminator. Undefined bytes are counted without warning. `length` is written only on success.

```c
CATALYST_RESULT catte_cp1252_length(CATALYST_CP1252 cp1252, CATALYST_NUINT* length);
```

| Parameter | Description |
| --- | --- |
| `cp1252` | Readable null-terminated input. |
| `length` | Receives the number of code units before the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `cp1252`.<br>**Detail `1`:** Null `length`. |
| `ERROR_BUFFER_OVERFLOW` | The count reaches `CATALYST_NUINT_MAX`. |
