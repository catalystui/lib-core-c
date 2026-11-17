# CATRLLIB

Provides fixed-length arrays, sets, maps, and stacks of byte-represented elements.

**C Header:** `catrllib.h`  
**C++ Header:** `catrllib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catrl`  
**Dependencies:** `catmm`

## Conventions

- All functions return void; `result` may be null. Other pointers must be non-null and refer to live handles or valid readable/writable storage.
- Sizes are nonzero byte counts. Lengths, counts and indices count elements or associations; indices start at zero.
- Collections copy exactly the configured number of bytes. Pointers embedded in stored values are shallow copies; referenced resources remain caller-owned. Set and map key equality compares every byte, including padding. Supply consistent byte representations.
- Inputs and outputs must not overlap. Provide buffers of the configured sizes; no typed alignment of internal bytes is promised.
- Creation clears a non-null output handle before validating sizes or allocating. Any existing handle is replaced, not destroyed.
- Query outputs are assigned only on success. Recoverable mutation failures leave logical contents unchanged. Set/map enumeration order is unspecified and can change on mutation.
- Clear retains capacity and does not securely erase bytes. After destroy is attempted, the handle must not be reused, even if cleanup reports failure. Foreign, stale or incorrectly typed handles are outside the contract. Concurrent access involving mutation or destruction requires external synchronization.

On failure, detail may identify an internal helper or CATMMLIB operation. Defensive invariant failures are listed even when unreachable with valid arguments and intact storage.

## Type Summary

