# UTF-16LE

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

All functions return `void`. Result context, operation and detail default to zero unless indicated.

## catte_utf16le_encode

Encodes a Unicode scalar as one UTF-16LE code unit or a surrogate pair. The destination must accommodate two units. After argument validation, `produced` is zero until encoding succeeds; an invalid scalar leaves the destination unchanged.

```c
void catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `codepoint` | Codepoint to encode. |
| `utf16lew` | Writable buffer for up to two code units. |
| `produced` | Receives the number of code units written. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint encoded. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf16lew`.<br>**Detail `1`:** Null `produced`. |
| `ERROR` | **Operation `1`, Detail `0`:** Above U+10FFFF.<br>**Operation `2`, Detail `0`:** Surrogate codepoint. |

## catte_utf16le_decode

Decodes one Unicode scalar from UTF-16LE and reports the number of code units consumed (1 or 2). After argument validation, both `codepoint` and `consumed` are zero on decoding failure.

```c
void catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `utf16le` | Readable input containing the complete encoded sequence. |
| `codepoint` | Receives the decoded codepoint. |
| `consumed` | Receives the number of code units read. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Codepoint decoded. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf16le`.<br>**Detail `1`:** Null `codepoint`.<br>**Detail `2`:** Null `consumed`. |
| `ERROR` | **Operation `1`, Detail `0`:** Isolated low surrogate.<br>**Operation `2`, Detail `0`:** High surrogate not followed by low surrogate. |

## catte_utf16le_length

Counts Unicode scalars in a null-terminated UTF-16LE string, excluding the terminator and any leading BOM. A surrogate pair counts as one scalar. After argument validation, `length` is zero on failure.

```c
void catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `utf16le` | Readable null-terminated input. |
| `length` | Receives the number of Unicode scalars. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `utf16le`.<br>**Detail `1`:** Null `length`. |
| `ERROR` | **Operation `1`:** Decode failure. |
| `ERROR_BUFFER_OVERFLOW` | The count reaches `CATALYST_NUINT_MAX`. |
