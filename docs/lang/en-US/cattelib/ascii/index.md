# ASCII

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_ascii_encode

```c
void catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw, RESULT* result);
```

Encodes one codepoint into one ASCII byte. Values outside U+0000–U+007F become '?'.

**Results:** SUCCESS for representable values; WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null asciiw.

## catte_ascii_decode

```c
void catte_ascii_decode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_decode(ASCII ascii, CODEPOINT* codepoint, RESULT* result);
```

Copies one ASCII byte into the least significant codepoint byte and zeroes the other three. The high bit is not validated.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null ascii, 1 = null codepoint.

## catte_ascii_length

```c
void catte_ascii_length(CATALYST_ASCII ascii, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::ascii_length(ASCII ascii, NUINT* length, RESULT* result);
```

Writes the number of nonzero bytes before the terminator to length, without validating ASCII values. Output is assigned only on success.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null ascii, 1 = null length; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.
