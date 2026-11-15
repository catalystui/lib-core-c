/* Shared realloc policy: delegate null/zero cases to the backend, otherwise
 * allocate, copy the requested prefix, then release the old block.
 * See catmmlib.h for ownership, output-state, and result contracts.
 */
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
    BYTE* newBytes = (BYTE*) newMemory;
    NUINT copySize = oldHeader->requested_size < size ? oldHeader->requested_size : size;
    for (NUINT i = 0; i < copySize; i++) {
        newBytes[i] = oldBytes[i];
    }

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
