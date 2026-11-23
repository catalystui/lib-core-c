# CATMMLIB

Memory allocation and byte operations. Definitions follow [FRELSPEC](https://catalystui.org/specifications/languages/frelspec/).

**C Header:** `catmmlib.h`  
**C++ Header:** `catmmlib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catmm`  
**Dependencies:** [CATALYST](../catalyst/index.md)

## Summary

| Type | Meaning |
| --- | --- |
| [CATMMLIB_WORD](#catmmlib_word) | One memory byte. |
| [CATMMLIB_ADDRESS](#catmmlib_address) | Native address integer. |
| [CATMMLIB_POINTER](#catmmlib_pointer) | Readable memory. |
| [CATMMLIB_POINTERW](#catmmlib_pointerw) | Writable memory. |

| Macro | Meaning |
| --- | --- |
| [CATMMLIB_API](#catmmlib_api) | Native symbol export/import annotation. |

| Function | Behavior |
| --- | --- |
| [catmm_alloc](#catmm_alloc) | Allocate zero-filled bytes. |
| [catmm_realloc](#catmm_realloc) | Resize while preserving existing bytes and zero-filling growth. |
| [catmm_free](#catmm_free) | Release a live allocation. |
| [catmm_copy](#catmm_copy) | Copy non-overlapping bytes. |
| [catmm_move](#catmm_move) | Move possibly overlapping bytes. |
| [catmm_compare](#catmm_compare) | Compare unsigned bytes. |
| [catmm_set](#catmm_set) | Fill bytes. |

## Macros

### CATMMLIB_API

Export/import annotation for CATMMLIB's C ABI. On Windows, `CATMMLIB_EXPORTS` selects `__declspec(dllexport)` and `CATMMLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the macro is empty. Both static linking and non-Windows builds use an empty annotation.

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
produces a compilation error. The public signatures are the same in C90 and C99+,
but the native address representation varies by compiler and available headers.
The `uintptr_t` alternative requires `<stdint.h>`.

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
| `0x90010300` | `ERROR_BUFFER_OVERFLOW`: size plus private prefix cannot be represented | Reject extent | — |
| `0x86010400` | `ERROR_SYSTEM_NOT_SUPPORTED`: native page size is unavailable or unrepresentable (macOS) | Native allocation | — |
| `0x90010400` | `ERROR_BUFFER_OVERFLOW`: native page rounding overflows (macOS) | Native allocation | — |
| `0x84010400` | `ERROR_ACCESS_DENIED`: Mach allocation was denied (macOS) | Native allocation | — |
| `0x91010400` | `ERROR_ALLOCATION_FAILED`: native allocation failed | Native allocation | — |
| `0x00010500` | `SUCCESS` | Allocation complete | — |

**See Also:** [catmm_free](#catmm_free)

### catmm_realloc

```c
CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT new_size);
```

Resize an allocation. Retain the first `min(old_size, new_size)` bytes and zero-fill growth. Failure preserves the original pointer, bytes, and ownership. Success invalidates previous aliases and retains fundamental alignment. A null input allocates; zero size with a live input releases it and clears the pointer on success. On Unix mapping backends, a rare cleanup failure can retain a replacement
mapping internally for later cleanup; the caller remains responsible only for
the original allocation pointer.

**Parameters:**

- `memory`: Valid writable slot outside the allocation; contains null or a live CATMMLIB payload pointer.  
- `new_size`: Requested byte extent; must be nonzero when the input pointer is null.

**C++ equivalent:** `catalyst::realloc` (same parameters).

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00020300` | `SUCCESS` | Allocate from null | — |
| `0x90020300` | `ERROR_BUFFER_OVERFLOW`: allocation extent is unrepresentable | Allocate from null | — |
| `0x86020300` | `ERROR_SYSTEM_NOT_SUPPORTED`: native page size unsupported (macOS) | Allocate from null | — |
| `0x84020300` | `ERROR_ACCESS_DENIED`: Mach allocation denied (macOS) | Allocate from null | — |
| `0x91020300` | `ERROR_ALLOCATION_FAILED` | Allocate from null | — |
| `0x00020400` | `SUCCESS` | Release for zero size | — |
| `0x82020400` | `ERROR_INVALID_STATE`: native release or pending cleanup failed | Release for zero size | — |
| `0x84020400` | `ERROR_ACCESS_DENIED`: Mach release or pending cleanup denied (macOS) | Release for zero size | — |
| `0x90020500` | `ERROR_BUFFER_OVERFLOW`: resized extent plus prefix is unrepresentable | Reject extent | — |
| `0x91020600` | `ERROR_ALLOCATION_FAILED`: native heap resize failed (Windows) | Native resize | — |
| `0x00020700` | `SUCCESS`: unchanged or resized allocation | Resize complete | — |
| `0x82020800` | `ERROR_INVALID_STATE`: previously retained mapping cleanup failed (Unix) | Pending cleanup | — |
| `0x84020800` | `ERROR_ACCESS_DENIED`: pending mapping cleanup denied (macOS) | Pending cleanup | — |
| `0x86020900` | `ERROR_SYSTEM_NOT_SUPPORTED`: replacement allocation cannot use native page size (macOS) | Replacement allocation | — |
| `0x90020900` | `ERROR_BUFFER_OVERFLOW`: replacement allocation page rounding overflows (macOS) | Replacement allocation | — |
| `0x84020900` | `ERROR_ACCESS_DENIED`: replacement allocation denied (macOS) | Replacement allocation | — |
| `0x91020900` | `ERROR_ALLOCATION_FAILED`: replacement allocation failed (Unix) | Replacement allocation | — |
| `0xSS020ADD` | Both original release and replacement cleanup failed (Unix); `SS` is the selected failure status (`82` or `84` for current backends) | Dual cleanup failure | `DD` is `00` when both statuses match, otherwise the other failure's status (`82` or `84`) |
| `0x82020B00` | `ERROR_INVALID_STATE`: original release failed but replacement cleanup succeeded (Unix) | Original release | — |
| `0x84020B00` | `ERROR_ACCESS_DENIED`: Mach original release denied but replacement cleanup succeeded (macOS) | Original release | — |

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
| `0x82030300` | `ERROR_INVALID_STATE`: native release failed | Native release | — |
| `0x84030300` | `ERROR_ACCESS_DENIED`: Mach release denied (macOS) | Native release | — |
| `0x00030400` | `SUCCESS` | Release complete | — |
| `0x82030500` | `ERROR_INVALID_STATE`: pending mapping cleanup failed (Unix) | Pending cleanup | — |
| `0x84030500` | `ERROR_ACCESS_DENIED`: pending mapping cleanup denied (macOS) | Pending cleanup | — |

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

Move bytes as if through temporary storage, including overlapping extents. Identical addresses and zero size are no-ops. The implementation handles overlap without ordering unrelated pointers or converting pointers to integers. No alignment beyond byte alignment is required.

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
