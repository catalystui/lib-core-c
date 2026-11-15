# Map

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_map_create

```c
void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_create(MAP** map, NUINT key_size, NUINT value_size, RESULT* result);
```

Creates an empty map with fixed key_size and value_size bytes per association.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map output, 1 = zero key_size, 2 = zero value_size; ERROR_BUFFER_OVERFLOW if the combined size overflows; [creation errors](../index.md#storage-results).

## catrl_map_destroy

```c
void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_destroy(MAP* map, RESULT* result);
```

Releases map storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map; [destruction errors](../index.md#storage-results).

## catrl_map_get

```c
void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_get(const MAP* map, const void* key, void* value, RESULT* result);
```

Copies the associated value for a byte-identical key into value.

**Results:** SUCCESS; ERROR_NOT_FOUND for an absent key; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null value.

## catrl_map_set

```c
void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_set(MAP* map, const void* key, const void* value, RESULT* result);
```

Copies key and value into a new association, or replaces the value of an existing key without increasing count. Grows storage for new keys as needed.

**Results:** SUCCESS for insertion or replacement; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null value; [growth errors](../index.md#storage-results).

## catrl_map_remove

```c
void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_remove(MAP* map, const void* key, RESULT* result);
```

Removes an association with a byte-identical key. Enumeration order may change.

**Results:** SUCCESS for removal; SUCCESS_NOOP when absent; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key.

## catrl_map_contains

```c
void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_contains(const MAP* map, const void* key, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if key exists, otherwise CATALYST_FALSE, to contains.

**Results:** SUCCESS for either membership result; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null key, 2 = null contains.

## catrl_map_count

```c
void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_count(const MAP* map, NUINT* count, RESULT* result);
```

Writes the number of key/value associations to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = null count.

## catrl_map_at

```c
void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_at(const MAP* map, NUINT index, void* key, void* value, RESULT* result);
```

Copies the key and value at an unordered enumeration index to their respective outputs, sized key_size and value_size bytes.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null map, 1 = index outside count, 2 = null key, 3 = null value.

## catrl_map_clear

```c
void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result);
```

```cpp
void catalyst::map_clear(MAP* map, RESULT* result);
```

Removes all associations without releasing capacity or zeroing stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null map.
