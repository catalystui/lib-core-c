# Allocation and release

See [CATMMLIB conventions](../index.md#conventions) for shared requirements and result conventions.

## catmm_alloc

```c
void catmm_alloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::alloc(void** memory, NUINT size, RESULT* result);
```

Allocates `size` nonzero bytes and writes the address to `*memory`. After validating the output pointer, clears `*memory`; it remains null on failure. An existing pointer is replaced without freeing its allocation.

| Status | Detail |
| --- | --- |
| `SUCCESS` | 0; payload allocated. |
| `ERROR_INVALID_ARGUMENT` | 0 = null memory output; 1 = zero size. |
| `ERROR_BUFFER_OVERFLOW` | 0 = header plus payload overflow; Linux 1 = page-rounding overflow. |
| `ERROR_INVALID_STATE` | Linux page-size query failed; errno narrowed to a detail byte. |
| `ERROR_ALLOCATION_FAILED` | Native allocation failed; Linux narrowed errno, Windows/Darwin 0. |

## catmm_realloc

```c
void catmm_realloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::realloc(void** memory, NUINT size, RESULT* result);
```

Resizes the allocation addressed by `*memory`. Null `*memory` delegates to allocation. A non-null allocation and size zero delegates to release and clears the pointer on success. Otherwise allocates a replacement, copies the smaller of the old and new requested sizes, and releases the old allocation. New bytes are zero-filled. Even equal sizes replace the allocation.

On success old pointers are invalid. On replacement allocation or old-block release failure the pointer value remains unchanged; the replacement is released on a best-effort basis. The old allocation's usability is not guaranteed after release failure.

**Results:** `ERROR_INVALID_ARGUMENT` (detail 0) for null memory output; otherwise the result from allocation or release, or `SUCCESS` (detail 0) on successful replacement. The cleanup result is not propagated.

## catmm_free

```c
void catmm_free(void* memory, CATALYST_RESULT* result);
```

```cpp
void catalyst::free(void* memory, RESULT* result);
```

Releases the allocation at the original payload address. The caller's pointer is not cleared. Allocation metadata must remain intact.

**Results:** `SUCCESS` on release; `ERROR_INVALID_ARGUMENT` with detail 0 for a null pointer, or on native release failure (Linux narrowed errno, Windows/Darwin detail 0). No payload access guarantee after release failure.
