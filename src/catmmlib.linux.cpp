#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX
#include "catmmlib.internal.hpp"
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#error "CATMMLIB requires native anonymous mappings on Linux."
#endif

#include <sched.h>

using namespace catalyst;

typedef char CATMMLIB_prefix_fits[(sizeof(CATMMLIB_NATIVE_PREFIX) <= CATALYST_NUINT_MAX) ? 1 : -1];
typedef char CATMMLIB_native_extent_fits[(sizeof(size_t) >= sizeof(CATALYST_NUINT)) ? 1 : -1];

void catmm_platform_yield() {
    sched_yield();
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
