# ASCII

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Every outcome carries the reporting method's context. See the consolidated API reference for exact result codes; the topic tables below may describe earlier operation and detail assignments.

## catte_ascii_encode

Encodes one codepoint as ASCII. Codepoints outside U+0000–U+007F are replaced with `?`.

```c
CATALYST_RESULT catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw);
```

| Parameter | Description |
| --- | --- |
| `codepoint` | Codepoint to encode. |
| `asciiw` | Writable buffer for one code unit. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint encoded. |
| `WARNING_PARTIAL` | Unrepresentable codepoint replaced with '?'. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `asciiw`. |

## catte_ascii_decode

Decodes a single ASCII code unit into a codepoint. The input byte becomes the least significant codepoint byte; the remaining bytes are zero. The ASCII range is not validated.

```c
CATALYST_RESULT catte_ascii_decode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint);
```

| Parameter | Description |
| --- | --- |
| `ascii` | Readable input containing one code unit. |
| `codepoint` | Receives the decoded codepoint. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint decoded. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `ascii`.<br>**Detail `1`:** Null `codepoint`. |

## catte_ascii_length

Counts the code units in a null-terminated ASCII string, excluding the terminator. ASCII values are not validated. `length` is written only on success.

```c
CATALYST_RESULT catte_ascii_length(CATALYST_ASCII ascii, CATALYST_NUINT* length);
```

| Parameter | Description |
| --- | --- |
| `ascii` | Readable null-terminated input. |
| `length` | Receives the number of code units before the terminator. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `ascii`.<br>**Detail `1`:** Null `length`. |
| `ERROR_BUFFER_OVERFLOW` | The count reaches `CATALYST_NUINT_MAX`. |
