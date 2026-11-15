# Set

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

All functions return `void`. Result context, operation and detail default to zero unless indicated.

## catrl_set_create

Creates an empty set of `element_size`-byte elements.

```c
void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Receives the new set handle; cleared on failure. |
| `element_size` | Element size in bytes. Must be nonzero. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle created. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set` output.<br>**Detail `1`:** Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Allocation failed.<br>**Operation `3`:** Release failed during creation cleanup.<br>Detail: dependency status byte. |

## catrl_set_destroy

Releases set storage and handle.

```c
void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle and storage released. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`. |
| `ERROR_BUFFER_OVERFLOW` | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `3`:** Release failed.<br>Detail: dependency status byte. |

## catrl_set_add

Copies value into the set if no byte-identical element exists. Grows storage as needed.

```c
void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `value` | Readable value to copy or match. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value inserted. |
| `SUCCESS_NOOP` | Value already exists. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`.<br>**Detail `1`:** Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `2`:** Reallocation failed.<br>Detail: dependency status byte. |
| `FATAL_INVARIANT_VIOLATION` | **Operation `2`, detail `0x81`:** Storage may no longer be usable. |

## catrl_set_remove

Removes a byte-identical element. Enumeration order may change.

```c
void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `value` | Readable value to copy or match. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Entry removed. |
| `SUCCESS_NOOP` | Entry does not exist. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`.<br>**Detail `1`:** Null `value`. |

## catrl_set_contains

Reports whether the set contains the specified value.

```c
void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `value` | Readable value to copy or match. |
| `contains` | Receives `CATALYST_TRUE` or `CATALYST_FALSE`. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Membership result written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`.<br>**Detail `1`:** Null `value`.<br>**Detail `2`:** Null `contains`. |

## catrl_set_count

Writes the number of distinct elements to count.

```c
void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `count` | Receives the number of elements. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`.<br>**Detail `1`:** Null `count`. |

## catrl_set_at

Copies the element at an unordered enumeration index into value.

```c
void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `index` | Zero-based enumeration index. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value retrieved. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`.<br>**Detail `1`:** `index` outside `count`.<br>**Detail `2`:** Null `value`. |

## catrl_set_clear

Removes all elements without releasing `capacity` or zeroing the stored bytes.

```c
void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `set` | Set handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Collection cleared. |
| `SUCCESS_NOOP` | Collection is already empty. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `set`. |
