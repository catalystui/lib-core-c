# Allocation and release

See [CATMMLIB conventions](../index.md#conventions) for shared requirements and result conventions.

All functions return `CATALYST_RESULT` by value. Every outcome carries the reporting method's context. See the consolidated API reference for exact result codes; the topic tables below may describe earlier operation and detail assignments.

## catmm_alloc

Allocates `size` zero-filled bytes and stores the address in `*memory`. The output is cleared before allocation and remains null on failure. An existing pointer is replaced without releasing its allocation.

```c
CATALYST_RESULT catmm_alloc(void** memory, CATALYST_NUINT size);
```

| Parameter | Description |
| --- | --- |
| `memory` | Receives the allocation address; cleared on failure. |
| `size` | Requested allocation size in bytes; must be nonzero. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Payload allocated. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `memory` output.<br>**Detail `1`:** Zero `size`. |
| `ERROR_BUFFER_OVERFLOW` | **Detail `0`:** Header plus payload overflow.<br>**Detail `1`:** Page-rounding overflow (Linux). |
| `ERROR_INVALID_STATE` | Page-size query failed (Linux). Detail: low byte of `errno`. |
| `ERROR_ALLOCATION_FAILED` | Native allocation failed. Detail: low byte of `errno` on Linux; `0` on Windows/Darwin. |

## catmm_realloc

Resizes the allocation at `*memory`, preserving the smaller of the original and requested byte counts. New bytes are zero-filled. A null allocation is treated as an allocation request; a zero size releases a non-null allocation and clears its pointer on success. Even an unchanged size replaces the allocation.

On replacement allocation or old-block release failure, the pointer value is unchanged. The replacement is released on a best-effort basis; the old allocation may no longer be usable after a release failure. On successful replacement, old pointers are invalid.

```c
CATALYST_RESULT catmm_realloc(void** memory, CATALYST_NUINT size);
```

| Parameter | Description |
| --- | --- |
| `memory` | Address of the allocation pointer; updated on success. |
| `size` | New allocation size in bytes; zero releases a non-null allocation. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Replacement or delegated operation succeeded. |
| `ERROR_INVALID_ARGUMENT` | Null `memory` output (detail `0`), or invalid argument reported by allocation/release. |
| `ERROR_BUFFER_OVERFLOW` | Replacement allocation exceeds representable extent; detail as in allocation. |
| `ERROR_INVALID_STATE` | Linux page-size query failed. Detail: low byte of `errno`. |
| `ERROR_ALLOCATION_FAILED` | Replacement allocation failed; detail as in allocation. |

Allocation and release results are propagated from [`catmm_alloc`](#catmm_alloc) and [`catmm_free`](#catmm_free). The cleanup result is not propagated.

## catmm_free

Releases a live CATMMLIB allocation at its original payload address. The caller's pointer is not cleared, and the allocation metadata must be intact.

```c
CATALYST_RESULT catmm_free(void* memory);
```

| Parameter | Description |
| --- | --- |
| `memory` | Address of the memory region. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Allocation released. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `memory`.<br>Native release failed. Detail: low byte of `errno` on Linux; `0` on Windows/Darwin. |

No payload access guarantee after release failure.
