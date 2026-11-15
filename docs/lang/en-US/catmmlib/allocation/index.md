# Allocation and release

See [CATMMLIB conventions](../index.md#conventions) for shared requirements and result conventions.

## catmm_alloc

**C**

```c
void catmm_alloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::alloc(void** memory, NUINT size, RESULT* result);
```

Allocates `size` nonzero bytes and writes the address to `*memory`. After validating the output pointer, clears `*memory`; it remains null on failure. An existing pointer is replaced without freeing its allocation.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Payload allocated. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `memory` output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Zero `size`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Header plus payload overflow. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 1 | Page-rounding overflow (Linux). |
| `ERROR_INVALID_STATE` | 0 | `errno` (low byte) | Page-size query failed (Linux). |
| `ERROR_ALLOCATION_FAILED` | 0 | Linux: `errno` (low byte); Windows/Darwin: 0 | Native allocation failed. |

## catmm_realloc

**C**

```c
void catmm_realloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::realloc(void** memory, NUINT size, RESULT* result);
```

Resizes the allocation addressed by `*memory`. Null `*memory` delegates to allocation. A non-null allocation and size zero delegates to release and clears the pointer on success. Otherwise allocates a replacement, copies the smaller of the old and new requested sizes, and releases the old allocation. New bytes are zero-filled. Even equal sizes replace the allocation.

On success old pointers are invalid. On replacement allocation or old-block release failure the pointer value remains unchanged; the replacement is released on a best-effort basis. The old allocation's usability is not guaranteed after release failure.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Replacement or delegated operation succeeded. |
| `ERROR_INVALID_ARGUMENT` | 0 | See allocation/release | Null `memory` output, or delegated allocation/release reported an invalid argument. |
| `ERROR_BUFFER_OVERFLOW` | 0 | See allocation | Replacement allocation exceeds representable extent. |
| `ERROR_INVALID_STATE` | 0 | `errno` (low byte) | Linux page-size query failed. |
| `ERROR_ALLOCATION_FAILED` | 0 | See allocation | Replacement allocation failed. |

Allocation and release results are propagated from [`catmm_alloc`](#catmm_alloc) and [`catmm_free`](#catmm_free). The cleanup result is not propagated.

## catmm_free

**C**

```c
void catmm_free(void* memory, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::free(void* memory, RESULT* result);
```

Releases the allocation at the original payload address. The caller's pointer is not cleared. Allocation metadata must remain intact.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Allocation released. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `memory`. |
| `ERROR_INVALID_ARGUMENT` | 0 | Linux: `errno` (low byte); Windows/Darwin: 0 | Native release failed. |

No payload access guarantee after release failure.
