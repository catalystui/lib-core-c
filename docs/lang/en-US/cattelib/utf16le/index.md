# UTF-16LE

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_utf16le_encode

```c
void catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced, RESULT* result);
```

Encodes one Unicode scalar into one code unit or a surrogate pair and writes the count to produced. Destination must have room for two units. After pointer validation, produced is zero until success; invalid scalars leave destination unchanged.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16lew, 1 = null produced; ERROR operation 1 = above U+10FFFF, 2 = surrogate codepoint.

## catte_utf16le_decode

```c
void catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed, RESULT* result);
```

Decodes one UTF-16LE unit or surrogate pair; consumed receives 1 or 2. After argument validation, codepoint and consumed are zero on decoding failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16le, 1 = null codepoint, 2 = null consumed; ERROR operation 1 = isolated low surrogate, 2 = high surrogate not followed by low surrogate.

## catte_utf16le_length

```c
void catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::utf16le_length(UTF16LE utf16le, NUINT* length, RESULT* result);
```

Counts decoded Unicode scalars before the zero terminator, excluding a leading BOM. A surrogate pair counts as one. After pointer validation, length is zero on failure.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null utf16le, 1 = null length; ERROR operation 1 = decode failure; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.
