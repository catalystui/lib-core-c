/* Win32 backend: reserve and commit writable virtual memory. VirtualFree
 * releases the entire region using MEM_RELEASE and a zero size; the header
 * tracks the requested total extent rather than the OS-rounded region size.
 * Native error codes are not exposed. See catmmlib.h for the public contract.
 */
#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32

#include "catalyst.hpp"
#include "catmmlib.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

using namespace catalyst;

// Allocation and release share context and equivalent operation IDs across backends.
// Alloc: 01 output, 02 size, 03 page query, 04 extent, 05 rounding, 06 OS allocation, 07 success.
// Free: 01 pointer, 02 OS release, 03 success.

extern "C" CATALYST_RESULT catmm_alloc(void** memory, CATALYST_NUINT size) {
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
    void* address = VirtualAlloc(0, (SIZE_T) totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (address == 0) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x06, 0);
    }

    // Initialize header and return
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) address;
    header->requested_size = size;
    header->allocated_size = totalSize;
    *memory = (void*) (((BYTE*) address) + sizeof(CATALYST_MEMORY_HEADER));
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x07, 0);
}

extern "C" CATALYST_RESULT catmm_free(void* memory) {
    // Validate, initialize parameters
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 0);
    }

    // Get header then deallocate
    BYTE* bytes = (BYTE*) memory;
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) (bytes - sizeof(CATALYST_MEMORY_HEADER));
    if (!VirtualFree((void*) header, 0, MEM_RELEASE)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x02, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

#endif /* TARGET_PLATFORM_WIN32 */
