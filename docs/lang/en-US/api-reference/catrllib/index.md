# CATRLLIB

Provides Set, Map, and Array storage and the File and Stream relationships defined by FRELSPEC Revision 2.1. Collections store words through [CATMMLIB](../catmmlib/index.md); this implementation stores fixed-size elements as byte sequences. File resolution and Stream production are supplied by the caller.

**C Header:** `catrllib.h`\
**C++ Header:** `catrllib.hpp`\
**Namespace:** `catalyst`\
**Library:** `catrl`\
**Dependencies:** [CATALYST](../catalyst/index.md), [CATMMLIB](../catmmlib/index.md)\
**Specification:** [FRELSPEC Revision 2.1](https://catalystui.org/specifications/languages/frelspec/)

## Summary

| Type | Meaning |
| --- | --- |
| [CATRLLIB_SET](#catrllib_set) | Finite, unordered collection of distinct elements. |
| [CATRLLIB_MAP](#catrllib_map) | Finite collection of unique keys and their values. |
| [CATRLLIB_ARRAY](#catrllib_array) | Finite collection with contiguous indices. |
| [CATRLLIB_FILE](#catrllib_file) | Public metadata/data Arrays interpreted by a resolver. |
| [CATRLLIB_STREAM](#catrllib_stream) | Producer-defined finite or infinite sequence of chunks. |

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
| [catrl_stream_init](#catrl_stream_init) | Bind borrowed state and a producer. |
| [catrl_stream_next](#catrl_stream_next) | Ask the producer for its next chunk pointer. |

C++ aliases use the same C structures. Inline wrappers omit `catrl_`, use Catalyst C++ type names, and forward directly. Callback return types remain `CATALYST_RESULT`, including in C++, to match the C function-pointer signatures. The C and C++ APIs have identical ownership and failure behavior.

## Macros

### CATRLLIB_API

Export/import annotation for the C ABI. On Windows, `CATRLLIB_EXPORTS` selects `__declspec(dllexport)` and `CATRLLIB_USE_DLL` selects `__declspec(dllimport)`; otherwise the annotation is empty. Link the static `catrl` archive with `catmm`, or use the shared `catrl` library and its shared `catmm` dependency. CMake's `catrllib_static` and `catrllib_shared` targets propagate the matching dependency.

## Types

### CATRLLIB_SET

A finite, unordered collection of distinct fixed-size elements. Equality compares all `size` bytes with `catmm_compare`; padding and other representation bytes participate. Callers must provide stable, initialized representations for their intended equality. No hashing, sorting, typed comparison, or element destructor is imposed.

Initialization establishes empty storage. Capacity is a count of elements, not bytes. CATRLLIB owns the CATMMLIB allocation and copies each inserted element by value. A stored pointer is only a copied pointer representation; its target remains owned by the caller. Do not shallow-copy an owning collection or replace its bookkeeping fields. Direct access must preserve distinctness, extents, and initialized state. Synchronize concurrent access involving mutation externally.

```c
typedef struct CATRLLIB_SET {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_SET;
```

**Fields:**

- `data`: Owned contiguous storage; null before allocation and after successful release. Only the first `cardinality` elements are members.\
- `size`: Nonzero bytes per individual element, fixed at initialization.\
- `cardinality`: Number of distinct live elements, from zero through `capacity`.\
- `capacity`: Number of slots available without allocation. Its byte extent `capacity * size` is representable in `CATALYST_NUINT`; CATMMLIB may impose a smaller allocation limit.

**C++ equivalent:** `catalyst::SET`, an alias of `CATRLLIB_SET`.

Use `catrl_set_expand` before adding new elements beyond capacity. Expansion honors an explicit larger request exactly; smaller/equal requests, including zero, are no-ops. There is no implicit expansion in insertion. A caller choosing geometric growth may request double its capacity after checking representability. Shrinking selects `max(cardinality, floor(capacity / 2))`; repeated shrinking of an empty Set eventually releases its allocation. Clear and removal retain capacity.

Lookup is linear in cardinality. Removal replaces a removed slot with the last member, so iteration order is unspecified. Insertion and removal allocate no memory. Removal ends the removed member's identity and moves the last member when necessary; pointers to either affected slot must not be reused as references to their former members. Clear ends every live membership while retaining storage. Unaffected member slots remain in place until resizing or release. Resize success invalidates all storage aliases, even if the address happens to be unchanged; single-buffer resize failure preserves pointer, contents, cardinality, and capacity. Clear/free do not erase bytes securely or release referenced objects. Successful free retains `size` for reuse.

### CATRLLIB_MAP

A finite collection of unique fixed-size keys, each associated with one fixed-size value. Keys compare bytewise across `ksize` bytes; duplicate add leaves the existing value unchanged. Values need not be distinct. Key and value sizes are independent, and each buffer's extent is checked separately before allocation.

CATRLLIB owns the key and value allocations and stores copies by value. Referenced objects remain caller-owned. Do not shallow-copy ownership or replace bookkeeping fields. Direct access must preserve key uniqueness, pair alignment by slot, counts, and extents; changing an existing value's bytes is permitted. Synchronize concurrent access involving mutation externally.

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

- `keys`: Owned contiguous key storage. Only the first `cardinality` slots are live.\
- `values`: Independently owned value storage; value slot `i` belongs to key slot `i`.\
- `ksize`: Nonzero bytes per individual key, fixed at initialization.\
- `vsize`: Nonzero bytes per individual value, fixed at initialization.\
- `cardinality`: Number of live key/value pairs, from zero through `capacity`.\
- `capacity`: Safe slot count in both buffers. Extra physical storage retained after a partial resize is not additional logical capacity.

**C++ equivalent:** `catalyst::MAP`, an alias of `CATRLLIB_MAP`.

Initialization sets both pointers, cardinality, and capacity to zero without allocation. Expansion requests an exact larger pair capacity; smaller/equal requests are no-ops. Adding a new key requires a spare slot. Shrink selects `max(cardinality, floor(capacity / 2))`. Lookup is linear; removal moves the last pair into a removed slot. Clear/removal retain capacity. No insertion, lookup, replacement, or removal allocates.

Removal changes pair identity at the removed and former last slots; pointers to those slots must not be reused as references to their former pairs. Clear ends every live association without releasing storage. Replacement changes the existing value's bytes but leaves key identity and slot addresses intact. Unaffected slots stay in place until resizing or release. Successful resizing invalidates aliases of the resized buffer even if its address is unchanged; the partial outcomes below apply to each buffer separately.

Key resizing precedes value resizing. If expansion's value resize fails, all pairs and logical capacity are preserved, but key aliases may already be invalidated and a larger key allocation remains owned. If shrinking's value resize fails, all pairs are preserved and the smaller logical capacity remains; values may retain extra storage. A key resize failure leaves the Map unchanged. No rollback allocation or unreachable temporary allocation is introduced.

Free releases keys before values. A key release failure leaves the Map unchanged. Successful key release clears `keys`, cardinality, and capacity; if value release then fails, `values` remains live and owned for retry. This is a valid empty Map with zero capacity. It may be freed again or expanded for reuse without discarding the retained allocation. Both element sizes are retained. At zero logical capacity, either pointer can therefore be non-null after a partial operation; only CATRLLIB manages those allocations.

### CATRLLIB_ARRAY

A finite, indexed collection of fixed-size element copies at contiguous zero-based indices. Elements may repeat. CATRLLIB owns the CATMMLIB allocation; stored pointer targets remain caller-owned. Do not shallow-copy ownership or replace bookkeeping fields. Direct element access must stay within the live extent and preserve initialized representations. Synchronize concurrent access involving mutation externally.

```c
typedef struct CATRLLIB_ARRAY {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
} CATRLLIB_ARRAY;
```

**Fields:**

- `data`: Owned contiguous storage, null when unallocated.\
- `size`: Nonzero bytes per individual element, fixed at initialization.\
- `length`: Number of live elements. Valid retrieval/replacement/removal indices are `[0, length)`.\
- `capacity`: Number of slots available without allocation; `length <= capacity`. The byte extent `capacity * size` must be representable and allocatable.

**C++ equivalent:** `catalyst::ARRAY`, an alias of `CATRLLIB_ARRAY`.

Initialize without allocation, then explicitly expand before appending or inserting beyond capacity. Expansion honors an exact larger request and treats smaller/equal requests as no-ops. Shrink selects `max(length, floor(capacity / 2))`, preserving order. Repeated shrinking of an empty Array eventually releases its allocation; clear/removal retain capacity. Successful free retains `size` for reuse.

Indexed access and append have constant overhead apart from copying an element. Insert/remove move the trailing byte extent with `catmm_move`; membership uses linear bytewise comparison. Mutating operations do not allocate except expand/shrink. Successful resizing invalidates all storage aliases, even if the address is unchanged; failure preserves pointer, length, capacity, and contents. Insertion/removal change the indexed elements at and after the affected index; pointers into that suffix must not be reused as references to their former indexed elements. Earlier elements remain in place. Append retains existing element slots, while replacement overwrites only the selected element's bytes. Clear ends all live elements without releasing storage. Unused slots are not elements; clear/remove do not securely erase them.

### CATRLLIB_FILE

Owns two publicly accessible Arrays and a caller-supplied resolver. Metadata represents the resolution rules, and the resolver interprets the current metadata and data on every call. CATRLLIB imposes no metadata format, filesystem mapping, or relationship between an index and either Array's length. The resolver establishes the finite contiguous index domain required by FRELSPEC and reports invalid indices or malformed contents according to its contract.

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

- `metadata`: Owned Array of metadata elements; its `size` is bytes per metadata element.\
- `data`: Owned Array of data elements; its `size` is bytes per data element and may differ from metadata's size.\
- `resolver`: Non-null callable while the File is available for resolution; cleared by successful File release.

**C++ equivalent:** `catalyst::FILE`, an alias of `CATRLLIB_FILE`.

`catrl_file_init` moves Array ownership without allocation and leaves the two source Arrays empty and reusable. Use ordinary Array operations on `file.metadata` and `file.data` to change their contents and capacities; the resolver sees those changes without a cached interpretation. Preserve the Array invariants and the resolver's own content requirements. Do not shallow-copy an owning File. Synchronization belongs to the caller.

The resolver determines output initialization requirements, output element size/count/capacity, whether output owns copies or borrows a view, the borrowed view's lifetime, and output validity for every status. An owned output Array is released with `catrl_array_free`; a borrowed view must not be resized or freed as owned storage. The writable `output` Array object itself must not overlap the File object or either owned allocation. Its `output->data` pointer may reference File storage when the resolver returns a borrowed view; this does not transfer ownership out of the File. Such a view is subject to the resolver's lifetime rules and the underlying Array's pointer invalidation rules. The resolver must preserve File ownership and release or expose every allocation it creates on failure. CATRLLIB does not initialize, allocate, free, or roll back resolver output.

`catrl_file_get` invokes the resolver exactly once and retains its status and detail, with CATRLLIB's context and operation. File release frees metadata before data and leaves any unreleased allocation reachable for retry. Earlier successful release is not rolled back. After complete release, initialize the File again before resolution.

### CATRLLIB_STREAM

A producer and its state, representing a finite or infinite sequence of arbitrary chunks. CATRLLIB imposes no element encoding, fixed chunk size, internal buffering, numeric sequence counter, or end-of-sequence sentinel.

```c
typedef struct CATRLLIB_STREAM CATRLLIB_STREAM;
struct CATRLLIB_STREAM {
    CATMMLIB_POINTERW state;
    CATALYST_RESULT (*next)(
        CATRLLIB_STREAM* stream,
        CATMMLIB_POINTERW* output
    );
};
```

**Fields:**

- `state`: Borrowed producer-defined state; may be null if the producer permits it. The producer specifies ownership, lifetime, and cleanup; CATRLLIB never allocates or releases this state.\
- `next`: Non-null producer receiving this Stream and an output pointer slot.

**C++ equivalent:** `catalyst::STREAM`, an alias of `CATRLLIB_STREAM`.

Initialize by binding state and producer. `catrl_stream_next` calls the producer exactly once with the same Stream and output slot. The pointer written to that slot identifies a producer-defined chunk; it is not necessarily an Array or a byte buffer. The producer determines its type, byte size, ownership, lifetime, initialization, output validity on every status, and whether later calls invalidate previous chunks. Any producer parameter or field named `size` denotes bytes per individual chunk, never total stream length.

The producer owns sequencing, progress on success/warning/failure, buffering, synchronization requirements, finite termination, and infinite behavior. Neither null output nor a particular status is assigned a universal termination meaning by CATRLLIB. Status and detail are retained at the CATRLLIB reporting boundary. CATRLLIB does not clear output before or after the call, retry the producer, or roll back producer state. There is no Stream release operation: the caller follows the producer's cleanup contract before disposing of or rebinding state.

## Functions

Valid initialized objects, non-null object/input/output pointers, readable and writable extents, non-overlap, lifetime, synchronization, and direct-access indices are caller preconditions, as described in [Assumptions](../../assumptions/index.md). Initialization functions receive valid unused storage, not a live owning collection. Element sizes are nonzero. These conditions are not checked or converted into sentinel return values. The existing void/Boolean mutation signatures require callers to reserve capacity before inserting a new element or key; duplicate Set/Map add may be called at full capacity because no insertion occurs.

For operations that copy caller data into a collection (`catrl_set_add`, `catrl_map_add`, `catrl_map_set`, `catrl_array_add`, `catrl_array_insert`, and `catrl_array_set`), the entire input extent being copied must be outside the collection object and its owned allocations. Retrieval output extents for `catrl_map_get` and `catrl_array_get` must likewise be outside the object and its allocations. These rules concern whole readable/writable extents, not just pointer starting addresses; no overlap detection or temporary allocation is performed. Read-only lookup inputs and removal search inputs may identify live stored bytes because searching finishes before mutation. The lookup key of `catrl_map_set` may also identify a live key, as documented below.

All allocation arithmetic uses `CATALYST_NUINT`, including on targets with one-byte pointers. Before expansion, each capacity is checked against `CATALYST_NUINT_MAX / element_size` before multiplication. Existing live counts and indices bound later offset and suffix products by that established extent. A spare slot makes count increments representable; valid direct-access indices make decrements and suffix subtraction nonnegative. Ordinary C/C++ integer promotions may occur, but correctness does not rely on their providing a wider result. No integer representation wider than the target's `CATALYST_NUINT`, packed Result, native byte order, alignment-sensitive element load, OS call, or standard-library routine is required by CATRLLIB. CATMMLIB supplies native allocation, fundamental alignment, and additional prefix/page limits; deployment on a target requires a CATMMLIB backend for that target. This does not imply that all targets already have one.

An empty Set or Array reaches zero capacity by shrinking from capacity one; CATMMLIB resizes the live pointer to zero, releasing it and clearing it only on success. An already zero-capacity collection is a no-op, so shrinking never calls `catmm_realloc` with both a null pointer and zero size, which is outside CATMMLIB's preconditions. Map uses the same rule for its two buffers, publishing zero capacity after successful key release and retaining any values whose release fails. Residual Map allocations at zero capacity remain owned and are released with `catrl_map_free`, even if another shrink is a no-op.

Reporting methods use fixed contexts from declaration order and distinct operations per reporting site. Dependency failures keep applicable status, fatal severity, and useful detail. In the resize tables, current CATMMLIB dual-cleanup failures can report `ERROR_INVALID_STATE` with detail `84`, or `ERROR_ACCESS_DENIED` with detail `82`; these are the other cleanup failure's status. Other listed dependency details are unused. A future dependency's error/fatal status and useful detail are likewise retained. `ERROR_INVALID_ARGUMENT` detail is always an argument index in the full reporting signature; resolver/producer signatures have the same argument positions as their CATRLLIB boundary.

File resolution and Stream production delegate operational validation to the callback. Their current reporting boundary replaces callback context and operation with fixed CATRLLIB codes, retaining only status and detail. [FDEFSPEC](https://catalystui.org/specifications/languages/fdefspec/) permits detail to depend on its original context or operation; that diagnostic provenance is therefore lost and can make a callback-specific detail ambiguous. The original callback context and operation cannot be recovered from the returned Result.

### catrl_set_init

```c
void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size);
```

Initialize an empty Set without allocation. Set `data`, `cardinality`, and `capacity` to zero and retain the supplied element size. Release existing ownership before reinitializing.

**Parameters:**

- `set`: Valid writable unused `CATRLLIB_SET` storage.\
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::set_init`.

### catrl_set_add

```c
CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Search by byte equality; if absent, append a copy of `size` bytes and increment cardinality. An existing member is unchanged. No allocation or capacity change occurs.

**Parameters:**

- `set`: Valid initialized Set; a new distinct element requires `cardinality < capacity`.\
- `element`: Non-null pointer to `set->size` readable bytes outside the Set object and its owned allocation.

**C++ equivalent:** `catalyst::set_add`.

**Returns:** `CATALYST_TRUE` if inserted; `CATALYST_FALSE` if already present, with no mutation. These are ordinary outcomes, not allocation diagnostics.

### catrl_set_remove

```c
CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Search by byte equality. On membership, replace the removed slot with the last member if necessary, then reduce cardinality. Order is unspecified; capacity is retained. No allocation or referenced-object release occurs.

**Parameters:**

- `set`: Valid initialized Set.\
- `element`: Non-null pointer to `set->size` readable bytes; may identify a live stored element because lookup completes before mutation.

**C++ equivalent:** `catalyst::set_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Set unchanged.

### catrl_set_contains

```c
CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element);
```

Search the live elements by byte equality without mutation or allocation.

**Parameters:**

- `set`: Valid initialized Set.\
- `element`: Non-null pointer to `set->size` readable bytes, including when the Set is empty.

**C++ equivalent:** `catalyst::set_contains`.

**Returns:** `CATALYST_TRUE` if present; `CATALYST_FALSE` if absent.

### catrl_set_clear

```c
void catrl_set_clear(CATRLLIB_SET* set);
```

Set `cardinality` to zero, retaining allocation, element size, and capacity. No allocation, release, or pointee cleanup occurs; old bytes remain outside the live extent.

**Parameters:**

- `set`: Valid initialized Set.

**C++ equivalent:** `catalyst::set_clear`.

### catrl_set_expand

```c
CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity);
```

Request exactly `capacity` element slots. A larger request resizes with CATMMLIB and preserves all live elements. Smaller/equal requests are no-ops. No automatic doubling overrides this explicit request. The element byte size is unchanged. On failure, pointer, contents, count, and logical capacity are unchanged. Successful resizing invalidates previous storage aliases. Newly available slots are not live elements.

**Parameters:**

- `set`: Valid initialized Set.\
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

Reduce capacity to `max(cardinality, floor(capacity / 2))`, preserving all live elements. If unchanged, return a no-op. The zero target releases storage and clears `data`; for an empty collection with capacity greater than one, a single call retains half the slots. Failure leaves the collection unchanged. Success invalidates storage aliases; element size remains available for reuse.

**Parameters:**

- `set`: Valid initialized Set.

**C++ equivalent:** `catalyst::set_shrink`.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01070100` | `SUCCESS_NOOP`: capacity cannot be reduced without dropping live elements | Keep storage | — |
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

Release owned storage and set `data`, `cardinality`, and `capacity` to zero. Preserve `size` so the Set can be expanded and reused. An unallocated Set is a no-op. Failure leaves pointer, elements, and metadata unchanged, with ownership retained for retry. Stored pointer targets are never released.

**Parameters:**

- `set`: Valid initialized Set, including an already freed/reusable one.

**C++ equivalent:** `catalyst::set_free`.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01080100` | `SUCCESS_NOOP`: no owned allocation | Keep empty collection | — |
| `0x82080200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release storage | — |
| `0x84080200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release storage | — |
| `0x00080300` | `SUCCESS` | Clear ownership and count | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_map_init

```c
void catrl_map_init(CATRLLIB_MAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize);
```

Initialize an empty Map without allocation. Set both storage pointers, cardinality, and capacity to zero. Release existing ownership before reinitializing.

**Parameters:**

- `map`: Valid writable unused Map storage.\
- `ksize`: Nonzero bytes per individual key.\
- `vsize`: Nonzero bytes per individual value; need not equal `ksize`.

**C++ equivalent:** `catalyst::map_init`.

### catrl_map_add

```c
CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);
```

Search keys by byte equality. Copy a new key and its value into matching slots, then increment cardinality. A duplicate key leaves its previous value unchanged; use `catrl_map_set` to replace it. No allocation or capacity change occurs.

**Parameters:**

- `map`: Valid initialized Map; a new key requires `cardinality < capacity`.\
- `key`: Non-null pointer to `ksize` readable bytes outside the Map object and both owned allocations.\
- `value`: Non-null pointer to `vsize` readable bytes outside the Map object and both owned allocations.

**C++ equivalent:** `catalyst::map_add`.

**Returns:** `CATALYST_TRUE` if inserted; `CATALYST_FALSE` if the key already exists, leaving the Map unchanged.

**See Also:** [catrl_map_set](#catrl_map_set)

### catrl_map_remove

```c
CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key);
```

Remove the matching key and value. Move the last pair into the removed slot if necessary and decrement cardinality. Capacity is retained; no allocation or referenced-object release occurs.

**Parameters:**

- `map`: Valid initialized Map.\
- `key`: Non-null pointer to `ksize` readable bytes; may identify a live key because lookup precedes mutation.

**C++ equivalent:** `catalyst::map_remove`.

**Returns:** `CATALYST_TRUE` if removed; `CATALYST_FALSE` if absent, leaving the Map unchanged.

### catrl_map_contains

```c
CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key);
```

Search the live keys by byte equality without mutation or allocation.

**Parameters:**

- `map`: Valid initialized Map.\
- `key`: Non-null pointer to `ksize` readable bytes, including when the Map is empty.

**C++ equivalent:** `catalyst::map_contains`.

**Returns:** `CATALYST_TRUE` if the key exists; `CATALYST_FALSE` otherwise.

### catrl_map_get

```c
CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value);
```

Search for a key, then copy its associated value into caller output. No allocation occurs. Absence is an ordinary Boolean outcome and leaves every output byte unchanged.

**Parameters:**

- `map`: Valid initialized Map.\
- `key`: Non-null pointer to `ksize` readable bytes.\
- `value`: Non-null writable output of at least `vsize` bytes outside the Map object and its owned allocations.

**C++ equivalent:** `catalyst::map_get`.

**Returns:** `CATALYST_TRUE` with `vsize` output bytes written if found; `CATALYST_FALSE` with output unchanged otherwise.

### catrl_map_set

```c
CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);
```

Search for a key, then copy `vsize` bytes over its existing value. Do not insert absent keys or change cardinality/capacity; no allocation occurs.

**Parameters:**

- `map`: Valid initialized Map.\
- `key`: Non-null pointer to `ksize` readable bytes; may identify a live stored key.\
- `value`: Non-null pointer to `vsize` readable bytes outside the Map object and both owned allocations.

**C++ equivalent:** `catalyst::map_set`.

**Returns:** `CATALYST_TRUE` if replaced, including an identical value; `CATALYST_FALSE` if absent, leaving the Map unchanged.

### catrl_map_clear

```c
void catrl_map_clear(CATRLLIB_MAP* map);
```

Set cardinality to zero, retaining both allocations, sizes, and logical capacity. No allocation, release, or pointee cleanup occurs; old bytes are outside the live extent.

**Parameters:**

- `map`: Valid initialized Map.

**C++ equivalent:** `catalyst::map_clear`.

### catrl_map_expand

```c
CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity);
```

Request exactly `capacity` pairs. Validate both byte extents before resizing keys, then values. Successful completion preserves every pair and establishes the requested logical capacity. Smaller/equal requests are no-ops. On key failure the Map is unchanged; on value failure all pairs and logical capacity remain unchanged, while key aliases may have been invalidated and the larger key allocation remains owned. Use the current public pointers after any attempted resize. This supports retry without leaking or requiring rollback allocation.

**Parameters:**

- `map`: Valid initialized Map, including an empty Map retaining storage after a partial operation.\
- `capacity`: Requested number of pairs, not bytes. Each independent product with `ksize` and `vsize` must be representable and allocatable.

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

Choose `max(cardinality, floor(capacity / 2))` slots. Preserve every live pair. An unchanged target is a no-op. Resize keys first and immediately publish the smaller safe logical capacity, then resize values. On key failure nothing changes; on value failure the smaller capacity and updated key pointer remain, and the original value allocation stays owned. A zero target releases buffers in the same order, with any unreleased values retained. Use current pointers after any attempted resize. Zero-capacity residual storage is released by `catrl_map_free`.

**Parameters:**

- `map`: Valid initialized Map.

**C++ equivalent:** `catalyst::map_shrink`.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01110100` | `SUCCESS_NOOP`: capacity cannot be reduced | Keep storage | — |
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

Release keys, then values, skipping null pointers. If key release fails, nothing changes. On successful key release, clear keys, cardinality, and capacity before attempting value release. A value failure retains the live value allocation for a later free or expansion; the Map is empty with zero logical capacity. Complete success clears both pointers. Preserve `ksize` and `vsize` for reuse. No referenced objects are released, and no live allocation becomes unreachable.

**Parameters:**

- `map`: Valid initialized Map, including one retaining a single buffer after a partial operation.

**C++ equivalent:** `catalyst::map_free`.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x82120100` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release keys | — |
| `0x84120100` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release keys | — |
| `0x82120200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release values | — |
| `0x84120200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release values | — |
| `0x01120300` | `SUCCESS_NOOP`: both pointers are null | Keep empty Map | — |
| `0x00120400` | `SUCCESS` | All owned storage released | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_array_init

```c
void catrl_array_init(CATRLLIB_ARRAY* array, CATALYST_NUINT size);
```

Initialize an empty Array without allocation. Set `data`, `length`, and `capacity` to zero and retain the supplied element size. Release existing ownership before reinitializing.

**Parameters:**

- `array`: Valid writable unused `CATRLLIB_ARRAY` storage.\
- `size`: Nonzero size in bytes of each individual element.

**C++ equivalent:** `catalyst::array_init`.

### catrl_array_add

```c
void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Copy one element to index `length`, then increment length. Capacity and all previous elements are retained; no allocation occurs.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.\
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_add`.

### catrl_array_insert

```c
void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Move the suffix starting at `index` right by one element with `catmm_move`, copy the new element, then increment length. Inserting at `length` appends. Preserve relative order of existing elements and retain capacity; no allocation occurs. An input pointing into the shifted suffix can be overwritten before the final copy reads it; internal aliasing is unsupported even when a particular self-copy would happen to work. The complete input extent must be outside both the Array object and its allocation.

**Parameters:**

- `array`: Valid initialized Array with `length < capacity`.\
- `index`: Insertion position from zero through `length`, inclusive.\
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation; must remain valid during the suffix move.

**C++ equivalent:** `catalyst::array_insert`.

### catrl_array_remove

```c
void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index);
```

Remove one element, shift its trailing elements left with `catmm_move`, and decrement length. Preserve their relative order and retain capacity. No allocation, shrinking, or pointee release occurs.

**Parameters:**

- `array`: Valid initialized non-empty Array.\
- `index`: Valid direct-access index, strictly less than `length`.

**C++ equivalent:** `catalyst::array_remove`.

### catrl_array_contains

```c
CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);
```

Search live elements by byte equality across `size` bytes, without mutation or allocation.

**Parameters:**

- `array`: Valid initialized Array.\
- `element`: Non-null pointer to `size` readable bytes, including when the Array is empty.

**C++ equivalent:** `catalyst::array_contains`.

**Returns:** `CATALYST_TRUE` if an equal element exists; `CATALYST_FALSE` otherwise.

### catrl_array_get

```c
void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element);
```

Copy exactly `size` bytes from a live indexed element. No allocation or metadata change occurs.

**Parameters:**

- `array`: Valid initialized Array.\
- `index`: Valid direct-access index, strictly less than `length`.\
- `element`: Non-null writable output of at least `size` bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_get`.

### catrl_array_set

```c
void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);
```

Replace exactly one live element with a copy of `size` bytes. Length, capacity, and other elements remain unchanged; no allocation occurs.

**Parameters:**

- `array`: Valid initialized Array.\
- `index`: Valid direct-access index, strictly less than `length`.\
- `element`: Non-null pointer to `size` readable bytes outside the Array object and its owned allocation.

**C++ equivalent:** `catalyst::array_set`.

### catrl_array_clear

```c
void catrl_array_clear(CATRLLIB_ARRAY* array);
```

Set `length` to zero, retaining allocation, element size, and capacity. No allocation, release, or pointee cleanup occurs; old bytes remain outside the live extent.

**Parameters:**

- `array`: Valid initialized Array.

**C++ equivalent:** `catalyst::array_clear`.

### catrl_array_expand

```c
CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity);
```

Request exactly `capacity` element slots. A larger request resizes with CATMMLIB and preserves all live elements and order. Smaller/equal requests are no-ops. No automatic doubling overrides this explicit request. The element byte size is unchanged. On failure, pointer, contents, count, and logical capacity are unchanged. Successful resizing invalidates previous storage aliases. Newly available slots are not live elements.

**Parameters:**

- `array`: Valid initialized Array.\
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

Reduce capacity to `max(length, floor(capacity / 2))`, preserving all live elements and order. If unchanged, return a no-op. The zero target releases storage and clears `data`; for an empty collection with capacity greater than one, a single call retains half the slots. Failure leaves the collection unchanged. Success invalidates storage aliases; element size remains available for reuse.

**Parameters:**

- `array`: Valid initialized Array.

**C++ equivalent:** `catalyst::array_shrink`.

**Context:** `0x1C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011C0100` | `SUCCESS_NOOP`: capacity cannot be reduced without dropping live elements | Keep storage | — |
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

Release owned storage and set `data`, `length`, and `capacity` to zero. Preserve `size` so the Array can be expanded and reused. An unallocated Array is a no-op. Failure leaves pointer, elements, and metadata unchanged, with ownership retained for retry. Stored pointer targets are never released.

**Parameters:**

- `array`: Valid initialized Array, including an already freed/reusable one.

**C++ equivalent:** `catalyst::array_free`.

**Context:** `0x1D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011D0100` | `SUCCESS_NOOP`: no owned allocation | Keep empty collection | — |
| `0x821D0200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release storage | — |
| `0x841D0200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release storage | — |
| `0x001D0300` | `SUCCESS` | Clear ownership and count | — |

**See Also:** [catmm_free](../catmmlib/index.md#catmm_free)

### catrl_file_init

```c
void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data, CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*));
```

Move the metadata and data Arrays into the File by value, including their allocations, sizes, lengths, and capacities; bind the resolver. Empty both source Arrays with their element sizes retained, so they are immediately reusable. No allocation, byte-buffer copy, or failure-producing operation occurs. The File now owns both original allocations; do not free them through previous aliases. The buffers do not move during ownership transfer, so existing element pointers retain their addresses but become subject to the File's storage lifetime and subsequent Array operations.

**Parameters:**

- `file`: Valid writable unused File storage, separate from both source Array objects and all their allocations.\
- `metadata`: Valid initialized owning Array, distinct from `data`, with independently owned storage.\
- `data`: Valid initialized owning Array, distinct from `metadata`, with independently owned storage.\
- `resolver`: Non-null callable with the declared C return/parameter types; it establishes metadata interpretation, valid index domain, and output/failure contracts.

**C++ equivalent:** `catalyst::file_init`.

**See Also:** [CATRLLIB_FILE](#catrllib_file), [catrl_file_free](#catrl_file_free)

### catrl_file_get

```c
CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output);
```

Invoke `file->resolver(file, index, output)` exactly once on current public metadata and data. No generic index bound, metadata decoding, output initialization, allocation, or copying is added. The resolver validates operational conditions such as unsupported indices or malformed metadata. All output fields, ownership, lifetime, and output validity on success/warning/error/fatal/no-op follow the resolver contract; CATRLLIB neither initializes nor rolls back output. Return the callback status/detail with the fixed context and call-site operation below.

**Parameters:**

- `file`: Valid initialized File with a non-null resolver and valid owned Arrays; satisfy resolver-specific preconditions.\
- `index`: Index presented unchanged to the resolver; CATRLLIB does not compare it with metadata/data length.\
- `output`: Valid writable Array object outside the File object and both owned allocations, initialized as required by the resolver. Any ownership already in it must be handled according to that resolver contract. The resolver may set `output->data` to a borrowed view of File storage, with ownership and lifetime as specified by that resolver.

**C++ equivalent:** `catalyst::file_get`.

**Context:** `0x1F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS1F01DD` | Resolver status `SS` retained, including success, warning, error, fatal, and no-op | Invoke resolver once | Resolver detail `DD`; invalid-argument indices are unchanged |

**See Also:** [CATRLLIB_FILE](#catrllib_file)

### catrl_file_free

```c
CATALYST_RESULT catrl_file_free(CATRLLIB_FILE* file);
```

Free the metadata Array, then the data Array, and finally clear the resolver. Array element sizes are retained. Metadata release failure leaves both Arrays and resolver unchanged. If data release fails, metadata is already empty but the data allocation and resolver remain for retry; previous metadata aliases are invalid. No earlier successful release is rolled back. All unreleased resources remain reachable. No referenced objects or separate resolver output are released. Resolve again only after restoring the resolver-specific content requirements; after complete release, initialize the File again before calling `catrl_file_get`.

**Parameters:**

- `file`: Valid initialized File, including an already released File or one left by a prior partial release.

**C++ equivalent:** `catalyst::file_free`.

**Context:** `0x20`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x01200100` | `SUCCESS_NOOP`: no allocations and resolver already null | Keep released File | — |
| `0x82200200` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release metadata Array | — |
| `0x84200200` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release metadata Array | — |
| `0x82200300` | `ERROR_INVALID_STATE`: release or pending cleanup failed | Release data Array | — |
| `0x84200300` | `ERROR_ACCESS_DENIED`: allocation, release, or cleanup denied | Release data Array | — |
| `0x00200400` | `SUCCESS` | Clear resolver after releases | — |

**See Also:** [catrl_array_free](#catrl_array_free)

### catrl_stream_init

```c
void catrl_stream_init(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW state, CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW*));
```

Bind the provided state and producer without allocation or taking ownership. Initialization does not invoke the producer. State ownership, lifetime, and cleanup are producer-defined. The caller must complete producer-specific cleanup before replacing any earlier binding.

**Parameters:**

- `stream`: Valid writable Stream storage separate from the borrowed state.\
- `state`: Borrowed producer-defined pointer; null is allowed when supported by the producer. Its lifetime must cover calls that use it.\
- `next`: Non-null producer with the declared C return/parameter types; defines sequence progress, termination, chunk contracts, and cleanup.

**C++ equivalent:** `catalyst::stream_init`.

**See Also:** [CATRLLIB_STREAM](#catrllib_stream)

### catrl_stream_next

```c
CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW* output);
```

Invoke `stream->next(stream, output)` exactly once. Pass the same output pointer slot unchanged. The pointer written by the producer identifies an arbitrary chunk whose type, byte size, ownership, and lifetime belong to the producer. The producer defines output validity and state progression for every status, including failure and termination. CATRLLIB does not allocate, buffer, advance a separate counter, clear output, infer termination, retry, or undo producer state. Retain callback status/detail with the reporting metadata below.

**Parameters:**

- `stream`: Valid initialized Stream with a non-null producer and state satisfying that producer contract.\
- `output`: Valid non-null writable `CATMMLIB_POINTERW` slot, separate from the Stream and state; the producer defines initial-value requirements and the returned pointer contract.

**C++ equivalent:** `catalyst::stream_next`.

**Context:** `0x22`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0xSS2201DD` | Producer status `SS` retained, including success, warning, error, fatal, and no-op | Invoke producer once | Producer detail `DD`; invalid-argument indices are unchanged |

**See Also:** [CATRLLIB_STREAM](#catrllib_stream)
