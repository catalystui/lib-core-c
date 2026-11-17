# Map

See [CATRLLIB conventions](../index.md#conventions) for shared requirements and the [API reference](../../api-reference/catrllib/index.md) for signatures, parameters and exact results.

## [catrl_map_create](../../api-reference/catrllib/index.md#catrl_map_create)

Creates an empty map with fixed `key_size` and `value_size` bytes per association.

## [catrl_map_destroy](../../api-reference/catrllib/index.md#catrl_map_destroy)

Releases map storage and handle.

## [catrl_map_get](../../api-reference/catrllib/index.md#catrl_map_get)

Retrieves the value associated with a byte-identical key.

## [catrl_map_set](../../api-reference/catrllib/index.md#catrl_map_set)

Inserts a key/value association or updates an existing key without increasing the count. Grows storage for new keys as needed.

## [catrl_map_remove](../../api-reference/catrllib/index.md#catrl_map_remove)

Removes an association with a byte-identical key. Enumeration order may change.

## [catrl_map_contains](../../api-reference/catrllib/index.md#catrl_map_contains)

Reports whether the map contains the specified key.

## [catrl_map_count](../../api-reference/catrllib/index.md#catrl_map_count)

Writes the number of key/value associations to count.

## [catrl_map_at](../../api-reference/catrllib/index.md#catrl_map_at)

Copies the key and value at an unordered enumeration index to their respective outputs, sized `key_size` and `value_size` bytes.

## [catrl_map_clear](../../api-reference/catrllib/index.md#catrl_map_clear)

Removes all associations without releasing `capacity` or zeroing stored bytes.
