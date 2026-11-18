# Byte operations

See [CATMMLIB conventions](../index.md#conventions) for shared requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Every outcome carries the reporting method's context. See the consolidated API reference for exact result codes; the topic tables below may describe earlier operation and detail assignments.

## catmm_copy

Copies `size` bytes from `source` to `destination`. The regions must not overlap unless both pointers are identical.

```c
CATALYST_RESULT catmm_copy(void* destination, const void* source, CATALYST_NUINT size);
```

| Parameter | Description |
| --- | --- |
| `destination` | Writable destination region. |
| `source` | Readable source region. |
| `size` | Number of bytes to process. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Bytes copied. |
| `SUCCESS_NOOP` | No action for zero size or identical addresses. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `destination`.<br>**Detail `1`:** Null `source`. |

## catmm_move

Copies `size` bytes from `source` to `destination`, preserving the original source bytes even when the regions overlap.

```c
CATALYST_RESULT catmm_move(void* destination, const void* source, CATALYST_NUINT size);
```

| Parameter | Description |
| --- | --- |
| `destination` | Writable destination region. |
| `source` | Readable source region. |
| `size` | Number of bytes to process. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Bytes copied. |
| `SUCCESS_NOOP` | No action for zero size or identical addresses. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `destination`.<br>**Detail `1`:** Null `source`. |

## catmm_compare

Compares `size` unsigned bytes lexicographically and writes −1, 0, or 1 to `*comparison` according to whether the first region precedes, equals, or follows the second. Empty regions and identical addresses compare equal. The output must not overlap either input region.

```c
CATALYST_RESULT catmm_compare(const void* first, const void* second, CATALYST_NUINT size, CATALYST_SBYTE* comparison);
```

| Parameter | Description |
| --- | --- |
| `first` | First readable region. |
| `second` | Second readable region. |
| `size` | Number of bytes to process. |
| `comparison` | Receives −1, 0, or 1. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Comparison written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `first`.<br>**Detail `1`:** Null `second`.<br>**Detail `3`:** Null `comparison` output. |

Invalid arguments leave the comparison output unchanged.

## catmm_set

Fills the `size`-byte region at `memory` with `value`.

```c
CATALYST_RESULT catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size);
```

| Parameter | Description |
| --- | --- |
| `memory` | Address of the memory region. |
| `value` | Byte to write throughout the region. |
| `size` | Number of bytes to process. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Bytes filled. |
| `SUCCESS_NOOP` | No action for zero size. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `memory`. |
