# Map

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

All functions return `void`. Result context, operation and detail default to zero unless indicated.

## catrl_map_create

Creates an empty map with fixed `key_size` and `value_size` bytes per association.

```c
void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Receives the new map handle; cleared on failure. |
| `key_size` | Key size in bytes. Must be nonzero. |
| `value_size` | Value size in bytes. Must be nonzero. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle created. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map` output.<br>**Detail `1`:** Zero `key_size`.<br>**Detail `2`:** Zero `value_size`. |
| `ERROR_BUFFER_OVERFLOW` | Combined key and value size exceeds the representable range. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Allocation failed.<br>**Operation `3`:** Release failed during creation cleanup.<br>Detail: dependency status byte. |

## catrl_map_destroy

Releases map storage and handle.

```c
void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle and storage released. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`. |
| `ERROR_BUFFER_OVERFLOW` | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `3`:** Release failed.<br>Detail: dependency status byte. |

## catrl_map_get

Retrieves the value associated with a byte-identical key.

```c
void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `key` | Readable key of the configured key size. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value retrieved. |
| `ERROR_NOT_FOUND` | Key not found. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** Null `key`.<br>**Detail `2`:** Null `value`. |

## catrl_map_set

Inserts a key/value association or updates an existing key without increasing the count. Grows storage for new keys as needed.

```c
void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `key` | Readable key of the configured key size. |
| `value` | Readable value to copy or match. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Association inserted or updated. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** Null `key`.<br>**Detail `2`:** Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `2`:** Reallocation failed.<br>Detail: dependency status byte. |
| `FATAL_INVARIANT_VIOLATION` | **Operation `2`, detail `0x81`:** Storage may no longer be usable. |

## catrl_map_remove

Removes an association with a byte-identical key. Enumeration order may change.

```c
void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `key` | Readable key of the configured key size. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Entry removed. |
| `SUCCESS_NOOP` | Entry does not exist. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** Null `key`. |

## catrl_map_contains

Reports whether the map contains the specified key.

```c
void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `key` | Readable key of the configured key size. |
| `contains` | Receives `CATALYST_TRUE` or `CATALYST_FALSE`. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Membership result written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** Null `key`.<br>**Detail `2`:** Null `contains`. |

## catrl_map_count

Writes the number of key/value associations to count.

```c
void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `count` | Receives the number of elements. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** Null `count`. |

## catrl_map_at

Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes.

```c
void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `index` | Zero-based enumeration index. |
| `key` | Writable buffer receiving the key. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value retrieved. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`.<br>**Detail `1`:** `index` outside `count`.<br>**Detail `2`:** Null `key`.<br>**Detail `3`:** Null `value`. |

## catrl_map_clear

Removes all associations without releasing `capacity` or zeroing stored bytes.

```c
void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `map` | Map handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Collection cleared. |
| `SUCCESS_NOOP` | Collection is already empty. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `map`. |
