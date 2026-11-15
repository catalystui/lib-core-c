# UTF-16LE

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_utf16le_encode

**C**

```c
void catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into one code unit or a surrogate pair and writes the count to produced. Destination must have room for two units. After pointer validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf16lew`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `produced`. |
| `ERROR` | 1 | 0 | Above U+10FFFF. |
| `ERROR` | 2 | 0 | Surrogate codepoint. |

## catte_utf16le_decode

**C**

```c
void catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-16LE unit or surrogate pair; consumed receives 1 or 2. After argument validation, codepoint and consumed are zero on decoding failure.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf16le`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `codepoint`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `consumed`. |
| `ERROR` | 1 | 0 | Isolated low surrogate. |
| `ERROR` | 2 | 0 | High surrogate not followed by low surrogate. |

## catte_utf16le_length

**C**

```c
void catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::utf16le_length(UTF16LE utf16le, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. A surrogate pair counts as one. After pointer validation, length is zero on failure.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `utf16le`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `length`. |
| `ERROR` | 1 | 0 | Decode failure. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Overflow if the count reaches `CATALYST_NUINT_MAX`. |
