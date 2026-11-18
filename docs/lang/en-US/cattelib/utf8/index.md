# UTF-8

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Every outcome carries the reporting method's context. See the consolidated API reference for exact result codes; the topic tables below may describe earlier operation and detail assignments.

## catte_utf8_encode

Encodes a Unicode scalar as one to four UTF-8 code units. The destination must accommodate four units. After argument validation, `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

```c
CATALYST_RESULT catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced);
```

| Parameter | Description |
| --- | --- |
| `codepoint` | Codepoint to encode. |
| `utf8w` | Writable buffer for up to four code units. |
| `produced` | Receives the number of code units written. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint encoded. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf8w`.<br>**Detail `1`:** Null `produced`. |
| `ERROR` | **Operation `1`, Detail `0`:** Overlong encoding branch (unreachable for valid input).<br>**Operation `2`, Detail `0`:** Surrogate.<br>**Operation `3`, Detail `0`:** Above U+10FFFF. |

## catte_utf8_decode

Decodes one Unicode scalar from UTF-8 and reports the number of code units consumed (1–4). After argument validation, both `codepoint` and `consumed` are zero on decoding failure.

```c
CATALYST_RESULT catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed);
```

| Parameter | Description |
| --- | --- |
| `utf8` | Readable input containing the complete encoded sequence. |
| `codepoint` | Receives the decoded codepoint. |
| `consumed` | Receives the number of code units read. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint decoded. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf8`.<br>**Detail `1`:** Null `codepoint`.<br>**Detail `2`:** Null `consumed`. |
| `ERROR` | **Operation `1`, Detail `0`:** Zero unit before sequence completion.<br>**Operation `2`, Detail `0`:** Invalid continuation.<br>**Operation `3`, Detail `0`:** Overlong sequence.<br>**Operation `4`, Detail `0`:** Surrogate.<br>**Operation `5`, Detail `0`:** Above U+10FFFF.<br>**Operation `6`, Detail `0`:** Invalid leading byte (including C0/C1). |

## catte_utf8_length

Counts Unicode scalars in a null-terminated UTF-8 string, excluding the terminator and any leading BOM. This is not a byte or grapheme count. After argument validation, `length` is zero on failure.

```c
CATALYST_RESULT catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length);
```

| Parameter | Description |
| --- | --- |
| `utf8` | Readable null-terminated input. |
| `length` | Receives the number of Unicode scalars. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf8`.<br>**Detail `1`:** Null `length`. |
| `ERROR` | **Operation `1`:** Decode failure. |
| `ERROR_BUFFER_OVERFLOW` | The count reaches `CATALYST_NUINT_MAX`. |
