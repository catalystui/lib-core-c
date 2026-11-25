#include <cstdio>

#include "catalyst.hpp"
#include "catmmlib.hpp"

using namespace catalyst;

static int test_memory(BOOL managed) {
    const CONTEXT_CODE allocContext = managed ? (CONTEXT_CODE) 0x04 : (CONTEXT_CODE) 0x01;
    const CONTEXT_CODE resizeContext = managed ? (CONTEXT_CODE) 0x05 : (CONTEXT_CODE) 0x02;
    const CONTEXT_CODE freeContext = managed ? (CONTEXT_CODE) 0x06 : (CONTEXT_CODE) 0x03;
    CATALYST_RESULT (*allocate)(POINTERW*, NUINT) = managed ? catmm_heap_alloc : catmm_alloc;
    CATALYST_RESULT (*resize)(POINTERW*, NUINT) = managed ? catmm_heap_realloc : catmm_realloc;
    CATALYST_RESULT (*release)(POINTERW) = managed ? catmm_heap_free : catmm_free;
    RESULT (*allocateCpp)(POINTERW*, NUINT) = managed ? catalyst::heap_alloc : catalyst::alloc;
    RESULT (*resizeCpp)(POINTERW*, NUINT) = managed ? catalyst::heap_realloc : catalyst::realloc;
    RESULT (*releaseCpp)(POINTERW) = managed ? catalyst::heap_free : catalyst::free;
    std::printf("Testing %s allocation family.\n", managed ? "managed" : "native");
    int failed = 0;
    RESULT result;

    // Keep extents representable even when NUINT is a single byte.
    NUINT largeSize = 128;
    if (CATALYST_NUINT_MAX / (NUINT) 128 >= (NUINT) 128) {
        largeSize = (NUINT) ((NUINT) 128 * (NUINT) 128);
        if (CATALYST_NUINT_MAX / largeSize >= (NUINT) 64) {
            largeSize = (NUINT) (largeSize * (NUINT) 64);
        }
    }

    // ------------------------------------------------------------
    // Allocation and release
    // ------------------------------------------------------------
    // Zero-sized allocation is outside the public preconditions.
    {
        NUINT sizes[] = { 1, 64, largeSize };
        for (NUINT test = 0; test < (NUINT) 3; ++test) {
            POINTERW memory = 0;
            result = allocate(&memory, sizes[test]);
            BOOL passed = result.status == STATUS_CODE_SUCCESS && result.context == allocContext &&
                result.operation == 0x05 && result.detail == 0 && memory != 0;
            if (memory != 0) {
                const BYTE* bytes = (const BYTE*) memory;
                for (NUINT i = 0; i < sizes[test]; ++i) {
                    if (bytes[i] != 0) passed = FALSE;
                }
            }
            if (!passed) {
                std::printf("FAIL: catmm_alloc %lu zero-filled bytes\n", (unsigned long) sizes[test]);
                ++failed;
            } else {
                std::printf("PASS: catmm_alloc %lu zero-filled bytes\n", (unsigned long) sizes[test]);
            }
            if (memory != 0) {
                result = release(memory);
                if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                    std::printf("FAIL: catmm_free %lu-byte allocation\n", (unsigned long) sizes[test]);
                    ++failed;
                } else {
                    std::printf("PASS: catmm_free %lu-byte allocation\n", (unsigned long) sizes[test]);
                }
            }
        }
    }

    // The implementation rejects prefix + NUINT_MAX before any OS allocation.
    {
        BYTE sentinel = 0xA5;
        POINTERW memory = &sentinel;
        result = allocate(&memory, CATALYST_NUINT_MAX);
        if (memory != 0 || sentinel != (BYTE) 0xA5 || result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW ||
            result.context != allocContext || result.operation != 0x03 || result.detail != 0) {
            std::printf("FAIL: catmm_alloc deterministic size overflow and null output\n");
            ++failed;
        } else {
            std::printf("PASS: catmm_alloc deterministic size overflow and null output\n");
        }
        if (memory != 0 && memory != &sentinel) {
            result = release(memory);
            if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                std::printf("FAIL: catmm_free unexpected overflow-test allocation\n");
                ++failed;
            } else {
                std::printf("PASS: catmm_free unexpected overflow-test allocation\n");
            }
        }
    }

    // ------------------------------------------------------------
    // Reallocation: null input, same size, growth, shrink, regrowth, zero
    // ------------------------------------------------------------
    {
        POINTERW memory = 0;
        result = resize(&memory, 32);
        BOOL passed = result.status == STATUS_CODE_SUCCESS && result.context == resizeContext &&
            result.operation == 0x03 && result.detail == 0 && memory != 0;
        if (memory != 0) {
            const BYTE* bytes = (const BYTE*) memory;
            for (NUINT i = 0; i < (NUINT) 32; ++i) {
                if (bytes[i] != 0) passed = FALSE;
            }
        }
        if (!passed) {
            std::printf("FAIL: catmm_realloc null input allocates zero-filled memory\n");
            ++failed;
        } else {
            std::printf("PASS: catmm_realloc null input allocates zero-filled memory\n");
        }
        if (memory != 0) {
            NUINT oldSize = 32;
            NUINT sizes[] = { 32, 33, 31, 63, largeSize, (NUINT) (largeSize / (NUINT) 2), largeSize, 1, 0 };
            const char* names[] = { "same size", "small growth", "small shrink", "regrowth", "large growth", "large shrink", "large regrowth", "shrink to one byte", "release to zero" };
            for (NUINT test = 0; test < (NUINT) 9; ++test) {
                BYTE* bytes = (BYTE*) memory;
                for (NUINT i = 0; i < oldSize; ++i) {
                    bytes[i] = (BYTE) ((NUINT) (i % (NUINT) 251) + (NUINT) 1);
                }
                result = resize(&memory, sizes[test]);
                passed = result.status == STATUS_CODE_SUCCESS && result.context == resizeContext &&
                    result.operation == (sizes[test] == 0 ? 0x04 : 0x07) && result.detail == 0;
                if (sizes[test] == 0) {
                    if (memory != 0) passed = FALSE;
                } else if (memory == 0) {
                    passed = FALSE;
                } else if (status_code_is_success(result.status)) {
                    bytes = (BYTE*) memory; // Retrieve aliases again after successful resizing.
                    NUINT retained = oldSize < sizes[test] ? oldSize : sizes[test];
                    for (NUINT i = 0; i < sizes[test]; ++i) {
                        BYTE expected = i < retained ? (BYTE) ((NUINT) (i % (NUINT) 251) + (NUINT) 1) : (BYTE) 0;
                        if (bytes[i] != expected) passed = FALSE;
                    }
                }
                if (!passed) {
                    std::printf("FAIL: catmm_realloc %s, data and Result\n", names[test]);
                    ++failed;
                } else {
                    std::printf("PASS: catmm_realloc %s, data and Result\n", names[test]);
                }
                if (!status_code_is_success(result.status) || memory == 0) break;
                oldSize = sizes[test];
            }
            if (memory != 0) {
                result = release(memory);
                if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                    std::printf("FAIL: catmm_free remaining resized allocation\n");
                    ++failed;
                } else {
                    std::printf("PASS: catmm_free remaining resized allocation\n");
                }
            }
        }
    }

    // ------------------------------------------------------------
    // Reallocation failure preserves pointer and contents
    // ------------------------------------------------------------
    {
        POINTERW memory = 0;
        result = allocate(&memory, 32);
        BOOL passed = result.status == STATUS_CODE_SUCCESS && result.context == allocContext &&
            result.operation == 0x05 && result.detail == 0 && memory != 0;
        if (!passed) {
            std::printf("FAIL: catmm_alloc overflow-preservation setup\n");
            ++failed;
        } else {
            std::printf("PASS: catmm_alloc overflow-preservation setup\n");
        }
        if (memory != 0) {
            BYTE* bytes = (BYTE*) memory;
            for (NUINT i = 0; i < (NUINT) 32; ++i) bytes[i] = (BYTE) (i + (NUINT) 1);
            POINTERW original = memory;
            result = resize(&memory, CATALYST_NUINT_MAX);
            passed = result.status == STATUS_CODE_ERROR_BUFFER_OVERFLOW && result.context == resizeContext &&
                result.operation == 0x05 && result.detail == 0;
            if (!status_code_is_success(result.status)) {
                if (memory != original) passed = FALSE;
                bytes = (BYTE*) original;
                for (NUINT i = 0; i < (NUINT) 32; ++i) {
                    if (bytes[i] != (BYTE) (i + (NUINT) 1)) passed = FALSE;
                }
                memory = original;
            }
            if (!passed) {
                std::printf("FAIL: catmm_realloc size overflow preserves pointer and data\n");
                ++failed;
            } else {
                std::printf("PASS: catmm_realloc size overflow preserves pointer and data\n");
            }
            if (memory != 0) {
                result = release(memory);
                if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                    std::printf("FAIL: catmm_free after rejected resize\n");
                    ++failed;
                } else {
                    std::printf("PASS: catmm_free after rejected resize\n");
                }
            }
        }
        memory = 0;
        result = resize(&memory, CATALYST_NUINT_MAX);
        if (memory != 0 || result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW ||
            result.context != resizeContext || result.operation != 0x03 || result.detail != 0) {
            std::printf("FAIL: catmm_realloc null-input overflow keeps null output\n");
            ++failed;
        } else {
            std::printf("PASS: catmm_realloc null-input overflow keeps null output\n");
        }
        if (memory != 0) {
            result = release(memory);
            if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                std::printf("FAIL: catmm_free unexpected null-input overflow allocation\n");
                ++failed;
            } else {
                std::printf("PASS: catmm_free unexpected null-input overflow allocation\n");
            }
        }
    }

    // ------------------------------------------------------------
    // Copy: independent expected bytes and surrounding sentinels
    // ------------------------------------------------------------
    {
        struct CopyTest { const char* name; NUINT size; BOOL identical; };
        CopyTest tests[] = {
            { "single byte", 1, FALSE }, { "normal", 31, FALSE },
            { "larger buffer", 128, FALSE }, { "zero length", 0, FALSE },
            { "identical addresses", 128, TRUE }, { "identical empty extent", 0, TRUE }
        };
        for (NUINT test = 0; test < (NUINT) (sizeof(tests) / sizeof(tests[0])); ++test) {
            BYTE source[130];
            BYTE destination[130];
            for (NUINT i = 0; i < (NUINT) 130; ++i) { source[i] = (BYTE) i; destination[i] = 0xCC; }
            catmm_copy(tests[test].identical ? source + 1 : destination + 1, source + 1, tests[test].size);
            BOOL passed = TRUE;
            for (NUINT i = 0; i < (NUINT) 130; ++i) {
                BYTE expected = !tests[test].identical && i > 0 && i <= tests[test].size ? (BYTE) i : (BYTE) 0xCC;
                if (source[i] != (BYTE) i || destination[i] != expected) passed = FALSE;
            }
            if (!passed) {
                std::printf("FAIL: catmm_copy %s\n", tests[test].name);
                ++failed;
            } else {
                std::printf("PASS: catmm_copy %s\n", tests[test].name);
            }
        }
    }

    // ------------------------------------------------------------
    // Move: overlap in both directions, disjoint, adjacent, and no-op
    // ------------------------------------------------------------
    {
        struct MoveTest { const char* name; NUINT source; NUINT destination; NUINT size; };
        MoveTest tests[] = {
            { "disjoint forward", 4, 80, 32 }, { "disjoint reverse", 80, 4, 32 },
            { "overlap toward higher addresses", 4, 9, 97 }, { "overlap toward lower addresses", 9, 4, 97 },
            { "one-byte backward overlap", 1, 2, 128 }, { "one-byte forward overlap", 2, 1, 128 },
            { "adjacent regions", 0, 66, 66 }, { "single byte", 4, 5, 1 },
            { "identical addresses", 3, 3, 97 }, { "zero length", 4, 9, 0 }
        };
        for (NUINT test = 0; test < (NUINT) (sizeof(tests) / sizeof(tests[0])); ++test) {
            BYTE bytes[132];
            BYTE expected[132];
            for (NUINT i = 0; i < (NUINT) 132; ++i) { bytes[i] = (BYTE) i; expected[i] = (BYTE) i; }
            for (NUINT i = 0; i < tests[test].size; ++i) {
                expected[tests[test].destination + i] = (BYTE) (tests[test].source + i);
            }
            catmm_move(bytes + tests[test].destination, bytes + tests[test].source, tests[test].size);
            BOOL passed = TRUE;
            for (NUINT i = 0; i < (NUINT) 132; ++i) {
                if (bytes[i] != expected[i]) passed = FALSE;
            }
            if (!passed) {
                std::printf("FAIL: catmm_move %s\n", tests[test].name);
                ++failed;
            } else {
                std::printf("PASS: catmm_move %s\n", tests[test].name);
            }
        }
    }

    // ------------------------------------------------------------
    // Compare: byte ordering, unaligned buffers, chunks, and tails
    // ------------------------------------------------------------
    {
        NUINT sizes[] = { 0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 24, 31, 32, 33, 127 };
        for (NUINT test = 0; test < (NUINT) (sizeof(sizes) / sizeof(sizes[0])); ++test) {
            BYTE first[132];
            BYTE second[132];
            for (NUINT i = 0; i < (NUINT) 132; ++i) { first[i] = 0x80; second[i] = 0x80; }
            if (sizes[test] == 0) first[1] = 0x7F;
            if (catmm_compare(first + 1, second + 2, sizes[test]) != 0 ||
                catmm_compare(first + 1, first + 1, sizes[test]) != 0) {
                std::printf("FAIL: catmm_compare equal/empty/identical, size %lu\n", (unsigned long) sizes[test]);
                ++failed;
            } else {
                std::printf("PASS: catmm_compare equal/empty/identical, size %lu\n", (unsigned long) sizes[test]);
            }
            if (sizes[test] == 0) continue;
            for (NUINT difference = 0; difference < sizes[test]; ++difference) {
                for (NUINT i = 0; i < (NUINT) 132; ++i) { first[i] = 0x80; second[i] = 0x80; }
                first[1 + difference] = 0x7F;
                // A later, opposite mismatch must not override the first one.
                if (difference + (NUINT) 1 < sizes[test]) {
                    first[2 + difference] = 0xFF;
                    second[3 + difference] = 0;
                }
                if (catmm_compare(first + 1, second + 2, sizes[test]) != (SBYTE) -1 ||
                    catmm_compare(second + 2, first + 1, sizes[test]) != (SBYTE) 1) {
                    std::printf("FAIL: catmm_compare size %lu difference %lu, both orders\n", (unsigned long) sizes[test], (unsigned long) difference);
                    ++failed;
                } else {
                    std::printf("PASS: catmm_compare size %lu difference %lu, both orders\n", (unsigned long) sizes[test], (unsigned long) difference);
                }
            }
        }
    }

    // ------------------------------------------------------------
    // Fill: full and partial extents with unchanged surrounding bytes
    // ------------------------------------------------------------
    {
        struct FillTest { const char* name; NUINT offset; NUINT size; BYTE value; };
        FillTest tests[] = {
            { "zero fill", 0, 130, 0 }, { "nonzero fill", 0, 130, 0xA5 },
            { "partial buffer", 3, 61, 0x7F }, { "single byte", 2, 1, 0xFF }, { "zero length", 5, 0, 0x7F }
        };
        for (NUINT test = 0; test < (NUINT) (sizeof(tests) / sizeof(tests[0])); ++test) {
            BYTE bytes[130];
            for (NUINT i = 0; i < (NUINT) 130; ++i) bytes[i] = 0xCC;
            catmm_fill(bytes + tests[test].offset, tests[test].value, tests[test].size);
            BOOL passed = TRUE;
            for (NUINT i = 0; i < (NUINT) 130; ++i) {
                BYTE expected = i >= tests[test].offset && i < tests[test].offset + tests[test].size ? tests[test].value : (BYTE) 0xCC;
                if (bytes[i] != expected) passed = FALSE;
            }
            if (!passed) {
                std::printf("FAIL: catmm_fill %s\n", tests[test].name);
                ++failed;
            } else {
                std::printf("PASS: catmm_fill %s\n", tests[test].name);
            }
        }
    }

    // ------------------------------------------------------------
    // Larger independent buffers: copy, move, compare, and release
    // ------------------------------------------------------------
    {
        POINTERW source = 0;
        POINTERW destination = 0;
        RESULT sourceResult = allocate(&source, largeSize);
        RESULT destinationResult = allocate(&destination, largeSize);
        BOOL passed = sourceResult.status == STATUS_CODE_SUCCESS && sourceResult.context == allocContext &&
            sourceResult.operation == 0x05 && sourceResult.detail == 0 && source != 0 &&
            destinationResult.status == STATUS_CODE_SUCCESS && destinationResult.context == allocContext &&
            destinationResult.operation == 0x05 && destinationResult.detail == 0 && destination != 0;
        if (!passed) {
            std::printf("FAIL: catmm_alloc two independent large buffers\n");
            ++failed;
        } else {
            std::printf("PASS: catmm_alloc two independent large buffers\n");
        }
        if (source != 0 && destination != 0) {
            BYTE* input = (BYTE*) source;
            BYTE* output = (BYTE*) destination;
            for (NUINT i = 0; i < largeSize; ++i) input[i] = (BYTE) (i % (NUINT) 251);
            for (NUINT operation = 0; operation < (NUINT) 2; ++operation) {
                for (NUINT i = 0; i < largeSize; ++i) output[i] = 0xFF;
                if (operation == 0) catmm_copy(destination, source, largeSize);
                else catmm_move(destination, source, largeSize);
                passed = catmm_compare(source, destination, largeSize) == 0;
                for (NUINT i = 0; i < largeSize; ++i) {
                    BYTE expected = (BYTE) (i % (NUINT) 251);
                    if (input[i] != expected || output[i] != expected) passed = FALSE;
                }
                if (!passed) {
                    std::printf("FAIL: %s independent large buffers, contents and comparison\n", operation == 0 ? "catmm_copy" : "catmm_move");
                    ++failed;
                } else {
                    std::printf("PASS: %s independent large buffers, contents and comparison\n", operation == 0 ? "catmm_copy" : "catmm_move");
                }
            }
        }
        POINTERW allocations[] = { source, destination };
        for (NUINT i = 0; i < (NUINT) 2; ++i) {
            if (allocations[i] == 0) continue;
            result = release(allocations[i]);
            if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) {
                std::printf("FAIL: catmm_free large buffer %lu\n", (unsigned long) i);
                ++failed;
            } else {
                std::printf("PASS: catmm_free large buffer %lu\n", (unsigned long) i);
            }
        }
    }

    // ------------------------------------------------------------
    // Repeated allocation / resize / release through C++ wrappers
    // ------------------------------------------------------------
    for (NUINT iteration = 0; iteration < (NUINT) 16; ++iteration) {
        POINTERW memory = 0;
        RESULT allocation = allocateCpp(&memory, 16);
        BOOL passed = allocation.status == STATUS_CODE_SUCCESS && allocation.context == allocContext &&
            allocation.operation == 0x05 && allocation.detail == 0 && memory != 0;
        if (memory != 0) {
            BYTE* bytes = (BYTE*) memory;
            for (NUINT i = 0; i < (NUINT) 16; ++i) {
                if (bytes[i] != 0) passed = FALSE;
            }
            BYTE expected[16];
            for (NUINT i = 0; i < (NUINT) 16; ++i) expected[i] = (BYTE) (iteration + i + (NUINT) 1);
            catalyst::fill(memory, (BYTE) (iteration + (NUINT) 1), 16);
            for (NUINT i = 0; i < (NUINT) 16; ++i) {
                if (bytes[i] != (BYTE) (iteration + (NUINT) 1)) passed = FALSE;
                bytes[i] = expected[i];
            }
            BYTE output[16];
            catalyst::copy(output, memory, 16);
            catalyst::move(output + 1, output, 15);
            if (catalyst::compare(output + 1, expected, 15) != 0) passed = FALSE;
            for (NUINT i = 0; i < (NUINT) 16; ++i) {
                if (bytes[i] != expected[i] || output[i] != expected[i == 0 ? (NUINT) 0 : (NUINT) (i - (NUINT) 1)]) passed = FALSE;
            }
            result = resizeCpp(&memory, 64);
            if (result.status != STATUS_CODE_SUCCESS || result.context != resizeContext || result.operation != 0x07 || result.detail != 0 || memory == 0) passed = FALSE;
            if (status_code_is_success(result.status) && memory != 0) {
                bytes = (BYTE*) memory;
                for (NUINT i = 0; i < (NUINT) 64; ++i) {
                    BYTE value = i < (NUINT) 16 ? expected[i] : (BYTE) 0;
                    if (bytes[i] != value) passed = FALSE;
                }
            }
            if (memory != 0) {
                result = releaseCpp(memory);
                if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext || result.operation != 0x04 || result.detail != 0) passed = FALSE;
            }
        }
        if (!passed) {
            std::printf("FAIL: C++ wrappers and repeated lifecycle %lu\n", (unsigned long) iteration);
            ++failed;
        } else {
            std::printf("PASS: C++ wrappers and repeated lifecycle %lu\n", (unsigned long) iteration);
        }
    }

