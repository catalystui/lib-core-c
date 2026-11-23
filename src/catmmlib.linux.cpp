#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX
#include "catmmlib.internal.hpp"
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#error "CATMMLIB requires native anonymous mappings on Linux."
#endif

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
typedef char CATMMLIB_native_extent_fits[(sizeof(size_t) >= sizeof(CATALYST_NUINT)) ? 1 : -1];

CATALYST_NUINT catmm_platform_prefix_size() {
    return (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
}

CATALYST_RESULT catmm_platform_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    *memory = 0;
    CATMMLIB_POINTERW base = mmap(0, (size_t) size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x01, 0);
    }
    *memory = base;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x02, 0);
}

CATALYST_RESULT catmm_platform_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size) {
    /* Linux rounds this original extent up to pages, as it did for mmap. */
    if (munmap(memory, (size_t) size) != 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x01, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x02, 0);
}
#endif /* TARGET_PLATFORM_LINUX */
