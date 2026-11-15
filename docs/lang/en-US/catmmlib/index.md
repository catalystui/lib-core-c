# Memory management (CATMMLIB)

Headers: `catmmlib.h` and `catmmlib.hpp`. Link CATMMLIB. The C++ wrappers have the same behavior and accept `catalyst::RESULT*`.

## Conventions

- All sizes are byte counts. `result` may be null; all other pointer parameters must be non-null, even for zero-length byte operations.
- Allocation returns zero-filled payloads; use only the requested extent. The payload is not the original page-aligned allocation base; no adjustable or over-alignment guarantee is provided.
- Release only live CATMMLIB allocations, through the same allocator backend. Foreign pointers, repeated frees and access during release are outside the contract.
- Copy, move, compare and set operate on any valid readable/writable byte storage, not just CATMMLIB allocations. They do not check capacity or allocate memory.
- All result contexts and operation codes are zero. Detail is zero unless specified below.

## Types

`CATALYST_MEMORY_HEADER` / `catalyst::MEMORY_HEADER` is allocator-owned metadata immediately preceding the payload. Do not modify it.

| Field | Type | Meaning |
| --- | --- | --- |
| `requested_size` | `CATALYST_NUINT` | Requested payload size in bytes. |
| `allocated_size` | `CATALYST_NUINT` | Tracked extent including the header; Linux rounds to pages, Windows and Darwin record header plus payload. |

## Functions

### catmm_alloc

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

### catmm_realloc

```c
void catmm_realloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::realloc(void** memory, NUINT size, RESULT* result);
```

Resizes the allocation addressed by `*memory`. Null `*memory` delegates to allocation. A non-null allocation and size zero delegates to release and clears the pointer on success. Otherwise allocates a replacement, copies the smaller of the old and new requested sizes, and releases the old allocation. New bytes are zero-filled. Even equal sizes replace the allocation.

On success old pointers are invalid. On replacement allocation or old-block release failure the pointer value remains unchanged; the replacement is released on a best-effort basis. The old allocation's usability is not guaranteed after release failure.

**Results:** `ERROR_INVALID_ARGUMENT` (detail 0) for null memory output; otherwise the result from allocation or release, or `SUCCESS` (detail 0) on successful replacement. The cleanup result is not propagated.

### catmm_free

```c
void catmm_free(void* memory, CATALYST_RESULT* result);
```

```cpp
void catalyst::free(void* memory, RESULT* result);
```

Releases the allocation at the original payload address. The caller's pointer is not cleared. Allocation metadata must remain intact.

**Results:** `SUCCESS` on release; `ERROR_INVALID_ARGUMENT` with detail 0 for a null pointer, or on native release failure (Linux narrowed errno, Windows/Darwin detail 0). No payload access guarantee after release failure.

### catmm_copy

```c
void catmm_copy(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::copy(void* destination, const void* source, NUINT size, RESULT* result);
```

Copies `size` bytes from `source` to `destination`. Regions must not overlap, except identical addresses are allowed.

**Results:** `SUCCESS` after copying; `SUCCESS_NOOP` for zero size or identical addresses; `ERROR_INVALID_ARGUMENT` detail 0 = null destination, 1 = null source.

### catmm_move

```c
void catmm_move(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::move(void* destination, const void* source, NUINT size, RESULT* result);
```

Copies `size` bytes from `source` to `destination`, allowing overlapping regions. The direction of copying preserves the original source bytes.

**Results:** `SUCCESS` after copying; `SUCCESS_NOOP` for zero size or identical addresses; `ERROR_INVALID_ARGUMENT` detail 0 = null destination, 1 = null source.

### catmm_compare

```c
void catmm_compare(const void* first, const void* second, CATALYST_NUINT size, CATALYST_SBYTE* comparison, CATALYST_RESULT* result);
```

```cpp
void catalyst::compare(const void* first, const void* second, NUINT size, SBYTE* comparison, RESULT* result);
```

Compares `size` unsigned bytes lexicographically. Writes -1 to `*comparison` if the first region precedes the second, 0 if equal, or 1 if the first follows the second. Empty regions or identical pointers compare equal. The comparison output must not overlap the input regions.

**Results:** `SUCCESS` on comparison; `ERROR_INVALID_ARGUMENT` detail 0 = null first, 1 = null second, 3 = null comparison output. Invalid arguments leave the comparison output unchanged.

### catmm_set

```c
void catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size, CATALYST_RESULT* result);
```

```cpp
void catalyst::set(void* memory, BYTE value, NUINT size, RESULT* result);
```

Writes `value` to every byte of the `size`-byte region at `memory`.

**Results:** `SUCCESS` after filling; `SUCCESS_NOOP` for zero size; `ERROR_INVALID_ARGUMENT` detail 0 = null memory.

## Allocation backends

| Target | Allocation / release |
| --- | --- |
| Windows | VirtualAlloc / VirtualFree |
| Darwin | vm_allocate / vm_deallocate |
| Linux | mmap / munmap |

## API export definitions

`CATMMLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATMMLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATMMLIB_USE_DLL` is defined. Otherwise it is empty. Build and link with matching library variants.
