# Stack

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_stack_create

**C**

```c
void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_create(STACK** stack, NUINT element_size, RESULT* result);
```

Creates an empty stack with element_size bytes per element.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack` output. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Zero `element_size`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocation exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | Allocation failed. [Storage results](../index.md#storage-results). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed during creation cleanup. [Storage results](../index.md#storage-results). |

## catrl_stack_destroy

**C**

```c
void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_destroy(STACK* stack, RESULT* result);
```

Releases stack storage and handle.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Allocator reported overflow (if returned). |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | Release failed. [Storage results](../index.md#storage-results). |

## catrl_stack_push

**C**

```c
void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_push(STACK* stack, const void* value, RESULT* result);
```

Copies value to the top of the stack. Grows storage as needed.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Growth exceeds representable storage. |
| `ERROR_DEPENDENCY_FAILURE` | 2 | Dependency status | Reallocation failed. [Storage results](../index.md#storage-results). |
| `FATAL_INVARIANT_VIOLATION` | 2 | `0x81` | Storage may no longer be usable. [Storage results](../index.md#storage-results). |

## catrl_stack_pop

**C**

```c
void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_pop(STACK* stack, void* value, RESULT* result);
```

Copies the top element into value and removes it.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_NOT_FOUND` | 0 | 0 | Collection is empty. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |

## catrl_stack_peek

**C**

```c
void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_peek(const STACK* stack, void* value, RESULT* result);
```

Copies the top element into value without removing it.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_NOT_FOUND` | 0 | 0 | Collection is empty. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `value`. |

## catrl_stack_count

**C**

```c
void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_count(const STACK* stack, NUINT* count, RESULT* result);
```

Writes the number of elements to count.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
| `ERROR_INVALID_ARGUMENT` | 0 | 1 | Null `count`. |

## catrl_stack_clear

**C**

```c
void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

**C++**

```cpp
void catalyst::stack_clear(STACK* stack, RESULT* result);
```

Removes all elements without releasing capacity or zeroing stored bytes.

**Results**

| Status | Operation | Detail | Meaning |
| --- | ---: | --- | --- |
| `SUCCESS` | 0 | 0 | Operation completed successfully. |
| `SUCCESS_NOOP` | 0 | 0 | No action if empty. |
| `ERROR_INVALID_ARGUMENT` | 0 | 0 | Null `stack`. |
