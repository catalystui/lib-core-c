#ifndef CATMMLIB_H
#define CATMMLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATMMLIB_EXPORTS)
        #define CATMMLIB_API __declspec(dllexport)
    #elif defined(CATMMLIB_USE_DLL)
        #define CATMMLIB_API __declspec(dllimport)
    #else
        #define CATMMLIB_API
    #endif
#else
    #define CATMMLIB_API
#endif

/**
 * Allocation metadata stored immediately before the returned payload.
 * requested_size is the caller's payload size in bytes. allocated_size is the
 * tracked extent, including this header; Linux rounds it to pages,
 * while Windows and Darwin record header size plus requested payload size.
 * The fields are allocator-owned and must not be modified by callers.
 */
typedef struct CATALYST_MEMORY_HEADER {
    CATALYST_NUINT requested_size;
    CATALYST_NUINT allocated_size;
} CATALYST_MEMORY_HEADER;

/**
 * Allocate size bytes of payload and return its address through memory.
 * memory must point to writable pointer storage. An existing *memory is
 * replaced without being freed; retain or free any previous allocation first.
 * size must be nonzero. After validating memory, *memory is set to null and
 * remains null on failure. result may be null to discard status information.
 *
 * The current backends use zero-filled OS virtual memory. The payload begins
 * sizeof(CATALYST_MEMORY_HEADER) bytes after the OS allocation base; it is
 * not the original page-aligned address. There is no adjustable alignment or
 * additional guarantee for over-aligned objects. Use only the requested
 * payload extent. Release through catmmFree using the same allocator backend.
 *
 * Results (context and operation are zero):
 * SUCCESS: *memory receives the payload.
 * ERROR_INVALID_ARGUMENT: detail 0 = null memory, 1 = zero size.
 * ERROR_BUFFER_OVERFLOW: detail 0 = header-plus-payload size overflow;
 *     Linux detail 1 = page-rounding overflow.
 * ERROR_INVALID_STATE: Linux page-size query failed; detail is errno narrowed
 *     to CATALYST_DETAIL_CODE.
 * ERROR_ALLOCATION_FAILED: OS allocation failed; Linux detail is narrowed
 *     errno, while Windows and Darwin use detail 0.
 */
CATMMLIB_API void catmmAlloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);

/**
 * Resize an allocation, allocating a replacement before releasing the old block.
 * memory must point to writable pointer storage. A non-null *memory must be a
 * live payload returned by this allocator, with its header intact.
 * If *memory is null, delegates to catmmAlloc (including its zero-size error).
 * If *memory is non-null and size is zero, frees it and sets *memory to null
 * only on success. Otherwise copies min(old requested_size, size) bytes into
 * new zero-filled storage and returns the new payload; any added bytes are zero.
 * Even an unchanged size uses a replacement allocation. On success, old
 * pointers into the allocation are invalid. If replacement allocation or
 * release of the old block fails, *memory is left unchanged; a replacement
 * block release is attempted on the latter path. Cleanup status is not returned.
 * result may be null. Results are propagated from catmmAlloc/catmmFree;
 * a null memory argument returns ERROR_INVALID_ARGUMENT with detail 0.
 */
CATMMLIB_API void catmmRealloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);

/**
 * Release a live payload returned by this allocator, using the same backend.
 * Pass the original payload address, not an interior pointer or OS base.
 * The header must remain intact. Foreign pointers, repeated frees, and
 * concurrent access to a block being released are outside this contract.
 * The caller's pointer value is not changed; it is invalid after success.
 * result may be null. context and operation are zero.
 * SUCCESS: the OS release completed, detail 0.
 * ERROR_INVALID_ARGUMENT: null memory uses detail 0. An OS release failure
 * also uses this status; Linux supplies narrowed errno, while Windows and
 * Darwin supply detail 0. No payload access guarantee is made after an OS
 * release failure.
 */
CATMMLIB_API void catmmFree(void* memory, CATALYST_RESULT* result);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATMMLIB_H */
