# Core

Core provides foundational value representations and native C ABI libraries with C++ namespace wrappers.

See the [API Reference](api-reference/index.md) for the consolidated, current reference by module. The topic-based references below are retained for now; their result tables may describe earlier assignments.

| Reference | Headers | Purpose |
| --- | --- | --- |
| [Foundational definitions](definitions/index.md) | `catalyst.h`, `catalyst.hpp`, `catalyst.c90.h`, `catalyst.c99.h` | Numeric and text types, encoding identifiers, status codes and result values. |
| [Memory management (CATMMLIB)](catmmlib/index.md) | `catmmlib.h`, `catmmlib.hpp` | Allocation, release, resizing, copying, moving, comparing and filling memory. |
| [Text encoding (CATTELIB)](cattelib/index.md) | `cattelib.h`, `cattelib.hpp` | ASCII, CP1252, UTF-8 and UTF-16LE encoding, decoding, length and conversion. |
| [Relational collections (CATRLLIB)](catrllib/index.md) | `catrllib.h`, `catrllib.hpp` | Arrays, sets, maps and stacks. Depends on CATMMLIB. |

Reporting C library functions return `CATALYST_RESULT` by value and use separate parameters for additional output. C++ wrappers reside in `catalyst` and return `RESULT` by value. Non-reporting functions return ordinary values directly or return `void`; numeric conversion helpers in the foundational headers return values directly.

Use the C header for the native ABI, or its C++ counterpart for namespace wrappers. Status names in the library references omit the `CATALYST_STATUS_CODE_` / `catalyst::STATUS_CODE_` prefix.
