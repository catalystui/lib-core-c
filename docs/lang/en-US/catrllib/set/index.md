# Set

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_set_create

```c
void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_create(SET** set, NUINT element_size, RESULT* result);
```

Creates an empty set of element_size-byte elements.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set output, 1 = zero element_size; [creation errors](../index.md#storage-results).

## catrl_set_destroy

```c
void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_destroy(SET* set, RESULT* result);
```

Releases set storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set; [destruction errors](../index.md#storage-results).

## catrl_set_add

```c
void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_add(SET* set, const void* value, RESULT* result);
```

Copies value into the set if no byte-identical element exists. Grows storage as needed.

**Results:** SUCCESS for insertion; SUCCESS_NOOP for duplicate; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value; [growth errors](../index.md#storage-results).

## catrl_set_remove

```c
void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_remove(SET* set, const void* value, RESULT* result);
```

Removes a byte-identical element. Enumeration order may change.

**Results:** SUCCESS for removal; SUCCESS_NOOP when absent; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value.

## catrl_set_contains

```c
void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_contains(const SET* set, const void* value, BOOL* contains, RESULT* result);
```

Writes CATALYST_TRUE if value exists, otherwise CATALYST_FALSE, to contains.

**Results:** SUCCESS for either membership result; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null value, 2 = null contains.

## catrl_set_count

```c
void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_count(const SET* set, NUINT* count, RESULT* result);
```

Writes the number of distinct elements to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = null count.

## catrl_set_at

```c
void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_at(const SET* set, NUINT index, void* value, RESULT* result);
```

Copies the element at an unordered enumeration index into value.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null set, 1 = index outside count, 2 = null value.

## catrl_set_clear

```c
void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result);
```

```cpp
void catalyst::set_clear(SET* set, RESULT* result);
```

Removes all elements without releasing capacity or zeroing the stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null set.
