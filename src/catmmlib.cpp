#include "catalyst.hpp"
#include "catmmlib.h"

using namespace catalyst;

// Context IDs follow CATMMLIB public declaration order (alloc = 0x01).

CATALYST_RESULT catmm_realloc(void** memory, CATALYST_NUINT size) {
    // Validate, initialize parameters
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x01, 0);
    }
    if (*memory == 0) {
        RESULT allocResult;
        allocResult = catmm_alloc(memory, size);
        return RESULT(allocResult.status, status_code_is_success(allocResult.status) ? 0 : 0x02, 0x02, allocResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? allocResult.detail : (status_code_is_success(allocResult.status) ? 0 : allocResult.operation));
    }
    if (size == 0) {
        RESULT freeResult;
        freeResult = catmm_free(*memory);
        if (status_code_is_success(freeResult.status)) {
            *memory = 0;
        }
        return RESULT(freeResult.status, status_code_is_success(freeResult.status) ? 0 : 0x02, 0x03, status_code_is_success(freeResult.status) ? 0 : freeResult.operation);
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
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x07, 0);
}

CATALYST_RESULT catmm_copy(void* destination, const void* source, CATALYST_NUINT size) {
    if (destination == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x01, 0);
    }
    if (source == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x02, 1);
    }
    if (size == 0 || destination == source) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x03, 0);
    }
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    do {
        *output++ = *input++;
    } while (--size != 0);
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x04, 0);
}

CATALYST_RESULT catmm_move(void* destination, const void* source, CATALYST_NUINT size) {
    if (destination == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x01, 0);
    }
    if (source == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x02, 1);
    }
    if (size == 0 || destination == source) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x03, 0);
    }
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
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x04, 0);
}

CATALYST_RESULT catmm_compare(const void* first, const void* second, CATALYST_NUINT size, CATALYST_SBYTE* comparison) {
    if (first == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x01, 0);
    }
    if (second == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x02, 1);
    }
    if (comparison == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x03, 3);
    }
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
    *comparison = order;
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x04, 0);
}

CATALYST_RESULT catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size) {
    if (memory == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 0);
    }
    if (size == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x02, 0);
    }
    BYTE* bytes = (BYTE*) memory;
    do {
        *bytes++ = value;
    } while (--size != 0);
    return RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}
