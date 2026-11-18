# CATRLLIB

Provides fixed-length arrays, sets, maps, indexed persistent files, and sequential byte streams.

**C Header:** `catrllib.h`  
**C++ Header:** `catrllib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catrl`  
**Dependencies:** `catmm`

## Type Summary

| C Type | C++ Type | Description |
| --- | --- | --- |
| [`CATALYST_ARRAY`](#catalyst_array) | `ARRAY` | Fixed-length indexed elements. |
| [`CATALYST_SET`](#catalyst_set) | `SET` | Distinct byte values. |
| [`CATALYST_MAP`](#catalyst_map) | `MAP` | Unique byte keys associated with values. |
| [`CATALYST_FILE`](#catalyst_file) | `FILE` | Indexed bytes from persistent storage. |
| [`CATALYST_STREAM`](#catalyst_stream) | `STREAM` | Sequential byte chunks. |
| [`CATALYST_FILE_READER`](#catalyst_file_reader) | `FILE_READER` | Persistent byte reader. |
| [`CATALYST_STREAM_READER`](#catalyst_stream_reader) | `STREAM_READER` | Sequential chunk producer. |

## Constant Summary

| Constant | Description |
| --- | --- |
| [`CATRLLIB_API`](#catrllib_api) | Export/import annotation. |

## Function Summary

### Array

| Function | Description |
| --- | --- |
| [`catrl_array_create`](#catrl_array_create) | Creates a fixed-length array of length elements, each `element_size` bytes. |
| [`catrl_array_destroy`](#catrl_array_destroy) | Releases array storage and handle. |
| [`catrl_array_get`](#catrl_array_get) | Copies the element at index into value, which must hold `element_size` bytes. |
| [`catrl_array_set`](#catrl_array_set) | Replaces the element at index with a copy of `element_size` bytes from value. |
| [`catrl_array_length`](#catrl_array_length) | Returns the fixed number of elements. |

### Set

| Function | Description |
| --- | --- |
| [`catrl_set_create`](#catrl_set_create) | Creates an empty set of `element_size`-byte elements. |
| [`catrl_set_destroy`](#catrl_set_destroy) | Releases set storage and handle. |
| [`catrl_set_add`](#catrl_set_add) | Copies value into the set if no byte-identical element exists. |
| [`catrl_set_remove`](#catrl_set_remove) | Removes a byte-identical element. |
| [`catrl_set_contains`](#catrl_set_contains) | Reports whether the set contains the specified value. |
| [`catrl_set_count`](#catrl_set_count) | Returns the number of distinct elements. |
| [`catrl_set_at`](#catrl_set_at) | Copies the element at an unordered enumeration index into value. |
| [`catrl_set_clear`](#catrl_set_clear) | Removes all elements without releasing storage capacity or zeroing the stored bytes. An empty collection is unchanged. |

### Map

| Function | Description |
| --- | --- |
| [`catrl_map_create`](#catrl_map_create) | Creates an empty map with fixed `key_size` and `value_size` bytes per association. |
| [`catrl_map_destroy`](#catrl_map_destroy) | Releases map storage and handle. |
| [`catrl_map_get`](#catrl_map_get) | Retrieves the value associated with a byte-identical key. |
| [`catrl_map_set`](#catrl_map_set) | Inserts a key/value association or updates an existing key without increasing the count. |
| [`catrl_map_remove`](#catrl_map_remove) | Removes an association with a byte-identical key. |
| [`catrl_map_contains`](#catrl_map_contains) | Reports whether the map contains the specified key. |
| [`catrl_map_count`](#catrl_map_count) | Returns the number of key/value associations. |
| [`catrl_map_at`](#catrl_map_at) | Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes. |
| [`catrl_map_clear`](#catrl_map_clear) | Removes all associations without releasing storage capacity or zeroing stored bytes. |

### File

| Function | Description |
| --- | --- |
| [`catrl_file_create`](#catrl_file_create) | Bind a fixed-length persistent byte source. |
| [`catrl_file_destroy`](#catrl_file_destroy) | Release the handle, not the underlying persistent source. |
| [`catrl_file_get`](#catrl_file_get) | Evaluate the persistent byte source at an index. |
| [`catrl_file_length`](#catrl_file_length) | Return the fixed length of the indexed source in bytes. |

### Stream

| Function | Description |
| --- | --- |
| [`catrl_stream_create`](#catrl_stream_create) | Bind a sequential byte producer without retrieving a chunk. |
| [`catrl_stream_destroy`](#catrl_stream_destroy) | Release the handle without destroying its producer or freeing borrowed chunks. |
| [`catrl_stream_next`](#catrl_stream_next) | Retrieve exactly the next chunk from the producer. |

## Type Details

In-memory collections own copies of their elements, keys, and values, each of the configured byte size. Embedded pointers are shallow copies; referenced resources remain caller-owned. Internal byte storage has no typed alignment guarantee. Handles must be live and correctly typed; concurrent access involving mutation or destruction requires external synchronization. A handle must not be reused after destruction is attempted, even if cleanup fails.

### CATALYST_ARRAY

Opaque handle to a fixed-length array. Create with [`catrl_array_create`](#catrl_array_create) and release with [`catrl_array_destroy`](#catrl_array_destroy).

```c
typedef struct CATALYST_ARRAY CATALYST_ARRAY;
```

**C++:** `catalyst::ARRAY`.

### CATALYST_SET

Opaque handle to a set of distinct byte values. Equality compares every byte, including padding. Enumeration order is unspecified and may change on mutation. Create with [`catrl_set_create`](#catrl_set_create) and release with [`catrl_set_destroy`](#catrl_set_destroy).

```c
typedef struct CATALYST_SET CATALYST_SET;
```

**C++:** `catalyst::SET`.

### CATALYST_MAP

Opaque handle to a map of unique byte keys and associated values. Key equality compares every byte, including padding. Enumeration order is unspecified and may change on mutation. Create with [`catrl_map_create`](#catrl_map_create) and release with [`catrl_map_destroy`](#catrl_map_destroy).

```c
typedef struct CATALYST_MAP CATALYST_MAP;
```

**C++:** `catalyst::MAP`.

### CATALYST_FILE

Opaque handle to a finite, contiguous indexed byte source originating from persistent storage, as defined by FRELSPEC. The provider supplies persistent storage; the handle binds its length and reader. No path, cursor, file descriptor, or native file handle is required. The provider must preserve the bound length for the handle's lifetime.

```c
typedef struct CATALYST_FILE CATALYST_FILE;
```

**C++:** `catalyst::FILE` (qualify the name when the C standard library's `FILE` is visible).

### CATALYST_STREAM

Opaque handle to a source of successive byte chunks. No fixed chunk size, known total length, or random access is required.

```c
typedef struct CATALYST_STREAM CATALYST_STREAM;
```

**C++:** `catalyst::STREAM`.

### CATALYST_FILE_READER

A provider callback implementing indexed evaluation of a persistent source. `state` is caller-owned; `index` is a valid zero-based byte index; `value` receives one byte on SUCCESS. The provider must permit arbitrary index order and repeated reads. It must obtain bytes from persistent storage, not merely an in-memory array.

```c
typedef CATALYST_RESULT (*CATALYST_FILE_READER)(
    void* state,
    CATALYST_NUINT index,
    CATALYST_BYTE* value
);
```

**C++:** `catalyst::FILE_READER`.

### CATALYST_STREAM_READER

A provider callback implementing successive chunk retrieval or generation. On SUCCESS, `chunk` receives a borrowed read-only pointer to a contiguous byte sequence and `length` receives its byte count. A zero-length chunk is permitted; nonempty chunks require non-null storage. SUCCESS_NOOP denotes permanent exhaustion. The producer owns the storage and must keep it readable until its next invocation or destruction of the backing state, whichever comes first. Copy the bytes if they are needed later.

```c
typedef CATALYST_RESULT (*CATALYST_STREAM_READER)(
    void* state,
    const CATALYST_BYTE** chunk,
    CATALYST_NUINT* length
);
```

**C++:** `catalyst::STREAM_READER`.

#### Provider result conventions

The caller supplies valid provider functions and keeps their state alive. The state may be null for stateless providers. Handles do not own, close, or destroy provider state. Access to provider state must be externally synchronized, including calls through different handles. The provider may fail even when all caller preconditions hold.

A provider returns SUCCESS with complete output, a defined error or fatal status on failure, or (for a stream only) SUCCESS_NOOP for permanent exhaustion. A file provider cannot report a successful no-op, since an indexed evaluation must return a byte. Provider warnings and other success-level statuses are not complete outputs under this contract.

Provider errors and fatal statuses are reported at the enclosing function's context and operation. In result patterns, SS is the provider status: for the direct failure row it is a defined error/fatal code other than 0x81 (0x80, 0x82–0x8A, 0x90, 0x91, 0xA0, 0xC0, 0xC1); DD is the provider detail for 0x8A and zero otherwise. For the dependency-failure row, SS is 0x81 or 0x01–0x7F (except 0x01 for stream retrieval). Provider argument indices are not public argument indices; an invalid provider argument therefore reports ERROR_DEPENDENCY_FAILURE instead. The provider must not return reserved status codes.

## Constant Details

### CATRLLIB_API

`CATRLLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATRLLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATRLLIB_USE_DLL` is defined. Otherwise it is empty.

## Function Details

Input and output buffers must not overlap. Creation clears the output handle before allocation and replaces any existing handle without destroying it. Recoverable mutation failures leave logical contents unchanged; fatal failures may invalidate the collection.

### Array

#### catrl_array_create

```c
CATALYST_RESULT catrl_array_create(
    CATALYST_ARRAY** array,
    CATALYST_NUINT element_size,
    CATALYST_NUINT length
);
```

Creates a fixed-length array of `length` elements, each `element_size` bytes. All elements are zero-filled; zero length is valid and requires no element storage.

**Parameters:**

`array` — Receives the new array handle; cleared on failure.  
`element_size` — Element size in bytes. Must be nonzero.  
`length` — Number of elements; may be zero.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010300` | `SUCCESS` | Create storage | — |
| `0x82010302` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x82010303` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x03`: Release handle during creation cleanup. |
| `0x82010304` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x90010301` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x90010302` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90010304` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x91010302` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91010304` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x04`: Allocate element storage. |

**C++:** `catalyst::RESULT catalyst::array_create(ARRAY** array, NUINT element_size, NUINT length)`.


#### catrl_array_destroy

```c
CATALYST_RESULT catrl_array_destroy(
    CATALYST_ARRAY* array
);
```

Releases array storage and handle.

**Parameters:**

`array` — Array handle.

**Context:** `0x02`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00020200` | `SUCCESS` | Destroy storage | — |
| `0x82020201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82020202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |

**C++:** `catalyst::RESULT catalyst::array_destroy(ARRAY* array)`.


#### catrl_array_get

```c
void catrl_array_get(
    const CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    void* value
);
```

Copies the element at `index` into `value`.

**Parameters:**

`array` — Array handle.  
`index` — Zero-based index strictly below the array length.  
`value` — Writable buffer of at least the configured element size in bytes.

**C++:** `void catalyst::array_get(const ARRAY* array, NUINT index, void* value)`.


#### catrl_array_set

```c
void catrl_array_set(
    CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    const void* value
);
```

Replaces the element at index with a copy of `element_size` bytes from value.

**Parameters:**

`array` — Array handle.  
`index` — Zero-based index strictly below the array length.  
`value` — Readable element of the configured byte size.

**C++:** `void catalyst::array_set(ARRAY* array, NUINT index, const void* value)`.


#### catrl_array_length

```c
CATALYST_NUINT catrl_array_length(const CATALYST_ARRAY* array);
```

Returns the fixed number of elements.

**Parameters:**

`array` — Valid live array handle.

**Returns:** The fixed number of elements.

**C++:** `catalyst::NUINT catalyst::array_length(const ARRAY* array)`.


### Set

#### catrl_set_create

```c
CATALYST_RESULT catrl_set_create(
    CATALYST_SET** set,
    CATALYST_NUINT element_size
);
```

Creates an empty set of `element_size`-byte elements. Element storage is allocated as needed.

**Parameters:**

`set` — Receives the new set handle; cleared on failure.  
`element_size` — Element size in bytes. Must be nonzero.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060300` | `SUCCESS` | Create storage | — |
| `0x82060302` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90060302` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91060302` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::RESULT catalyst::set_create(SET** set, NUINT element_size)`.


#### catrl_set_destroy

```c
CATALYST_RESULT catrl_set_destroy(
    CATALYST_SET* set
);
```

Releases set storage and handle.

**Parameters:**

`set` — Set handle.

**Context:** `0x07`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00070200` | `SUCCESS` | Destroy storage | — |
| `0x82070201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82070202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |

**C++:** `catalyst::RESULT catalyst::set_destroy(SET* set)`.


#### catrl_set_add

```c
CATALYST_RESULT catrl_set_add(
    CATALYST_SET* set,
    const void* value
);
```

Copies `value` into the set if no byte-identical element exists. Grows storage as needed. An existing element leaves the set unchanged and returns `SUCCESS_NOOP`.

**Parameters:**

`set` — Set handle.  
`value` — Readable element of the configured byte size.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00080700` | `SUCCESS` | Insert element | — |
| `0x01080400` | `SUCCESS_NOOP` | Skip existing element | — |
| `0x82080502` | `ERROR_INVALID_STATE` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90080501` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90080502` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91080502` | `ERROR_ALLOCATION_FAILED` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1080502` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |

**C++:** `catalyst::RESULT catalyst::set_add(SET* set, const void* value)`.


#### catrl_set_remove

```c
void catrl_set_remove(
    CATALYST_SET* set,
    const void* value
);
```

Removes a byte-identical element. Absence or emptiness is a no-op.

**Parameters:**

`set` — Set handle.  
`value` — Readable element of the configured byte size.

**C++:** `void catalyst::set_remove(SET* set, const void* value)`.


#### catrl_set_contains

```c
CATALYST_BOOL catrl_set_contains(
    const CATALYST_SET* set,
    const void* value
);
```

Reports whether the set contains the specified value.

**Parameters:**

`set` — Set handle.  
`value` — Readable element of the configured byte size.

**Returns:** `TRUE` if present, otherwise `FALSE`.

**C++:** `catalyst::BOOL catalyst::set_contains(const SET* set, const void* value)`.


#### catrl_set_count

```c
CATALYST_NUINT catrl_set_count(const CATALYST_SET* set);
```

Returns the number of distinct elements.

**Parameters:**

`set` — Valid live set handle.

**Returns:** The number of distinct elements.

**C++:** `catalyst::NUINT catalyst::set_count(const SET* set)`.


#### catrl_set_at

```c
void catrl_set_at(
    const CATALYST_SET* set,
    CATALYST_NUINT index,
    void* value
);
```

Copies the element at an unordered enumeration index into value.

**Parameters:**

`set` — Set handle.  
`index` — Zero-based index strictly below the collection count.  
`value` — Writable buffer of at least the configured element size in bytes.

**C++:** `void catalyst::set_at(const SET* set, NUINT index, void* value)`.


#### catrl_set_clear

```c
void catrl_set_clear(
    CATALYST_SET* set
);
```

Removes all elements without releasing storage capacity or zeroing the stored bytes. An empty collection is unchanged.

**Parameters:**

`set` — Set handle.

**C++:** `void catalyst::set_clear(SET* set)`.


### Map

#### catrl_map_create

```c
CATALYST_RESULT catrl_map_create(
    CATALYST_MAP** map,
    CATALYST_NUINT key_size,
    CATALYST_NUINT value_size
);
```

Creates an empty map with fixed `key_size` and `value_size` bytes per association. Element storage is allocated as needed.

**Parameters:**

`map` — Receives the new map handle; cleared on failure.  
`key_size` — Key size in bytes. Must be nonzero.  
`value_size` — Value size in bytes. Must be nonzero.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000E0500` | `SUCCESS` | Create storage | — |
| `0x820E0502` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x900E0400` | `ERROR_BUFFER_OVERFLOW` | Check combined key and value size | — |
| `0x900E0502` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x910E0502` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::RESULT catalyst::map_create(MAP** map, NUINT key_size, NUINT value_size)`.


#### catrl_map_destroy

```c
CATALYST_RESULT catrl_map_destroy(
    CATALYST_MAP* map
);
```

Releases map storage and handle.

**Parameters:**

`map` — Map handle.

**Context:** `0x0F`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000F0200` | `SUCCESS` | Destroy storage | — |
| `0x820F0201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x820F0202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |

**C++:** `catalyst::RESULT catalyst::map_destroy(MAP* map)`.


#### catrl_map_get

```c
CATALYST_RESULT catrl_map_get(
    const CATALYST_MAP* map,
    const void* key,
    void* value
);
```

Retrieves the value associated with a byte-identical key. If the key is absent, returns `ERROR_NOT_FOUND` without modifying the output buffer.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.  
`value` — Writable buffer of at least the configured value size in bytes.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00100700` | `SUCCESS` | Retrieve value | — |
| `0x88100500` | `ERROR_NOT_FOUND` | Locate key | — |

**C++:** `catalyst::RESULT catalyst::map_get(const MAP* map, const void* key, void* value)`.


#### catrl_map_set

```c
CATALYST_RESULT catrl_map_set(
    CATALYST_MAP* map,
    const void* key,
    const void* value
);
```

Inserts a key/value association or updates an existing key without increasing the count. Grows storage for new keys as needed.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.  
`value` — Readable value of the configured value size in bytes.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00110800` | `SUCCESS` | Store value | — |
| `0x82110502` | `ERROR_INVALID_STATE` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90110501` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90110502` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91110502` | `ERROR_ALLOCATION_FAILED` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1110502` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |

**C++:** `catalyst::RESULT catalyst::map_set(MAP* map, const void* key, const void* value)`.


#### catrl_map_remove

```c
void catrl_map_remove(
    CATALYST_MAP* map,
    const void* key
);
```

Removes an association with a byte-identical key. Absence or emptiness is a no-op.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.

**C++:** `void catalyst::map_remove(MAP* map, const void* key)`.


#### catrl_map_contains

```c
CATALYST_BOOL catrl_map_contains(
    const CATALYST_MAP* map,
    const void* key
);
```

Reports whether the map contains the specified key.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.

**Returns:** `TRUE` if present, otherwise `FALSE`.

**C++:** `catalyst::BOOL catalyst::map_contains(const MAP* map, const void* key)`.


#### catrl_map_count

```c
CATALYST_NUINT catrl_map_count(const CATALYST_MAP* map);
```

Returns the number of key/value associations.

**Parameters:**

`map` — Valid live map handle.

**Returns:** The number of key/value associations.

**C++:** `catalyst::NUINT catalyst::map_count(const MAP* map)`.


#### catrl_map_at

```c
void catrl_map_at(
    const CATALYST_MAP* map,
    CATALYST_NUINT index,
    void* key,
    void* value
);
```

Copies the key and value at an unordered enumeration index to their respective output buffers.

**Parameters:**

`map` — Map handle.  
`index` — Zero-based index strictly below the collection count.  
`key` — Writable buffer of at least the configured key size in bytes.  
`value` — Writable buffer of at least the configured value size in bytes.

**C++:** `void catalyst::map_at(const MAP* map, NUINT index, void* key, void* value)`.


#### catrl_map_clear

```c
void catrl_map_clear(
    CATALYST_MAP* map
);
```

Removes all associations without releasing storage capacity or zeroing stored bytes. An empty collection is unchanged.

**Parameters:**

`map` — Map handle.

**C++:** `void catalyst::map_clear(MAP* map)`.


### File

#### catrl_file_create

```c
CATALYST_RESULT catrl_file_create(
    CATALYST_FILE** file,
    void* state,
    CATALYST_FILE_READER reader,
    CATALYST_NUINT length
);
```

Bind a fixed-length persistent byte source. The provider remains caller-owned.

**Parameters:**

`file` — Writable output handle; cleared on failure.  
`state` — Borrowed provider state; may be null.  
`reader` — Non-null persistent byte reader.  
`length` — Fixed byte count; zero is valid.

**Context:** `0x17`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00170200` | SUCCESS | Bind provider | — |
| `0x82170103` | ERROR_INVALID_STATE | Allocate handle | Allocator operation 0x03. |
| `0x90170104` | ERROR_BUFFER_OVERFLOW | Allocate handle | Allocator operation 0x04. |
| `0x90170105` | ERROR_BUFFER_OVERFLOW | Allocate handle | Allocator operation 0x05. |
| `0x91170106` | ERROR_ALLOCATION_FAILED | Allocate handle | Allocator operation 0x06. |

**C++:** `catalyst::RESULT catalyst::file_create(FILE** file, void* state, FILE_READER reader, NUINT length)`.

#### catrl_file_destroy

```c
CATALYST_RESULT catrl_file_destroy(
    CATALYST_FILE* file
);
```

Release the handle, not the underlying persistent source.

**Parameters:**

`file` — Live handle; consumed by this call.

**Context:** `0x18`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00180200` | SUCCESS | Release handle | — |
| `0x82180102` | ERROR_INVALID_STATE | Release handle | Allocator operation 0x02. |

**C++:** `catalyst::RESULT catalyst::file_destroy(FILE* file)`.

#### catrl_file_get

```c
CATALYST_RESULT catrl_file_get(
    const CATALYST_FILE* file,
    CATALYST_NUINT index,
    CATALYST_BYTE* value
);
```

Evaluate the persistent byte source at an index. No cursor is advanced.

**Parameters:**

`file` — Live file handle.  
`index` — Valid zero-based byte index, less than the bound length.  
`value` — Writable byte; assigned on success only.

**Context:** `0x19`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00190300` | SUCCESS | Return byte | — |
| `0x8A1901SS` | ERROR_DEPENDENCY_FAILURE | Read byte | Provider status SS (defined below). |
| `0xSS1902DD` | Provider error or fatal status | Read byte | Provider detail for dependency failure; otherwise zero. See provider result conventions. |

**C++:** `catalyst::RESULT catalyst::file_get(const FILE* file, NUINT index, BYTE* value)`.

#### catrl_file_length

```c
CATALYST_NUINT catrl_file_length(
    const CATALYST_FILE* file
);
```

Return the fixed length of the indexed source in bytes.

**Parameters:**

`file` — Live file handle.

**Returns:** The bound byte count.

**C++:** `catalyst::NUINT catalyst::file_length(const FILE* file)`.

### Stream

#### catrl_stream_create

```c
CATALYST_RESULT catrl_stream_create(
    CATALYST_STREAM** stream,
    void* state,
    CATALYST_STREAM_READER reader
);
```

Bind a sequential byte producer without retrieving a chunk.

**Parameters:**

`stream` — Writable output handle; cleared on failure.  
`state` — Borrowed producer state; may be null.  
`reader` — Non-null sequential chunk producer.

**Context:** `0x1B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001B0200` | SUCCESS | Bind producer | — |
| `0x821B0103` | ERROR_INVALID_STATE | Allocate handle | Allocator operation 0x03. |
| `0x901B0104` | ERROR_BUFFER_OVERFLOW | Allocate handle | Allocator operation 0x04. |
| `0x901B0105` | ERROR_BUFFER_OVERFLOW | Allocate handle | Allocator operation 0x05. |
| `0x911B0106` | ERROR_ALLOCATION_FAILED | Allocate handle | Allocator operation 0x06. |

**C++:** `catalyst::RESULT catalyst::stream_create(STREAM** stream, void* state, STREAM_READER reader)`.

#### catrl_stream_destroy

```c
CATALYST_RESULT catrl_stream_destroy(
    CATALYST_STREAM* stream
);
```

Release the handle without destroying its producer or freeing borrowed chunks.

**Parameters:**

`stream` — Live handle; consumed by this call.

**Context:** `0x1C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001C0200` | SUCCESS | Release handle | — |
| `0x821C0102` | ERROR_INVALID_STATE | Release handle | Allocator operation 0x02. |

**C++:** `catalyst::RESULT catalyst::stream_destroy(STREAM* stream)`.

#### catrl_stream_next

```c
CATALYST_RESULT catrl_stream_next(
    CATALYST_STREAM* stream,
    const CATALYST_BYTE** chunk,
    CATALYST_NUINT* length
);
```

Retrieve exactly the next chunk from the producer. SUCCESS returns a chunk, which may be empty; SUCCESS_NOOP returns null and zero at permanent exhaustion. Later calls after exhaustion do not invoke the producer. On any other result, outputs are unchanged; the producer may have advanced and previously borrowed bytes may no longer be valid.

**Parameters:**

`stream` — Live stream handle.  
`chunk` — Writable output for the borrowed chunk; null at exhaustion.  
`length` — Writable byte count; zero at exhaustion.

**Context:** `0x1D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x011D0100` | SUCCESS_NOOP | Already exhausted | — |
| `0x011D0200` | SUCCESS_NOOP | Producer exhausted | — |
| `0x8A1D03SS` | ERROR_DEPENDENCY_FAILURE | Retrieve chunk | Provider status SS (defined below). |
| `0xSS1D04DD` | Provider error or fatal status | Retrieve chunk | Provider detail for dependency failure; otherwise zero. See provider result conventions. |
| `0xC11D0500` | FATAL_INVARIANT_VIOLATION | Validate chunk | A nonempty chunk has no storage. |
| `0x001D0600` | SUCCESS | Return chunk | — |

**C++:** `catalyst::RESULT catalyst::stream_next(STREAM* stream, const BYTE** chunk, NUINT* length)`.
