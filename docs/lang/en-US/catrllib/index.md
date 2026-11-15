# Relational collections (CATRLLIB)

Headers: `catrllib.h`, `catrllib.hpp`. Link CATRLLIB and its CATMMLIB dependency.

## Types

| C type | C++ alias | Collection |
| --- | --- | --- |
| `CATALYST_ARRAY` | `catalyst::ARRAY` | Fixed-length indexed elements |
| `CATALYST_SET` | `catalyst::SET` | Distinct byte values |
| `CATALYST_MAP` | `catalyst::MAP` | Unique byte keys associated with values |
| `CATALYST_STACK` | `catalyst::STACK` | Last-in, first-out elements |

All four are opaque structures. Create returns a handle through a pointer-to-pointer; destroy releases it. Their internal storage is not exposed.

## Conventions

- All functions return void; `result` may be null. Other pointers must be non-null and refer to live handles or valid readable/writable storage.
- Sizes are nonzero byte counts. Lengths, counts and indices count elements or associations; indices start at zero.
- Collections copy exactly the configured number of bytes. Pointers embedded in stored values are shallow copies; referenced resources remain caller-owned. Set and map key equality compares every byte, including padding. Supply consistent byte representations.
- Inputs and outputs must not overlap. Provide buffers of the configured sizes; no typed alignment of internal bytes is promised.
- Creation clears a non-null output handle before validating sizes or allocating. Any existing handle is replaced, not destroyed.
- Query outputs are assigned only on success. Recoverable mutation failures leave logical contents unchanged. Set/map enumeration order is unspecified and can change on mutation.
- Clear retains capacity and does not securely erase bytes. Destroy consumes a valid handle even on cleanup failure. Foreign, stale or incorrectly typed handles are outside the contract. Concurrent access involving mutation or destruction requires external synchronization.

## Storage results

Context is always zero; operation and detail are zero unless specified. The following results apply to allocation, growth and release in addition to the function-specific results.

| Status | Operation | Detail | Meaning |
| --- | --- | --- | --- |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Requested size or growth exceeds representable storage or CATMMLIB allocation limits. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | CATMMLIB allocation failed during creation. |
| `ERROR_DEPENDENCY_FAILURE` | 2 | Dependency status | CATMMLIB reallocation failed during growth. |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | CATMMLIB release failed during destruction or creation cleanup. |
| `FATAL_INVARIANT_VIOLATION` | 2 | 0x81 (ERROR_INVALID_ARGUMENT) | Growth failed during a release; storage may no longer be usable. |

Creation uses allocation and may need cleanup on failure; destruction releases both data and handle, reporting the first failed release. Growth errors apply to set insertion, new map associations and stack push. Underlying allocator overflow is reported directly as ERROR_BUFFER_OVERFLOW. A fatal growth failure is not a recoverable mutation error.


## Array

### catrl_array_create

```c
void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_create(ARRAY** array, NUINT element_size, NUINT length, RESULT* result);
```

