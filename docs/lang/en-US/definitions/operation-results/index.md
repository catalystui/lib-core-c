# Operation results

`CATALYST_RESULT` contains four byte fields in declaration order:

| Field | C type | C++ type | Meaning |
| --- | --- | --- | --- |
| `status` | `CATALYST_STATUS_CODE` | `STATUS_CODE` | Operation outcome |
| `context` | `CATALYST_CONTEXT_CODE` | `CONTEXT_CODE` | Operation-defined context |
| `operation` | `CATALYST_OPERATION_CODE` | `OPERATION_CODE` | Function-specific stage |
| `detail` | `CATALYST_DETAIL_CODE` | `DETAIL_CODE` | Status-specific detail |

C++ constants `CONTEXT_CODE_NONE`, `OPERATION_CODE_NONE` and `DETAIL_CODE_NONE` are zero. Each library documents its own metadata. Every reporting function uses its assigned context on every outcome, including success and no-op. A default-constructed Result is not a report from a function. Reporting functions return all four bytes by value. Additional operation data uses output parameters. Callers may ignore the returned Result. Non-reporting functions return their ordinary value or `void`, according to their contract.

## Status codes

The C prefix is `CATALYST_STATUS_CODE_`; the C++ prefix is `catalyst::STATUS_CODE_`. Other values are not assigned a named status by these headers.

| Suffix | Value | Meaning |
| --- | --- | --- |
| `SUCCESS` | 0x00 | Operation completed. |
| `SUCCESS_NOOP` | 0x01 | Succeeded without an action. |
| `WARNING` | 0x40 | Completed with an unexpected or undesirable result. |
| `WARNING_PARTIAL` | 0x41 | Completed with incomplete or substituted output. |
| `WARNING_DEPRECATED` | 0x42 | Used behavior that may be removed. |
| `ERROR` | 0x80 | Recoverable failure. |
| `ERROR_INVALID_ARGUMENT` | 0x81 | Invalid argument; detail identifies the argument according to the API. |
| `ERROR_INVALID_STATE` | 0x82 | Invalid, corrupted or inconsistent state. |
| `ERROR_MALFORMED_INPUT` | 0x83 | Malformed required input. |
| `ERROR_ACCESS_DENIED` | 0x84 | Access denied. |
| `ERROR_NOT_IMPLEMENTED` | 0x85 | Requested behavior not implemented. |
| `ERROR_SYSTEM_NOT_SUPPORTED` | 0x86 | Unsupported target system or configuration. |
| `ERROR_TIMEOUT` | 0x87 | Required timeout occurred. |
| `ERROR_NOT_FOUND` | 0x88 | Required resource does not exist. |
| `ERROR_INTERRUPTED` | 0x89 | External interruption. |
| `ERROR_DEPENDENCY_FAILURE` | 0x8A | Dependency failed; detail contains its status byte. |
| `ERROR_BUFFER_OVERFLOW` | 0x90 | Data exceeds available buffer or representable extent. |
| `ERROR_ALLOCATION_FAILED` | 0x91 | Memory allocation failed. |
| `ERROR_IO_ERROR` | 0xA0 | Input/output failure. |
| `FATAL` | 0xC0 | Unrecoverable failure. |
| `FATAL_INVARIANT_VIOLATION` | 0xC1 | Fundamental invariant violated. |

## Severity

The upper two bits identify severity. The mask `CATALYST_STATUS_CODE_LEVEL_MASK` / `STATUS_CODE_LEVEL_MASK` is 0xC0.

| C macro | C++ function | Return value |
| --- | --- | --- |
| `CATALYST_STATUS_CODE_IS_SUCCESS(status)` | `status_code_is_success(status)` | True for success severity (including NOOP) |
| `CATALYST_STATUS_CODE_IS_WARNING(status)` | `status_code_is_warning(status)` | True for warning severity |
| `CATALYST_STATUS_CODE_IS_ERROR(status)` | `status_code_is_error(status)` | True for recoverable error severity |
| `CATALYST_STATUS_CODE_IS_FATAL(status)` | `status_code_is_fatal(status)` | True for fatal severity |

The C predicates are comparison expressions; C++ predicates return `BOOL`. Severity classification also applies to unnamed status values.

## Result conversions

`catalyst::RESULT` derives from `CATALYST_RESULT`.

| Member | Behavior |
| --- | --- |
| `RESULT()` | SUCCESS, all other fields zero. |
| `RESULT(STATUS_CODE, CONTEXT_CODE, OPERATION_CODE, DETAIL_CODE)` | Initializes the four fields. |
| `RESULT(const CATALYST_RESULT&)`, `operator=(const CATALYST_RESULT&)` | Copies the C representation. |
| `explicit RESULT(UINT)`, `operator=(UINT)`, `operator UINT() const` | Packs or unpacks the four fields when 32-bit integers are supported. |

C aggregate results must be initialized explicitly when needed. C++ assignments return `RESULT&`.
