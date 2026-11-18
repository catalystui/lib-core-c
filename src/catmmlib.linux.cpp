/* Linux backend: anonymous, private, writable mmap storage. The tracked
 * allocation extent is page-rounded and passed to munmap. Native errno
 * is reported only when representable in the eight-bit detail field.
 * Public allocation/free contracts are documented in catmmlib.h.
 */
#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX

#include "catalyst.hpp"
#include "catmmlib.h"

#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

using namespace catalyst;

// Allocation and release share context and equivalent operation IDs across backends.
// Alloc: 03 page query, 04 extent, 05 rounding, 06 OS allocation, 07 success.
// Free: 02 OS release, 03 success.

extern "C" CATALYST_RESULT catmm_alloc(void** memory, CATALYST_NUINT size) {
    *memory = 0;

    // Get system page size
    long pageSizeLong = sysconf(_SC_PAGESIZE);
    if (pageSizeLong <= 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x01, 0x03, errno > 0 && errno <= 0xFF ? (BYTE) errno : 0);
    }

    // Check against overflow, then allocate
    NUINT pageSize = (NUINT) pageSizeLong;
    NUINT headerSize = (NUINT) sizeof(CATALYST_MEMORY_HEADER);
    NUINT totalSize = headerSize + size;
    if (totalSize < size || totalSize < headerSize) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x01, 0x04, 0);
    }
    if (totalSize > ((NUINT) ~0) - (pageSize - 1)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x01, 0x05, 0);
    }
    NUINT allocatedSize = ((totalSize + pageSize - 1) / pageSize) * pageSize;

    // Allocate memory
    void* address = mmap(0, (size_t) allocatedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (address == MAP_FAILED) {
        return RESULT(STATUS_CODE_ERROR_ALLOCATION_FAILED, 0x01, 0x06, errno > 0 && errno <= 0xFF ? (BYTE) errno : 0);
    }

    // Initialize header and return
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) address;
    header->requested_size = size;
    header->allocated_size = allocatedSize;
    *memory = (void*) (((BYTE*) address) + sizeof(CATALYST_MEMORY_HEADER));
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x07, 0);
}

extern "C" CATALYST_RESULT catmm_free(void* memory) {

    // Get header then deallocate
    BYTE* bytes = (BYTE*) memory;
    CATALYST_MEMORY_HEADER* header = (CATALYST_MEMORY_HEADER*) (bytes - sizeof(CATALYST_MEMORY_HEADER));
    if (munmap((void*) header, (size_t) header->allocated_size) != 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x03, 0x02, errno > 0 && errno <= 0xFF ? (BYTE) errno : 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x03, 0);
}

#endif /* TARGET_PLATFORM_LINUX */
