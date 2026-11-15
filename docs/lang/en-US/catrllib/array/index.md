# Array

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

All functions return `void`. Result context, operation and detail default to zero unless indicated.

## catrl_array_create

Creates a fixed-length array of length elements, each `element_size` bytes. All elements are zero-filled; zero length is valid.

```c
void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `array` | Receives the new array handle; cleared on failure. |
| `element_size` | Element size in bytes. Must be nonzero. |
| `length` | Number of elements; may be zero. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle created. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `array` output.<br>**Detail `1`:** Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Allocation failed.<br>**Operation `3`:** Release failed during creation cleanup.<br>Detail: dependency status byte. |

## catrl_array_destroy

Releases array storage and handle.

```c
void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `array` | Array handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle and storage released. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `array`. |
| `ERROR_BUFFER_OVERFLOW` | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `3`:** Release failed.<br>Detail: dependency status byte. |

## catrl_array_get

Copies the element at index into value, which must hold `element_size` bytes.

```c
void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `array` | Array handle. |
| `index` | Zero-based element index. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value retrieved. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `array`.<br>**Detail `1`:** `index` outside `length`.<br>**Detail `2`:** Null `value`. |

## catrl_array_set

Replaces the element at index with a copy of `element_size` bytes from value.

```c
void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `array` | Array handle. |
| `index` | Zero-based element index. |
| `value` | Readable value to copy or match. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value replaced. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `array`.<br>**Detail `1`:** `index` outside `length`.<br>**Detail `2`:** Null `value`. |

## catrl_array_length

Writes the fixed element count to length.

```c
void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `array` | Array handle. |
| `length` | Receives the fixed element count. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `array`.<br>**Detail `1`:** Null `length` output. |
