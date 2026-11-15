#include "catalyst.hpp"
#include "catmmlib.h"

using namespace catalyst;

void catmm_realloc(void** memory, CATALYST_NUINT size, CATALYST_RESULT* result) {
    // Validate, initialize parameters
    if (memory == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 0);
        return;
    }
    if (*memory == 0) {
        catmm_alloc(memory, size, result);
        return;
    }
    if (size == 0) {
        RESULT freeResult;
        catmm_free(*memory, &freeResult);
        if (status_code_is_success(freeResult.status)) {
            *memory = 0;
        }
        if (result != 0) *result = freeResult;
        return;
    }

    // Fetch the old block, then allocate the new one
    BYTE* oldBytes = (BYTE*) *memory;
    CATALYST_MEMORY_HEADER* oldHeader = (CATALYST_MEMORY_HEADER*) (oldBytes - sizeof(CATALYST_MEMORY_HEADER));
    void* newMemory = 0;
    RESULT allocResult;
    catmm_alloc(&newMemory, size, &allocResult);
    if (!status_code_is_success(allocResult.status)) {
        if (result != 0) *result = allocResult;
        return;
    }

    // Copy the data
    NUINT copySize = oldHeader->requested_size < size ? oldHeader->requested_size : size;
    catmm_copy(newMemory, oldBytes, copySize, 0);

    // Free the old block and return
    RESULT freeResult;
    catmm_free(*memory, &freeResult);
    if (!status_code_is_success(freeResult.status)) {
        // Restore original state if we failed to free the old block
        catmm_free(newMemory, 0);
        if (result != 0) *result = freeResult;
        return;
    }
    *memory = newMemory;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
}

void catmm_copy(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result) {
    if (destination == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 0);
        return;
    }
    if (source == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 1);
        return;
    }
    if (size == 0 || destination == source) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0, 0);
        return;
    }
    BYTE* output = static_cast<BYTE*>(destination);
    const BYTE* input = static_cast<const BYTE*>(source);
    for (NUINT i = 0; i < size; i++) {
        output[i] = input[i];
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
}

void catmm_move(void* destination, const void* source, CATALYST_NUINT size, CATALYST_RESULT* result) {
    if (destination == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 0);
        return;
    }
    if (source == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 1);
        return;
    }
    if (size == 0 || destination == source) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0, 0);
        return;
    }
    BYTE* output = static_cast<BYTE*>(destination);
    const BYTE* input = static_cast<const BYTE*>(source);

    // Direction only matters for overlapping regions of the same byte array.
    // Either direction is safe for disjoint regions, regardless of pointer order.
    if (output > input) {
        for (NUINT i = size; i != 0; i--) {
            output[i - 1] = input[i - 1];
        }
    } else {
        for (NUINT i = 0; i < size; i++) {
            output[i] = input[i];
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
}

void catmm_compare(const void* first, const void* second, CATALYST_NUINT size, CATALYST_SBYTE* comparison, CATALYST_RESULT* result) {
    if (first == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 0);
        return;
    }
    if (second == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 1);
        return;
    }
    if (comparison == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 3);
        return;
    }

    const BYTE* a = static_cast<const BYTE*>(first);
    const BYTE* b = static_cast<const BYTE*>(second);
    SBYTE order = 0;
    if (first != second) {
        for (NUINT i = 0; i < size; i++) {
            if (a[i] != b[i]) {
                order = a[i] < b[i] ? (SBYTE) -1 : (SBYTE) 1;
                break;
            }
        }
    }
    *comparison = order;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
}

void catmm_set(void* memory, CATALYST_BYTE value, CATALYST_NUINT size, CATALYST_RESULT* result) {
    if (memory == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0, 0);
        return;
    }
    if (size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0, 0);
        return;
    }
    BYTE* bytes = static_cast<BYTE*>(memory);
    for (NUINT i = 0; i < size; i++) {
        bytes[i] = value;
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
}
