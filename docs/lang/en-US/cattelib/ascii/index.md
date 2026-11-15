# ASCII

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_ascii_encode

**C**

```c
void catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw, RESULT* result);
```

Encodes one codepoint into one ASCII byte. Values outside U+0000–U+007F become '?'.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for representable values. |
| `WARNING_PARTIAL` | 0 | 0 | Substituted output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `asciiw`. |

## catte_ascii_decode

**C**

```c
void catte_ascii_decode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::ascii_decode(ASCII ascii, CODEPOINT* codepoint, RESULT* result);
```

Copies one ASCII byte into the least significant codepoint byte and zeroes the other three. The high bit is not validated.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `ascii`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `codepoint`. |

## catte_ascii_length

**C**

```c
void catte_ascii_length(CATALYST_ASCII ascii, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::ascii_length(ASCII ascii, NUINT* length, RESULT* result);
```

Writes the number of nonzero bytes before the terminator to length, without validating ASCII values. Output is assigned only on success.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `ascii`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `length`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Overflow if the count reaches `CATALYST_NUINT_MAX`. |
