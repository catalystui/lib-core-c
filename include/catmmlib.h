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

/* -------------------------------------------------------------------------------------------------
 * C API :: MEMORY UNITS
 * ------------------------------------------------------------------------------------------------- */

/* Memory units representation. */
typedef CATALYST_BYTE   CATMMLIB_WORD;
#if TARGET_ADDRESS_KIND == 1
    typedef __UINTPTR_TYPE__    CATMMLIB_ADDRESS;
#elif TARGET_ADDRESS_KIND == 2
    typedef unsigned __int3264  CATMMLIB_ADDRESS;
#elif TARGET_ADDRESS_KIND == 3
    #include <stdint.h>
    typedef uintptr_t           CATMMLIB_ADDRESS;
#else
    #error "catmmlib.h :: Unsupported Platform :: Unable to find suitable integer type to represent memory addresses."
#endif
typedef const void*     CATMMLIB_POINTER;
typedef void*           CATMMLIB_POINTERW;

/* -------------------------------------------------------------------------------------------------
 * C API :: MEMORY ALLOCATION & MANAGEMENT
 * ------------------------------------------------------------------------------------------------- */

/** Allocate nonzero size bytes into valid output storage; output is null on
 * failure. Storage is zero-filled and supports native fundamental alignment,
 * not extended alignment. The output slot must not own a live allocation.
 */
CATMMLIB_API CATALYST_RESULT catmm_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size);

/** Resize a live CATMMLIB allocation, preserving its pointer and bytes on
 * failure. The output slot must be valid and outside the allocation. A null
 * input allocates (new_size must be nonzero); zero size releases a live input
 * and clears the pointer on success. Growth is zero-filled; the retained
 * prefix is preserved. Successful resizing invalidates prior aliases.
 */
CATMMLIB_API CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT new_size);

/** Release a live, non-null CATMMLIB allocation; the caller's pointer is not
 * cleared. Release failure leaves the allocation live. Allocation operations
 * on the same block require external synchronization.
 */
CATMMLIB_API CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory);

/* -------------------------------------------------------------------------------------------------
 * C API :: MEMORY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Copy bytes between valid, non-null, non-overlapping memory regions; zero size is a no-op. */
CATMMLIB_API void catmm_copy(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size);

/** Move bytes between valid, non-null memory regions, including overlap; zero size is a no-op. */
CATMMLIB_API void catmm_move(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size);

/** Compare valid, non-null regions as unsigned bytes; return -1, 0, or 1 (empty regions compare equal). */
CATMMLIB_API CATALYST_SBYTE catmm_compare(CATMMLIB_POINTER memory1, CATMMLIB_POINTER memory2, CATALYST_NUINT size);

/** Fill a valid, non-null memory region with the specified word; zero size is a no-op. */
CATMMLIB_API void catmm_set(CATMMLIB_POINTERW memory, CATMMLIB_WORD value, CATALYST_NUINT size);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATMMLIB_H */
