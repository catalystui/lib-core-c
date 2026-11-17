# Array

See [CATRLLIB conventions](../index.md#conventions) for shared requirements and the [API reference](../../api-reference/catrllib/index.md) for signatures, parameters and exact results.

## [catrl_array_create](../../api-reference/catrllib/index.md#catrl_array_create)

Creates a fixed-length array of length elements, each `element_size` bytes. All elements are zero-filled; zero length is valid.

## [catrl_array_destroy](../../api-reference/catrllib/index.md#catrl_array_destroy)

Releases array storage and handle.

## [catrl_array_get](../../api-reference/catrllib/index.md#catrl_array_get)

Copies the element at index into value, which must hold `element_size` bytes.

## [catrl_array_set](../../api-reference/catrllib/index.md#catrl_array_set)

Replaces the element at index with a copy of `element_size` bytes from value.

## [catrl_array_length](../../api-reference/catrllib/index.md#catrl_array_length)

Writes the fixed element count to length.
