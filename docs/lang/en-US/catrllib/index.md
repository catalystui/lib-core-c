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

- All functions return void; `result` may be null. Other pointers must be non-null and refer to live handles or valid readable/writable storage.
- Sizes are nonzero byte counts. Lengths, counts and indices count elements or associations; indices start at zero.
- Collections copy exactly the configured number of bytes. Pointers embedded in stored values are shallow copies; referenced resources remain caller-owned. Set and map key equality compares every byte, including padding. Supply consistent byte representations.
- Inputs and outputs must not overlap. Provide buffers of the configured sizes; no typed alignment of internal bytes is promised.
- Creation clears a non-null output handle before validating sizes or allocating. Any existing handle is replaced, not destroyed.
- Query outputs are assigned only on success. Recoverable mutation failures leave logical contents unchanged. Set/map enumeration order is unspecified and can change on mutation.
- Clear retains capacity and does not securely erase bytes. Destroy consumes a valid handle even on cleanup failure. Foreign, stale or incorrectly typed handles are outside the contract. Concurrent access involving mutation or destruction requires external synchronization.

## Storage results

Context is always zero; operation and detail are zero unless specified. The following results apply to allocation, growth and release in addition to the function-specific results.

| Status | Operation | Detail | Meaning |
| --- | --- | --- | --- |
| `ERROR_BUFFER_OVERFLOW` | 0 | 0 | Requested size or growth exceeds representable storage or CATMMLIB allocation limits. |
| `ERROR_DEPENDENCY_FAILURE` | 1 | Dependency status | CATMMLIB allocation failed during creation. |
| `ERROR_DEPENDENCY_FAILURE` | 2 | Dependency status | CATMMLIB reallocation failed during growth. |
| `ERROR_DEPENDENCY_FAILURE` | 3 | Dependency status | CATMMLIB release failed during destruction or creation cleanup. |
| `FATAL_INVARIANT_VIOLATION` | 2 | 0x81 (ERROR_INVALID_ARGUMENT) | Growth failed during a release; storage may no longer be usable. |

Creation uses allocation and may need cleanup on failure; destruction releases both data and handle, reporting the first failed release. Growth errors apply to set insertion, new map associations and stack push. Underlying allocator overflow is reported directly as ERROR_BUFFER_OVERFLOW. A fatal growth failure is not a recoverable mutation error.


## API export definitions

`CATRLLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATRLLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATRLLIB_USE_DLL` is defined. Otherwise it is empty.

## Reference

- [Array](array/index.md)
- [Set](set/index.md)
- [Map](map/index.md)
- [Stack](stack/index.md)
