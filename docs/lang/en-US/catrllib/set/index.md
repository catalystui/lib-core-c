# Set

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_set_create

**C**

```c
void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_create(SET** set, NUINT element_size, RESULT* result);
```

Creates an empty set of element_size-byte elements.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set` output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | Allocation failed. [Storage results](../index.md#storage-results). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed during creation cleanup. [Storage results](../index.md#storage-results). |

## catrl_set_destroy

**C**

```c
void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_destroy(SET* set, RESULT* result);
```

Releases set storage and handle.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed. [Storage results](../index.md#storage-results). |

## catrl_set_add

**C**

```c
void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_add(SET* set, const void* value, RESULT* result);
```

Copies value into the set if no byte-identical element exists. Grows storage as needed.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for insertion. |
| `SUCCESS_NOOP` | 0 | 0 | No action for duplicate. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 2 | Dependency status | Reallocation failed. [Storage results](../index.md#storage-results). |
| `FATAL_INVARIANT_VIOLATION` | 2 | `0x81` | Storage may no longer be usable. [Storage results](../index.md#storage-results). |

## catrl_set_remove

**C**

```c
void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_remove(SET* set, const void* value, RESULT* result);
```

Removes a byte-identical element. Enumeration order may change.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for removal. |
| `SUCCESS_NOOP` | 0 | 0 | No action when absent. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |

## catrl_set_contains

**C**

```c
void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_contains(const SET* set, const void* value, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if value exists, otherwise CATALYST_FALSE, to contains.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation succeeded for either membership result. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `contains`. |

## catrl_set_count

**C**

```c
void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_count(const SET* set, NUINT* count, RESULT* result);
```

Writes the number of distinct elements to count.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `count`. |

## catrl_set_at

**C**

```c
void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_at(const SET* set, NUINT index, void* value, RESULT* result);
```

Copies the element at an unordered enumeration index into value.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | `index` outside `count`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `value`. |

## catrl_set_clear

**C**

```c
void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::set_clear(SET* set, RESULT* result);
```

Removes all elements without releasing capacity or zeroing the stored bytes.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `SUCCESS_NOOP` | 0 | 0 | No action if empty. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `set`. |
