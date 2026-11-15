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
CATMMLIB_API void catmm_alloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);

/** Resize an allocation, preserving existing bytes. A zero size frees a non-null allocation. */
CATMMLIB_API void catmm_realloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result);

/** Release an allocation returned by CATMMLIB; the caller's pointer is not cleared. */
CATMMLIB_API void catmm_free(void* memory, CATALYST_RESULT* result);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATMMLIB_H */
