#include "catmmlib.internal.hpp"

using namespace catalyst;

/* Contexts: alloc 01, realloc 02, free 03. Every reporting site has a distinct
 * operation within its context. Native backend sites reserve 01/02 (and alloc
 * 06/07 for page-rounding overflow/unsupported page size). Shared resize 0A
 * reports cleanup failure (or a fatal original-release failure); detail is the
 * other failure's status when different, zero otherwise.
 * Shared operations are assigned below.
 */

/* Source/destination provide size readable/writable bytes. Regions may overlap
 * only when forward copying cannot overwrite unread source bytes.
 */
static void catmm_copy_forward(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    while (size != 0) {
        *output++ = *input++;
        --size;
    }
}

CATALYST_RESULT catmm_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    *memory = 0;
    NUINT prefix = catmm_platform_prefix_size();
    if (size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x01, 0x03, 0);
    }
    CATMMLIB_POINTERW base = 0;
    RESULT allocation = catmm_platform_alloc(&base, (NUINT) (prefix + size));
    if (!status_code_is_success(allocation.status)) {
        return RESULT(allocation.status, 0x01, 0x04, allocation.detail);
    }
    CATMMLIB_MEMORY_HEADER header;
    header.requested_size = size;
#if !TARGET_PLATFORM_WIN32
    header.pending = 0;
#endif
    catmm_copy(base, &header, (NUINT) sizeof(header));
    BYTE* payload = (BYTE*) base + prefix;
    *memory = payload;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x05, 0);
}

CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT new_size) {
    if (*memory == 0) {
        RESULT allocation = catmm_alloc(memory, new_size);
        return RESULT(allocation.status, 0x02, 0x03, allocation.detail);
    }
    if (new_size == 0) {
        RESULT release = catmm_free(*memory);
        if (status_code_is_success(release.status)) {
            *memory = 0;
        }
        return RESULT(release.status, 0x02, 0x04, release.detail);
    }
    NUINT prefix = catmm_platform_prefix_size();
    if (new_size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x02, 0x05, 0);
    }
    CATMMLIB_POINTERW base = (BYTE*) *memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
#if TARGET_PLATFORM_WIN32
    if (new_size != header.requested_size) {
        RESULT resize = catmm_platform_realloc(&base, (NUINT) (prefix + new_size));
        if (!status_code_is_success(resize.status)) {
            return RESULT(resize.status, 0x02, 0x06, resize.detail);
        }
        header.requested_size = new_size;
        catmm_copy(base, &header, (NUINT) sizeof(header));
        *memory = (BYTE*) base + prefix;
    }
#else
    if (header.pending != 0) {
        CATMMLIB_MEMORY_HEADER pendingHeader;
        catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
        RESULT cleanup = catmm_platform_free(header.pending, (NUINT) (prefix + pendingHeader.requested_size));
        if (!status_code_is_success(cleanup.status)) {
            return RESULT(cleanup.status, 0x02, 0x08, cleanup.detail);
        }
        header.pending = 0;
        catmm_copy(base, &header, (NUINT) sizeof(header));
    }
    if (new_size != header.requested_size) {
#if TARGET_PLATFORM_DARWIN
        if (catmm_platform_extent_matches((NUINT) (prefix + header.requested_size), (NUINT) (prefix + new_size))) {
            /* Shrink retains the mapped page; clear bytes re-exposed by a
             * later growth, since they may contain the caller's old data.
             */
            if (new_size > header.requested_size) {
                catmm_set((BYTE*) *memory + header.requested_size, (BYTE) 0, (NUINT) (new_size - header.requested_size));
            }
            header.requested_size = new_size;
            catmm_copy(base, &header, (NUINT) sizeof(header));
        } else
#endif
        {
            CATMMLIB_POINTERW replacement = 0;
            RESULT allocation = catmm_alloc(&replacement, new_size);
            if (!status_code_is_success(allocation.status)) {
                return RESULT(allocation.status, 0x02, 0x09, allocation.detail);
            }
            NUINT retained = header.requested_size < new_size ? header.requested_size : new_size;
            catmm_copy(replacement, *memory, retained);
            RESULT release = catmm_platform_free(base, (NUINT) (prefix + header.requested_size));
            if (!status_code_is_success(release.status)) {
                CATMMLIB_POINTERW replacementBase = (BYTE*) replacement - prefix;
                NUINT replacementSize = (NUINT) (prefix + new_size);
                RESULT cleanup = catmm_platform_free(replacementBase, replacementSize);
                if (!status_code_is_success(cleanup.status)) {
                    /* Retain ownership without touching the original payload. No
                     * further mapping is created until this cleanup has succeeded.
                     */
                    header.pending = replacementBase;
                    catmm_copy(base, &header, (NUINT) sizeof(header));
                    STATUS_CODE failureStatus = status_code_is_fatal(release.status) ? release.status : cleanup.status;
                    DETAIL_CODE failureDetail = release.status == cleanup.status ? (DETAIL_CODE) 0
                        : (failureStatus == release.status ? cleanup.status : release.status);
                    return RESULT(failureStatus, 0x02, 0x0A, failureDetail);
                }
                return RESULT(release.status, 0x02, 0x0B, release.detail);
            }
            *memory = replacement;
        }
    }
