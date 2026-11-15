# Windows-1252

See [CATTELIB conventions](../index.md#conventions) for buffer requirements and result conventions.

## catte_cp1252_encode

```c
void catte_cp1252_encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_encode(const CODEPOINT codepoint, CP1252W cp1252w, RESULT* result);
```

Encodes one codepoint as a Windows-1252 byte. Unrepresentable values become '?'.

**Results:** SUCCESS for representable values; WARNING_PARTIAL for substitution; ERROR_INVALID_ARGUMENT detail 0 = null cp1252w.

## catte_cp1252_decode

```c
void catte_cp1252_decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_decode(CP1252 cp1252, CODEPOINT* codepoint, RESULT* result);
```

Decodes one Windows-1252 byte into a codepoint. Undefined bytes 0x81, 0x8D, 0x8F, 0x90 and 0x9D become U+003F ('?').

**Results:** SUCCESS for defined bytes; WARNING_PARTIAL for undefined bytes; ERROR_INVALID_ARGUMENT detail 0 = null cp1252, 1 = null codepoint.

## catte_cp1252_length

```c
void catte_cp1252_length(CATALYST_CP1252 cp1252, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::cp1252_length(CP1252 cp1252, NUINT* length, RESULT* result);
```

Writes the number of nonzero bytes before the terminator to length. Undefined CP1252 bytes are counted without warning. Output is assigned only on success.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null cp1252, 1 = null length; ERROR_BUFFER_OVERFLOW if the count reaches CATALYST_NUINT_MAX.
