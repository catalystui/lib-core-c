# CATRLLIB

Provides fixed-length arrays, sets, maps, and stacks of byte-represented elements.

**C Header:** `catrllib.h`  
**C++ Header:** `catrllib.hpp`  
**Namespace:** `catalyst`  
**Library:** `catrl`  
**Dependencies:** `catmm`

## Conventions

- All functions return `CATALYST_RESULT` by value. Pointers must be non-null and refer to live handles or valid readable/writable storage.
- Sizes are nonzero byte counts. Lengths, counts and indices count elements or associations; indices start at zero.
- Collections copy exactly the configured number of bytes. Pointers embedded in stored values are shallow copies; referenced resources remain caller-owned. Set and map key equality compares every byte, including padding. Supply consistent byte representations.
- Inputs and outputs must not overlap. Provide buffers of the configured sizes; no typed alignment of internal bytes is promised.
- Creation clears a non-null output handle before validating sizes or allocating. Any existing handle is replaced, not destroyed.
- Query outputs are unchanged on recoverable failure and may be unspecified after fatal failure. Recoverable mutation failures leave logical contents unchanged. Set/map enumeration order is unspecified and can change on mutation.
- Clear retains capacity and does not securely erase bytes. After destroy is attempted, the handle must not be reused, even if cleanup reports failure. Foreign, stale or incorrectly typed handles are outside the contract. Concurrent access involving mutation or destruction requires external synchronization.

On failure, detail may identify an internal helper or CATMMLIB operation. Defensive invariant failures are listed even when unreachable with valid arguments and intact storage. Fatal results indicate that the affected collection or outputs cannot safely be assumed usable.

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
CATALYST_RESULT catrl_array_create(
    CATALYST_ARRAY** array,
    CATALYST_NUINT element_size,
    CATALYST_NUINT length
);
```

Creates a fixed-length array of length elements, each `element_size` bytes. All elements are zero-filled; zero length is valid.

**Parameters:**

`array` — Receives the new array handle; cleared on failure.  
`element_size` — Element size in bytes. Must be nonzero.  
`length` — Number of elements; may be zero.

**Context:** `0x01`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00010300` | `SUCCESS` | Create storage | — |
| `0x81010100` | `ERROR_INVALID_ARGUMENT` | Validate `array` | Argument 0 (`array`) |
| `0x81010201` | `ERROR_INVALID_ARGUMENT` | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82010302` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x82010303` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x03`: Release handle during creation cleanup. |
| `0x82010304` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x90010301` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x01`: Storage extent exceeds representable range. |
| `0x90010302` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90010304` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0x91010302` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91010304` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x04`: Allocate element storage. |
| `0xC1010302` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1010303` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x03`: Release handle during creation cleanup (internal argument failure). |
| `0xC1010304` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x04`: Allocate element storage. |

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
| `0x81020100` | `ERROR_INVALID_ARGUMENT` | Validate `array` | Argument 0 (`array`) |
| `0x82020201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82020202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1020201` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1020202` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |
| `0xC1020301` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1020302` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1020303` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1020304` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1020306` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Fixed array count differs from capacity. |

**C++:** `catalyst::RESULT catalyst::array_destroy(ARRAY* array)`.


#### catrl_array_get

```c
CATALYST_RESULT catrl_array_get(
    const CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    void* value
);
```

Copies the element at index into value, which must hold `element_size` bytes.

**Parameters:**

`array` — Array handle.  
`index` — Zero-based element index.  
`value` — Writable buffer receiving the value.

**Context:** `0x03`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00030500` | `SUCCESS` | Retrieve value | — |
| `0x81030100` | `ERROR_INVALID_ARGUMENT` | Validate `array` | Argument 0 (`array`) |
| `0x81030201` | `ERROR_INVALID_ARGUMENT` | Validate `index` | Argument 1 (`index`) |
| `0x81030302` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 2 (`value`) |
| `0xC1030401` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1030402` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1030601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1030602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1030603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1030604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1030606` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Fixed array count differs from capacity. |

**C++:** `catalyst::RESULT catalyst::array_get(const ARRAY* array, NUINT index, void* value)`.


#### catrl_array_set

