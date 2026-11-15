# Byte operations

See [CATMMLIB conventions](../index.md#conventions) for shared requirements and result conventions.

## catmm_copy

```c
void catmm_copy(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::copy(void* destination, const void* source, NUINT size, RESULT* result);
```

Copies `size` bytes from `source` to `destination`. Regions must not overlap, except identical addresses are allowed.

**Results:** `SUCCESS` after copying; `SUCCESS_NOOP` for zero size or identical addresses; `ERROR_INVALID_ARGUMENT` detail 0 = null destination, 1 = null source.

## catmm_move

```c
void catmm_move(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::move(void* destination, const void* source, NUINT size, RESULT* result);
```

Copies `size` bytes from `source` to `destination`, allowing overlapping regions. The direction of copying preserves the original source bytes.

**Results:** `SUCCESS` after copying; `SUCCESS_NOOP` for zero size or identical addresses; `ERROR_INVALID_ARGUMENT` detail 0 = null destination, 1 = null source.

## catmm_compare

```c
void catmm_compare(const void* first, const void* second, CATALYST_NUINT size, CATALYST_SBYTE* comparison, CATALYST_RESULT* result);
```

```cpp
void catalyst::compare(const void* first, const void* second, NUINT size, SBYTE* comparison, RESULT* result);
```

Compares `size` unsigned bytes lexicographically. Writes -1 to `*comparison` if the first region precedes the second, 0 if equal, or 1 if the first follows the second. Empty regions or identical pointers compare equal. The comparison output must not overlap the input regions.

**Results:** `SUCCESS` on comparison; `ERROR_INVALID_ARGUMENT` detail 0 = null first, 1 = null second, 3 = null comparison output. Invalid arguments leave the comparison output unchanged.

## catmm_set

```c
void catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::set(void* memory, BYTE value, NUINT size, RESULT* result);
```

Writes `value` to every byte of the `size`-byte region at `memory`.

**Results:** `SUCCESS` after filling; `SUCCESS_NOOP` for zero size; `ERROR_INVALID_ARGUMENT` detail 0 = null memory.
