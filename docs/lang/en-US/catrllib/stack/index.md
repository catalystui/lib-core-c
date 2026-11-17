# Stack

See [CATRLLIB conventions](../index.md#conventions) for shared requirements and the [API reference](../../api-reference/catrllib/index.md) for signatures, parameters and exact results.

## [catrl_stack_create](../../api-reference/catrllib/index.md#catrl_stack_create)

Creates an empty stack with `element_size` bytes per element.

## [catrl_stack_destroy](../../api-reference/catrllib/index.md#catrl_stack_destroy)

Releases stack storage and handle.

## [catrl_stack_push](../../api-reference/catrllib/index.md#catrl_stack_push)

Copies value to the top of the stack. Grows storage as needed.

## [catrl_stack_pop](../../api-reference/catrllib/index.md#catrl_stack_pop)

Copies the top element into value and removes it.

## [catrl_stack_peek](../../api-reference/catrllib/index.md#catrl_stack_peek)

Copies the top element into value without removing it.

## [catrl_stack_count](../../api-reference/catrllib/index.md#catrl_stack_count)

Writes the number of elements to count.

## [catrl_stack_clear](../../api-reference/catrllib/index.md#catrl_stack_clear)

Removes all elements without releasing `capacity` or zeroing stored bytes.