```c
CATALYST_RESULT catrl_array_set(
    CATALYST_ARRAY* array,
    CATALYST_NUINT index,
    const void* value
);
```

Replaces the element at index with a copy of `element_size` bytes from value.

**Parameters:**

`array` — Array handle.  
`index` — Zero-based element index.  
`value` — Readable value to copy or match.

**Context:** `0x04`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00040500` | `SUCCESS` | Store value | — |
| `0x81040100` | `ERROR_INVALID_ARGUMENT` | Validate `array` | Argument 0 (`array`) |
| `0x81040201` | `ERROR_INVALID_ARGUMENT` | Validate `index` | Argument 1 (`index`) |
| `0x81040302` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 2 (`value`) |
| `0xC1040401` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1040402` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1040601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1040602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1040603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1040604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1040606` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Fixed array count differs from capacity. |

**C++:** `catalyst::RESULT catalyst::array_set(ARRAY* array, NUINT index, const void* value)`.


#### catrl_array_length

```c
CATALYST_RESULT catrl_array_length(
    const CATALYST_ARRAY* array,
    CATALYST_NUINT* length
);
```

Writes the fixed element count to length.

**Parameters:**

`array` — Array handle.  
`length` — Receives the fixed element count.

**Context:** `0x05`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00050300` | `SUCCESS` | Write length | — |
| `0x81050100` | `ERROR_INVALID_ARGUMENT` | Validate `array` | Argument 0 (`array`) |
| `0x81050201` | `ERROR_INVALID_ARGUMENT` | Validate `length` | Argument 1 (`length`) |
| `0xC1050401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1050402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1050403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1050404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1050406` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Fixed array count differs from capacity. |

**C++:** `catalyst::RESULT catalyst::array_length(const ARRAY* array, NUINT* length)`.


### Set

#### catrl_set_create

```c
CATALYST_RESULT catrl_set_create(
    CATALYST_SET** set,
    CATALYST_NUINT element_size
);
```

Creates an empty set of `element_size`-byte elements.

**Parameters:**

`set` — Receives the new set handle; cleared on failure.  
`element_size` — Element size in bytes. Must be nonzero.

**Context:** `0x06`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00060300` | `SUCCESS` | Create storage | — |
| `0x81060100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x81060201` | `ERROR_INVALID_ARGUMENT` | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82060302` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90060302` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91060302` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1060302` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

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
| `0x81070100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x82070201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82070202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1070201` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1070202` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |
| `0xC1070301` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1070302` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1070303` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1070304` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_destroy(SET* set)`.


#### catrl_set_add

```c
CATALYST_RESULT catrl_set_add(
    CATALYST_SET* set,
    const void* value
);
```

Copies value into the set if no byte-identical element exists. Grows storage as needed.

**Parameters:**

`set` — Set handle.  
`value` — Readable value to copy or match.

**Context:** `0x08`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00080700` | `SUCCESS` | Insert element | — |
| `0x01080400` | `SUCCESS_NOOP` | Skip existing element | — |
| `0x81080100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x81080201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0x82080502` | `ERROR_INVALID_STATE` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90080501` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90080502` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91080502` | `ERROR_ALLOCATION_FAILED` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1080301` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1080502` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1080601` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1080602` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1080801` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1080802` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1080803` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1080804` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1080504` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x04`: Calculated capacity or extent violated growth bounds. |

**C++:** `catalyst::RESULT catalyst::set_add(SET* set, const void* value)`.


#### catrl_set_remove

```c
CATALYST_RESULT catrl_set_remove(
    CATALYST_SET* set,
    const void* value
);
```

Removes a byte-identical element. Enumeration order may change.

**Parameters:**

`set` — Set handle.  
`value` — Readable value to copy or match.

**Context:** `0x09`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00090600` | `SUCCESS` | Remove entry | — |
| `0x01090400` | `SUCCESS_NOOP` | Skip absent entry | — |
| `0x81090100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x81090201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0xC1090301` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1090501` | `FATAL_INVARIANT_VIOLATION` | Move last entry into removed slot | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1090502` | `FATAL_INVARIANT_VIOLATION` | Move last entry into removed slot | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1090701` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1090702` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1090703` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1090704` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_remove(SET* set, const void* value)`.


#### catrl_set_contains

