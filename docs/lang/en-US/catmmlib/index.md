# Memory management (CATMMLIB)

Headers: `catmmlib.h` and `catmmlib.hpp`. Link CATMMLIB. The C++ wrappers have the same behavior and accept `catalyst::RESULT*`.

## Conventions

- All sizes are byte counts. `result` may be null; all other pointer parameters must be non-null, even for zero-length byte operations.
- Allocation returns zero-filled payloads; use only the requested extent. The payload is not the original page-aligned allocation base; no adjustable or over-alignment guarantee is provided.
- Release only live CATMMLIB allocations, through the same allocator backend. Foreign pointers, repeated frees and access during release are outside the contract.
- Copy, move, compare and set operate on any valid readable/writable byte storage, not just CATMMLIB allocations. They do not check capacity or allocate memory.
- All result contexts and operation codes are zero. Detail is zero unless specified below.

## Types

`CATALYST_MEMORY_HEADER` / `catalyst::MEMORY_HEADER` is allocator-owned metadata immediately preceding the payload. Do not modify it.

| Field | Type | Meaning |
| --- | --- | --- |
| `requested_size` | `CATALYST_NUINT` | Requested payload size in bytes. |
| `allocated_size` | `CATALYST_NUINT` | Tracked extent including the header; Linux rounds to pages, Windows and Darwin record header plus payload. |

## Allocation backends

| Target | Allocation / release |
| --- | --- |
| Windows | VirtualAlloc / VirtualFree |
| Darwin | vm_allocate / vm_deallocate |
| Linux | mmap / munmap |

## API export definitions

`CATMMLIB_API` expands to `__declspec(dllexport)` on Windows/Cygwin when `CATMMLIB_EXPORTS` is defined, or `__declspec(dllimport)` when `CATMMLIB_USE_DLL` is defined. Otherwise it is empty. Build and link with matching library variants.

## Reference

- [Allocation and release](allocation/index.md)
- [Byte operations](byte-operations/index.md)
