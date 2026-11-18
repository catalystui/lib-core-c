/* Darwin backend: Mach virtual memory allocated in the current task.
 * The header's total extent is passed to vm_deallocate. Mach failures are
 * mapped to Catalyst status without exposing the native return code.
 * Public allocation/free contracts are documented in catmmlib.h.
 */
#include "cmakecfg.h"
#if TARGET_PLATFORM_DARWIN

#include "catalyst.hpp"
#include "catmmlib.h"

#import <mach/mach.h>

using namespace catalyst;

// Allocation and release share context and equivalent operation IDs across backends.
// Alloc: 01 output, 02 size, 03 page query, 04 extent, 05 rounding, 06 OS allocation, 07 success.
// Free: 01 pointer, 02 OS release, 03 success.

extern "C" CATALYST_RESULT catmm_alloc(void** memory, CATALYST_NUINT size) {
    // Validate, initialize parameters
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x01, 0);
    }
    *memory = 0;
    if (size == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x02, 1);
    }

    // Check against overflow, then allocate
    NUINT totalSize = (NUINT) sizeof(CATALYST_MEMORY_HEADER) + size;
    if (totalSize < size) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x01, 0x04, 0);
    }
    vm_address_t address = 0;
    if (vm_allocate(mach_task_self(), &address, (vm_size_t) totalSize, VM_FLAGS_ANYWHERE) != KERN_SUCCESS) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x06, 0);
    }

    // Initialize header and return
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) address;
    header->requested_size = size;
    header->allocated_size = totalSize;
    *memory = (void*) (((BYTE*) address) + sizeof(CATALYST_MEMORY_HEADER));
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

extern "C" CATALYST_RESULT catmm_free(void* memory) {
    // Validate, initialize parameters
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 0);
    }

    // Get header then deallocate
    BYTE* bytes = (BYTE*) memory;
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) (bytes - sizeof(CATALYST_MEMORY_HEADER));
    kern_return_t deallocResult = vm_deallocate(mach_task_self(), (vm_address_t) header, (vm_size_t) header->allocated_size);
    if (deallocResult != KERN_SUCCESS) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x02, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x03, 0);
}

#endif /* TARGET_PLATFORM_DARWIN */
