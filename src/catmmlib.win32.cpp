#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "catmmlib.internal.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

using namespace catalyst;

typedef char CATMMLIB_prefix_fits[(sizeof(CATMMLIB_NATIVE_PREFIX) <= CATALYST_NUINT_MAX) ? 1 : -1];
typedef char CATMMLIB_native_extent_fits[(sizeof(SIZE_T) >= sizeof(CATALYST_NUINT)) ? 1 : -1];

#if CATMMLIB_MANAGED_HEAP
void catmm_platform_lock(CATALYST_BYTE* lock) {
#if defined(_MSC_VER)
    while (InterlockedExchange8((volatile CHAR*) lock, 1) != 0) {
        SwitchToThread();
    }
#else
    while (__atomic_exchange_n(lock, (CATALYST_BYTE) 1, __ATOMIC_ACQUIRE) != 0) {
        while (__atomic_load_n(lock, __ATOMIC_RELAXED) != 0) {
            SwitchToThread();
        }
    }
#endif
}

void catmm_platform_unlock(CATALYST_BYTE* lock) {
#if defined(_MSC_VER)
    InterlockedExchange8((volatile CHAR*) lock, 0);
#else
    __atomic_store_n(lock, (CATALYST_BYTE) 0, __ATOMIC_RELEASE);
#endif
}
#endif

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
