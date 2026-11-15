# UTF-8

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_utf8_encode

```c
void catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into 1–4 UTF-8 bytes and writes their count to produced. Destination must have room for four units. After argument validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8w, 1 = null produced; ERROR operation 1 = overlong encoding branch (unreachable for valid input), 2 = surrogate, 3 = above U+10FFFF.

## catte_utf8_decode

```c
void catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-8 scalar and writes its width (1–4 units) to consumed. After argument validation, codepoint and consumed are zero on decoding failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8, 1 = null codepoint, 2 = null consumed; ERROR operation 1 = zero unit before sequence completion, 2 = invalid continuation, 3 = overlong sequence, 4 = surrogate, 5 = above U+10FFFF, 6 = invalid leading byte (including C0/C1).

## catte_utf8_length

```c
void catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf8_length(UTF8 utf8, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. This is not a byte or grapheme count. After pointer validation, length is zero on failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf8, 1 = null length; ERROR operation 1 = decode failure; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.
