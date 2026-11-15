# Array

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_array_create

```c
void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_create(ARRAY** array, NUINT element_size, NUINT length, RESULT* result);
```

Creates a fixed-length array of length elements, each element_size bytes. All elements are zero-filled; zero length is valid.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array output, 1 = zero element_size; [creation errors](../index.md#storage-results).

## catrl_array_destroy

```c
void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_destroy(ARRAY* array, RESULT* result);
```

Releases array storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array; [destruction errors](../index.md#storage-results).

## catrl_array_get

```c
void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_get(const ARRAY* array, NUINT index, void* value, RESULT* result);
```

Copies the element at index into value, which must hold element_size bytes.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = index outside length, 2 = null value.

## catrl_array_set

```c
void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_set(ARRAY* array, NUINT index, const void* value, RESULT* result);
```

Replaces the element at index with a copy of element_size bytes from value.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = index outside length, 2 = null value.

## catrl_array_length

```c
void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

```cpp
void catalyst::array_length(const ARRAY* array, NUINT* length, RESULT* result);
```

Writes the fixed element count to length.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null array, 1 = null length output.
