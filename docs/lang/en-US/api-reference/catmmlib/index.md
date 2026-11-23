# CATMMLIB

Memory allocation and byte operations. Definitions follow [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/).

**Headers:** `catmmlib.h` (C), `catmmlib.hpp` (C++)  
**Namespace:** `catalyst` (C++)  
**Library:** catmm  
**Dependencies:** [CATALYST](../catalyst/index.md)

## Summary

| Type | Meaning |
| --- | --- |
| [CATMMLIB_WORD](#catmmlib_word) | One memory byte. |
| [CATMMLIB_ADDRESS](#catmmlib_address) | Native address integer. |
| [CATMMLIB_POINTER](#catmmlib_pointer) | Readable memory. |
| [CATMMLIB_POINTERW](#catmmlib_pointerw) | Writable memory. |

| Function | Behavior |
| --- | --- |
| [catmm_alloc](#catmm_alloc) | Allocate zero-filled bytes. |
| [catmm_realloc](#catmm_realloc) | Resize, retaining the prefix and zero-filling growth. |
| [catmm_free](#catmm_free) | Release a live allocation. |
| [catmm_copy](#catmm_copy) | Copy non-overlapping bytes. |
| [catmm_move](#catmm_move) | Move possibly overlapping bytes. |
| [catmm_compare](#catmm_compare) | Compare unsigned bytes. |
| [catmm_set](#catmm_set) | Fill bytes. |

## Types

### CATMMLIB_WORD

One eight-bit memory unit.

```c
typedef CATALYST_BYTE CATMMLIB_WORD;
```

**C++ equivalent:** `catalyst::WORD`

### CATMMLIB_ADDRESS

Native unsigned integer capable of holding a pointer. This does not imply a
portable ordering of pointers or permit address arithmetic on arbitrary objects.
Availability is selected by generated `TARGET_ADDRESS_KIND`; an unsupported kind
produces a compilation error. Signatures are identical in C90 and C99+.

```c
#if TARGET_ADDRESS_KIND == 1
typedef __UINTPTR_TYPE__ CATMMLIB_ADDRESS;
#elif TARGET_ADDRESS_KIND == 2
typedef unsigned __int3264 CATMMLIB_ADDRESS;
#elif TARGET_ADDRESS_KIND == 3
typedef uintptr_t CATMMLIB_ADDRESS; /* <stdint.h> */
#endif
```

**C++ equivalent:** `catalyst::ADDRESS`

### CATMMLIB_POINTER

Readable memory; the pointer does not convey ownership or an extent.

```c
typedef const void* CATMMLIB_POINTER;
```

**C++ equivalent:** `catalyst::POINTER`

### CATMMLIB_POINTERW

Writable memory; the pointer does not convey ownership or an extent.

```c
typedef void* CATMMLIB_POINTERW;
```

**C++ equivalent:** `catalyst::POINTERW`

## Functions

### catmm_alloc

```c
CATALYST_RESULT catmm_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size);
```

Allocate `size` zero-filled bytes, with native fundamental alignment. Extended alignment is not supported. Private metadata reduces the largest representable payload. The allocation remains owned by the caller until released or successfully resized.

**Parameters:**

- `memory`: Valid writable output slot that does not own a live allocation; receives the allocation on success and null on every failure.  
- `size`: Nonzero requested extent in bytes.

**C++ equivalent:** `catalyst::alloc` (same parameters).

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0x90010300 | ERROR_BUFFER_OVERFLOW: private prefix plus size is unrepresentable | Extent check | — |
| 0x91010400 | ERROR_ALLOCATION_FAILED | Native allocation | — |
| 0x00010500 | SUCCESS | Allocation complete | — |

**See Also:** [catmm_free](#catmm_free)

### catmm_realloc

```c
CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT new_size);
```

Resize an allocation. Retain the first `min(old_size, new_size)` bytes and zero-fill growth. Failure preserves the original pointer, bytes, and ownership. Success invalidates previous aliases and retains fundamental alignment. A null input allocates; zero size with a live input releases it and clears the pointer on success. No separate replacement allocation needs cleanup after resize failure.

**Parameters:**

- `memory`: Valid writable slot outside the allocation; contains null or a live CATMMLIB payload pointer.  
- `new_size`: Requested byte extent; must be nonzero when the input pointer is null.

**C++ equivalent:** `catalyst::realloc` (same parameters).

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0x00020300 | SUCCESS | Allocate from null | — |
| 0x90020300 | ERROR_BUFFER_OVERFLOW | Allocate from null | — |
| 0x91020300 | ERROR_ALLOCATION_FAILED | Allocate from null | — |
| 0x00020400 | SUCCESS | Release for zero size | — |
| 0x82020400 | ERROR_INVALID_STATE: native release failed (Windows) | Release for zero size | — |
| 0x90020500 | ERROR_BUFFER_OVERFLOW | Extent check | — |
| 0x91020600 | ERROR_ALLOCATION_FAILED | Native resize | — |
| 0x00020700 | SUCCESS | Resize complete | — |

**See Also:** [catmm_alloc](#catmm_alloc)

### catmm_free

```c
CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory);
```

Release an allocation. Success ends its lifetime without clearing the caller’s pointer. On failure the block remains live and owned by the caller. Operations on the same allocation require external synchronization.

**Parameters:**

- `memory`: Live, non-null payload pointer returned by CATMMLIB; not an interior pointer or an already released allocation.

**C++ equivalent:** `catalyst::free` (same parameters).

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| 0x82030300 | ERROR_INVALID_STATE: native release failed (Windows) | Native release | — |
| 0x00030400 | SUCCESS | Release complete | — |

**See Also:** [catmm_alloc](#catmm_alloc)

### catmm_copy

```c
void catmm_copy(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size);
```

Copy bytes without allocation. Identical addresses and zero size are no-ops; otherwise the extents must not overlap. No alignment beyond byte alignment is required.

**Parameters:**

- `destination`: Valid non-null pointer to at least size writable bytes.  
- `source`: Valid non-null pointer to at least size readable bytes.  
- `size`: Byte count; both regions must remain accessible for the operation.

**C++ equivalent:** `catalyst::copy` (same parameters).

**See Also:** [catmm_move](#catmm_move)

### catmm_move

```c
void catmm_move(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size);
```

Move bytes as if through temporary storage, including overlapping extents. Identical addresses and zero size are no-ops. Pointer equality detects overlap without ordering unrelated objects or converting pointers to integers. No alignment beyond byte alignment is required.

**Parameters:**

- `destination`: Valid non-null pointer to at least size writable bytes.  
- `source`: Valid non-null pointer to at least size readable bytes.  
- `size`: Byte count; both regions must remain accessible for the operation.

**C++ equivalent:** `catalyst::move` (same parameters).

**See Also:** [catmm_copy](#catmm_copy)

### catmm_compare

```c
CATALYST_SBYTE catmm_compare(CATMMLIB_POINTER memory1, CATMMLIB_POINTER memory2, CATALYST_NUINT size);
```

Compare unsigned bytes lexicographically. Stop at the first difference; equal addresses or an empty extent compare equal. No alignment beyond byte alignment is required.

**Parameters:**

- `memory1`: Valid non-null pointer to at least size readable bytes.  
- `memory2`: Valid non-null pointer to at least size readable bytes.  
- `size`: Byte count; both regions must remain accessible for the operation.

**C++ equivalent:** `catalyst::compare` (same parameters).

**Returns:** `-1` when the first region is less, `0` when equal, `1` when greater.

**See Also:** [catmm_copy](#catmm_copy)

### catmm_set

```c
void catmm_set(CATMMLIB_POINTERW memory, CATMMLIB_WORD value, CATALYST_NUINT size);
```

Fill bytes with `value`. Zero size is a no-op. No alignment beyond byte alignment is required.

**Parameters:**

- `memory`: Valid non-null pointer to at least size writable bytes.  
- `value`: Byte value written to each unit.  
- `size`: Byte count accessible for the operation.

**C++ equivalent:** `catalyst::set` (same parameters).

**See Also:** [catmm_copy](#catmm_copy)
