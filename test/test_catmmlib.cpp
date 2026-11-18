#include "catmmlib.hpp"

#include <stdio.h>

using namespace catalyst;

static void print_result(const char* label, const RESULT& result) {
    printf(
        "%s -> status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
        label,
        (unsigned int) result.status,
        (unsigned int) result.context,
        (unsigned int) result.operation,
        (unsigned int) result.detail
    );
}

int main() {
    RESULT result;
    void* memory;
    BYTE* bytes;
    NUINT i;

    printf("CATMMTEST :: Starting allocation lifecycle test...\n");

    /* ---------------------------------------------------------------------------------------------
     * Allocate 64 bytes.
     * --------------------------------------------------------------------------------------------- */

    memory = 0;
    result = RESULT();

    printf("Allocating 64 bytes...\n");
    result = catalyst::alloc(&memory, 64);
    print_result("alloc", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x07 || result.detail != 0) {
        printf("Allocation failed.\n");
        return 1;
    }

    if (memory == 0) {
        printf("Allocation returned null memory.\n");
        return 2;
    }

    printf("Memory allocated at %p\n", memory);

    /* ---------------------------------------------------------------------------------------------
     * Write and verify the first pattern.
     * --------------------------------------------------------------------------------------------- */

    bytes = (BYTE*) memory;

    printf("Writing 64-byte test pattern...\n");
    for (i = 0; i < 64; i++) {
        bytes[i] = (BYTE) i;
    }

    printf("Verifying 64-byte test pattern...\n");
    for (i = 0; i < 64; i++) {
        if (bytes[i] != (BYTE) i) {
            printf("Mismatch before realloc at index %lu: expected %lu, got %u\n", (unsigned long) i, (unsigned long) i, (unsigned int) bytes[i]);
            return 3;
        }
    }

    printf("Initial pattern verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Grow allocation from 64 bytes to 128 bytes.
     * The original 64 bytes should survive.
     * --------------------------------------------------------------------------------------------- */

    printf("Reallocating 64 bytes to 128 bytes...\n");
    result = catalyst::realloc(&memory, 128);
    print_result("realloc grow", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x07 || result.detail != 0) {
        printf("Realloc grow failed.\n");
        return 4;
    }

    if (memory == 0) {
        printf("Realloc grow returned null memory.\n");
        return 5;
    }

    printf("Memory after grow is at %p\n", memory);

    bytes = (BYTE*) memory;

    printf("Verifying original 64 bytes survived grow...\n");
    for (i = 0; i < 64; i++) {
        if (bytes[i] != (BYTE) i) {
            printf("Mismatch after grow at index %lu: expected %lu, got %u\n", (unsigned long) i, (unsigned long) i, (unsigned int) bytes[i]);
            return 6;
        }
    }

    printf("Writing extended 128-byte pattern...\n");
    for (i = 64; i < 128; i++) {
        bytes[i] = (BYTE) i;
    }

    printf("Verifying full 128-byte pattern...\n");
    for (i = 0; i < 128; i++) {
        if (bytes[i] != (BYTE) i) {
            printf("Mismatch after extension at index %lu: expected %lu, got %u\n", (unsigned long) i, (unsigned long) i, (unsigned int) bytes[i]);
            return 7;
        }
    }

    printf("Grow pattern verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Shrink allocation from 128 bytes to 32 bytes.
     * The first 32 bytes should survive.
     * --------------------------------------------------------------------------------------------- */

    printf("Reallocating 128 bytes down to 32 bytes...\n");
    result = catalyst::realloc(&memory, 32);
    print_result("realloc shrink", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x07 || result.detail != 0) {
        printf("Realloc shrink failed.\n");
        return 8;
    }

    if (memory == 0) {
        printf("Realloc shrink returned null memory.\n");
        return 9;
    }

    printf("Memory after shrink is at %p\n", memory);

    bytes = (BYTE*) memory;

    printf("Verifying first 32 bytes survived shrink...\n");
    for (i = 0; i < 32; i++) {
        if (bytes[i] != (BYTE) i) {
            printf("Mismatch after shrink at index %lu: expected %lu, got %u\n", (unsigned long) i, (unsigned long) i, (unsigned int) bytes[i]);
            return 10;
        }
    }

    printf("Shrink pattern verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Realloc size 0 should behave like free and null the pointer.
     * --------------------------------------------------------------------------------------------- */

    printf("Reallocating to 0 bytes. This should free the memory...\n");
    result = catalyst::realloc(&memory, 0);
    print_result("realloc zero", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x03 || result.detail != 0) {
        printf("Realloc zero/free failed.\n");
        return 11;
    }

    if (memory != 0) {
        printf("Realloc zero did not null the memory pointer.\n");
        return 12;
    }

    printf("Realloc zero/free verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Realloc with null memory should behave like alloc.
     * --------------------------------------------------------------------------------------------- */

    printf("Testing realloc with null pointer. This should behave like alloc...\n");
    result = catalyst::realloc(&memory, 16);
    print_result("realloc null", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x02 || result.detail != 0) {
        printf("Realloc null-as-alloc failed.\n");
        return 13;
    }

    if (memory == 0) {
        printf("Realloc null-as-alloc returned null memory.\n");
        return 14;
    }

    bytes = (BYTE*) memory;

    printf("Writing 16-byte pattern after realloc null-as-alloc...\n");
    for (i = 0; i < 16; i++) {
        bytes[i] = (BYTE) (i + 1);
    }

    printf("Verifying 16-byte pattern...\n");
    for (i = 0; i < 16; i++) {
        if (bytes[i] != (BYTE) (i + 1)) {
            printf("Mismatch after realloc null-as-alloc at index %lu: expected %lu, got %u\n", (unsigned long) i, (unsigned long) (i + 1), (unsigned int) bytes[i]);
            return 15;
        }
    }

    printf("Realloc null-as-alloc verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Free final allocation.
     * --------------------------------------------------------------------------------------------- */

    printf("Freeing final allocation...\n");
    result = catalyst::free(memory);
    print_result("free final", result);

    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x03 || result.operation != 0x03 || result.detail != 0) {
        printf("Final free failed.\n");
        return 16;
    }

    /* ---------------------------------------------------------------------------------------------
     * Invalid argument tests.
     * --------------------------------------------------------------------------------------------- */

    printf("Testing invalid realloc: memory == 0...\n");
    result = RESULT();
    result = catalyst::realloc(0, 64);
    print_result("realloc invalid memory", result);

    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x02 || result.operation != 0x01 || result.detail != 0) {
        printf("Expected invalid argument error for memory == 0.\n");
        return 17;
    }

    // Exercise the byte operations, including both overlap directions.
    BYTE source[6] = {0, 1, 2, 3, 4, 5};
    BYTE destination[6] = {0, 0, 0, 0, 0, 0};
    SBYTE comparison = 0;
    catalyst::copy(destination, source, 6);
    for (i = 0; i < 6; i++) {
        if (destination[i] != source[i]) return 19;
    }
    catalyst::move(destination + 1, destination, 5);
    for (i = 1; i < 6; i++) {
        if (destination[i] != source[i - 1]) return 21;
    }
    catalyst::move(destination, destination + 1, 5);
    for (i = 0; i < 5; i++) {
        if (destination[i] != source[i]) return 23;
    }
    comparison = catalyst::compare(destination, source, 6);
    if (comparison != -1) return 24;
    catalyst::set(destination, (BYTE) 0xFF, 6);
    for (i = 0; i < 6; i++) {
        if (destination[i] != (BYTE) 0xFF) return 26;
    }
    comparison = catalyst::compare(destination, source, 6);
    if (comparison != 1) return 27;
    comparison = catalyst::compare(source, source, 6);
    if (comparison != 0) return 28;
    catalyst::copy(destination, source, 0);
    catalyst::move(destination, destination, 6);
    catalyst::set(destination, 0, 0);
    for (i = 0; i < 6; ++i) {
        if (destination[i] != (BYTE) 0xFF) return 31;
    }
    catalyst::copy(source, source, 6);
    for (i = 0; i < 6; ++i) {
        if (source[i] != (BYTE) i) return 30;
    }

    // A direct allocation failure and a delegated realloc failure have different contexts.
    memory = 0;
    result = catalyst::alloc(&memory, 0);
    if (memory != 0 || result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x01 || result.operation != 0x02 || result.detail != 1) return 32;
    result = catalyst::realloc(&memory, 0);
    if (memory != 0 || result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x02 || result.operation != 0x02 || result.detail != 1) return 33;
    result = catalyst::alloc(&memory, CATALYST_NUINT_MAX);
    if (memory != 0 || result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x01 || result.operation != 0x04 || result.detail != 0) return 34;
    result = catalyst::realloc(&memory, CATALYST_NUINT_MAX);
    if (memory != 0 || result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x02 || result.operation != 0x02 || result.detail != 0x04) return 35;
    result = catalyst::free(0);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x01 || result.detail != 0) return 36;
    if (catalyst::compare(destination, source, 0) != 0) return 37;
    catalyst::set(destination, (BYTE) 0, 6);
    if (destination[0] != 0) return 38;

    printf("CATMMTEST :: All allocation lifecycle and byte operation tests passed. 🔥\n");

    return 0;
}
