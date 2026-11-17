# CATMMLIB

Provides memory allocation, release, resizing, and byte operations.

**C Header:** `catmmlib.h`  
**C++ Header:** `catmmlib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catmm`

## Conventions

- All sizes are byte counts. `result` may be null; all other pointer parameters must be non-null, even for zero-length byte operations.
- Allocation returns zero-filled payloads; use only the requested extent. The payload is not the original page-aligned allocation base; no adjustable or over-alignment guarantee is provided.
- Release only live CATMMLIB allocations, through the same allocator backend. Foreign pointers, repeated frees and access during release are outside the contract.
- Copy, move, compare and set operate on any valid readable/writable byte storage, not just CATMMLIB allocations. They do not check capacity or allocate memory.

In result patterns, `DD` is a runtime detail byte (`0x00`–`0xFF`). Allocation and release use the same context and operation IDs across backends.

## Type Summary

| C Type | C++ Type | Description |
| --- | --- | --- |
| [`CATALYST_MEMORY_HEADER`](#catalyst_memory_header) | `MEMORY_HEADER` | Allocator-owned allocation metadata. |

## Constant Summary

| Constant | Description |
| --- | --- |
| [`CATMMLIB_API`](#catmmlib_api) | Export/import annotation. |

## Function Summary

### Allocation and release

| Function | Description |
| --- | --- |
| [`catmm_alloc`](#catmm_alloc) | Allocates `size` zero-filled bytes and stores the address in `*memory`. |
| [`catmm_realloc`](#catmm_realloc) | Resizes the allocation at `*memory`, preserving the smaller of the original and requested byte counts. |
| [`catmm_free`](#catmm_free) | Releases a live CATMMLIB allocation at its original payload address. |

### Byte operations

| Function | Description |
| --- | --- |
| [`catmm_copy`](#catmm_copy) | Copies `size` bytes from `source` to `destination`. |
| [`catmm_move`](#catmm_move) | Copies `size` bytes from `source` to `destination`, preserving the original source bytes even when the regions overlap. |
| [`catmm_compare`](#catmm_compare) | Compares `size` unsigned bytes lexicographically and writes −1, 0, or 1 to `*comparison` according to whether the first region precedes, equals, or follows the second. |
| [`catmm_set`](#catmm_set) | Fills the `size`-byte region at `memory` with `value`. |

## Type Details

### CATALYST_MEMORY_HEADER

```c
typedef struct CATALYST_MEMORY_HEADER {
    CATALYST_NUINT requested_size;
    CATALYST_NUINT allocated_size;
} CATALYST_MEMORY_HEADER;
```

Allocator-owned metadata immediately preceding the payload. Do not modify it.

**Fields:**

`requested_size` — Requested payload size in bytes.

`allocated_size` — Tracked extent including the header; Linux rounds to pages, Windows and Darwin record header plus payload.

**C++:** `catalyst::MEMORY_HEADER`.

## Constant Details

### CATMMLIB_API

`CATMMLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATMMLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATMMLIB_USE_DLL` is defined. Otherwise it is empty. Build and link with matching library variants.

## Function Details

### Allocation and release

#### catmm_alloc

```c
void catmm_alloc(
    void** memory,
    CATALYST_NUINT size,
    CATALYST_RESULT* result
);
```

Allocates `size` zero-filled bytes and stores the address in `*memory`. The output is cleared before allocation and remains null on failure. An existing pointer is replaced without releasing its allocation.

**Parameters:**

`memory` — Receives the allocation address; cleared on failure.

`size` — Requested allocation size in bytes; must be nonzero.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | — | Allocate payload | — |
| `0x81010100` | `ERROR_INVALID_ARGUMENT` | `0x01` (catmm_alloc) | Validate `memory` | Argument 0 (`memory`) |
| `0x81010201` | `ERROR_INVALID_ARGUMENT` | `0x01` (catmm_alloc) | Validate `size` | Argument 1 (`size`) |
| `0x820103DD` | `ERROR_INVALID_STATE` | `0x01` (catmm_alloc) | Query page size (Linux) | Linux `errno` if 1–255, otherwise zero. |
| `0x90010400` | `ERROR_BUFFER_OVERFLOW` | `0x01` (catmm_alloc) | Check header plus payload extent | — |
| `0x90010500` | `ERROR_BUFFER_OVERFLOW` | `0x01` (catmm_alloc) | Check page rounding (Linux) | — |
| `0x910106DD` | `ERROR_ALLOCATION_FAILED` | `0x01` (catmm_alloc) | Allocate native memory | Linux `errno` if 1–255, otherwise zero; zero on Windows/Darwin. |

**C++:** `catalyst::alloc(void** memory, NUINT size, RESULT* result)`.


#### catmm_realloc

```c
void catmm_realloc(
    void** memory,
    CATALYST_NUINT size,
    CATALYST_RESULT* result
);
```

Resizes the allocation at `*memory`, preserving the smaller of the original and requested byte counts. New bytes are zero-filled. A null allocation is treated as an allocation request; a zero size releases a non-null allocation and clears its pointer on success. Even an unchanged size replaces the allocation.

On replacement allocation or old-block release failure, the pointer value is unchanged. The replacement is released on a best-effort basis; the old allocation may no longer be usable after a release failure. On successful replacement, old pointers are invalid.

**Parameters:**

`memory` — Address of the allocation pointer; updated on success.

`size` — New allocation size in bytes; zero releases a non-null allocation.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | — | Allocate when `*memory` is null | — |
| `0x00000300` | `SUCCESS` | — | Release when `size` is zero | — |
| `0x00000700` | `SUCCESS` | — | Replace allocation | — |
| `0x81020100` | `ERROR_INVALID_ARGUMENT` | `0x02` (catmm_realloc) | Validate `memory` | Argument 0 (`memory`) |
| `0x81020201` | `ERROR_INVALID_ARGUMENT` | `0x02` (catmm_realloc) | Allocate when `*memory` is null | Argument 1 (`size`) |
| `0x82020203` | `ERROR_INVALID_STATE` | `0x02` (catmm_realloc) | Allocate when `*memory` is null | `catmm_alloc` operation `0x03`: Query page size (Linux) |
| `0x82020302` | `ERROR_INVALID_STATE` | `0x02` (catmm_realloc) | Release when `size` is zero | `catmm_free` operation `0x02`: native release failed. |
| `0x82020403` | `ERROR_INVALID_STATE` | `0x02` (catmm_realloc) | Allocate replacement | `catmm_alloc` operation `0x03`: Query page size (Linux) |
| `0x82020602` | `ERROR_INVALID_STATE` | `0x02` (catmm_realloc) | Release old allocation | `catmm_free` operation `0x02`: native release failed; replacement released. |
| `0x90020204` | `ERROR_BUFFER_OVERFLOW` | `0x02` (catmm_realloc) | Allocate when `*memory` is null | `catmm_alloc` operation `0x04`: Check header plus payload extent |
| `0x90020205` | `ERROR_BUFFER_OVERFLOW` | `0x02` (catmm_realloc) | Allocate when `*memory` is null | `catmm_alloc` operation `0x05`: Check page rounding (Linux) |
| `0x90020404` | `ERROR_BUFFER_OVERFLOW` | `0x02` (catmm_realloc) | Allocate replacement | `catmm_alloc` operation `0x04`: Check header plus payload extent |
| `0x90020405` | `ERROR_BUFFER_OVERFLOW` | `0x02` (catmm_realloc) | Allocate replacement | `catmm_alloc` operation `0x05`: Check page rounding (Linux) |
| `0x91020206` | `ERROR_ALLOCATION_FAILED` | `0x02` (catmm_realloc) | Allocate when `*memory` is null | `catmm_alloc` operation `0x06`: Allocate native memory |
| `0x91020406` | `ERROR_ALLOCATION_FAILED` | `0x02` (catmm_realloc) | Allocate replacement | `catmm_alloc` operation `0x06`: Allocate native memory |
| `0xC1020502` | `FATAL_INVARIANT_VIOLATION` | `0x02` (catmm_realloc) | Release replacement after old-block release failed | `catmm_free` operation `0x02`: cleanup failed. |

If the allocation pointer is null and the requested size is zero, the delegated allocation reports argument 1 as invalid.

**C++:** `catalyst::realloc(void** memory, NUINT size, RESULT* result)`.


#### catmm_free

```c
void catmm_free(
    void* memory,
    CATALYST_RESULT* result
);
```

Releases a live CATMMLIB allocation at its original payload address. The caller's pointer is not cleared, and the allocation metadata must be intact.

**Parameters:**

`memory` — Address of the memory region.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Release allocation | — |
| `0x81030100` | `ERROR_INVALID_ARGUMENT` | `0x03` (catmm_free) | Validate `memory` | Argument 0 (`memory`) |
| `0x820302DD` | `ERROR_INVALID_STATE` | `0x03` (catmm_free) | Release native memory | Linux `errno` if 1–255, otherwise zero; zero on Windows/Darwin. |

No payload access guarantee after release failure.

**C++:** `catalyst::free(void* memory, RESULT* result)`.


### Byte operations

#### catmm_copy

```c
void catmm_copy(
    void* destination,
    const void* source,
    CATALYST_NUINT size,
    CATALYST_RESULT* result
);
```

Copies `size` bytes from `source` to `destination`. The regions must not overlap unless both pointers are identical.

**Parameters:**

`destination` — Writable destination region.

`source` — Readable source region.

`size` — Number of bytes to process.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | — | Copy bytes | — |
| `0x01000300` | `SUCCESS_NOOP` | — | Skip zero-size operation or identical addresses | — |
| `0x81040100` | `ERROR_INVALID_ARGUMENT` | `0x04` (catmm_copy) | Validate `destination` | Argument 0 (`destination`) |
| `0x81040201` | `ERROR_INVALID_ARGUMENT` | `0x04` (catmm_copy) | Validate `source` | Argument 1 (`source`) |

**C++:** `catalyst::copy(void* destination, const void* source, NUINT size, RESULT* result)`.


#### catmm_move

```c
void catmm_move(
    void* destination,
    const void* source,
    CATALYST_NUINT size,
    CATALYST_RESULT* result
);
```

Copies `size` bytes from `source` to `destination`, preserving the original source bytes even when the regions overlap.

**Parameters:**

`destination` — Writable destination region.

`source` — Readable source region.

`size` — Number of bytes to process.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | — | Move bytes | — |
| `0x01000300` | `SUCCESS_NOOP` | — | Skip zero-size operation or identical addresses | — |
| `0x81050100` | `ERROR_INVALID_ARGUMENT` | `0x05` (catmm_move) | Validate `destination` | Argument 0 (`destination`) |
| `0x81050201` | `ERROR_INVALID_ARGUMENT` | `0x05` (catmm_move) | Validate `source` | Argument 1 (`source`) |

**C++:** `catalyst::move(void* destination, const void* source, NUINT size, RESULT* result)`.


#### catmm_compare

```c
void catmm_compare(
    const void* first,
    const void* second,
    CATALYST_NUINT size,
    CATALYST_SBYTE* comparison,
    CATALYST_RESULT* result
);
```

Compares `size` unsigned bytes lexicographically and writes −1, 0, or 1 to `*comparison` according to whether the first region precedes, equals, or follows the second. Empty regions and identical addresses compare equal. The output must not overlap either input region.

**Parameters:**

`first` — First readable region.

`second` — Second readable region.

`size` — Number of bytes to process.

`comparison` — Receives −1, 0, or 1.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000400` | `SUCCESS` | — | Write comparison | — |
| `0x81060100` | `ERROR_INVALID_ARGUMENT` | `0x06` (catmm_compare) | Validate `first` | Argument 0 (`first`) |
| `0x81060201` | `ERROR_INVALID_ARGUMENT` | `0x06` (catmm_compare) | Validate `second` | Argument 1 (`second`) |
| `0x81060303` | `ERROR_INVALID_ARGUMENT` | `0x06` (catmm_compare) | Validate `comparison` | Argument 3 (`comparison`) |

Invalid arguments leave the comparison output unchanged.

**C++:** `catalyst::compare(const void* first, const void* second, NUINT size, SBYTE* comparison, RESULT* result)`.


#### catmm_set

```c
void catmm_set(
    void* memory,
    CATALYST_BYTE value,
    CATALYST_NUINT size,
    CATALYST_RESULT* result
);
```

Fills the `size`-byte region at `memory` with `value`.

**Parameters:**

`memory` — Address of the memory region.

`value` — Byte to write throughout the region.

`size` — Number of bytes to process.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Fill bytes | — |
| `0x01000200` | `SUCCESS_NOOP` | — | Skip zero-size fill | — |
| `0x81070100` | `ERROR_INVALID_ARGUMENT` | `0x07` (catmm_set) | Validate `memory` | Argument 0 (`memory`) |

**C++:** `catalyst::set(void* memory, BYTE value, NUINT size, RESULT* result)`.


## Allocation backends

| Target | Allocation / release |
| --- | --- |
| Windows | VirtualAlloc / VirtualFree |
| Darwin | vm_allocate / vm_deallocate |
| Linux | mmap / munmap |
