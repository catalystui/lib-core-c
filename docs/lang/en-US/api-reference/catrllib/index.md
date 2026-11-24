# CATRLLIB

Provides Set, Map, Array, File, and Stream operations with managed memory.

**C Header:** `catrllib.h`  
**C++ Header:** `catrllib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catrl`  
**Dependencies:** [CATALYST](../catalyst/index.md), [CATMMLIB](../catmmlib/index.md)  
**Specification:** [FRELSPEC Revision 2.1](https://catalystui.org/specifications/languages/frelspec/)

## Contents

- [Summary](#summary)
- [Macros](#macros)
- [Types](#types)
- [Functions](#functions)

## Summary

| Type | Meaning |
| --- | --- |
| [CATRLLIB_SET](#catrllib_set) | Finite, unordered collection of distinct elements. |
| [CATRLLIB_MAP](#catrllib_map) | Finite collection of unique keys and their values. |
| [CATRLLIB_ARRAY](#catrllib_array) | Finite collection with contiguous indices. |
| [CATRLLIB_FILE](#catrllib_file) | Public metadata/data Arrays interpreted by a resolver. |
| [CATRLLIB_STREAM](#catrllib_stream) | Managed state and chunk storage for a finite or infinite producer-defined sequence. |

| Macro | Meaning |
| --- | --- |
| [CATRLLIB_API](#catrllib_api) | Native symbol export/import annotation. |

| Function | Behavior |
| --- | --- |
| [catrl_set_init](#catrl_set_init) | Initialize an empty Set without allocation. |
| [catrl_set_add](#catrl_set_add) | Copy an element if absent. |
| [catrl_set_remove](#catrl_set_remove) | Remove a member if present. |
| [catrl_set_contains](#catrl_set_contains) | Determine membership by byte equality. |
| [catrl_set_clear](#catrl_set_clear) | Empty the Set while retaining capacity. |
| [catrl_set_expand](#catrl_set_expand) | Request an exact larger element capacity. |
| [catrl_set_shrink](#catrl_set_shrink) | Halve capacity, retaining all members. |
| [catrl_set_free](#catrl_set_free) | Release owned storage. |
| [catrl_map_init](#catrl_map_init) | Initialize an empty Map without allocation. |
| [catrl_map_add](#catrl_map_add) | Copy a new key and value if the key is absent. |
| [catrl_map_remove](#catrl_map_remove) | Remove a key and its value if present. |
| [catrl_map_contains](#catrl_map_contains) | Determine key membership by byte equality. |
| [catrl_map_get](#catrl_map_get) | Copy the associated value if present. |
| [catrl_map_set](#catrl_map_set) | Replace the value of an existing key. |
| [catrl_map_clear](#catrl_map_clear) | Empty the Map while retaining capacity. |
| [catrl_map_expand](#catrl_map_expand) | Request an exact larger pair capacity. |
| [catrl_map_shrink](#catrl_map_shrink) | Halve capacity, retaining all pairs. |
| [catrl_map_free](#catrl_map_free) | Release owned key/value storage. |
| [catrl_array_init](#catrl_array_init) | Initialize an empty Array without allocation. |
| [catrl_array_add](#catrl_array_add) | Append an element copy. |
| [catrl_array_insert](#catrl_array_insert) | Insert an element and shift the suffix right. |
| [catrl_array_remove](#catrl_array_remove) | Remove an element and shift the suffix left. |
| [catrl_array_contains](#catrl_array_contains) | Determine membership by byte equality. |
| [catrl_array_get](#catrl_array_get) | Copy the element at an index. |
| [catrl_array_set](#catrl_array_set) | Replace the element at an index. |
| [catrl_array_clear](#catrl_array_clear) | Empty the Array while retaining capacity. |
| [catrl_array_expand](#catrl_array_expand) | Request an exact larger element capacity. |
| [catrl_array_shrink](#catrl_array_shrink) | Halve capacity, retaining elements and order. |
| [catrl_array_free](#catrl_array_free) | Release owned storage. |
| [catrl_file_init](#catrl_file_init) | Transfer metadata/data Array ownership into a File. |
| [catrl_file_get](#catrl_file_get) | Invoke the resolver on the current File contents. |
| [catrl_file_free](#catrl_file_free) | Release metadata and data Arrays. |
| [catrl_stream_init](#catrl_stream_init) | Allocate managed state and chunk-lifetime bookkeeping. |
| [catrl_stream_next](#catrl_stream_next) | Produce an exact-sized chunk into managed storage. |
| [catrl_stream_free](#catrl_stream_free) | Release managed chunks, state, and bookkeeping. |

The C++ aliases use the corresponding C structures, and the inline functions forward directly to the C API. Their ownership and failure guarantees are identical. Use `CATALYST_RESULT` as the callback return type in both languages to match the declared C function pointers.

## Macros

### CATRLLIB_API

Export/import annotation for CATRLLIB's C ABI. On Windows/Cygwin, `CATRLLIB_EXPORTS` selects `__declspec(dllexport)` and `CATRLLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the macro is empty. The `catrl` library depends on `catmm`. CMake's `catrllib_static` and `catrllib_shared` targets propagate the corresponding dependency.

## Types

### CATRLLIB_SET

A finite, unordered collection of distinct, fixed-size elements. Elements are compared bytewise, including padding.

```c
typedef struct CATRLLIB_SET {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_SET;
```

**Fields:**

- `data`: Contiguous element storage, null when unallocated. Only the first `cardinality` slots contain members.  
- `size`: Nonzero size of each element, in bytes. Fixed at initialization.  
- `cardinality`: Number of distinct members, from zero through `capacity`.  
- `capacity`: Number of element slots in the allocation. Its byte extent, `capacity * size`, must be representable in `CATALYST_NUINT`; CATMMLIB may impose a smaller limit.

**C++ equivalent:** `catalyst::SET`.

Owns its element storage. Membership uses a linear search. Direct access must preserve uniqueness.

### CATRLLIB_MAP

A finite collection associating each unique key with one value. Keys are compared bytewise, including padding; values need not be unique.

```c
typedef struct CATRLLIB_MAP {
    CATMMLIB_POINTERW keys;
    CATMMLIB_POINTERW values;
    CATALYST_NUINT ksize;
    CATALYST_NUINT vsize;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_MAP;
```

**Fields:**

- `keys`: Owned contiguous key storage. Only the first `cardinality` slots are live.  
- `values`: Independently owned value storage; value slot `i` belongs to key slot `i`.  
- `ksize`: Nonzero size of each key, in bytes. Fixed at initialization.  
- `vsize`: Nonzero size of each value, in bytes. Fixed at initialization.  
- `cardinality`: Number of live key/value pairs, from zero through `capacity`.  
- `capacity`: Number of pairs that both buffers can accommodate. Extra storage retained after a partial resize does not increase this value.

**C++ equivalent:** `catalyst::MAP`.

Key lookup is linear. Direct access must preserve key uniqueness and matching key/value slots.

Partial resizing or release may leave extra owned storage, even at zero capacity. See [catrl_map_expand](#catrl_map_expand), [catrl_map_shrink](#catrl_map_shrink), and [catrl_map_free](#catrl_map_free).

### CATRLLIB_ARRAY

A finite collection of fixed-size elements with contiguous, zero-based indices. Duplicate elements are permitted.

```c
typedef struct CATRLLIB_ARRAY {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
} CATRLLIB_ARRAY;
```

**Fields:**

- `data`: Owned contiguous storage, null when unallocated.  
- `size`: Nonzero bytes per individual element, fixed at initialization.  
- `length`: Number of live elements. Valid retrieval/replacement/removal indices are `[0, length)`.  
- `capacity`: Number of slots available without allocation; `length <= capacity`. The byte extent `capacity * size` must be representable and allocatable.

**C++ equivalent:** `catalyst::ARRAY`.

Owns its element storage. Indexed access and append require constant work apart from copying the element. Insertion and removal shift the affected suffix; membership uses a linear bytewise search. Direct access is limited to live elements.

### CATRLLIB_FILE

A collection resolving Arrays from data according to metadata. The resolver defines the format and finite, contiguous index domain independently of the underlying Array indices or any filesystem.

```c
typedef struct CATRLLIB_FILE CATRLLIB_FILE;
struct CATRLLIB_FILE {
    CATRLLIB_ARRAY metadata;
    CATRLLIB_ARRAY data;
    CATALYST_RESULT (*resolver)(
        const CATRLLIB_FILE* file,
        CATALYST_NUINT index,
        CATRLLIB_ARRAY* output
    );
};
```

**Fields:**

- `metadata`: Owned Array of metadata elements; its `size` is bytes per metadata element.  
- `data`: Owned Array of data elements; its `size` is bytes per data element and may differ from metadata's size.  
- `resolver`: Non-null callable while the File is available for resolution; cleared by successful File release.

**C++ equivalent:** `catalyst::FILE`.

The owned Arrays may be modified using Array operations, subject to the resolver's requirements. Resolution uses their current contents without CATRLLIB caching.

The resolver defines output ownership, lifetime, and validity on every outcome. Free owned outputs with [catrl_array_free](#catrl_array_free); borrowed views must not be resized or freed as owning Arrays.

### CATRLLIB_STREAM

A mechanism for generating or retrieving successive chunks from a finite or infinite sequence. The producer defines contents, order, and exhaustion; the consumer specifies chunk sizes and retention.

```c
typedef struct CATRLLIB_STREAM CATRLLIB_STREAM;
struct CATRLLIB_STREAM {
    CATMMLIB_POINTERW state;
    CATALYST_NUINT ssize;
    CATALYST_RESULT (*next)(
        CATRLLIB_STREAM* stream,
        CATMMLIB_POINTERW chunk,
        CATALYST_NUINT csize
    );
};
```

**Fields:**

- `state`: Pointer to the producer's state, stable during active production. CATRLLIB keeps private bookkeeping after the first `ssize` bytes; do not access that bookkeeping or replace, resize, or free the pointer independently.  
- `ssize`: Size of the producer's state, in bytes, excluding bookkeeping. Fixed at initialization; may be zero.  
- `next`: Producer function, non-null while active. Receives the Stream, a writable chunk buffer, and its requested size in bytes. A free attempt clears this field even if release fails.

**C++ equivalent:** `catalyst::STREAM`.

State and chunks have CATMMLIB fundamental alignment, not extended alignment. Zero-filled storage does not construct typed objects or necessarily represent null pointers. Initialize typed state as required.

A lifetime of three retains the three most recently published chunks; publishing a fourth expires the first. Calls without publication do not expire chunks. Returned pointers are borrowed until expiration or a free attempt; do not resize or free them independently.

There is no universal end-of-stream status or pointer sentinel. The producer defines whether further calls are valid.

## Functions

The preconditions in [Assumptions](../../assumptions/index.md) apply throughout. Initialize unused storage only; do not shallow-copy owning collections or modify bookkeeping. Synchronize access externally.

Set, Map, and Array copy fixed-size representations by value. Initialize representation bytes consistently, including padding. No typed comparison, construction, destruction, pointee release, or secure erasure is performed. Stream likewise owns raw storage, not referenced resources.

Capacity counts slots, not bytes. Reserve space before insertion; insertion never allocates. Only live slots represent elements. Successful resizing invalidates aliases even at the same address; failed resizing preserves that allocation. Use current pointers after Map resizing, including partial failure.

Copy inputs and outputs must be separate from the collection object and its allocations; search inputs may identify live elements or keys, including Map replacement keys. Allocation extents use `CATALYST_NUINT`; CATMMLIB supplies fundamental alignment and may impose smaller allocation limits. No wider numeric representation or platform-specific operation is required by CATRLLIB.

Result tables use logical `0xSSCCOODD` order ([FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/)). Dependency status and detail are retained except as specified; undefined status codes are reserved.

### catrl_set_init

```c
void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size);
```

Initialize an empty Set without allocating memory. All fields except `size` become zero or null.

**Parameters:**

- `set`: Valid writable unused `CATRLLIB_SET` storage.  
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::set_init`.

### catrl_set_add

```c
CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Copy an element into the Set if absent, increasing cardinality without allocating memory.

**Parameters:**

- `set`: Valid initialized Set; a new distinct element requires `cardinality < capacity`.  
- `element`: Non-null pointer to `set->size` readable bytes outside the Set object and its owned allocation.

**C++ equivalent:** `catalyst::set_add`.

**Returns:** `CATALYST_TRUE` if inserted; `CATALYST_FALSE` if already present, with no mutation. 

### catrl_set_remove

```c
CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Remove a member and retain capacity. Move the last member into the vacated slot if necessary; affected aliases no longer identify their former members.

**Parameters:**

- `set`: Valid initialized Set.  
- `element`: Non-null pointer to `set->size` readable bytes; may identify a live member.

**C++ equivalent:** `catalyst::set_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Set unchanged.

### catrl_set_contains

```c
CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Determine membership by byte equality.

**Parameters:**

- `set`: Valid initialized Set.  
- `element`: Non-null pointer to `set->size` readable bytes, including when the Set is empty.

**C++ equivalent:** `catalyst::set_contains`.

**Returns:** `CATALYST_TRUE` if present; `CATALYST_FALSE` if absent.

### catrl_set_clear

```c
void catrl_set_clear(CATRLLIB_SET* set);
```

Set cardinality to zero without releasing storage or erasing its bytes.

**Parameters:**

- `set`: Valid initialized Set.

**C++ equivalent:** `catalyst::set_clear`.

### catrl_set_expand

```c
CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity);
```

Increase capacity to the requested element count, preserving all members. Smaller or equal requests are no-ops. Failure leaves the Set unchanged.

**Parameters:**

- `set`: Valid initialized Set.  
- `capacity`: Requested number of elements, not a byte count; zero is a no-op.

**C++ equivalent:** `catalyst::set_expand`.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01060100` | `SUCCESS_NOOP`: requested capacity is not larger | Keep storage | — |
| `0x90060200` | `ERROR_BUFFER_OVERFLOW`: capacity times element size is unrepresentable | Reject extent | — |
| `0x82060300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize storage | — |
| `0x84060300` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize storage | — |
| `0x86060300` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize storage | — |
| `0x90060300` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize storage | — |
| `0x91060300` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize storage | — |
| `0x82060384` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize storage | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84060382` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize storage | Other failure is `ERROR_INVALID_STATE` |
| `0x00060400` | `SUCCESS` | Commit requested capacity | — |

**See Also:** [catrl_set_shrink](#catrl_set_shrink), [catmm_realloc](../catmmlib/index.md#catmm_realloc)

### catrl_set_shrink

```c
CATALYST_RESULT catrl_set_shrink(CATRLLIB_SET* set);
```

Reduce capacity to `max(cardinality, capacity / 2)`, rounded down. Preserve all members; release storage at zero capacity. An unchanged capacity is a no-op; failure leaves the Set unchanged.

**Parameters:**

- `set`: Valid initialized Set.

**C++ equivalent:** `catalyst::set_shrink`.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01070100` | `SUCCESS_NOOP`: target capacity equals the current capacity | Keep storage | — |
| `0x82070200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize storage | — |
| `0x84070200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize storage | — |
| `0x86070200` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize storage | — |
| `0x90070200` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize storage | — |
| `0x91070200` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize storage | — |
| `0x82070284` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize storage | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84070282` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize storage | Other failure is `ERROR_INVALID_STATE` |
| `0x00070300` | `SUCCESS` | Commit smaller capacity | — |

**See Also:** [catrl_set_expand](#catrl_set_expand), [catrl_set_free](#catrl_set_free)

### catrl_set_free

```c
CATALYST_RESULT catrl_set_free(CATRLLIB_SET* set);
```

Release storage and clear `data`, cardinality, and capacity, retaining `size` for reuse. An unallocated Set is a no-op; failure leaves it unchanged and may be retried.

**Parameters:**

- `set`: Valid initialized Set, including an already freed/reusable one.

**C++ equivalent:** `catalyst::set_free`.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01080100` | `SUCCESS_NOOP`: no owned allocation | Keep empty collection | — |
| `0x82080200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release storage | — |
| `0x84080200` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release storage | — |
| `0x00080300` | `SUCCESS` | Clear ownership and count | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_map_init

```c
void catrl_map_init(CATRLLIB_MAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize);
```

Initialize an empty Map without allocating memory. Both pointers, cardinality, and capacity become zero or null.

**Parameters:**

- `map`: Valid writable unused Map storage.  
- `ksize`: Nonzero bytes per individual key.  
- `vsize`: Nonzero bytes per individual value; need not equal `ksize`.

**C++ equivalent:** `catalyst::map_init`.

### catrl_map_add

```c
CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);
```

Copy a new key and value without allocating memory. An existing key retains its value; use [catrl_map_set](#catrl_map_set) to replace it.

**Parameters:**

- `map`: Valid initialized Map; a new key requires `cardinality < capacity`.  
- `key`: Non-null pointer to `ksize` readable bytes outside the Map object and both owned allocations.  
- `value`: Non-null pointer to `vsize` readable bytes outside the Map object and both owned allocations.

**C++ equivalent:** `catalyst::map_add`.

**Returns:** `CATALYST_TRUE` if inserted; `CATALYST_FALSE` if the key already exists, leaving the Map unchanged.

**See Also:** [catrl_map_set](#catrl_map_set)

### catrl_map_remove

```c
CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key);
```

Remove a key and its value, retaining capacity. Move the last pair into the vacated slot if necessary; affected aliases no longer identify their former pairs.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes; may identify a live key.

**C++ equivalent:** `catalyst::map_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Map unchanged.

### catrl_map_contains

```c
CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key);
```

Determine key membership by byte equality.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes, including when the Map is empty.

**C++ equivalent:** `catalyst::map_contains`.

**Returns:** `CATALYST_TRUE` if the key exists; `CATALYST_FALSE` otherwise.

### catrl_map_get

```c
CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value);
```

Copy the value associated with a key into caller-provided storage.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes.  
- `value`: Non-null writable output of at least `vsize` bytes outside the Map object and its owned allocations.

**C++ equivalent:** `catalyst::map_get`.

**Returns:** `CATALYST_TRUE` with `vsize` output bytes written if found; `CATALYST_FALSE` with output unchanged otherwise.

### catrl_map_set

```c
CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);
```

Replace the value of an existing key without changing cardinality, capacity, or slot addresses.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes; may identify a live stored key.  
- `value`: Non-null pointer to `vsize` readable bytes outside the Map object and both owned allocations.

**C++ equivalent:** `catalyst::map_set`.

**Returns:** `CATALYST_TRUE` if replaced, including an identical value; `CATALYST_FALSE` if absent, leaving the Map unchanged.

### catrl_map_clear

```c
void catrl_map_clear(CATRLLIB_MAP* map);
```

Set cardinality to zero without releasing storage or erasing its bytes.

**Parameters:**

- `map`: Valid initialized Map.

**C++ equivalent:** `catalyst::map_clear`.

### catrl_map_expand

```c
CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity);
```

Increase capacity to the requested pair count. Smaller or equal requests are no-ops. Both extents are validated before resizing.

If key resizing fails, the Map is unchanged. If value resizing fails, all pairs and the original logical capacity are preserved, but the resized key buffer remains owned.

**Parameters:**

- `map`: Valid initialized Map, including an empty Map retaining storage after a partial operation.  
- `capacity`: Requested total pair capacity, not bytes. Extent overflow and allocation failures are reported.

**C++ equivalent:** `catalyst::map_expand`.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01100100` | `SUCCESS_NOOP`: request is not larger | Keep storage | — |
| `0x90100200` | `ERROR_BUFFER_OVERFLOW`: key extent unrepresentable | Reject key extent | — |
| `0x90100300` | `ERROR_BUFFER_OVERFLOW`: value extent unrepresentable | Reject value extent | — |
| `0x82100400` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize keys | — |
| `0x84100400` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize keys | — |
| `0x86100400` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize keys | — |
| `0x90100400` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize keys | — |
| `0x91100400` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize keys | — |
| `0x82100484` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize keys | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84100482` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize keys | Other failure is `ERROR_INVALID_STATE` |
| `0x82100500` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize values | — |
| `0x84100500` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize values | — |
| `0x86100500` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize values | — |
| `0x90100500` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize values | — |
| `0x91100500` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize values | — |
| `0x82100584` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize values | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84100582` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize values | Other failure is `ERROR_INVALID_STATE` |
| `0x00100600` | `SUCCESS` | Commit requested capacity | — |

**See Also:** [catrl_map_shrink](#catrl_map_shrink), [catmm_realloc](../catmmlib/index.md#catmm_realloc)

### catrl_map_shrink

```c
CATALYST_RESULT catrl_map_shrink(CATRLLIB_MAP* map);
```

Reduce capacity to `max(cardinality, capacity / 2)`, rounded down. Preserve all pairs; an unchanged capacity is a no-op. Zero capacity releases storage.

If key resizing fails, the Map is unchanged. If value resizing fails, the smaller capacity remains in effect and the original value allocation remains owned.

**Parameters:**

- `map`: Valid initialized Map.

**C++ equivalent:** `catalyst::map_shrink`.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01110100` | `SUCCESS_NOOP`: target capacity equals the current capacity | Keep storage | — |
| `0x82110200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize keys | — |
| `0x84110200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize keys | — |
| `0x86110200` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize keys | — |
| `0x90110200` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize keys | — |
| `0x91110200` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize keys | — |
| `0x82110284` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize keys | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84110282` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize keys | Other failure is `ERROR_INVALID_STATE` |
| `0x82110300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize values | — |
| `0x84110300` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize values | — |
| `0x86110300` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize values | — |
| `0x90110300` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize values | — |
| `0x91110300` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize values | — |
| `0x82110384` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize values | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84110382` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize values | Other failure is `ERROR_INVALID_STATE` |
| `0x00110400` | `SUCCESS` | Both buffers resized | — |

**See Also:** [catrl_map_expand](#catrl_map_expand), [catrl_map_free](#catrl_map_free)

### catrl_map_free

```c
CATALYST_RESULT catrl_map_free(CATRLLIB_MAP* map);
```

Release keys, then values, skipping null pointers. If key release fails, the Map is unchanged. If value release fails, the Map is empty with zero capacity and retains its value allocation for retry or expansion.

Success clears both pointers and retains the key and value sizes for reuse. Two null pointers are a no-op.

**Parameters:**

- `map`: Valid initialized Map, including one retaining a single buffer after a partial operation.

**C++ equivalent:** `catalyst::map_free`.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82120100` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release keys | — |
| `0x84120100` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release keys | — |
| `0x82120200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release values | — |
| `0x84120200` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release values | — |
| `0x01120300` | `SUCCESS_NOOP`: both pointers are null | Keep empty Map | — |
| `0x00120400` | `SUCCESS` | All owned storage released | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_array_init

```c
void catrl_array_init(CATRLLIB_ARRAY* array, CATALYST_NUINT size);
```

Initialize an empty Array without allocating memory. All fields except `size` become zero or null.

**Parameters:**

- `array`: Valid writable unused `CATRLLIB_ARRAY` storage.  
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::array_init`.

### catrl_array_add

```c
void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Append a copy of an element at index `length` without allocating memory or moving existing elements.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_add`.

### catrl_array_insert

```c
void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Insert an element copy, shifting subsequent elements right. Insertion at `length` appends. Capacity is unchanged; aliases to shifted elements no longer identify their former elements.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.  
- `index`: Insertion position from zero through `length`, inclusive.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its allocation.

**C++ equivalent:** `catalyst::array_insert`.

### catrl_array_remove

```c
void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index);
```

Remove an element and shift subsequent elements left, preserving order and capacity. Aliases to removed or shifted elements no longer identify their former elements.

**Parameters:**

- `array`: Valid initialized non-empty Array.  
- `index`: Valid direct-access index, strictly less than `length`.

**C++ equivalent:** `catalyst::array_remove`.

### catrl_array_contains

```c
CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Determine whether an equal element exists by byte equality.

**Parameters:**

- `array`: Valid initialized Array.  
- `element`: Non-null pointer to `size` readable bytes, including when the Array is empty.

**C++ equivalent:** `catalyst::array_contains`.

**Returns:** `CATALYST_TRUE` if an equal element exists; `CATALYST_FALSE` otherwise.

### catrl_array_get

```c
void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element);
```

Copy exactly `size` bytes from the specified element to caller-provided storage.

**Parameters:**

- `array`: Valid initialized Array.  
- `index`: Valid direct-access index, strictly less than `length`.  
- `element`: Non-null writable output of at least `size` bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_get`.

### catrl_array_set

```c
void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Replace the element at the specified index without changing length, capacity, or other elements.

**Parameters:**

- `array`: Valid initialized Array.  
- `index`: Valid direct-access index, strictly less than `length`.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_set`.

### catrl_array_clear

```c
void catrl_array_clear(CATRLLIB_ARRAY* array);
```

Set length to zero without releasing storage or erasing its bytes.

**Parameters:**

- `array`: Valid initialized Array.

**C++ equivalent:** `catalyst::array_clear`.

### catrl_array_expand

```c
CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity);
```

Increase capacity to the requested element count, preserving order and elements. Smaller or equal requests are no-ops. Failure leaves the Array unchanged.

**Parameters:**

- `array`: Valid initialized Array.  
- `capacity`: Requested number of elements, not a byte count; zero is a no-op.

**C++ equivalent:** `catalyst::array_expand`.

**Context:** `0x1B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011B0100` | `SUCCESS_NOOP`: requested capacity is not larger | Keep storage | — |
| `0x901B0200` | `ERROR_BUFFER_OVERFLOW`: capacity times element size is unrepresentable | Reject extent | — |
| `0x821B0300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize storage | — |
| `0x841B0300` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize storage | — |
| `0x861B0300` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize storage | — |
| `0x901B0300` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize storage | — |
| `0x911B0300` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize storage | — |
| `0x821B0384` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize storage | Other failure is `ERROR_ACCESS_DENIED` |
| `0x841B0382` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize storage | Other failure is `ERROR_INVALID_STATE` |
| `0x001B0400` | `SUCCESS` | Commit requested capacity | — |

**See Also:** [catrl_array_shrink](#catrl_array_shrink), [catmm_realloc](../catmmlib/index.md#catmm_realloc)

### catrl_array_shrink

```c
CATALYST_RESULT catrl_array_shrink(CATRLLIB_ARRAY* array);
```

Reduce capacity to `max(length, capacity / 2)`, rounded down. Preserve elements and order; release storage at zero capacity. An unchanged capacity is a no-op; failure leaves the Array unchanged.

**Parameters:**

- `array`: Valid initialized Array.

**C++ equivalent:** `catalyst::array_shrink`.

**Context:** `0x1C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011C0100` | `SUCCESS_NOOP`: target capacity equals the current capacity | Keep storage | — |
| `0x821C0200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Resize storage | — |
| `0x841C0200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Resize storage | — |
| `0x861C0200` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Resize storage | — |
| `0x901C0200` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Resize storage | — |
| `0x911C0200` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Resize storage | — |
| `0x821C0284` | `ERROR_INVALID_STATE`: dual cleanup failure | Resize storage | Other failure is `ERROR_ACCESS_DENIED` |
| `0x841C0282` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Resize storage | Other failure is `ERROR_INVALID_STATE` |
| `0x001C0300` | `SUCCESS` | Commit smaller capacity | — |

**See Also:** [catrl_array_expand](#catrl_array_expand), [catrl_array_free](#catrl_array_free)

### catrl_array_free

```c
CATALYST_RESULT catrl_array_free(CATRLLIB_ARRAY* array);
```

Release storage and clear `data`, length, and capacity, retaining `size` for reuse. An unallocated Array is a no-op; failure leaves it unchanged and may be retried.

**Parameters:**

- `array`: Valid initialized Array, including an already freed/reusable one.

**C++ equivalent:** `catalyst::array_free`.

**Context:** `0x1D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011D0100` | `SUCCESS_NOOP`: no owned allocation | Keep empty collection | — |
| `0x821D0200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release storage | — |
| `0x841D0200` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release storage | — |
| `0x001D0300` | `SUCCESS` | Clear ownership and count | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_file_init

```c
void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data, CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*));
```

Transfer both Arrays into the File without allocation or copying elements. The source Arrays become empty and retain their sizes. Existing element addresses remain valid, but ownership belongs to the File.

**Parameters:**

- `file`: Writable unused File, separate from both source Arrays and their allocations.  
- `metadata`: Initialized owning metadata Array, distinct from `data`, with independent storage.  
- `data`: Initialized owning data Array, distinct from `metadata`, with independent storage.  
- `resolver`: Non-null callable with the declared signature; defines interpretation, valid indices, and output contract.

**C++ equivalent:** `catalyst::file_init`.

**See Also:** [CATRLLIB_FILE](#catrllib_file), [catrl_file_free](#catrl_file_free)

### catrl_file_get

```c
CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output);
```

Invoke the resolver once with the current File and index. The resolver validates the index and manages output, including failure cleanup; CATRLLIB does not initialize, allocate, or roll back output.

Return the resolver's status and detail using this function's context and operation.

**Parameters:**

- `file`: Initialized File with valid owned Arrays and a callable resolver; satisfy resolver-specific requirements.  
- `index`: File index, passed unchanged to the resolver.  
- `output`: Writable Array object outside the File and its allocations, initialized and managed as required by the resolver.

**C++ equivalent:** `catalyst::file_get`.

**Context:** `0x1F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1F01DD` | Defined resolver status `SS` retained, other than `ERROR_INVALID_ARGUMENT` | Invoke resolver once | Resolver detail `DD`, subject to status-defined meaning |
| `0x811F0100` | `ERROR_INVALID_ARGUMENT`: resolver rejected the File | Invoke resolver once | Full-signature argument index `0` (`file`) |
| `0x811F0101` | `ERROR_INVALID_ARGUMENT`: resolver rejected the requested index | Invoke resolver once | Full-signature argument index `1` (`index`) |
| `0x811F0102` | `ERROR_INVALID_ARGUMENT`: resolver rejected the output argument | Invoke resolver once | Full-signature argument index `2` (`output`) |

**See Also:** [CATRLLIB_FILE](#catrllib_file)

### catrl_file_free

```c
CATALYST_RESULT catrl_file_free(CATRLLIB_FILE* file);
```

Release metadata, then data. If metadata release fails, the File is unchanged. If data release fails, metadata is empty and its former aliases are invalid; data and resolver remain available for cleanup or reuse.

Success clears the resolver and retains both Array sizes. Separate resolver outputs are not released. An already released File is a no-op.

**Parameters:**

- `file`: Initialized, partially released, or released File. Before further resolution after partial release, restore the resolver's content requirements; reinitialize after complete release.

**C++ equivalent:** `catalyst::file_free`.

**Context:** `0x20`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01200100` | `SUCCESS_NOOP`: no allocations and resolver already null | Keep released File | — |
| `0x82200200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release metadata Array | — |
| `0x84200200` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release metadata Array | — |
| `0x82200300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release data Array | — |
| `0x84200300` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release data Array | — |
| `0x00200400` | `SUCCESS` | Clear resolver after releases | — |

**See Also:** [catrl_array_free](#catrl_array_free)

### catrl_stream_init

```c
CATALYST_RESULT catrl_stream_init(CATRLLIB_STREAM* stream, CATALYST_NUINT ssize, CATALYST_NUINT lifetime, CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW, CATALYST_NUINT));
```

Allocate zero-filled producer state and private bookkeeping. No chunk storage is allocated until requested. The state size has no additional alignment restriction.

Failure clears all public fields and leaves no owned allocation; initialization may be retried or the Stream freed as a no-op.

**Parameters:**

- `stream`: Writable unused Stream, separate from other live owned storage.  
- `ssize`: Producer-state size in bytes; may be zero. Private bookkeeping is additional.  
- `lifetime`: Nonzero number of published chunks to retain. Extent overflow is reported.  
- `next`: Non-null callable with the declared signature. May access only `ssize` state bytes and `csize` chunk bytes; must not alter Stream fields, access retained chunks or bookkeeping, change the consumer output slot, or reenter the Stream. `SUCCESS` guarantees all requested bytes were produced.

**C++ equivalent:** `catalyst::stream_init`.

**Context:** `0x21`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x90210100` | `ERROR_BUFFER_OVERFLOW`: private bookkeeping representations cannot fit the size type | Reject bookkeeping extent | — |
| `0x90210200` | `ERROR_BUFFER_OVERFLOW`: state plus fixed bookkeeping is unrepresentable | Reject state extent | — |
| `0x90210300` | `ERROR_BUFFER_OVERFLOW`: lifetime-dependent bookkeeping extent is unrepresentable | Reject retention extent | — |
| `0x84210400` | `ERROR_ACCESS_DENIED`: native allocation denied | Allocate state and bookkeeping | — |
| `0x86210400` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Allocate state and bookkeeping | — |
| `0x90210400` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB prefix or native allocation extent unrepresentable | Allocate state and bookkeeping | — |
| `0x91210400` | `ERROR_ALLOCATION_FAILED`: allocation failed | Allocate state and bookkeeping | — |
| `0x00210500` | `SUCCESS` | Managed Stream initialized | — |

**See Also:** [CATRLLIB_STREAM](#catrllib_stream), [catrl_stream_free](#catrl_stream_free), [catmm_alloc](../catmmlib/index.md#catmm_alloc)

### catrl_stream_next

```c
CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATALYST_NUINT csize, CATMMLIB_POINTERW* output);
```

Request exactly `csize` bytes from the producer. Only producer `SUCCESS` publishes a chunk and advances retention. Other producer outcomes leave output and retained chunks unchanged; producer state changes are not rolled back.

Spare storage doubles as needed (or grows to the exact requirement if doubling would overflow). Required allocation failure prevents the producer call and preserves state, output, and retained chunks.

After publication, the expired spare may be halved at quarter utilization, using a minimum request of one byte. Retained chunks are never resized. Shrink failure retains the spare and does not undo publication.

Publication depends on the Result's operation and status:

- `0x03`: Producer outcome; publishes only on `SUCCESS`.
- `0x04`: Recoverable shrink warning after publication; detail contains the shrink status.
- `0x05`: Fatal shrink failure after publication; original fatal status and detail are retained.

Producer context and operation are replaced. `ERROR_INVALID_ARGUMENT` details `0` and `2` map to public indices `0` and `1`; detail `1` (managed chunk) becomes `ERROR_INVALID_STATE` with unused detail.

**Parameters:**

- `stream`: Active initialized Stream; synchronize externally and do not reenter from its producer.  
- `csize`: Requested chunk size in bytes; zero still supplies a non-null producer buffer.  
- `output`: Non-null writable pointer slot outside the Stream and all its allocations. Receives a borrowed pointer on publication; unchanged otherwise.

**C++ equivalent:** `catalyst::stream_next`.

**Context:** `0x22`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82220100` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Grow spare storage | — |
| `0x84220100` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Grow spare storage | — |
| `0x86220100` | `ERROR_SYSTEM_NOT_SUPPORTED`: native allocation configuration unsupported | Grow spare storage | — |
| `0x90220100` | `ERROR_BUFFER_OVERFLOW`: CATMMLIB allocation extent unrepresentable | Grow spare storage | — |
| `0x91220100` | `ERROR_ALLOCATION_FAILED`: allocation or resize failed | Grow spare storage | — |
| `0x82220184` | `ERROR_INVALID_STATE`: dual cleanup failure | Grow spare storage | Other failure is `ERROR_ACCESS_DENIED` |
| `0x84220182` | `ERROR_ACCESS_DENIED`: dual cleanup failure | Grow spare storage | Other failure is `ERROR_INVALID_STATE` |
| `0x82220200` | `ERROR_INVALID_STATE`: producer rejected the managed chunk argument | Reject managed-buffer diagnostic | — |
| `0x002203DD` | `SUCCESS`: exactly the requested bytes produced | Publish chunk; advance retention | Producer detail `DD`, zero when unused |
| `0x012203DD` | `SUCCESS_NOOP` | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x402203DD` | `WARNING`: producer warning | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x412203DD` | `WARNING_PARTIAL`: producer warning | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x422203DD` | `WARNING_DEPRECATED`: producer warning | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x81220300` | `ERROR_INVALID_ARGUMENT`: producer rejected the Stream | Return producer argument diagnostic; no publication | Public argument index `0` (`stream`) |
| `0x81220301` | `ERROR_INVALID_ARGUMENT`: producer rejected the chunk size | Return producer argument diagnostic; no publication | Public argument index `1` (`csize`) |
| `0x802203DD` | `ERROR`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x822203DD` | `ERROR_INVALID_STATE`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x832203DD` | `ERROR_MALFORMED_INPUT`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x842203DD` | `ERROR_ACCESS_DENIED`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x852203DD` | `ERROR_NOT_IMPLEMENTED`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x862203DD` | `ERROR_SYSTEM_NOT_SUPPORTED`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x872203DD` | `ERROR_TIMEOUT`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x882203DD` | `ERROR_NOT_FOUND`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x892203DD` | `ERROR_INTERRUPTED`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x8A2203DD` | `ERROR_DEPENDENCY_FAILURE`: producer outcome | Return producer outcome; no publication | Producer dependency status `DD` |
| `0x902203DD` | `ERROR_BUFFER_OVERFLOW`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x912203DD` | `ERROR_ALLOCATION_FAILED`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0xA02203DD` | `ERROR_IO_ERROR`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0xC02203DD` | `FATAL`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0xC12203DD` | `FATAL_INVARIANT_VIOLATION`: producer outcome | Return producer outcome; no publication | Producer detail `DD`, zero when unused |
| `0x40220482` | `WARNING` (NONE): optional shrink failed after exact production | Keep published chunk; retain spare allocation | Shrink status `ERROR_INVALID_STATE` |
| `0x40220484` | `WARNING` (NONE): optional shrink failed after exact production | Keep published chunk; retain spare allocation | Shrink status `ERROR_ACCESS_DENIED` |
| `0x40220486` | `WARNING` (NONE): optional shrink failed after exact production | Keep published chunk; retain spare allocation | Shrink status `ERROR_SYSTEM_NOT_SUPPORTED` |
| `0x40220490` | `WARNING` (NONE): optional shrink failed after exact production | Keep published chunk; retain spare allocation | Shrink status `ERROR_BUFFER_OVERFLOW` |
| `0x40220491` | `WARNING` (NONE): optional shrink failed after exact production | Keep published chunk; retain spare allocation | Shrink status `ERROR_ALLOCATION_FAILED` |
| `0xSS2205DD` | Fatal shrink status `SS` retained (`C0` or `C1`), if returned by CATMMLIB | Keep published chunk; report fatal optimization outcome | Original shrink detail `DD` |

**See Also:** [CATRLLIB_STREAM](#catrllib_stream), [catrl_stream_init](#catrl_stream_init), [catrl_stream_free](#catrl_stream_free), [catmm_realloc](../catmmlib/index.md#catmm_realloc)

### catrl_stream_free

```c
CATALYST_RESULT catrl_stream_free(CATRLLIB_STREAM* stream);
```

End production, clear `next`, and invalidate all chunk pointers on the first attempt. Release retained chunks, spare storage, then state and bookkeeping. Failed initialization or an already freed Stream is a no-op.

On failure, unreleased allocations remain owned, with `state` and `ssize` retained; retry free only. Complete success clears all public fields.

**Parameters:**

- `stream`: Initialized, failed-init, partially freed, or freed Stream. Release producer-owned referenced resources before the first attempt; keep the object alive and synchronized until cleanup succeeds.

**C++ equivalent:** `catalyst::stream_free`.

**Context:** `0x23`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01230100` | `SUCCESS_NOOP`: state is already null | Keep freed Stream | — |
| `0x82230200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release retained chunk | — |
| `0x84230200` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release retained chunk | — |
| `0x82230300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release spare chunk | — |
| `0x84230300` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release spare chunk | — |
| `0x82230400` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release state and bookkeeping | — |
| `0x84230400` | `ERROR_ACCESS_DENIED`: release or cleanup denied | Release state and bookkeeping | — |
| `0x00230500` | `SUCCESS` | All owned storage released; public fields cleared | — |

**See Also:** [catrl_stream_init](#catrl_stream_init), [catmm_free](../catmmlib/index.md#catmm_free)
