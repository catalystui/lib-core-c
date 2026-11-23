#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "catmmlib.internal.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

using namespace catalyst;

/* Native fundamental types determine payload alignment only; no floating-point
 * arithmetic is performed. A union's size is a multiple of its alignment.
 * Native storage supports fundamental alignment, not over-aligned objects.
 */
union CATMMLIB_NATIVE_PREFIX {
    CATMMLIB_MEMORY_HEADER header;
    long double floating_alignment;
    void* pointer_alignment;
    void (*function_alignment)();
};
typedef char CATMMLIB_prefix_fits[(sizeof(CATMMLIB_NATIVE_PREFIX) <= CATALYST_NUINT_MAX) ? 1 : -1];
typedef char CATMMLIB_native_extent_fits[(sizeof(SIZE_T) >= sizeof(CATALYST_NUINT)) ? 1 : -1];

CATALYST_NUINT catmm_platform_prefix_size() {
    return (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
}

CATALYST_RESULT catmm_platform_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    *memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T) size);
    if (*memory == 0) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x01, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x02, 0);
}

CATALYST_RESULT catmm_platform_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    CATMMLIB_POINTERW replacement = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *memory, (SIZE_T) size);
    if (replacement == 0) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x02, 0x01, 0);
    }
    *memory = replacement;
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x02, 0);
}

CATALYST_RESULT catmm_platform_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size) {
    (void) size;
    if (!HeapFree(GetProcessHeap(), 0, memory)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x01, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x02, 0);
}
#endif /* TARGET_PLATFORM_WIN32 */
