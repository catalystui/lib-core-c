#include "catalyst.hpp"
#include "catmmlib.h"

using namespace catalyst;

// Allocation context IDs: alloc = 0x01, realloc = 0x02, free = 0x03.
// Contexts 0x04 through 0x07 are retired; byte primitives do not report.

CATALYST_RESULT catmm_realloc(void** memory, CATALYST_NUINT size) {
    // Validate, initialize parameters
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x01, 0);
    }
    if (*memory == 0) {
        RESULT allocResult;
        allocResult = catmm_alloc(memory, size);
        return RESULT(allocResult.status, 0x02, 0x02, allocResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? allocResult.detail : (status_code_is_success(allocResult.status) ? 0 : allocResult.operation));
    }
    if (size == 0) {
        RESULT freeResult;
        freeResult = catmm_free(*memory);
        if (status_code_is_success(freeResult.status)) {
            *memory = 0;
        }
        return RESULT(freeResult.status, 0x02, 0x03, status_code_is_success(freeResult.status) ? 0 : freeResult.operation);
    }

    // Fetch the old block, then allocate the new one
    BYTE* oldBytes = (BYTE*) *memory;
    CATALYST_MEMORY_HEADER* oldHeader = (CATALYST_MEMORY_HEADER*) (oldBytes - sizeof(CATALYST_MEMORY_HEADER));
    void* newMemory = 0;
    RESULT allocResult;
    allocResult = catmm_alloc(&newMemory, size);
    if (!status_code_is_success(allocResult.status)) {
        return RESULT(allocResult.status, 0x02, 0x04, allocResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? allocResult.detail : allocResult.operation);
    }

    // Copy the data
    BYTE* newBytes = (BYTE*) newMemory;
    NUINT copySize = oldHeader->requested_size < size ? oldHeader->requested_size : size;
    while (copySize != 0) {
        *newBytes++ = *oldBytes++;
        copySize--;
    }

    // Free the old block and return
    RESULT freeResult;
    freeResult = catmm_free(*memory);
    if (!status_code_is_success(freeResult.status)) {
        // Release the replacement after a failed old-block release.
        RESULT cleanupResult;
        cleanupResult = catmm_free(newMemory);
        if (!status_code_is_success(cleanupResult.status)) {
            return RESULT(status_code_is_fatal(freeResult.status) ? freeResult.status : STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x02, 0x05, cleanupResult.operation);
        }
        return RESULT(freeResult.status, 0x02, 0x06, freeResult.operation);
    }
    *memory = newMemory;
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x07, 0);
}

void catmm_copy(void* destination, const void* source, CATALYST_NUINT size) {
    if (size == 0 || destination == source) return;
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    do {
        *output++ = *input++;
    } while (--size != 0);
}

void catmm_move(void* destination, const void* source, CATALYST_NUINT size) {
    if (size == 0 || destination == source) return;
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    // Copy backward when the destination follows the source.
    if (output > input) {
        output += size;
        input += size;
        do {
            *--output = *--input;
        } while (--size != 0);
    } else {
        do {
            *output++ = *input++;
        } while (--size != 0);
    }
}

CATALYST_SBYTE catmm_compare(const void* first, const void* second, CATALYST_NUINT size) {
    const BYTE* a = (const BYTE*) first;
    const BYTE* b = (const BYTE*) second;
    SBYTE order = 0;
    if (first != second) {
        while (size != 0) {
            BYTE firstByte = *a++;
            BYTE secondByte = *b++;
            if (firstByte != secondByte) {
                order = firstByte < secondByte ? (SBYTE) -1 : (SBYTE) 1;
                break;
            }
            size--;
        }
    }
    return order;
}

void catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size) {
    if (size == 0) return;
    BYTE* bytes = (BYTE*) memory;
    do {
        *bytes++ = value;
    } while (--size != 0);
}
