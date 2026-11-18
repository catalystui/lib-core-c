# Relational collections (CATRLLIB)

Headers: `catrllib.h`, `catrllib.hpp`. Link CATRLLIB and its CATMMLIB dependency.

## Types

| C type | C++ alias | Collection |
| --- | --- | --- |
| `CATALYST_ARRAY` | `catalyst::ARRAY` | Fixed-length indexed elements |
| `CATALYST_SET` | `catalyst::SET` | Distinct byte values |
| `CATALYST_MAP` | `catalyst::MAP` | Unique byte keys associated with values |
| `CATALYST_STACK` | `catalyst::STACK` | Last-in, first-out elements |

All four are opaque structures. Create returns a handle through a pointer-to-pointer; destroy releases it. Their internal storage is not exposed.

## Conventions

- All functions return `CATALYST_RESULT` by value. Pointers must be non-null and refer to live handles or valid readable/writable storage.
- Sizes are nonzero byte counts. Lengths, counts and indices count elements or associations; indices start at zero.
- Collections copy exactly the configured number of bytes. Pointers embedded in stored values are shallow copies; referenced resources remain caller-owned. Set and map key equality compares every byte, including padding. Supply consistent byte representations.
- Inputs and outputs must not overlap. Provide buffers of the configured sizes; no typed alignment of internal bytes is promised.
- Creation clears a non-null output handle before validating sizes or allocating. Any existing handle is replaced, not destroyed.
- Query outputs are assigned only on success or may be unspecified after a fatal failure. Recoverable mutation failures leave logical contents unchanged. Set/map enumeration order is unspecified and can change on mutation.
- Clear retains capacity and does not securely erase bytes. Destroy consumes a valid handle even on cleanup failure. Foreign, stale or incorrectly typed handles are outside the contract. Concurrent access involving mutation or destruction requires external synchronization.

## Storage and results

CATRLLIB uses contiguous byte storage. Sizes, counts, indices and offsets are calculated byte-by-byte, with the pointer-width ABI preserved at public interfaces and address boundaries.

Creation allocates a handle and, for nonempty arrays, element storage. Set, map and stack capacity grows as needed. Recoverable allocation or extent failures do not change logical contents. Detected invalid internal storage invariants produce fatal results; these checks do not validate arbitrary foreign pointers.

Each reporting method has its own context ID, and each reporting site has its own operation ID. Success-level results have zero context. The [CATRLLIB API Reference](../api-reference/catrllib/index.md) defines the exact result values, including allocation, growth and cleanup failures.

## API export definitions

See [`CATRLLIB_API`](../api-reference/catrllib/index.md#catrllib_api) in the API reference.

## Collection guides

- [Array](array/index.md)
- [Set](set/index.md)
- [Map](map/index.md)
- [Stack](stack/index.md)