Creates a fixed-length array of length elements, each element_size bytes. All elements are zero-filled; zero length is valid.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array output, 1 = zero element_size; [creation errors](#storage-results).

### catrl_array_destroy

```c
void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_destroy(ARRAY* array, RESULT* result);
```

Releases array storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array; [destruction errors](#storage-results).

### catrl_array_get

```c
void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_get(const ARRAY* array, NUINT index, void* value, RESULT* result);
```

Copies the element at index into value, which must hold element_size bytes.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = index outside length, 2 = null value.

### catrl_array_set

```c
void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_set(ARRAY* array, NUINT index, const void* value, RESULT* result);
```

Replaces the element at index with a copy of element_size bytes from value.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = index outside length, 2 = null value.

### catrl_array_length

```c
void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_length(const ARRAY* array, NUINT* length, RESULT* result);
```

Writes the fixed element count to length.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = null length output.

## Set

### catrl_set_create

```c
void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_create(SET** set, NUINT element_size, RESULT* result);
```

Creates an empty set of element_size-byte elements.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set output, 1 = zero element_size; [creation errors](#storage-results).

### catrl_set_destroy

```c
void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_destroy(SET* set, RESULT* result);
```

Releases set storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set; [destruction errors](#storage-results).

### catrl_set_add

```c
void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_add(SET* set, const void* value, RESULT* result);
```

Copies value into the set if no byte-identical element exists. Grows storage as needed.

**Results:** SUCCESS for insertion; SUCCESS_NOOP for duplicate; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value; [growth errors](#storage-results).

### catrl_set_remove

```c
void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_remove(SET* set, const void* value, RESULT* result);
```

Removes a byte-identical element. Enumeration order may change.

**Results:** SUCCESS for removal; SUCCESS_NOOP when absent; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value.

### catrl_set_contains

```c
void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_contains(const SET* set, const void* value, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if value exists, otherwise CATALYST_FALSE, to contains.

**Results:** SUCCESS for either membership result; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value, 2 = null contains.

### catrl_set_count

```c
void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_count(const SET* set, NUINT* count, RESULT* result);
```

Writes the number of distinct elements to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null count.

### catrl_set_at

```c
void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_at(const SET* set, NUINT index, void* value, RESULT* result);
```

Copies the element at an unordered enumeration index into value.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = index outside count, 2 = null value.

### catrl_set_clear

```c
void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_clear(SET* set, RESULT* result);
```

Removes all elements without releasing capacity or zeroing the stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null set.

## Map

### catrl_map_create

```c
void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_create(MAP** map, NUINT key_size, NUINT value_size, RESULT* result);
```

Creates an empty map with fixed key_size and value_size bytes per association.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map output, 1 = zero key_size, 2 = zero value_size; ERROR_BUFFER_OVERFLOW if the combined size overflows; [creation errors](#storage-results).

### catrl_map_destroy

```c
void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_destroy(MAP* map, RESULT* result);
```

Releases map storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map; [destruction errors](#storage-results).

### catrl_map_get

```c
void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_get(const MAP* map, const void* key, void* value, RESULT* result);
```

Copies the associated value for a byte-identical key into value.

**Results:** SUCCESS; ERROR_NOT_FOUND for an absent key; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null value.

### catrl_map_set

```c
void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_set(MAP* map, const void* key, const void* value, RESULT* result);
```

Copies key and value into a new association, or replaces the value of an existing key without increasing count. Grows storage for new keys as needed.

**Results:** SUCCESS for insertion or replacement; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null value; [growth errors](#storage-results).

### catrl_map_remove

```c
void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_remove(MAP* map, const void* key, RESULT* result);
```

Removes an association with a byte-identical key. Enumeration order may change.

**Results:** SUCCESS for removal; SUCCESS_NOOP when absent; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key.

### catrl_map_contains

```c
void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_contains(const MAP* map, const void* key, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if key exists, otherwise CATALYST_FALSE, to contains.

**Results:** SUCCESS for either membership result; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null contains.

### catrl_map_count

```c
void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_count(const MAP* map, NUINT* count, RESULT* result);
```

Writes the number of key/value associations to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null count.

### catrl_map_at

```c
void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_at(const MAP* map, NUINT index, void* key, void* value, RESULT* result);
```

Copies the key and value at an unordered enumeration index to their respective outputs, sized key_size and value_size bytes.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = index outside count, 2 = null key, 3 = null value.

### catrl_map_clear

```c
void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_clear(MAP* map, RESULT* result);
```

Removes all associations without releasing capacity or zeroing stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null map.

## Stack

### catrl_stack_create

```c
void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_create(STACK** stack, NUINT element_size, RESULT* result);
```

Creates an empty stack with element_size bytes per element.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack output, 1 = zero element_size; [creation errors](#storage-results).

### catrl_stack_destroy

```c
void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_destroy(STACK* stack, RESULT* result);
```

Releases stack storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack; [destruction errors](#storage-results).

### catrl_stack_push

```c
void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_push(STACK* stack, const void* value, RESULT* result);
```

Copies value to the top of the stack. Grows storage as needed.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value; [growth errors](#storage-results).

### catrl_stack_pop

```c
void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_pop(STACK* stack, void* value, RESULT* result);
```

Copies the top element into value and removes it.

**Results:** SUCCESS; ERROR_NOT_FOUND if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value.

### catrl_stack_peek

```c
void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_peek(const STACK* stack, void* value, RESULT* result);
```

Copies the top element into value without removing it.

**Results:** SUCCESS; ERROR_NOT_FOUND if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value.

### catrl_stack_count

```c
void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_count(const STACK* stack, NUINT* count, RESULT* result);
```

Writes the number of elements to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null count.

### catrl_stack_clear

```c
void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_clear(STACK* stack, RESULT* result);
```

Removes all elements without releasing capacity or zeroing stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack.

## API export definitions

`CATRLLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATRLLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATRLLIB_USE_DLL` is defined. Otherwise it is empty.