```c
CATALYST_RESULT catrl_set_contains(
    const CATALYST_SET* set,
    const void* value,
    CATALYST_BOOL* contains
);
```

Reports whether the set contains the specified value.

**Parameters:**

`set` — Set handle.  
`value` — Readable value to copy or match.  
`contains` — Receives `CATALYST_TRUE` or `CATALYST_FALSE`.

**Context:** `0x0A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000A0500` | `SUCCESS` | Write membership result | — |
| `0x810A0100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x810A0201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0x810A0302` | `ERROR_INVALID_ARGUMENT` | Validate `contains` | Argument 2 (`contains`) |
| `0xC10A0401` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC10A0601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC10A0602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC10A0603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC10A0604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_contains(const SET* set, const void* value, BOOL* contains)`.


#### catrl_set_count

```c
CATALYST_RESULT catrl_set_count(
    const CATALYST_SET* set,
    CATALYST_NUINT* count
);
```

Writes the number of distinct elements to count.

**Parameters:**

`set` — Set handle.  
`count` — Receives the number of elements.

**Context:** `0x0B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000B0300` | `SUCCESS` | Write count | — |
| `0x810B0100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x810B0201` | `ERROR_INVALID_ARGUMENT` | Validate `count` | Argument 1 (`count`) |
| `0xC10B0401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC10B0402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC10B0403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC10B0404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_count(const SET* set, NUINT* count)`.


#### catrl_set_at

```c
CATALYST_RESULT catrl_set_at(
    const CATALYST_SET* set,
    CATALYST_NUINT index,
    void* value
);
```

Copies the element at an unordered enumeration index into value.

**Parameters:**

`set` — Set handle.  
`index` — Zero-based enumeration index.  
`value` — Writable buffer receiving the value.

**Context:** `0x0C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000C0500` | `SUCCESS` | Retrieve entry | — |
| `0x810C0100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0x810C0201` | `ERROR_INVALID_ARGUMENT` | Validate `index` | Argument 1 (`index`) |
| `0x810C0302` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 2 (`value`) |
| `0xC10C0401` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC10C0402` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC10C0601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC10C0602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC10C0603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC10C0604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_at(const SET* set, NUINT index, void* value)`.


#### catrl_set_clear

```c
CATALYST_RESULT catrl_set_clear(
    CATALYST_SET* set
);
```

Removes all elements without releasing `capacity` or zeroing the stored bytes.

**Parameters:**

`set` — Set handle.

**Context:** `0x0D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000D0300` | `SUCCESS` | Clear collection | — |
| `0x010D0200` | `SUCCESS_NOOP` | Skip empty collection | — |
| `0x810D0100` | `ERROR_INVALID_ARGUMENT` | Validate `set` | Argument 0 (`set`) |
| `0xC10D0401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC10D0402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC10D0403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC10D0404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::set_clear(SET* set)`.


### Map

#### catrl_map_create

```c
CATALYST_RESULT catrl_map_create(
    CATALYST_MAP** map,
    CATALYST_NUINT key_size,
    CATALYST_NUINT value_size
);
```

Creates an empty map with fixed `key_size` and `value_size` bytes per association.

**Parameters:**

`map` — Receives the new map handle; cleared on failure.  
`key_size` — Key size in bytes. Must be nonzero.  
`value_size` — Value size in bytes. Must be nonzero.

**Context:** `0x0E`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x000E0500` | `SUCCESS` | Create storage | — |
| `0x810E0100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x810E0201` | `ERROR_INVALID_ARGUMENT` | Validate `key_size` | Argument 1 (`key_size`) |
| `0x810E0302` | `ERROR_INVALID_ARGUMENT` | Validate `value_size` | Argument 2 (`value_size`) |
| `0x820E0502` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x900E0400` | `ERROR_BUFFER_OVERFLOW` | Check combined key and value size | — |
| `0x900E0502` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x910E0502` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC10E0502` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

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
| `0x810F0100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x820F0201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x820F0202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC10F0201` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC10F0202` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |
| `0xC10F0301` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC10F0302` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC10F0303` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC10F0304` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC10F0305` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_destroy(MAP* map)`.


#### catrl_map_get

```c
CATALYST_RESULT catrl_map_get(
    const CATALYST_MAP* map,
    const void* key,
    void* value
);
```

Retrieves the value associated with a byte-identical key.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.  
`value` — Writable buffer receiving the value.

**Context:** `0x10`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00100700` | `SUCCESS` | Retrieve value | — |
| `0x81100100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81100201` | `ERROR_INVALID_ARGUMENT` | Validate `key` | Argument 1 (`key`) |
| `0x81100302` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 2 (`value`) |
| `0x88100500` | `ERROR_NOT_FOUND` | Locate key | — |
| `0xC1100401` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1100601` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1100602` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1100801` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1100802` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1100803` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1100804` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1100805` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

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
`value` — Readable value to copy or match.

**Context:** `0x11`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00110800` | `SUCCESS` | Store value | — |
| `0x81110100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81110201` | `ERROR_INVALID_ARGUMENT` | Validate `key` | Argument 1 (`key`) |
| `0x81110302` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 2 (`value`) |
| `0x82110502` | `ERROR_INVALID_STATE` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90110501` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90110502` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91110502` | `ERROR_ALLOCATION_FAILED` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1110401` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1110502` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1110601` | `FATAL_INVARIANT_VIOLATION` | Copy new key | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1110602` | `FATAL_INVARIANT_VIOLATION` | Copy new key | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1110701` | `FATAL_INVARIANT_VIOLATION` | Copy value | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1110702` | `FATAL_INVARIANT_VIOLATION` | Copy value | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1110901` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1110902` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1110903` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1110904` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1110905` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |
| `0xC1110504` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x04`: Calculated capacity or extent violated growth bounds. |

**C++:** `catalyst::RESULT catalyst::map_set(MAP* map, const void* key, const void* value)`.


#### catrl_map_remove

```c
CATALYST_RESULT catrl_map_remove(
    CATALYST_MAP* map,
    const void* key
);
```

Removes an association with a byte-identical key. Enumeration order may change.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.

**Context:** `0x12`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00120600` | `SUCCESS` | Remove entry | — |
| `0x01120400` | `SUCCESS_NOOP` | Skip absent entry | — |
| `0x81120100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81120201` | `ERROR_INVALID_ARGUMENT` | Validate `key` | Argument 1 (`key`) |
| `0xC1120301` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1120501` | `FATAL_INVARIANT_VIOLATION` | Move last entry into removed slot | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1120502` | `FATAL_INVARIANT_VIOLATION` | Move last entry into removed slot | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1120701` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1120702` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1120703` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1120704` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1120705` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_remove(MAP* map, const void* key)`.


