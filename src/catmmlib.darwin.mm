#include "cmakecfg.h"
#if TARGET_PLATFORM_DARWIN
#include "catmmlib.internal.hpp"
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <mach/vm_statistics.h>

#include <sched.h>

using namespace catalyst;

typedef char CATMMLIB_prefix_fits[(sizeof(CATMMLIB_NATIVE_PREFIX) <= CATALYST_NUINT_MAX) ? 1 : -1];
typedef char CATMMLIB_native_extent_fits[(sizeof(vm_size_t) >= sizeof(CATALYST_NUINT)) ? 1 : -1];

typedef char CATMMLIB_native_address_fits[(sizeof(vm_address_t) >= sizeof(void*)) ? 1 : -1];

#if CATMMLIB_MANAGED_HEAP
void catmm_platform_lock(CATALYST_BYTE* lock) {
    while (__atomic_exchange_n(lock, (BYTE) 1, __ATOMIC_ACQUIRE) != 0) {
        while (__atomic_load_n(lock, __ATOMIC_RELAXED) != 0) {
            sched_yield();
        }
    }
}

void catmm_platform_unlock(CATALYST_BYTE* lock) {
    __atomic_store_n(lock, (BYTE) 0, __ATOMIC_RELEASE);
}
#endif

CATALYST_BOOL catmm_platform_extent_matches(CATALYST_NUINT old_size, CATALYST_NUINT new_size) {
    NUINT pageSize = (NUINT) vm_page_size;
    return (NUINT) ((NUINT) (old_size - (NUINT) 1) / pageSize)
        == (NUINT) ((NUINT) (new_size - (NUINT) 1) / pageSize) ? TRUE : FALSE;
}

/* Allocation already established representable page rounding. Both extents
 * are nonzero, and the new one occupies fewer pages than the old one. */
CATALYST_RESULT catmm_platform_shrink(CATMMLIB_POINTERW memory, CATALYST_NUINT old_size, CATALYST_NUINT new_size) {
    NUINT pageSize = (NUINT) vm_page_size;
    NUINT oldRounded = (NUINT) (old_size + (NUINT) ((pageSize - old_size % pageSize) % pageSize));
    NUINT newRounded = (NUINT) (new_size + (NUINT) ((pageSize - new_size % pageSize) % pageSize));
    kern_return_t release = vm_deallocate(mach_task_self(),
        (vm_address_t) ((BYTE*) memory + newRounded), (vm_size_t) (oldRounded - newRounded));
    if (release != KERN_SUCCESS) {
        return RESULT(release == KERN_PROTECTION_FAILURE ? STATUS_CODE_ERROR_ACCESS_DENIED : STATUS_CODE_ERROR_INVALID_STATE, 0x02, 0x01, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x02, 0);
}

CATALYST_RESULT catmm_platform_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    *memory = 0;
    /* Guard the native page-rounding addition before calling Mach. Read the
     * OS-provided page size at the platform boundary; no fixed page size or
     * power-of-two arithmetic is assumed in shared Core.
     */
    if (vm_page_size == 0 || vm_page_size > CATALYST_NUINT_MAX) {
        return RESULT(STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED, 0x01, 0x07, 0);
    }
    NUINT pageSize = (NUINT) vm_page_size;
    NUINT remainder = (NUINT) (size % pageSize);
    if (remainder != 0 && size > (NUINT) (CATALYST_NUINT_MAX - (NUINT) (pageSize - remainder))) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x01, 0x06, 0);
    }
    vm_address_t address = 0;
    kern_return_t allocation = vm_allocate(mach_task_self(), &address, (vm_size_t) size, VM_FLAGS_ANYWHERE);
    if (allocation != KERN_SUCCESS) {
        return RESULT(allocation == KERN_PROTECTION_FAILURE ? STATUS_CODE_ERROR_ACCESS_DENIED : STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x01, 0);
    }
    *memory = (CATMMLIB_POINTERW) address;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x02, 0);
}

CATALYST_RESULT catmm_platform_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size) {
    /* Mach rounds this original extent up to pages, as it did on allocation. */
    kern_return_t release = vm_deallocate(mach_task_self(), (vm_address_t) memory, (vm_size_t) size);
    if (release != KERN_SUCCESS) {
        return RESULT(release == KERN_PROTECTION_FAILURE ? STATUS_CODE_ERROR_ACCESS_DENIED : STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x01, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x02, 0);
}
#endif /* TARGET_PLATFORM_DARWIN */
