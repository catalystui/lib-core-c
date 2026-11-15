# Stack

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

All functions return `void`. Result context, operation and detail default to zero unless indicated.

## catrl_stack_create

Creates an empty stack with `element_size` bytes per element.

```c
void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Receives the new stack handle; cleared on failure. |
| `element_size` | Element size in bytes. Must be nonzero. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle created. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack` output.<br>**Detail `1`:** Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `1`:** Allocation failed.<br>**Operation `3`:** Release failed during creation cleanup.<br>Detail: dependency status byte. |

## catrl_stack_destroy

Releases stack storage and handle.

```c
void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Handle and storage released. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`. |
| `ERROR_BUFFER_OVERFLOW` | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `3`:** Release failed.<br>Detail: dependency status byte. |

## catrl_stack_push

Copies value to the top of the stack. Grows storage as needed.

```c
void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `value` | Readable value to copy or match. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value pushed. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`.<br>**Detail `1`:** Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | **Operation `2`:** Reallocation failed.<br>Detail: dependency status byte. |
| `FATAL_INVARIANT_VIOLATION` | **Operation `2`, detail `0x81`:** Storage may no longer be usable. |

## catrl_stack_pop

Copies the top element into value and removes it.

```c
void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Top element removed and copied. |
| `ERROR_NOT_FOUND` | Collection is empty. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`.<br>**Detail `1`:** Null `value`. |

## catrl_stack_peek

Copies the top element into value without removing it.

```c
void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `value` | Writable buffer receiving the value. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Value retrieved. |
| `ERROR_NOT_FOUND` | Collection is empty. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`.<br>**Detail `1`:** Null `value`. |

## catrl_stack_count

Writes the number of elements to count.

```c
void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `count` | Receives the number of elements. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Count written. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`.<br>**Detail `1`:** Null `count`. |

## catrl_stack_clear

Removes all elements without releasing `capacity` or zeroing stored bytes.

```c
void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

| Parameter | Description |
| --- | --- |
| `stack` | Stack handle. |
| `result` | Optional result output. |

| Status | Meaning |
| --- | --- |
| `SUCCESS` | Collection cleared. |
| `SUCCESS_NOOP` | Collection is already empty. |
| `ERROR_INVALID_ARGUMENT` | **Detail `0`:** Null `stack`. |
