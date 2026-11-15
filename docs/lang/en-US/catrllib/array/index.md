# Array

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_array_create

**C**

```c
void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::array_create(ARRAY** array, NUINT element_size, NUINT length, RESULT* result);
```

Creates a fixed-length array of length elements, each element_size bytes. All elements are zero-filled; zero length is valid.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `array` output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | Allocation failed. [Storage results](../index.md#storage-results). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed during creation cleanup. [Storage results](../index.md#storage-results). |

## catrl_array_destroy

**C**

```c
void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::array_destroy(ARRAY* array, RESULT* result);
```

Releases array storage and handle.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `array`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed. [Storage results](../index.md#storage-results). |

## catrl_array_get

**C**

```c
void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::array_get(const ARRAY* array, NUINT index, void* value, RESULT* result);
```

Copies the element at index into value, which must hold element_size bytes.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `array`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | `index` outside `length`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `value`. |

## catrl_array_set

**C**

```c
void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::array_set(ARRAY* array, NUINT index, const void* value, RESULT* result);
```

Replaces the element at index with a copy of element_size bytes from value.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `array`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | `index` outside `length`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 2 | Null `value`. |

## catrl_array_length

**C**

```c
void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::array_length(const ARRAY* array, NUINT* length, RESULT* result);
```

Writes the fixed element count to length.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `array`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `length` output. |