#### catrl_map_contains

```c
CATALYST_RESULT catrl_map_contains(
    const CATALYST_MAP* map,
    const void* key,
    CATALYST_BOOL* contains
);
```

Reports whether the map contains the specified key.

**Parameters:**

`map` — Map handle.  
`key` — Readable key of the configured key size.  
`contains` — Receives `CATALYST_TRUE` or `CATALYST_FALSE`.

**Context:** `0x13`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00130500` | `SUCCESS` | Write membership result | — |
| `0x81130100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81130201` | `ERROR_INVALID_ARGUMENT` | Validate `key` | Argument 1 (`key`) |
| `0x81130302` | `ERROR_INVALID_ARGUMENT` | Validate `contains` | Argument 2 (`contains`) |
| `0xC1130401` | `FATAL_INVARIANT_VIOLATION` | Search collection | Search helper operation `0x01`: Compare stored element or key (internal argument failure). |
| `0xC1130601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1130602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1130603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1130604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1130605` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_contains(const MAP* map, const void* key, BOOL* contains)`.


#### catrl_map_count

```c
CATALYST_RESULT catrl_map_count(
    const CATALYST_MAP* map,
    CATALYST_NUINT* count
);
```

Writes the number of key/value associations to count.

**Parameters:**

`map` — Map handle.  
`count` — Receives the number of elements.

**Context:** `0x14`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00140300` | `SUCCESS` | Write count | — |
| `0x81140100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81140201` | `ERROR_INVALID_ARGUMENT` | Validate `count` | Argument 1 (`count`) |
| `0xC1140401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1140402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1140403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1140404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1140405` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_count(const MAP* map, NUINT* count)`.


