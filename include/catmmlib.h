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

/** Allocator-owned metadata immediately preceding a payload. */
typedef struct CATALYST_MEMORY_HEADER {
    CATALYST_NUINT requested_size;
    CATALYST_NUINT allocated_size;
} CATALYST_MEMORY_HEADER;

/** Allocate a nonzero number of zero-filled bytes; return a null pointer on failure. */
CATMMLIB_API CATALYST_RESULT catmm_alloc(void** memory, CATALYST_NUINT size);

/** Resize an allocation, preserving existing bytes. A zero size frees a non-null allocation. */
CATMMLIB_API CATALYST_RESULT catmm_realloc(void** memory, CATALYST_NUINT size);

/** Release an allocation returned by CATMMLIB; the caller's pointer is not cleared. */
CATMMLIB_API CATALYST_RESULT catmm_free(void* memory);

/** Copy bytes between valid non-null, non-overlapping regions; zero size or identical addresses are a no-op. */
CATMMLIB_API void catmm_copy(void* destination, const void* source, CATALYST_NUINT size);

/** Move bytes between valid non-null regions that may overlap; zero size or identical addresses are a no-op. */
CATMMLIB_API void catmm_move(void* destination, const void* source, CATALYST_NUINT size);

/** Compare valid non-null regions lexicographically; return -1, 0, or 1 (zero for empty regions). */
CATMMLIB_API CATALYST_SBYTE catmm_compare(const void* first, const void* second, CATALYST_NUINT size);

/** Fill a valid non-null region with the specified byte value; zero size is a no-op. */
CATMMLIB_API void catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATMMLIB_H */
