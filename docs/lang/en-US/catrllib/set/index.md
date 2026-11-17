# Set

See [CATRLLIB conventions](../index.md#conventions) for shared requirements and the [API reference](../../api-reference/catrllib/index.md) for signatures, parameters and exact results.

## [catrl_set_create](../../api-reference/catrllib/index.md#catrl_set_create)

Creates an empty set of `element_size`-byte elements.

## [catrl_set_destroy](../../api-reference/catrllib/index.md#catrl_set_destroy)

Releases set storage and handle.

## [catrl_set_add](../../api-reference/catrllib/index.md#catrl_set_add)

Copies value into the set if no byte-identical element exists. Grows storage as needed.

## [catrl_set_remove](../../api-reference/catrllib/index.md#catrl_set_remove)

Removes a byte-identical element. Enumeration order may change.

## [catrl_set_contains](../../api-reference/catrllib/index.md#catrl_set_contains)

Reports whether the set contains the specified value.

## [catrl_set_count](../../api-reference/catrllib/index.md#catrl_set_count)

Writes the number of distinct elements to count.

## [catrl_set_at](../../api-reference/catrllib/index.md#catrl_set_at)

Copies the element at an unordered enumeration index into value.

## [catrl_set_clear](../../api-reference/catrllib/index.md#catrl_set_clear)

Removes all elements without releasing `capacity` or zeroing the stored bytes.