| C Type | C++ Type | Description |
| --- | --- | --- |
| [`CATALYST_ARRAY`](#catalyst_array) | `ARRAY` | Fixed-length indexed elements. |
| [`CATALYST_SET`](#catalyst_set) | `SET` | Distinct byte values. |
| [`CATALYST_MAP`](#catalyst_map) | `MAP` | Unique byte keys associated with values. |
| [`CATALYST_STACK`](#catalyst_stack) | `STACK` | Last-in, first-out elements. |

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
| [`catrl_array_length`](#catrl_array_length) | Writes the fixed element count to length. |

### Set

| Function | Description |
| --- | --- |
| [`catrl_set_create`](#catrl_set_create) | Creates an empty set of `element_size`-byte elements. |
| [`catrl_set_destroy`](#catrl_set_destroy) | Releases set storage and handle. |
| [`catrl_set_add`](#catrl_set_add) | Copies value into the set if no byte-identical element exists. |
| [`catrl_set_remove`](#catrl_set_remove) | Removes a byte-identical element. |
| [`catrl_set_contains`](#catrl_set_contains) | Reports whether the set contains the specified value. |
| [`catrl_set_count`](#catrl_set_count) | Writes the number of distinct elements to count. |
| [`catrl_set_at`](#catrl_set_at) | Copies the element at an unordered enumeration index into value. |
| [`catrl_set_clear`](#catrl_set_clear) | Removes all elements without releasing `capacity` or zeroing the stored bytes. |

### Map

| Function | Description |
| --- | --- |
| [`catrl_map_create`](#catrl_map_create) | Creates an empty map with fixed `key_size` and `value_size` bytes per association. |
| [`catrl_map_destroy`](#catrl_map_destroy) | Releases map storage and handle. |
| [`catrl_map_get`](#catrl_map_get) | Retrieves the value associated with a byte-identical key. |
| [`catrl_map_set`](#catrl_map_set) | Inserts a key/value association or updates an existing key without increasing the count. |
| [`catrl_map_remove`](#catrl_map_remove) | Removes an association with a byte-identical key. |
| [`catrl_map_contains`](#catrl_map_contains) | Reports whether the map contains the specified key. |
| [`catrl_map_count`](#catrl_map_count) | Writes the number of key/value associations to count. |
| [`catrl_map_at`](#catrl_map_at) | Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes. |
| [`catrl_map_clear`](#catrl_map_clear) | Removes all associations without releasing `capacity` or zeroing stored bytes. |

### Stack

| Function | Description |
| --- | --- |
| [`catrl_stack_create`](#catrl_stack_create) | Creates an empty stack with `element_size` bytes per element. |
| [`catrl_stack_destroy`](#catrl_stack_destroy) | Releases stack storage and handle. |
| [`catrl_stack_push`](#catrl_stack_push) | Copies value to the top of the stack. |
| [`catrl_stack_pop`](#catrl_stack_pop) | Copies the top element into value and removes it. |
| [`catrl_stack_peek`](#catrl_stack_peek) | Copies the top element into value without removing it. |
| [`catrl_stack_count`](#catrl_stack_count) | Writes the number of elements to count. |
| [`catrl_stack_clear`](#catrl_stack_clear) | Removes all elements without releasing `capacity` or zeroing stored bytes. |

## Type Details

### CATALYST_ARRAY

```c
typedef struct CATALYST_ARRAY CATALYST_ARRAY;
```

Opaque handle to a fixed-length array. Create with [`catrl_array_create`](#catrl_array_create) and release with [`catrl_array_destroy`](#catrl_array_destroy).

**C++:** `catalyst::ARRAY`.

### CATALYST_SET

```c
typedef struct CATALYST_SET CATALYST_SET;
```

Opaque handle to a set of distinct byte values. Create with [`catrl_set_create`](#catrl_set_create) and release with [`catrl_set_destroy`](#catrl_set_destroy).

**C++:** `catalyst::SET`.

### CATALYST_MAP

```c
typedef struct CATALYST_MAP CATALYST_MAP;
```

Opaque handle to a map of unique byte keys and associated values. Create with [`catrl_map_create`](#catrl_map_create) and release with [`catrl_map_destroy`](#catrl_map_destroy).

**C++:** `catalyst::MAP`.

### CATALYST_STACK

```c
typedef struct CATALYST_STACK CATALYST_STACK;
```

Opaque handle to a last-in, first-out stack. Create with [`catrl_stack_create`](#catrl_stack_create) and release with [`catrl_stack_destroy`](#catrl_stack_destroy).

**C++:** `catalyst::STACK`.

## Constant Details

### CATRLLIB_API

`CATRLLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATRLLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATRLLIB_USE_DLL` is defined. Otherwise it is empty.

## Function Details

### Array

#### catrl_array_create

```c
void catrl_array_create(
    CATALYST_ARRAY** array,
    CATALYST_NUINT element_size,
    CATALYST_NUINT length,
    CATALYST_RESULT* result
);
```

Creates a fixed-length array of length elements, each `element_size` bytes. All elements are zero-filled; zero length is valid.

**Parameters:**

`array` — Receives the new array handle; cleared on failure.

`element_size` — Element size in bytes. Must be nonzero.

`length` — Number of elements; may be zero.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Create storage | — |
| `0x81010100` | `ERROR_INVALID_ARGUMENT` | `0x01` (catrl_array_create) | Validate `array` | Argument 0 (`array`) |
| `0x81010201` | `ERROR_INVALID_ARGUMENT` | `0x01` (catrl_array_create) | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82010302` | `ERROR_INVALID_STATE` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x82010303` | `ERROR_INVALID_STATE` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x03`: Release handle during creation cleanup. |
| `0x82010304` | `ERROR_INVALID_STATE` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x90010301` | `ERROR_BUFFER_OVERFLOW` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x90010302` | `ERROR_BUFFER_OVERFLOW` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90010304` | `ERROR_BUFFER_OVERFLOW` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x91010302` | `ERROR_ALLOCATION_FAILED` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91010304` | `ERROR_ALLOCATION_FAILED` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0xC1010302` | `FATAL_INVARIANT_VIOLATION` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1010303` | `FATAL_INVARIANT_VIOLATION` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x03`: Release handle during creation cleanup (internal argument failure). |
| `0xC1010304` | `FATAL_INVARIANT_VIOLATION` | `0x01` (catrl_array_create) | Create storage | Storage helper operation `0x04`: Allocate element storage. |

**C++:** `catalyst::array_create(ARRAY** array, NUINT element_size, NUINT length, RESULT* result)`.


#### catrl_array_destroy

```c
void catrl_array_destroy(
    CATALYST_ARRAY* array,
    CATALYST_RESULT* result
);
```

Releases array storage and handle.

**Parameters:**

`array` — Array handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | — | Destroy storage | — |
| `0x81020100` | `ERROR_INVALID_ARGUMENT` | `0x02` (catrl_array_destroy) | Validate `array` | Argument 0 (`array`) |
| `0x82020201` | `ERROR_INVALID_STATE` | `0x02` (catrl_array_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82020202` | `ERROR_INVALID_STATE` | `0x02` (catrl_array_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1020201` | `FATAL_INVARIANT_VIOLATION` | `0x02` (catrl_array_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1020202` | `FATAL_INVARIANT_VIOLATION` | `0x02` (catrl_array_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |

**C++:** `catalyst::array_destroy(ARRAY* array, RESULT* result)`.


#### catrl_array_get

```c
void catrl_array_get(
    const CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    void* value,
    CATALYST_RESULT* result
);
```

Copies the element at index into value, which must hold `element_size` bytes.

**Parameters:**

`array` — Array handle.

`index` — Zero-based element index.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Retrieve value | — |
| `0x81030100` | `ERROR_INVALID_ARGUMENT` | `0x03` (catrl_array_get) | Validate `array` | Argument 0 (`array`) |
| `0x81030201` | `ERROR_INVALID_ARGUMENT` | `0x03` (catrl_array_get) | Validate `index` | Argument 1 (`index`) |
| `0x81030302` | `ERROR_INVALID_ARGUMENT` | `0x03` (catrl_array_get) | Validate `value` | Argument 2 (`value`) |
| `0xC1030401` | `FATAL_INVARIANT_VIOLATION` | `0x03` (catrl_array_get) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1030402` | `FATAL_INVARIANT_VIOLATION` | `0x03` (catrl_array_get) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::array_get(const ARRAY* array, NUINT index, void* value, RESULT* result)`.


#### catrl_array_set

```c
void catrl_array_set(
    CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    const void* value,
    CATALYST_RESULT* result
);
```

Replaces the element at index with a copy of `element_size` bytes from value.

**Parameters:**

`array` — Array handle.

`index` — Zero-based element index.

`value` — Readable value to copy or match.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Store value | — |
| `0x81040100` | `ERROR_INVALID_ARGUMENT` | `0x04` (catrl_array_set) | Validate `array` | Argument 0 (`array`) |
| `0x81040201` | `ERROR_INVALID_ARGUMENT` | `0x04` (catrl_array_set) | Validate `index` | Argument 1 (`index`) |
| `0x81040302` | `ERROR_INVALID_ARGUMENT` | `0x04` (catrl_array_set) | Validate `value` | Argument 2 (`value`) |
| `0xC1040401` | `FATAL_INVARIANT_VIOLATION` | `0x04` (catrl_array_set) | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1040402` | `FATAL_INVARIANT_VIOLATION` | `0x04` (catrl_array_set) | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::array_set(ARRAY* array, NUINT index, const void* value, RESULT* result)`.


#### catrl_array_length

```c
void catrl_array_length(
    const CATALYST_ARRAY* array,
    CATALYST_NUINT* length,
    CATALYST_RESULT* result
);
```

Writes the fixed element count to length.

**Parameters:**

`array` — Array handle.

`length` — Receives the fixed element count.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Write length | — |
| `0x81050100` | `ERROR_INVALID_ARGUMENT` | `0x05` (catrl_array_length) | Validate `array` | Argument 0 (`array`) |
| `0x81050201` | `ERROR_INVALID_ARGUMENT` | `0x05` (catrl_array_length) | Validate `length` | Argument 1 (`length`) |

**C++:** `catalyst::array_length(const ARRAY* array, NUINT* length, RESULT* result)`.


### Set

#### catrl_set_create

```c
void catrl_set_create(
    CATALYST_SET** set,
    CATALYST_NUINT element_size,
    CATALYST_RESULT* result
);
```

Creates an empty set of `element_size`-byte elements.

**Parameters:**

`set` — Receives the new set handle; cleared on failure.

`element_size` — Element size in bytes. Must be nonzero.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Create storage | — |
| `0x81060100` | `ERROR_INVALID_ARGUMENT` | `0x06` (catrl_set_create) | Validate `set` | Argument 0 (`set`) |
| `0x81060201` | `ERROR_INVALID_ARGUMENT` | `0x06` (catrl_set_create) | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82060302` | `ERROR_INVALID_STATE` | `0x06` (catrl_set_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90060301` | `ERROR_BUFFER_OVERFLOW` | `0x06` (catrl_set_create) | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x90060302` | `ERROR_BUFFER_OVERFLOW` | `0x06` (catrl_set_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91060302` | `ERROR_ALLOCATION_FAILED` | `0x06` (catrl_set_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1060302` | `FATAL_INVARIANT_VIOLATION` | `0x06` (catrl_set_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::set_create(SET** set, NUINT element_size, RESULT* result)`.


#### catrl_set_destroy

```c
void catrl_set_destroy(
    CATALYST_SET* set,
    CATALYST_RESULT* result
);
```

Releases set storage and handle.

**Parameters:**

`set` — Set handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | — | Destroy storage | — |
| `0x81070100` | `ERROR_INVALID_ARGUMENT` | `0x07` (catrl_set_destroy) | Validate `set` | Argument 0 (`set`) |
| `0x82070201` | `ERROR_INVALID_STATE` | `0x07` (catrl_set_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82070202` | `ERROR_INVALID_STATE` | `0x07` (catrl_set_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1070201` | `FATAL_INVARIANT_VIOLATION` | `0x07` (catrl_set_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1070202` | `FATAL_INVARIANT_VIOLATION` | `0x07` (catrl_set_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |

**C++:** `catalyst::set_destroy(SET* set, RESULT* result)`.


#### catrl_set_add

```c
void catrl_set_add(
    CATALYST_SET* set,
    const void* value,
    CATALYST_RESULT* result
);
```

Copies value into the set if no byte-identical element exists. Grows storage as needed.

**Parameters:**

`set` — Set handle.

`value` — Readable value to copy or match.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | — | Insert element | — |
| `0x01000400` | `SUCCESS_NOOP` | — | Skip existing element | — |
| `0x81080100` | `ERROR_INVALID_ARGUMENT` | `0x08` (catrl_set_add) | Validate `set` | Argument 0 (`set`) |
| `0x81080201` | `ERROR_INVALID_ARGUMENT` | `0x08` (catrl_set_add) | Validate `value` | Argument 1 (`value`) |
| `0x82080502` | `ERROR_INVALID_STATE` | `0x08` (catrl_set_add) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90080501` | `ERROR_BUFFER_OVERFLOW` | `0x08` (catrl_set_add) | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90080502` | `ERROR_BUFFER_OVERFLOW` | `0x08` (catrl_set_add) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91080502` | `ERROR_ALLOCATION_FAILED` | `0x08` (catrl_set_add) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1080301` | `FATAL_INVARIANT_VIOLATION` | `0x08` (catrl_set_add) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1080502` | `FATAL_INVARIANT_VIOLATION` | `0x08` (catrl_set_add) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1080601` | `FATAL_INVARIANT_VIOLATION` | `0x08` (catrl_set_add) | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1080602` | `FATAL_INVARIANT_VIOLATION` | `0x08` (catrl_set_add) | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::set_add(SET* set, const void* value, RESULT* result)`.


#### catrl_set_remove

```c
void catrl_set_remove(
    CATALYST_SET* set,
    const void* value,
    CATALYST_RESULT* result
);
```

Removes a byte-identical element. Enumeration order may change.

**Parameters:**

`set` — Set handle.

`value` — Readable value to copy or match.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000600` | `SUCCESS` | — | Remove entry | — |
| `0x01000400` | `SUCCESS_NOOP` | — | Skip absent entry | — |
| `0x81090100` | `ERROR_INVALID_ARGUMENT` | `0x09` (catrl_set_remove) | Validate `set` | Argument 0 (`set`) |
| `0x81090201` | `ERROR_INVALID_ARGUMENT` | `0x09` (catrl_set_remove) | Validate `value` | Argument 1 (`value`) |
| `0xC1090301` | `FATAL_INVARIANT_VIOLATION` | `0x09` (catrl_set_remove) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1090501` | `FATAL_INVARIANT_VIOLATION` | `0x09` (catrl_set_remove) | Move last entry into removed slot | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1090502` | `FATAL_INVARIANT_VIOLATION` | `0x09` (catrl_set_remove) | Move last entry into removed slot | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::set_remove(SET* set, const void* value, RESULT* result)`.


#### catrl_set_contains

```c
void catrl_set_contains(
    const CATALYST_SET* set,
    const void* value,
    CATALYST_BOOL* contains,
    CATALYST_RESULT* result
);
```

Reports whether the set contains the specified value.

**Parameters:**

`set` — Set handle.

`value` — Readable value to copy or match.

`contains` — Receives `CATALYST_TRUE` or `CATALYST_FALSE`.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Write membership result | — |
| `0x810A0100` | `ERROR_INVALID_ARGUMENT` | `0x0A` (catrl_set_contains) | Validate `set` | Argument 0 (`set`) |
| `0x810A0201` | `ERROR_INVALID_ARGUMENT` | `0x0A` (catrl_set_contains) | Validate `value` | Argument 1 (`value`) |
| `0x810A0302` | `ERROR_INVALID_ARGUMENT` | `0x0A` (catrl_set_contains) | Validate `contains` | Argument 2 (`contains`) |
| `0xC10A0401` | `FATAL_INVARIANT_VIOLATION` | `0x0A` (catrl_set_contains) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |

**C++:** `catalyst::set_contains(const SET* set, const void* value, BOOL* contains, RESULT* result)`.


#### catrl_set_count

```c
void catrl_set_count(
    const CATALYST_SET* set,
    CATALYST_NUINT* count,
    CATALYST_RESULT* result
);
```

Writes the number of distinct elements to count.

**Parameters:**

`set` — Set handle.

`count` — Receives the number of elements.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Write count | — |
| `0x810B0100` | `ERROR_INVALID_ARGUMENT` | `0x0B` (catrl_set_count) | Validate `set` | Argument 0 (`set`) |
| `0x810B0201` | `ERROR_INVALID_ARGUMENT` | `0x0B` (catrl_set_count) | Validate `count` | Argument 1 (`count`) |

**C++:** `catalyst::set_count(const SET* set, NUINT* count, RESULT* result)`.


#### catrl_set_at

```c
void catrl_set_at(
    const CATALYST_SET* set,
    CATALYST_NUINT index,
    void* value,
    CATALYST_RESULT* result
);
```

Copies the element at an unordered enumeration index into value.

**Parameters:**

`set` — Set handle.

`index` — Zero-based enumeration index.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Retrieve entry | — |
| `0x810C0100` | `ERROR_INVALID_ARGUMENT` | `0x0C` (catrl_set_at) | Validate `set` | Argument 0 (`set`) |
| `0x810C0201` | `ERROR_INVALID_ARGUMENT` | `0x0C` (catrl_set_at) | Validate `index` | Argument 1 (`index`) |
| `0x810C0302` | `ERROR_INVALID_ARGUMENT` | `0x0C` (catrl_set_at) | Validate `value` | Argument 2 (`value`) |
| `0xC10C0401` | `FATAL_INVARIANT_VIOLATION` | `0x0C` (catrl_set_at) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC10C0402` | `FATAL_INVARIANT_VIOLATION` | `0x0C` (catrl_set_at) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::set_at(const SET* set, NUINT index, void* value, RESULT* result)`.


#### catrl_set_clear

```c
void catrl_set_clear(
    CATALYST_SET* set,
    CATALYST_RESULT* result
);
```

Removes all elements without releasing `capacity` or zeroing the stored bytes.

**Parameters:**

`set` — Set handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Clear collection | — |
| `0x01000200` | `SUCCESS_NOOP` | — | Skip empty collection | — |
| `0x810D0100` | `ERROR_INVALID_ARGUMENT` | `0x0D` (catrl_set_clear) | Validate `set` | Argument 0 (`set`) |

**C++:** `catalyst::set_clear(SET* set, RESULT* result)`.


### Map

#### catrl_map_create

```c
void catrl_map_create(
    CATALYST_MAP** map,
    CATALYST_NUINT key_size,
    CATALYST_NUINT value_size,
    CATALYST_RESULT* result
);
```

Creates an empty map with fixed `key_size` and `value_size` bytes per association.

**Parameters:**

`map` — Receives the new map handle; cleared on failure.

`key_size` — Key size in bytes. Must be nonzero.

`value_size` — Value size in bytes. Must be nonzero.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Create storage | — |
| `0x810E0100` | `ERROR_INVALID_ARGUMENT` | `0x0E` (catrl_map_create) | Validate `map` | Argument 0 (`map`) |
| `0x810E0201` | `ERROR_INVALID_ARGUMENT` | `0x0E` (catrl_map_create) | Validate `key_size` | Argument 1 (`key_size`) |
| `0x810E0302` | `ERROR_INVALID_ARGUMENT` | `0x0E` (catrl_map_create) | Validate `value_size` | Argument 2 (`value_size`) |
| `0x820E0502` | `ERROR_INVALID_STATE` | `0x0E` (catrl_map_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x900E0400` | `ERROR_BUFFER_OVERFLOW` | `0x0E` (catrl_map_create) | Check combined key and value size | — |
| `0x900E0501` | `ERROR_BUFFER_OVERFLOW` | `0x0E` (catrl_map_create) | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x900E0502` | `ERROR_BUFFER_OVERFLOW` | `0x0E` (catrl_map_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x910E0502` | `ERROR_ALLOCATION_FAILED` | `0x0E` (catrl_map_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC10E0502` | `FATAL_INVARIANT_VIOLATION` | `0x0E` (catrl_map_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::map_create(MAP** map, NUINT key_size, NUINT value_size, RESULT* result)`.


#### catrl_map_destroy

```c
void catrl_map_destroy(
    CATALYST_MAP* map,
    CATALYST_RESULT* result
);
```

Releases map storage and handle.

**Parameters:**

`map` — Map handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | — | Destroy storage | — |
| `0x810F0100` | `ERROR_INVALID_ARGUMENT` | `0x0F` (catrl_map_destroy) | Validate `map` | Argument 0 (`map`) |
| `0x820F0201` | `ERROR_INVALID_STATE` | `0x0F` (catrl_map_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x820F0202` | `ERROR_INVALID_STATE` | `0x0F` (catrl_map_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC10F0201` | `FATAL_INVARIANT_VIOLATION` | `0x0F` (catrl_map_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC10F0202` | `FATAL_INVARIANT_VIOLATION` | `0x0F` (catrl_map_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |

**C++:** `catalyst::map_destroy(MAP* map, RESULT* result)`.


#### catrl_map_get

```c
void catrl_map_get(
    const CATALYST_MAP* map,
    const void* key,
    void* value,
    CATALYST_RESULT* result
);
```

Retrieves the value associated with a byte-identical key.

**Parameters:**

`map` — Map handle.

`key` — Readable key of the configured key size.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | — | Retrieve value | — |
| `0x81100100` | `ERROR_INVALID_ARGUMENT` | `0x10` (catrl_map_get) | Validate `map` | Argument 0 (`map`) |
| `0x81100201` | `ERROR_INVALID_ARGUMENT` | `0x10` (catrl_map_get) | Validate `key` | Argument 1 (`key`) |
| `0x81100302` | `ERROR_INVALID_ARGUMENT` | `0x10` (catrl_map_get) | Validate `value` | Argument 2 (`value`) |
| `0x88100500` | `ERROR_NOT_FOUND` | `0x10` (catrl_map_get) | Locate key | — |
| `0xC1100401` | `FATAL_INVARIANT_VIOLATION` | `0x10` (catrl_map_get) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1100601` | `FATAL_INVARIANT_VIOLATION` | `0x10` (catrl_map_get) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1100602` | `FATAL_INVARIANT_VIOLATION` | `0x10` (catrl_map_get) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::map_get(const MAP* map, const void* key, void* value, RESULT* result)`.


#### catrl_map_set

```c
void catrl_map_set(
    CATALYST_MAP* map,
    const void* key,
    const void* value,
    CATALYST_RESULT* result
);
```

Inserts a key/value association or updates an existing key without increasing the count. Grows storage for new keys as needed.

**Parameters:**

`map` — Map handle.

`key` — Readable key of the configured key size.

`value` — Readable value to copy or match.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000800` | `SUCCESS` | — | Store value | — |
| `0x81110100` | `ERROR_INVALID_ARGUMENT` | `0x11` (catrl_map_set) | Validate `map` | Argument 0 (`map`) |
| `0x81110201` | `ERROR_INVALID_ARGUMENT` | `0x11` (catrl_map_set) | Validate `key` | Argument 1 (`key`) |
| `0x81110302` | `ERROR_INVALID_ARGUMENT` | `0x11` (catrl_map_set) | Validate `value` | Argument 2 (`value`) |
| `0x82110502` | `ERROR_INVALID_STATE` | `0x11` (catrl_map_set) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90110501` | `ERROR_BUFFER_OVERFLOW` | `0x11` (catrl_map_set) | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90110502` | `ERROR_BUFFER_OVERFLOW` | `0x11` (catrl_map_set) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91110502` | `ERROR_ALLOCATION_FAILED` | `0x11` (catrl_map_set) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1110401` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1110502` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1110601` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Copy new key | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1110602` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Copy new key | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1110701` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Copy value | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1110702` | `FATAL_INVARIANT_VIOLATION` | `0x11` (catrl_map_set) | Copy value | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::map_set(MAP* map, const void* key, const void* value, RESULT* result)`.


#### catrl_map_remove

```c
void catrl_map_remove(
    CATALYST_MAP* map,
    const void* key,
    CATALYST_RESULT* result
);
```

Removes an association with a byte-identical key. Enumeration order may change.

**Parameters:**

`map` — Map handle.

`key` — Readable key of the configured key size.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000600` | `SUCCESS` | — | Remove entry | — |
| `0x01000400` | `SUCCESS_NOOP` | — | Skip absent entry | — |
| `0x81120100` | `ERROR_INVALID_ARGUMENT` | `0x12` (catrl_map_remove) | Validate `map` | Argument 0 (`map`) |
| `0x81120201` | `ERROR_INVALID_ARGUMENT` | `0x12` (catrl_map_remove) | Validate `key` | Argument 1 (`key`) |
| `0xC1120301` | `FATAL_INVARIANT_VIOLATION` | `0x12` (catrl_map_remove) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1120501` | `FATAL_INVARIANT_VIOLATION` | `0x12` (catrl_map_remove) | Move last entry into removed slot | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1120502` | `FATAL_INVARIANT_VIOLATION` | `0x12` (catrl_map_remove) | Move last entry into removed slot | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::map_remove(MAP* map, const void* key, RESULT* result)`.


#### catrl_map_contains

```c
void catrl_map_contains(
    const CATALYST_MAP* map,
    const void* key,
    CATALYST_BOOL* contains,
    CATALYST_RESULT* result
);
```

Reports whether the map contains the specified key.

**Parameters:**

`map` — Map handle.

`key` — Readable key of the configured key size.

`contains` — Receives `CATALYST_TRUE` or `CATALYST_FALSE`.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Write membership result | — |
| `0x81130100` | `ERROR_INVALID_ARGUMENT` | `0x13` (catrl_map_contains) | Validate `map` | Argument 0 (`map`) |
| `0x81130201` | `ERROR_INVALID_ARGUMENT` | `0x13` (catrl_map_contains) | Validate `key` | Argument 1 (`key`) |
| `0x81130302` | `ERROR_INVALID_ARGUMENT` | `0x13` (catrl_map_contains) | Validate `contains` | Argument 2 (`contains`) |
| `0xC1130401` | `FATAL_INVARIANT_VIOLATION` | `0x13` (catrl_map_contains) | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |

**C++:** `catalyst::map_contains(const MAP* map, const void* key, BOOL* contains, RESULT* result)`.


#### catrl_map_count

```c
void catrl_map_count(
    const CATALYST_MAP* map,
    CATALYST_NUINT* count,
    CATALYST_RESULT* result
);
```

Writes the number of key/value associations to count.

**Parameters:**

`map` — Map handle.

`count` — Receives the number of elements.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Write count | — |
| `0x81140100` | `ERROR_INVALID_ARGUMENT` | `0x14` (catrl_map_count) | Validate `map` | Argument 0 (`map`) |
| `0x81140201` | `ERROR_INVALID_ARGUMENT` | `0x14` (catrl_map_count) | Validate `count` | Argument 1 (`count`) |

**C++:** `catalyst::map_count(const MAP* map, NUINT* count, RESULT* result)`.


#### catrl_map_at

```c
void catrl_map_at(
    const CATALYST_MAP* map,
    CATALYST_NUINT index,
    void* key,
    void* value,
    CATALYST_RESULT* result
);
```

Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes.

**Parameters:**

`map` — Map handle.

`index` — Zero-based enumeration index.

`key` — Writable buffer receiving the key.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000700` | `SUCCESS` | — | Retrieve entry | — |
| `0x81150100` | `ERROR_INVALID_ARGUMENT` | `0x15` (catrl_map_at) | Validate `map` | Argument 0 (`map`) |
| `0x81150201` | `ERROR_INVALID_ARGUMENT` | `0x15` (catrl_map_at) | Validate `index` | Argument 1 (`index`) |
| `0x81150302` | `ERROR_INVALID_ARGUMENT` | `0x15` (catrl_map_at) | Validate `key` | Argument 2 (`key`) |
| `0x81150403` | `ERROR_INVALID_ARGUMENT` | `0x15` (catrl_map_at) | Validate `value` | Argument 3 (`value`) |
| `0xC1150501` | `FATAL_INVARIANT_VIOLATION` | `0x15` (catrl_map_at) | Copy key to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1150502` | `FATAL_INVARIANT_VIOLATION` | `0x15` (catrl_map_at) | Copy key to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1150601` | `FATAL_INVARIANT_VIOLATION` | `0x15` (catrl_map_at) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1150602` | `FATAL_INVARIANT_VIOLATION` | `0x15` (catrl_map_at) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::map_at(const MAP* map, NUINT index, void* key, void* value, RESULT* result)`.


#### catrl_map_clear

```c
void catrl_map_clear(
    CATALYST_MAP* map,
    CATALYST_RESULT* result
);
```

Removes all associations without releasing `capacity` or zeroing stored bytes.

**Parameters:**

`map` — Map handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Clear collection | — |
| `0x01000200` | `SUCCESS_NOOP` | — | Skip empty collection | — |
| `0x81160100` | `ERROR_INVALID_ARGUMENT` | `0x16` (catrl_map_clear) | Validate `map` | Argument 0 (`map`) |

**C++:** `catalyst::map_clear(MAP* map, RESULT* result)`.


### Stack

#### catrl_stack_create

```c
void catrl_stack_create(
    CATALYST_STACK** stack,
    CATALYST_NUINT element_size,
    CATALYST_RESULT* result
);
```

Creates an empty stack with `element_size` bytes per element.

**Parameters:**

`stack` — Receives the new stack handle; cleared on failure.

`element_size` — Element size in bytes. Must be nonzero.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Create storage | — |
| `0x81170100` | `ERROR_INVALID_ARGUMENT` | `0x17` (catrl_stack_create) | Validate `stack` | Argument 0 (`stack`) |
| `0x81170201` | `ERROR_INVALID_ARGUMENT` | `0x17` (catrl_stack_create) | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82170302` | `ERROR_INVALID_STATE` | `0x17` (catrl_stack_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90170301` | `ERROR_BUFFER_OVERFLOW` | `0x17` (catrl_stack_create) | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x90170302` | `ERROR_BUFFER_OVERFLOW` | `0x17` (catrl_stack_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91170302` | `ERROR_ALLOCATION_FAILED` | `0x17` (catrl_stack_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1170302` | `FATAL_INVARIANT_VIOLATION` | `0x17` (catrl_stack_create) | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::stack_create(STACK** stack, NUINT element_size, RESULT* result)`.


#### catrl_stack_destroy

```c
void catrl_stack_destroy(
    CATALYST_STACK* stack,
    CATALYST_RESULT* result
);
```

Releases stack storage and handle.

**Parameters:**

`stack` — Stack handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000200` | `SUCCESS` | — | Destroy storage | — |
| `0x81180100` | `ERROR_INVALID_ARGUMENT` | `0x18` (catrl_stack_destroy) | Validate `stack` | Argument 0 (`stack`) |
| `0x82180201` | `ERROR_INVALID_STATE` | `0x18` (catrl_stack_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82180202` | `ERROR_INVALID_STATE` | `0x18` (catrl_stack_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1180201` | `FATAL_INVARIANT_VIOLATION` | `0x18` (catrl_stack_destroy) | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1180202` | `FATAL_INVARIANT_VIOLATION` | `0x18` (catrl_stack_destroy) | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |

**C++:** `catalyst::stack_destroy(STACK* stack, RESULT* result)`.


#### catrl_stack_push

```c
void catrl_stack_push(
    CATALYST_STACK* stack,
    const void* value,
    CATALYST_RESULT* result
);
```

Copies value to the top of the stack. Grows storage as needed.

**Parameters:**

`stack` — Stack handle.

`value` — Readable value to copy or match.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Push value | — |
| `0x81190100` | `ERROR_INVALID_ARGUMENT` | `0x19` (catrl_stack_push) | Validate `stack` | Argument 0 (`stack`) |
| `0x81190201` | `ERROR_INVALID_ARGUMENT` | `0x19` (catrl_stack_push) | Validate `value` | Argument 1 (`value`) |
| `0x82190302` | `ERROR_INVALID_STATE` | `0x19` (catrl_stack_push) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90190301` | `ERROR_BUFFER_OVERFLOW` | `0x19` (catrl_stack_push) | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90190302` | `ERROR_BUFFER_OVERFLOW` | `0x19` (catrl_stack_push) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91190302` | `ERROR_ALLOCATION_FAILED` | `0x19` (catrl_stack_push) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1190302` | `FATAL_INVARIANT_VIOLATION` | `0x19` (catrl_stack_push) | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1190401` | `FATAL_INVARIANT_VIOLATION` | `0x19` (catrl_stack_push) | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1190402` | `FATAL_INVARIANT_VIOLATION` | `0x19` (catrl_stack_push) | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::stack_push(STACK* stack, const void* value, RESULT* result)`.


#### catrl_stack_pop

```c
void catrl_stack_pop(
    CATALYST_STACK* stack,
    void* value,
    CATALYST_RESULT* result
);
```

Copies the top element into value and removes it.

**Parameters:**

`stack` — Stack handle.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Remove top element | — |
| `0x811A0100` | `ERROR_INVALID_ARGUMENT` | `0x1A` (catrl_stack_pop) | Validate `stack` | Argument 0 (`stack`) |
| `0x811A0201` | `ERROR_INVALID_ARGUMENT` | `0x1A` (catrl_stack_pop) | Validate `value` | Argument 1 (`value`) |
| `0x881A0300` | `ERROR_NOT_FOUND` | `0x1A` (catrl_stack_pop) | Check for top element | — |
| `0xC11A0401` | `FATAL_INVARIANT_VIOLATION` | `0x1A` (catrl_stack_pop) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC11A0402` | `FATAL_INVARIANT_VIOLATION` | `0x1A` (catrl_stack_pop) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::stack_pop(STACK* stack, void* value, RESULT* result)`.


#### catrl_stack_peek

```c
void catrl_stack_peek(
    const CATALYST_STACK* stack,
    void* value,
    CATALYST_RESULT* result
);
```

Copies the top element into value without removing it.

**Parameters:**

`stack` — Stack handle.

`value` — Writable buffer receiving the value.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000500` | `SUCCESS` | — | Retrieve top element | — |
| `0x811B0100` | `ERROR_INVALID_ARGUMENT` | `0x1B` (catrl_stack_peek) | Validate `stack` | Argument 0 (`stack`) |
| `0x811B0201` | `ERROR_INVALID_ARGUMENT` | `0x1B` (catrl_stack_peek) | Validate `value` | Argument 1 (`value`) |
| `0x881B0300` | `ERROR_NOT_FOUND` | `0x1B` (catrl_stack_peek) | Check for top element | — |
| `0xC11B0401` | `FATAL_INVARIANT_VIOLATION` | `0x1B` (catrl_stack_peek) | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC11B0402` | `FATAL_INVARIANT_VIOLATION` | `0x1B` (catrl_stack_peek) | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |

**C++:** `catalyst::stack_peek(const STACK* stack, void* value, RESULT* result)`.


#### catrl_stack_count

```c
void catrl_stack_count(
    const CATALYST_STACK* stack,
    CATALYST_NUINT* count,
    CATALYST_RESULT* result
);
```

Writes the number of elements to count.

**Parameters:**

`stack` — Stack handle.

`count` — Receives the number of elements.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Write count | — |
| `0x811C0100` | `ERROR_INVALID_ARGUMENT` | `0x1C` (catrl_stack_count) | Validate `stack` | Argument 0 (`stack`) |
| `0x811C0201` | `ERROR_INVALID_ARGUMENT` | `0x1C` (catrl_stack_count) | Validate `count` | Argument 1 (`count`) |

**C++:** `catalyst::stack_count(const STACK* stack, NUINT* count, RESULT* result)`.


#### catrl_stack_clear

```c
void catrl_stack_clear(
    CATALYST_STACK* stack,
    CATALYST_RESULT* result
);
```

Removes all elements without releasing `capacity` or zeroing stored bytes.

**Parameters:**

`stack` — Stack handle.

`result` — Optional result output.

**Results:**

| Result | Status | Context | Operation | Detail |
| --- | --- | --- | --- | --- |
| `0x00000300` | `SUCCESS` | — | Clear collection | — |
| `0x01000200` | `SUCCESS_NOOP` | — | Skip empty collection | — |
| `0x811D0100` | `ERROR_INVALID_ARGUMENT` | `0x1D` (catrl_stack_clear) | Validate `stack` | Argument 0 (`stack`) |

**C++:** `catalyst::stack_clear(STACK* stack, RESULT* result)`.
