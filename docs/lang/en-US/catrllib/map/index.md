# Map

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_map_create

**C**

```c
void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_create(MAP** map, NUINT key_size, NUINT value_size, RESULT* result);
```

Creates an empty map with fixed key_size and value_size bytes per association.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map` output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Zero `key_size`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Zero `value_size`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Overflow if the combined size overflows. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | Allocation failed. [Storage results](../index.md#storage-results). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed during creation cleanup. [Storage results](../index.md#storage-results). |

## catrl_map_destroy

**C**

```c
void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_destroy(MAP* map, RESULT* result);
```

Releases map storage and handle.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed. [Storage results](../index.md#storage-results). |

## catrl_map_get

**C**

```c
void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_get(const MAP* map, const void* key, void* value, RESULT* result);
```

Copies the associated value for a byte-identical key into value.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_NOT_FOUND` | 0 | 0 | An absent key. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `key`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `value`. |

## catrl_map_set

**C**

```c
void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_set(MAP* map, const void* key, const void* value, RESULT* result);
```

Copies key and value into a new association, or replaces the value of an existing key without increasing count. Grows storage for new keys as needed.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for insertion or replacement. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `key`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 2 | Dependency status | Reallocation failed. [Storage results](../index.md#storage-results). |
| `FATAL_INVARIANT_VIOLATION` | 2 | `0x81` | Storage may no longer be usable. [Storage results](../index.md#storage-results). |

## catrl_map_remove

**C**

```c
void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_remove(MAP* map, const void* key, RESULT* result);
```

Removes an association with a byte-identical key. Enumeration order may change.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for removal. |
| `SUCCESS_NOOP` | 0 | 0 | No action when absent. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `key`. |

## catrl_map_contains

**C**

```c
void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_contains(const MAP* map, const void* key, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if key exists, otherwise CATALYST_FALSE, to contains.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for either membership result. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `key`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `contains`. |

## catrl_map_count

**C**

```c
void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_count(const MAP* map, NUINT* count, RESULT* result);
```

Writes the number of key/value associations to count.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `count`. |

## catrl_map_at

**C**

```c
void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_at(const MAP* map, NUINT index, void* key, void* value, RESULT* result);
```

Copies the key and value at an unordered enumeration index to their respective outputs, sized key_size and value_size bytes.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | `index` outside `count`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `key`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 3 | Null `value`. |

## catrl_map_clear

**C**

```c
void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::map_clear(MAP* map, RESULT* result);
```

Removes all associations without releasing capacity or zeroing stored bytes.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `SUCCESS_NOOP` | 0 | 0 | No action if empty. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `map`. |