#### catrl_map_at

```c
CATALYST_RESULT catrl_map_at(
    const CATALYST_MAP* map,
    CATALYST_NUINT index,
    void* key,
    void* value
);
```

Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes.

**Parameters:**

`map` — Map handle.  
`index` — Zero-based enumeration index.  
`key` — Writable buffer receiving the key.  
`value` — Writable buffer receiving the value.

**Context:** `0x15`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00150700` | `SUCCESS` | Retrieve entry | — |
| `0x81150100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0x81150201` | `ERROR_INVALID_ARGUMENT` | Validate `index` | Argument 1 (`index`) |
| `0x81150302` | `ERROR_INVALID_ARGUMENT` | Validate `key` | Argument 2 (`key`) |
| `0x81150403` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 3 (`value`) |
| `0xC1150501` | `FATAL_INVARIANT_VIOLATION` | Copy key to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1150502` | `FATAL_INVARIANT_VIOLATION` | Copy key to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1150601` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1150602` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1150801` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1150802` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1150803` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1150804` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1150805` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_at(const MAP* map, NUINT index, void* key, void* value)`.


#### catrl_map_clear

```c
CATALYST_RESULT catrl_map_clear(
    CATALYST_MAP* map
);
```

Removes all associations without releasing `capacity` or zeroing stored bytes.

**Parameters:**

`map` — Map handle.

**Context:** `0x16`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00160300` | `SUCCESS` | Clear collection | — |
| `0x01160200` | `SUCCESS_NOOP` | Skip empty collection | — |
| `0x81160100` | `ERROR_INVALID_ARGUMENT` | Validate `map` | Argument 0 (`map`) |
| `0xC1160401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1160402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1160403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1160404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1160405` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Map key extent is zero or not smaller than the entry stride. |

**C++:** `catalyst::RESULT catalyst::map_clear(MAP* map)`.


### Stack

#### catrl_stack_create

```c
CATALYST_RESULT catrl_stack_create(
    CATALYST_STACK** stack,
    CATALYST_NUINT element_size
);
```

Creates an empty stack with `element_size` bytes per element.

**Parameters:**

`stack` — Receives the new stack handle; cleared on failure.  
`element_size` — Element size in bytes. Must be nonzero.

**Context:** `0x17`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00170300` | `SUCCESS` | Create storage | — |
| `0x81170100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x81170201` | `ERROR_INVALID_ARGUMENT` | Validate `element_size` | Argument 1 (`element_size`) |
| `0x82170302` | `ERROR_INVALID_STATE` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x90170302` | `ERROR_BUFFER_OVERFLOW` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0x91170302` | `ERROR_ALLOCATION_FAILED` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |
| `0xC1170302` | `FATAL_INVARIANT_VIOLATION` | Create storage | Storage helper operation `0x02`: Allocate collection handle. |

**C++:** `catalyst::RESULT catalyst::stack_create(STACK** stack, NUINT element_size)`.


#### catrl_stack_destroy

```c
CATALYST_RESULT catrl_stack_destroy(
    CATALYST_STACK* stack
);
```

Releases stack storage and handle.

**Parameters:**

`stack` — Stack handle.

**Context:** `0x18`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00180200` | `SUCCESS` | Destroy storage | — |
| `0x81180100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x82180201` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x01`: Release element storage. |
| `0x82180202` | `ERROR_INVALID_STATE` | Destroy storage | Storage helper operation `0x02`: Release handle. |
| `0xC1180201` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x01`: Release element storage (internal argument failure). |
| `0xC1180202` | `FATAL_INVARIANT_VIOLATION` | Destroy storage | Storage helper operation `0x02`: Release handle (internal argument failure). |
| `0xC1180301` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1180302` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1180303` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1180304` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::stack_destroy(STACK* stack)`.


#### catrl_stack_push

```c
CATALYST_RESULT catrl_stack_push(
    CATALYST_STACK* stack,
    const void* value
);
```

Copies value to the top of the stack. Grows storage as needed.

**Parameters:**

`stack` — Stack handle.  
`value` — Readable value to copy or match.

**Context:** `0x19`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x00190500` | `SUCCESS` | Push value | — |
| `0x81190100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x81190201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0x82190302` | `ERROR_INVALID_STATE` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x90190301` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x01`: Check collection growth limit. |
| `0x90190302` | `ERROR_BUFFER_OVERFLOW` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0x91190302` | `ERROR_ALLOCATION_FAILED` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1190302` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x02`: Reallocate collection storage. |
| `0xC1190401` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC1190402` | `FATAL_INVARIANT_VIOLATION` | Copy stored bytes | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC1190601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC1190602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC1190603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC1190604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |
| `0xC1190304` | `FATAL_INVARIANT_VIOLATION` | Grow storage | Growth helper operation `0x04`: Calculated capacity or extent violated growth bounds. |