#if TARGET_SIZEOF_ADDRESS >= 4
    // Multiple live blocks, alternating releases, and zero-filled reuse.
    // The extent and count also exercise multiple slabs on Linux.
    {
        const NUINT count = 72;
        const NUINT extent = 1024;
        POINTERW blocks[count] = { 0 };
        BOOL passed = TRUE;
        for (NUINT round = 0; round < (NUINT) 2 && passed; ++round) {
            for (NUINT i = 0; i < count; ++i) {
                if (blocks[i] != 0) continue;
                result = allocate(&blocks[i], extent);
                if (result.status != STATUS_CODE_SUCCESS || result.context != allocContext ||
                    result.operation != 0x05 || result.detail != 0 || blocks[i] == 0) {
                    passed = FALSE;
                    break;
                }
                BYTE* bytes = (BYTE*) blocks[i];
                for (NUINT j = 0; j < extent; ++j) {
                    if (bytes[j] != 0) passed = FALSE;
                }
                catmm_fill(blocks[i], (BYTE) (i + (NUINT) 1), extent);
            }
            if (!passed) break;
            for (NUINT i = 0; i < count; ++i) {
                const BYTE* bytes = (const BYTE*) blocks[i];
                for (NUINT j = 0; j < extent; ++j) {
                    if (bytes[j] != (BYTE) (i + (NUINT) 1)) passed = FALSE;
                }
            }
            if (!passed) break;
            for (NUINT i = 0; i < count; i += (NUINT) 2) {
                result = release(blocks[i]);
                if (status_code_is_success(result.status)) blocks[i] = 0;
                if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext ||
                    result.operation != 0x04 || result.detail != 0) {
                    passed = FALSE;
                    break;
                }
            }
        }
        for (NUINT i = 0; i < count; ++i) {
            if (blocks[i] == 0) continue;
            result = release(blocks[i]);
            if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext ||
                result.operation != 0x04 || result.detail != 0) passed = FALSE;
        }
        if (!passed) {
            std::printf("FAIL: independent live allocations and zero-filled reuse\n");
            ++failed;
        } else {
            std::printf("PASS: independent live allocations and zero-filled reuse\n");
        }
    }
    // Exercise reusable native spans and transitions between extent classes.
    {
        NUINT sizes[] = { 12000, 13000, 11000, 13000, 24000, 25000, 40000, 60000, 70000, 24000 };
        POINTERW blocks[3] = { 0 };
        BOOL passed = TRUE;
        for (NUINT i = 0; i < (NUINT) 3; ++i) {
            result = allocate(&blocks[i], sizes[0]);
            if (result.status != STATUS_CODE_SUCCESS || result.context != allocContext ||
                result.operation != 0x05 || result.detail != 0 || blocks[i] == 0) {
                passed = FALSE;
                break;
            }
            for (NUINT j = 0; j < sizes[0]; ++j) {
                if (((BYTE*) blocks[i])[j] != 0) passed = FALSE;
            }
            catmm_fill(blocks[i], (BYTE) (i + (NUINT) 1), sizes[0]);
        }
        for (NUINT step = 1; step < (NUINT) (sizeof(sizes) / sizeof(sizes[0])) && passed; ++step) {
            for (NUINT i = 0; i < (NUINT) 3; ++i) {
                result = resize(&blocks[i], sizes[step]);
                if (result.status != STATUS_CODE_SUCCESS || result.context != resizeContext ||
                    result.operation != 0x07 || result.detail != 0 || blocks[i] == 0) {
                    passed = FALSE;
                    break;
                }
                NUINT retained = sizes[step - (NUINT) 1] < sizes[step] ? sizes[step - (NUINT) 1] : sizes[step];
                for (NUINT j = 0; j < sizes[step]; ++j) {
                    BYTE expected = j < retained ? (BYTE) (i + (NUINT) 1) : (BYTE) 0;
                    if (((BYTE*) blocks[i])[j] != expected) passed = FALSE;
                }
                catmm_fill(blocks[i], (BYTE) (i + (NUINT) 1), sizes[step]);
            }
        }
        for (NUINT i = 0; i < (NUINT) 3; ++i) {
            if (blocks[i] == 0) continue;
            result = release(blocks[i]);
            if (result.status != STATUS_CODE_SUCCESS || result.context != freeContext ||
                result.operation != 0x04 || result.detail != 0) passed = FALSE;
        }
        if (!passed) {
            std::printf("FAIL: multi-block span growth, shrink and reuse\n");
            ++failed;
        } else {
            std::printf("PASS: multi-block span growth, shrink and reuse\n");
        }
    }
#endif

    // ------------------------------------------------------------
    // Done
    // ------------------------------------------------------------
    if (failed != 0) {
        std::printf("\n%d CATMMLIB test(s) failed.\n", failed);
        return 1;
    }
    std::printf("\nAll CATMMLIB tests passed.\n");
    return 0;
}

int main() {
    const int native = test_memory(FALSE);
    const int managed = test_memory(TRUE);
    return native != 0 || managed != 0 ? 1 : 0;
}
