# UTF-8

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_utf8_encode

**C**

```c
void catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into 1–4 UTF-8 bytes and writes their count to produced. Destination must have room for four units. After argument validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf8w`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `produced`. |
| `ERROR` | 1 | 0 | Overlong encoding branch (unreachable for valid input). |
| `ERROR` | 2 | 0 | Surrogate. |
| `ERROR` | 3 | 0 | Above U+10FFFF. |

## catte_utf8_decode

**C**

```c
void catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-8 scalar and writes its width (1–4 units) to consumed. After argument validation, codepoint and consumed are zero on decoding failure.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf8`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `codepoint`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `consumed`. |
| `ERROR` | 1 | 0 | Zero unit before sequence completion. |
| `ERROR` | 2 | 0 | Invalid continuation. |
| `ERROR` | 3 | 0 | Overlong sequence. |
| `ERROR` | 4 | 0 | Surrogate. |
| `ERROR` | 5 | 0 | Above U+10FFFF. |
| `ERROR` | 6 | 0 | Invalid leading byte (including C0/C1). |

## catte_utf8_length

**C**

```c
void catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf8_length(UTF8 utf8, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. This is not a byte or grapheme count. After pointer validation, length is zero on failure.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf8`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `length`. |
| `ERROR` | 1 | 0 | Decode failure. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Overflow if the count reaches `CATALYST_NUINT_MAX`. |