**C++:** `catalyst::RESULT catalyst::stack_push(STACK* stack, const void* value)`.


#### catrl_stack_pop

```c
CATALYST_RESULT catrl_stack_pop(
    CATALYST_STACK* stack,
    void* value
);
```

Copies the top element into value and removes it.

**Parameters:**

`stack` — Stack handle.  
`value` — Writable buffer receiving the value.

**Context:** `0x1A`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001A0500` | `SUCCESS` | Remove top element | — |
| `0x811A0100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x811A0201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0x881A0300` | `ERROR_NOT_FOUND` | Check for top element | — |
| `0xC11A0401` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC11A0402` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC11A0601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC11A0602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC11A0603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC11A0604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::stack_pop(STACK* stack, void* value)`.


#### catrl_stack_peek

```c
CATALYST_RESULT catrl_stack_peek(
    const CATALYST_STACK* stack,
    void* value
);
```

Copies the top element into value without removing it.

**Parameters:**

`stack` — Stack handle.  
`value` — Writable buffer receiving the value.

**Context:** `0x1B`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001B0500` | `SUCCESS` | Retrieve top element | — |
| `0x811B0100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x811B0201` | `ERROR_INVALID_ARGUMENT` | Validate `value` | Argument 1 (`value`) |
| `0x881B0300` | `ERROR_NOT_FOUND` | Check for top element | — |
| `0xC11B0401` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x01`: Validate CATMMLIB destination (internal argument failure). |
| `0xC11B0402` | `FATAL_INVARIANT_VIOLATION` | Copy value to output | `catmm_copy` operation `0x02`: Validate CATMMLIB source (internal argument failure). |
| `0xC11B0601` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC11B0602` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC11B0603` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC11B0604` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::stack_peek(const STACK* stack, void* value)`.


#### catrl_stack_count

```c
CATALYST_RESULT catrl_stack_count(
    const CATALYST_STACK* stack,
    CATALYST_NUINT* count
);
```

Writes the number of elements to count.

**Parameters:**

`stack` — Stack handle.  
`count` — Receives the number of elements.

**Context:** `0x1C`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001C0300` | `SUCCESS` | Write count | — |
| `0x811C0100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0x811C0201` | `ERROR_INVALID_ARGUMENT` | Validate `count` | Argument 1 (`count`) |
| `0xC11C0401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC11C0402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC11C0403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC11C0404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::stack_count(const STACK* stack, NUINT* count)`.


#### catrl_stack_clear

```c
CATALYST_RESULT catrl_stack_clear(
    CATALYST_STACK* stack
);
```

Removes all elements without releasing `capacity` or zeroing stored bytes.

**Parameters:**

`stack` — Stack handle.

**Context:** `0x1D`

**Results:**

| Result | Status | Operation | Detail |
| --- | --- | --- | --- |
| `0x001D0300` | `SUCCESS` | Clear collection | — |
| `0x011D0200` | `SUCCESS_NOOP` | Skip empty collection | — |
| `0x811D0100` | `ERROR_INVALID_ARGUMENT` | Validate `stack` | Argument 0 (`stack`) |
| `0xC11D0401` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage stride is zero. |
| `0xC11D0402` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Element count exceeds capacity. |
| `0xC11D0403` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage pointer and capacity disagree. |
| `0xC11D0404` | `FATAL_INVARIANT_VIOLATION` | Validate collection invariants | Storage capacity multiplied by stride is unrepresentable. |

**C++:** `catalyst::RESULT catalyst::stack_clear(STACK* stack)`.
