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

A finite, unordered collection of distinct elements. All elements occupy the same number of bytes. Two elements are equal when their byte representations match, including any padding. The Set does not interpret element types or assign an order to its members.

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

Initialize a Set with an element size, then reserve storage with [catrl_set_expand](#catrl_set_expand). The Set owns its storage and copies each inserted element by value. Capacity counts elements, not bytes. Adding an existing element leaves the Set unchanged, even when it is full. Removing an element or clearing the Set does not reduce capacity.

Membership is determined by a linear search. Removal moves the last member into the vacated slot when necessary, so pointers to either affected slot no longer identify their former members. Other slots remain in place until the storage is resized or released. Direct access to live elements must preserve their uniqueness.

A successful resize invalidates all pointers into the allocation; a failed resize preserves the Set. Shrinking retains every member and can eventually release all storage when the Set is empty. A successful free retains the element size for reuse.

### CATRLLIB_MAP

A finite collection that associates each unique key with exactly one value. Keys and values have independently specified byte sizes. Keys are compared byte by byte, including padding; values need not be distinct. Adding an existing key does not replace its value.

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

The Map owns two contiguous allocations: one for keys and one for values. The key at a given slot corresponds to the value at the same slot. Although storage is indexed internally, a Map is accessed by key rather than by position. Direct access must preserve key uniqueness and the correspondence between keys and values.

Reserve capacity before adding a new key. Lookups search the keys linearly, and removal moves the last pair into the vacated slot when necessary. Pointers to either affected slot no longer identify their former pairs. Updating a value leaves the key and slot addresses unchanged. Removing or clearing associations retains capacity.

Because the buffers are resized separately, one resize may succeed before the other fails. The Map always reports a capacity that both buffers can accommodate. Resizing invalidates aliases of each successfully resized buffer, even when the overall operation fails. At zero capacity, a Map may still own an allocation after a partial resize or release. See [catrl_map_expand](#catrl_map_expand), [catrl_map_shrink](#catrl_map_shrink), and [catrl_map_free](#catrl_map_free) for the resulting ownership and failure guarantees.

### CATRLLIB_ARRAY

A finite collection of fixed-size elements identified by contiguous, zero-based indices. Unlike a Set, an Array may contain equal elements at different indices. Elements are stored contiguously and copied by value.

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

The Array owns its storage. Reserve capacity before appending or inserting an element. Appending leaves existing elements in place; insertion and removal shift subsequent elements to maintain contiguous indices and preserve their relative order. Pointers into the shifted portion no longer identify their former indexed elements. Earlier slots remain in place. Replacing an element changes only the bytes at its index.

Indexed access and append require constant work apart from copying the element. Insertion and removal move the affected suffix, while membership is determined by a linear bytewise search. Direct element access must remain within the live extent. Removing elements or clearing the Array retains capacity.

A successful resize invalidates all pointers into the allocation; a failed resize preserves the Array. Shrinking retains every element in order and can eventually release all storage when the Array is empty. A successful free retains the element size for reuse.

### CATRLLIB_FILE

A collection that resolves data through metadata. A File contains two Arrays, one containing metadata and one containing data, and a resolver that determines how the metadata is used to retrieve an Array from the data. The resolver defines the format and the finite, contiguous domain of valid File indices. A File index need not correspond to an index in either underlying Array, and a File need not represent a filesystem object.

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

Use [catrl_file_init](#catrl_file_init) to transfer ownership of the two Arrays into the File. Their contents can then be read or modified using ordinary Array operations on `file.metadata` and `file.data`. Each resolution uses the current contents; CATRLLIB does not cache an interpretation of the metadata. Changes must satisfy the resolver's content requirements.

The resolver determines how the output Array is initialized, what its elements represent, and whether it contains an owned copy or borrows existing File storage. Release an owned output with [catrl_array_free](#catrl_array_free); do not resize or free a borrowed view as an owning Array. Its validity depends on the resolver's contract and the lifetime of the underlying storage. The output Array object itself must be separate from the File and its allocations.

The resolver also defines output validity on every outcome and is responsible for allocations it creates, including on failure. CATRLLIB does not allocate, initialize, or roll back resolver output. [catrl_file_free](#catrl_file_free) releases the two owned Arrays, retaining any unreleased allocations for a subsequent cleanup attempt.

### CATRLLIB_STREAM

A mechanism for generating or retrieving successive chunks from a finite or infinite sequence. The producer determines the contents and order of the chunks, while the Stream manages the memory used to hold them. The consumer specifies the size of each chunk and, at initialization, how many recent chunks must remain valid.

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

Initialize a Stream with the number of bytes the producer needs for its state, a nonzero chunk lifetime, and a producer function. State is allocated immediately and zero-filled; chunk buffers are allocated as needed. The state and chunk buffers have CATMMLIB's fundamental alignment, but not extended alignment. Zero-filled bytes do not construct typed objects or necessarily represent null pointers. Initialize typed state as required by the producer.

Each call to [catrl_stream_next](#catrl_stream_next) supplies the producer with writable memory for the requested number of bytes. The producer writes directly into that memory and returns a Result. Only producer `SUCCESS` guarantees that all requested bytes have been produced. CATRLLIB does not copy the completed chunk or require the consumer to allocate an output buffer. A zero-byte request still receives a non-null buffer and, on success, produces an empty chunk.

The lifetime counts published chunks, not calls, bytes, or elapsed time. For example, with a lifetime of three, the first chunk remains valid while the second and third chunks are produced. Producing the fourth chunk invalidates the first. Failed, warning, and no-op producer calls do not count toward this limit. With a lifetime of one, only the most recently published chunk remains valid; failed attempts do not invalidate it.

To preserve this guarantee, CATRLLIB keeps up to `lifetime` retained buffers and one spare. The producer writes to the spare, which becomes the newest retained buffer only after successful production. An expired buffer can then be reused or resized. This allows differently sized chunks to be produced without moving any still-valid chunk. See [catrl_stream_next](#catrl_stream_next) for the resizing policy and the distinction between producer and allocation diagnostics.

The producer defines how the sequence advances, how its state changes, and how exhaustion is reported. There is no universal end-of-stream status or pointer sentinel, and CATRLLIB does not retry a producer or undo its state changes. The producer must use defined FDEFSPEC statuses, access only its `ssize` state bytes and supplied `csize` chunk bytes, and leave Stream fields, private bookkeeping, retained chunks, and the consumer's output slot unchanged. It must not reenter the Stream.

The Stream owns its raw state and chunk memory, not objects or resources referenced by those bytes. The producer and consumer are responsible for the lifetime and cleanup of such resources, including on failed production; no constructors or destructors are invoked. Returned chunk pointers are borrowed: do not free or resize them independently, or use them after they expire. Calling [catrl_stream_free](#catrl_stream_free) ends production and invalidates all chunks, even if cleanup subsequently fails. If cleanup fails, retry free without reinitializing the Stream.

## Functions

All collection objects must be initialized before use. Pass writable, unused storage to initialization functions, and release any existing ownership before reinitializing. Do not shallow-copy owning collections or replace their bookkeeping fields. Synchronize access externally when mutation may occur. The pointer, extent, alignment, lifetime, and index requirements in [Assumptions](../../assumptions/index.md) apply throughout this reference.

Set, Map, and Array store fixed-size byte representations by value. Their element, key, and value sizes must be nonzero. Initialize representation bytes, including padding, consistently when using bytewise equality. These collections do not provide typed comparisons, invoke destructors, release objects referenced by stored pointers, or securely erase removed data.

Capacity is the number of elements or pairs that can be stored without allocating additional memory. Use the corresponding expand function before inserting beyond capacity; insertion never expands storage automatically. Duplicate Set or Map insertion is valid at full capacity because it does not add an entry. Expansion honors the requested total capacity exactly, and smaller or equal requests are no-ops. Shrinking halves capacity, rounded down, without dropping live entries. Repeated shrinking of an empty collection can release its storage entirely. Clear and removal retain capacity; unused slots are not live elements. Successful release retains element sizes for reuse.

Successful resizing invalidates pointers into the resized allocation, even if the numeric address is unchanged. A failed resize preserves that allocation and its bytes. A Map may therefore retain a resized key buffer after a value-buffer failure; its logical capacity remains safe for both. At zero capacity, a Map may still own a buffer after a partial operation. Release any remaining storage through the Map API.

Inputs copied into a collection and outputs copied out of it must lie entirely outside that collection object and all its allocations. Lookup and removal search inputs may identify live stored elements or keys, since the search completes before mutation. The key supplied to Map replacement may likewise identify a stored key. Valid non-null pointers and extents are required even for empty lookups. These are caller preconditions, not conditions checked by the functions. Stream state and chunk sizes may be zero; its lifetime must be nonzero.

Allocation extents and arithmetic use the target's pointer-width Catalyst types. Capacity is checked against `CATALYST_NUINT_MAX / element_size` before multiplication, and valid counts and indices keep subsequent offsets within that extent. CATMMLIB may impose additional allocation limits and supplies fundamental alignment. CATRLLIB does not depend on wider numeric representations, native byte order, typed element access, or platform-specific operations. Ordinary language promotions may occur without affecting the arithmetic guarantees. Running on a particular target requires a working CATMMLIB backend.

Results use the logical `0xSSCCOODD` representation defined by [FDEFSPEC Revision 1](https://catalystui.org/specifications/languages/fdefspec/). Each reporting function has a fixed context and each reporting site has its own operation code. Dependency diagnostics retain applicable status and detail, including fatal severity. For CATMMLIB resize failures involving two cleanup operations, detail `84` or `82` identifies the other failure's status; otherwise the listed dependency details are unused. Optional Stream shrinking has a separate warning contract. Callback status and detail are retained where applicable, but the callback's original context and operation are replaced. Callback-specific diagnostic provenance may therefore be lost. Invalid-argument details refer to the reporting signature, with Stream producer arguments translated as described in [catrl_stream_next](#catrl_stream_next). Undefined status codes remain reserved.

### catrl_set_init

```c
void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size);
```

Initialize an empty Set with the given element size. No storage is allocated until expansion; `data`, `cardinality`, and `capacity` are zero. Release existing ownership before reinitializing.

**Parameters:**

- `set`: Valid writable unused `CATRLLIB_SET` storage.  
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::set_init`.

### catrl_set_add

```c
CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Add a copy of an element if it is not already present. The Set compares the supplied bytes with its existing elements. A duplicate leaves the Set unchanged; adding a new member increases cardinality by one without allocating memory.

**Parameters:**

- `set`: Valid initialized Set; a new distinct element requires `cardinality < capacity`.  
- `element`: Non-null pointer to `set->size` readable bytes outside the Set object and its owned allocation.

**C++ equivalent:** `catalyst::set_add`.

**Returns:** `CATALYST_TRUE` if inserted; `CATALYST_FALSE` if already present, with no mutation. 

### catrl_set_remove

```c
CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Remove an element if present. If it is not the last stored member, move the last member into its place. Decrease cardinality without changing capacity. No referenced objects are released.

**Parameters:**

- `set`: Valid initialized Set.  
- `element`: Non-null pointer to `set->size` readable bytes; may identify a live stored element because lookup completes before mutation.

**C++ equivalent:** `catalyst::set_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Set unchanged.

### catrl_set_contains

```c
CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Determine whether the Set contains an element with the same byte representation.

**Parameters:**

- `set`: Valid initialized Set.  
- `element`: Non-null pointer to `set->size` readable bytes, including when the Set is empty.

**C++ equivalent:** `catalyst::set_contains`.

**Returns:** `CATALYST_TRUE` if present; `CATALYST_FALSE` if absent.

### catrl_set_clear

```c
void catrl_set_clear(CATRLLIB_SET* set);
```

Remove all members by setting cardinality to zero. Keep the allocated storage, element size, and capacity available for reuse. The former elements' bytes are not erased or treated as live members.

**Parameters:**

- `set`: Valid initialized Set.

**C++ equivalent:** `catalyst::set_clear`.

### catrl_set_expand

```c
CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity);
```

Increase capacity to the requested number of elements. All members are preserved. A request no greater than the current capacity is a no-op. Failure leaves the Set unchanged; successful resizing invalidates previous storage aliases. Newly available slots are not members until inserted.

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

Reduce capacity to half its current value, rounded down, or to cardinality if that is greater. If capacity would not change, return a no-op. Reaching zero capacity releases storage and clears `data`. Failure leaves the Set unchanged; successful resizing invalidates previous storage aliases.

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

Release the Set's storage and clear `data`, cardinality, and capacity. Retain the element size so the Set can be reused. An unallocated Set is a no-op; failure leaves the Set unchanged and may be retried. Objects referenced by stored pointers are not released.

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

Initialize an empty Map with the specified key and value sizes. Both storage pointers, cardinality, and capacity are initially zero; no memory is allocated. Release existing ownership before reinitializing.

**Parameters:**

- `map`: Valid writable unused Map storage.  
- `ksize`: Nonzero bytes per individual key.  
- `vsize`: Nonzero bytes per individual value; need not equal `ksize`.

**C++ equivalent:** `catalyst::map_init`.

### catrl_map_add

```c
CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);
```

Add a copy of a key and its associated value if the key does not already exist. A duplicate leaves the existing association unchanged; use [catrl_map_set](#catrl_map_set) to replace its value. A new association increases cardinality without allocating memory.

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

Remove a key and its associated value if present. Move the last pair into the vacated slot when necessary and decrease cardinality without changing capacity. No referenced objects are released.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes; may identify a live key because lookup precedes mutation.

**C++ equivalent:** `catalyst::map_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Map unchanged.

### catrl_map_contains

```c
CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key);
```

Determine whether the Map contains a key with the same byte representation.

**Parameters:**

- `map`: Valid initialized Map.  
- `key`: Non-null pointer to `ksize` readable bytes, including when the Map is empty.

**C++ equivalent:** `catalyst::map_contains`.

**Returns:** `CATALYST_TRUE` if the key exists; `CATALYST_FALSE` otherwise.

### catrl_map_get

```c
CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value);
```

Find the key and copy its associated value into the supplied output buffer. If the key is absent, the output remains unchanged.

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

Replace the value associated with an existing key. If the key is absent, no association is added. Cardinality and capacity remain unchanged, and no memory is allocated.

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

Remove all associations by setting cardinality to zero. Keep both storage allocations, their element sizes, and capacity available for reuse. The former entries' bytes are not erased or treated as live associations.

**Parameters:**

- `map`: Valid initialized Map.

**C++ equivalent:** `catalyst::map_clear`.

### catrl_map_expand

```c
CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity);
```

Increase capacity to the requested number of pairs. Check both byte extents before resizing either allocation. A request no greater than the current capacity is a no-op.

Keys are resized first, followed by values. If the key resize fails, the Map is unchanged. If the value resize fails, all associations and the original logical capacity are preserved, but the resized key allocation remains owned and previous key aliases are invalid. Use the current pointers after an attempted resize.

**Parameters:**

- `map`: Valid initialized Map, including an empty Map retaining storage after a partial operation.  
- `capacity`: Requested number of pairs, not bytes. The operation validates the independent key/value extents and reports overflow or allocation failure; representability and allocatability are not caller preconditions.

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

Reduce capacity to half its current value, rounded down, or to cardinality if that is greater. Preserve all associations; an unchanged capacity is a no-op.

Keys are resized first. If that fails, the Map is unchanged. Once keys are resized, the smaller capacity takes effect even if resizing values fails. The original value allocation remains owned in that case. Use the current pointers after an attempted resize. Reaching zero capacity releases both buffers in the same order; use [catrl_map_free](#catrl_map_free) to release any remaining allocation if the second release fails.

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

Release the key allocation, then the value allocation, skipping null pointers. If releasing keys fails, the Map remains unchanged. Once keys are released, the Map is empty with zero capacity. If releasing values then fails, that allocation remains owned and may be released by retrying this function or resized by expanding the Map again. Complete success clears both pointers. If both pointers are already null, the operation is a no-op. Key and value sizes are retained for reuse; referenced objects are not released.

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

Initialize an empty Array with the given element size. No storage is allocated until expansion; `data`, `length`, and `capacity` are zero. Release existing ownership before reinitializing.

**Parameters:**

- `array`: Valid writable unused `CATRLLIB_ARRAY` storage.  
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::array_init`.

### catrl_array_add

```c
void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Append a copy of an element at index `length`, increasing length by one. Existing elements remain in place and capacity is unchanged. No memory is allocated.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_add`.

### catrl_array_insert

```c
void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Insert a copy of an element at the specified index, shifting subsequent elements one position to the right. Inserting at `length` appends the element. Capacity is unchanged and no memory is allocated. The input must not reference Array storage, since shifting elements may overwrite the source before it is copied.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.  
- `index`: Insertion position from zero through `length`, inclusive.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation; must remain valid during the suffix move.

**C++ equivalent:** `catalyst::array_insert`.

### catrl_array_remove

```c
void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index);
```

Remove the element at the specified index and shift subsequent elements one position to the left. Decrease length while preserving their order and retaining capacity. No referenced objects are released.

**Parameters:**

- `array`: Valid initialized non-empty Array.  
- `index`: Valid direct-access index, strictly less than `length`.

**C++ equivalent:** `catalyst::array_remove`.

### catrl_array_contains

```c
CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Determine whether any element in the Array has the same byte representation as the supplied element.

**Parameters:**

- `array`: Valid initialized Array.  
- `element`: Non-null pointer to `size` readable bytes, including when the Array is empty.

**C++ equivalent:** `catalyst::array_contains`.

**Returns:** `CATALYST_TRUE` if an equal element exists; `CATALYST_FALSE` otherwise.

### catrl_array_get

```c
void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element);
```

Copy the element at the specified index into the supplied output buffer. Exactly `size` bytes are written without modifying the Array.

**Parameters:**

- `array`: Valid initialized Array.  
- `index`: Valid direct-access index, strictly less than `length`.  
- `element`: Non-null writable output of at least `size` bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_get`.

### catrl_array_set

```c
void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Replace the element at the specified index with a copy of the supplied element. Length, capacity, and all other elements remain unchanged.

**Parameters:**

- `array`: Valid initialized Array.  
- `index`: Valid direct-access index, strictly less than `length`.  
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_set`.

### catrl_array_clear

```c
void catrl_array_clear(CATRLLIB_ARRAY* array);
```

Remove all elements by setting length to zero. Keep the allocated storage, element size, and capacity available for reuse. The former elements' bytes are not erased or treated as live elements.

**Parameters:**

- `array`: Valid initialized Array.

**C++ equivalent:** `catalyst::array_clear`.

### catrl_array_expand

```c
CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity);
```

Increase capacity to the requested number of elements. Preserve all elements and their order. A request no greater than the current capacity is a no-op. Failure leaves the Array unchanged; successful resizing invalidates previous storage aliases. Newly available slots are not elements until inserted.

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

Reduce capacity to half its current value, rounded down, or to length if that is greater. Preserve all elements and their order. An unchanged capacity is a no-op. Reaching zero capacity releases storage and clears `data`. Failure leaves the Array unchanged; successful resizing invalidates previous storage aliases.

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

Release the Array's storage and clear `data`, length, and capacity. Retain the element size so the Array can be reused. An unallocated Array is a no-op; failure leaves the Array unchanged and may be retried. Objects referenced by stored pointers are not released.

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

Initialize a File by taking ownership of the metadata and data Arrays and associating them with a resolver. The original Arrays become empty, retaining their element sizes for reuse. No allocation or element copying takes place. Existing element addresses are unchanged, but their storage is now owned by the File and must not be released through previous aliases.

**Parameters:**

- `file`: Valid writable unused File storage, separate from both source Array objects and all their allocations.  
- `metadata`: Valid initialized owning Array, distinct from `data`, with independently owned storage.  
- `data`: Valid initialized owning Array, distinct from `metadata`, with independently owned storage.  
- `resolver`: Non-null callable with the declared C return/parameter types; it establishes metadata interpretation, valid index domain, and output/failure contracts.

**C++ equivalent:** `catalyst::file_init`.

**See Also:** [CATRLLIB_FILE](#catrllib_file), [catrl_file_free](#catrl_file_free)

### catrl_file_get

```c
CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output);
```

Resolve an Array from the File by calling its resolver once with the supplied index. The resolver interprets the current metadata and data, checks the index against its own domain, and determines the output's contents, ownership, and validity on every outcome. CATRLLIB imposes no additional index bound, does not initialize or allocate the output, and does not undo changes made by the resolver.

The returned Result uses the resolver's status and detail with this function's context and operation.

**Parameters:**

- `file`: Valid initialized File with a non-null resolver and valid owned Arrays; satisfy resolver-specific preconditions.  
- `index`: Index presented unchanged to the resolver; CATRLLIB does not compare it with metadata/data length.  
- `output`: Valid writable Array object outside the File object and both owned allocations, initialized as required by the resolver. Any ownership already in it must be handled according to that resolver contract. The resolver may set `output->data` to a borrowed view of File storage, with ownership and lifetime as specified by that resolver.

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

Release the metadata Array, then the data Array. If releasing metadata fails, both Arrays and the resolver remain unchanged. If releasing data fails, metadata is already empty and its old aliases are invalid, but data and the resolver remain available for cleanup or reuse. Restore the resolver's content requirements before resolving again.

Complete success clears the resolver and retains both Array element sizes. Separate resolver outputs and referenced objects are not released. An already released File is a no-op. Initialize the File again before resolving after complete release.

**Parameters:**

- `file`: Valid initialized File, including an already released File or one left by a prior partial release.

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

Initialize a Stream with zero-filled producer state and private bookkeeping in a single allocation. No chunks are allocated until requested. The state pointer retains CATMMLIB's fundamental alignment; private bookkeeping does not require the producer's state size to satisfy any additional alignment restriction.

The function checks that the combined state and bookkeeping extent is representable before allocation. CATMMLIB may impose further limits. On success, the Stream owns the allocation and stores the supplied state size and producer. On failure, all public fields are zero or null, no allocation is owned, and initialization may be retried or the Stream freed as a no-op.

**Parameters:**

- `stream`: Valid writable unused Stream object, separate from other live owned storage.  
- `ssize`: Requested producer-state bytes; zero is allowed. Bookkeeping is additional and remains private. Any overflow or allocation limit is reported by the function.  
- `lifetime`: Nonzero number of most recently published chunks to retain. Bookkeeping extent overflow is an operational failure, not a caller precondition.  
- `next`: Non-null callable with the exact declared return and parameter types. It receives `(stream, chunk, csize)` and may write only `ssize` state bytes and `csize` chunk bytes. A `SUCCESS` result guarantees all requested bytes were produced. It must not change Stream fields, access bookkeeping or retained chunks, or reenter the Stream.

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

Request the next chunk of exactly `csize` bytes. CATRLLIB prepares a spare buffer of at least `max(csize, 1)` bytes and calls the producer once. If more memory is needed, the spare grows by doubling its capacity, or to the exact requirement when doubling would overflow. Failure to allocate the required memory leaves the output, producer state, and retained chunks unchanged and does not call the producer. Any pending allocation cleanup remains owned through CATMMLIB.

Only producer `SUCCESS` publishes a chunk. The function writes the managed pointer to `*output`, retains the new chunk, and expires the oldest if the lifetime limit has been reached. A producer warning, no-op, error, or fatal result leaves `*output` and all retained chunks unchanged. Producer state changes are not rolled back, and the spare remains owned for reuse.

After publishing, the function may halve the expired buffer's capacity if the current request uses at most one quarter of it (using a minimum request of one byte). It never resizes the newly published chunk or a still-valid chunk. A recoverable shrink failure retains the spare and reports `WARNING` with the shrink status in detail. A fatal shrink failure retains its original fatal status and detail. In both cases the completed chunk remains published and lifetimes have advanced normally.

Check the Result operation as well as its status when determining whether a chunk was published. A producer warning at operation `03` does not publish, whereas an optimization warning at operation `04` does. A fatal optimization diagnostic at operation `05` also accompanies an already published chunk. Neither pointer equality nor status level alone identifies every publication outcome. The producer defines whether the sequence has ended and whether another call is valid.

The callback's context and operation are replaced. For `ERROR_INVALID_ARGUMENT`, callback argument `0` (`stream`) remains index `0`, and callback argument `2` (`csize`) becomes public index `1`. Rejection of callback argument `1` (the managed buffer) is reported as `ERROR_INVALID_STATE` instead, with unused detail. Other callback status and detail bytes are retained.

**Parameters:**

- `stream`: Active initialized Stream with valid producer state and non-null callback. A Stream whose cleanup has been attempted is not active. Synchronize externally; do not reenter from its producer.  
- `csize`: Bytes requested for this individual chunk, not total Stream length or buffer capacity. May be zero; the producer still receives a non-null buffer. Required allocation failure/overflow is reported before production.  
- `output`: Valid non-null writable `CATMMLIB_POINTERW` slot outside the Stream object, its complete state/bookkeeping allocation, and every chunk allocation. On publication it receives a borrowed managed pointer valid until displacement by later publications or a free attempt. Do not independently resize or release it. Every non-publication outcome leaves the slot unchanged; an initially uninitialized slot remains uninitialized.

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

End production and release all retained chunks, the spare buffer, and finally the state and bookkeeping allocation. Skip null pointers; a failed initialization or already freed Stream is a no-op. Any pending CATMMLIB cleanup is released through the allocation that owns it.

The first free attempt clears `next` and invalidates all consumer chunks, even if cleanup fails. Each successful release is recorded before the next is attempted. If a release fails, keep the Stream and retry this function; do not resume production, access former chunks, free allocations independently, or reinitialize it. All unreleased memory remains reachable, with `state` and `ssize` retained until the final allocation is released. On complete success, `state` and `next` are null and `ssize` is zero. Clean up any producer-owned referenced resources before the first free attempt.

**Parameters:**

- `stream`: Valid initialized Stream, one left by failed initialization or partial cleanup, or an already freed Stream. Clean up producer-owned referenced resources before the first free attempt. Synchronize externally; keep the object alive until cleanup succeeds.

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