#endif
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x07, 0);
}

CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory) {
    NUINT prefix = catmm_platform_prefix_size();
    CATMMLIB_POINTERW base = (BYTE*) memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
#if !TARGET_PLATFORM_WIN32
    if (header.pending != 0) {
        CATMMLIB_MEMORY_HEADER pendingHeader;
        catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
        RESULT cleanup = catmm_platform_free(header.pending, (NUINT) (prefix + pendingHeader.requested_size));
        if (!status_code_is_success(cleanup.status)) {
            return RESULT(cleanup.status, 0x03, 0x05, cleanup.detail);
        }
        header.pending = 0;
        catmm_copy(base, &header, (NUINT) sizeof(header));
    }
#endif
    RESULT release = catmm_platform_free(base, (NUINT) (prefix + header.requested_size));
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x03, 0x03, release.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x04, 0);
}

void catmm_copy(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (destination == source) return;
    catmm_copy_forward(destination, source, size);
}

void catmm_move(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (size == 0 || destination == source) return;
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    /* Equality is defined for unrelated pointers. Only scan within the readable
     * source extent; avoid ordering unrelated objects or flattening addresses.
     */
    NUINT offset = size;
    const BYTE* candidate = input;
    while (offset != 0) {
        if (output == candidate) {
            output += size;
            input += size;
            do {
                *--output = *--input;
                --size;
            } while (size != 0);
            return;
        }
        ++candidate;
        --offset;
    }
    catmm_copy_forward(destination, source, size);
}

CATALYST_SBYTE catmm_compare(CATMMLIB_POINTER memory1, CATMMLIB_POINTER memory2, CATALYST_NUINT size) {
    const BYTE* first = (const BYTE*) memory1;
    const BYTE* second = (const BYTE*) memory2;
    if (memory1 == memory2) return (SBYTE) 0;
#if CATALYST_SUPPORTS_64BIT
    typedef ULONG CATMMLIB_COMPARE_WORD;
#elif CATALYST_SUPPORTS_32BIT
    typedef UINT CATMMLIB_COMPARE_WORD;
#elif CATALYST_SUPPORTS_16BIT
    typedef USHORT CATMMLIB_COMPARE_WORD;
#endif
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT || CATALYST_SUPPORTS_16BIT
    const NUINT chunkSize = (NUINT) sizeof(CATMMLIB_COMPARE_WORD);
    while (size >= chunkSize) {
        CATMMLIB_COMPARE_WORD firstChunk;
        CATMMLIB_COMPARE_WORD secondChunk;
        /* Copy object representations into aligned unsigned locals. Never
         * load a scalar through an unaligned or aliased pointer, or derive
         * byte ordering from the native scalar's ordering.
         */
        catmm_copy(&firstChunk, first, chunkSize);
        catmm_copy(&secondChunk, second, chunkSize);
        if (firstChunk != secondChunk) break;
        first += chunkSize;
        second += chunkSize;
        size = (NUINT) (size - chunkSize);
    }
#endif
    while (size != 0) {
        if (*first != *second) {
            return *first < *second ? (SBYTE) -1 : (SBYTE) 1;
        }
        ++first;
        ++second;
        --size;
    }
    return (SBYTE) 0;
}

void catmm_set(CATMMLIB_POINTERW memory, CATMMLIB_WORD value, CATALYST_NUINT size) {
    BYTE* output = (BYTE*) memory;
    while (size != 0) {
        *output++ = value;
        --size;
    }
}
