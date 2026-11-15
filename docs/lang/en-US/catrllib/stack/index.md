# Stack

See [CATRLLIB conventions](../index.md#conventions) and [storage results](../index.md#storage-results) for shared requirements.

## catrl_stack_create

```c
void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_create(STACK** stack, NUINT element_size, RESULT* result);
```

Creates an empty stack with element_size bytes per element.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack output, 1 = zero element_size; [creation errors](../index.md#storage-results).

## catrl_stack_destroy

```c
void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_destroy(STACK* stack, RESULT* result);
```

Releases stack storage and handle.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack; [destruction errors](../index.md#storage-results).

## catrl_stack_push

```c
void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_push(STACK* stack, const void* value, RESULT* result);
```

Copies value to the top of the stack. Grows storage as needed.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value; [growth errors](../index.md#storage-results).

## catrl_stack_pop

```c
void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_pop(STACK* stack, void* value, RESULT* result);
```

Copies the top element into value and removes it.

**Results:** SUCCESS; ERROR_NOT_FOUND if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value.

## catrl_stack_peek

```c
void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_peek(const STACK* stack, void* value, RESULT* result);
```

Copies the top element into value without removing it.

**Results:** SUCCESS; ERROR_NOT_FOUND if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null value.

## catrl_stack_count

```c
void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_count(const STACK* stack, NUINT* count, RESULT* result);
```

Writes the number of elements to count.

**Results:** SUCCESS; ERROR_INVALID_ARGUMENT detail 0 = null stack, 1 = null count.

## catrl_stack_clear

```c
void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result);
```

```cpp
void catalyst::stack_clear(STACK* stack, RESULT* result);
```

Removes all elements without releasing capacity or zeroing stored bytes.

**Results:** SUCCESS; SUCCESS_NOOP if empty; ERROR_INVALID_ARGUMENT detail 0 = null stack.
