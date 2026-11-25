#include "catmmlib.internal.hpp"

using namespace catalyst;

/* Size-oriented x86 compilers may leave byte loops unoptimized. Fixed-size
 * copies preserve unaligned access and avoid a dependency on runtime memcpy.
 * The optional scalar is selected only when the representation exists.
 */
#if defined(__OPTIMIZE_SIZE__) && (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__)) && (CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT || CATALYST_SUPPORTS_16BIT)
#define CATMMLIB_FIXED_COPY 1
#if CATALYST_SUPPORTS_64BIT
    typedef ULONG CATMMLIB_COPY_WORD;
#elif CATALYST_SUPPORTS_32BIT
    typedef UINT CATMMLIB_COPY_WORD;
#else
    typedef USHORT CATMMLIB_COPY_WORD;
#endif
#else
#define CATMMLIB_FIXED_COPY 0
#endif

/* Contexts: native alloc/realloc/free 01/02/03; heap alloc/realloc/free 04/05/06. Every reporting site has a distinct
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
#if CATMMLIB_FIXED_COPY
    const NUINT chunkSize = (NUINT) sizeof(CATMMLIB_COPY_WORD);
    while (size >= chunkSize) {
        CATMMLIB_COPY_WORD chunk;
        /* Stage through a local to preserve forward-overlap semantics. */
        __builtin_memcpy(&chunk, input, sizeof(chunk));
        __builtin_memcpy(output, &chunk, sizeof(chunk));
        output += chunkSize;
        input += chunkSize;
        size = (NUINT) (size - chunkSize);
    }
#endif
    while (size != 0) {
        *output++ = *input++;
        --size;
    }
}

/* The allocation family is a compile-time choice. Native storage keeps the
 * original platform resize behavior; managed storage can reuse a size class. */
template<BOOL managed>
static RESULT catmm_extent_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return managed ? catmm_heap_extent_alloc(memory, size) : catmm_platform_alloc(memory, size);
}

template<BOOL managed>
static RESULT catmm_extent_free(CATMMLIB_POINTERW memory, NUINT size) {
    return managed ? catmm_heap_extent_free(memory, size) : catmm_platform_free(memory, size);
}

template<BOOL managed>
static BOOL catmm_extent_matches(NUINT old_size, NUINT new_size) {
    if (managed) return catmm_heap_extent_matches(old_size, new_size);
#if TARGET_PLATFORM_DARWIN
    return catmm_platform_extent_matches(old_size, new_size);
#else
    return FALSE;
#endif
}

template<BOOL managed>
static RESULT catmm_release(CATMMLIB_POINTERW memory);

template<BOOL managed>
static RESULT catmm_allocate(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x04 : (CONTEXT_CODE) 0x01;
    *memory = 0;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    if (size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x03, 0);
    }
    CATMMLIB_POINTERW base = 0;
    RESULT allocation = catmm_extent_alloc<managed>(&base, (NUINT) (prefix + size));
    if (!status_code_is_success(allocation.status)) {
        return RESULT(allocation.status, context, 0x04, allocation.detail);
    }
    CATMMLIB_MEMORY_HEADER header;
    header.requested_size = size;
    header.pending = 0;
    catmm_copy(base, &header, (NUINT) sizeof(header));
    BYTE* payload = (BYTE*) base + prefix;
    *memory = payload;
    return RESULT(STATUS_CODE_SUCCESS, context, 0x05, 0);
}

template<BOOL managed>
static RESULT catmm_resize(CATMMLIB_POINTERW* memory, CATALYST_NUINT new_size) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x05 : (CONTEXT_CODE) 0x02;
    if (*memory == 0) {
        RESULT allocation = catmm_allocate<managed>(memory, new_size);
        return RESULT(allocation.status, context, 0x03, allocation.detail);
    }
    if (new_size == 0) {
        RESULT release = catmm_release<managed>(*memory);
        if (status_code_is_success(release.status)) {
            *memory = 0;
        }
        return RESULT(release.status, context, 0x04, release.detail);
    }
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    if (new_size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x05, 0);
    }
    CATMMLIB_POINTERW base = (BYTE*) *memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
#if TARGET_PLATFORM_WIN32
    if (!managed) {
        if (new_size != header.requested_size) {
            RESULT resize = catmm_platform_realloc(&base, (NUINT) (prefix + new_size));
            if (!status_code_is_success(resize.status)) {
                return RESULT(resize.status, context, 0x06, resize.detail);
            }
            header.requested_size = new_size;
            catmm_copy(base, &header, (NUINT) sizeof(header));
            *memory = (BYTE*) base + prefix;
        }
    } else
#endif
    {
        if (header.pending != 0) {
            CATMMLIB_MEMORY_HEADER pendingHeader;
            catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
            RESULT cleanup = catmm_extent_free<managed>(header.pending, (NUINT) (prefix + pendingHeader.requested_size));
            if (!status_code_is_success(cleanup.status)) {
                return RESULT(cleanup.status, context, 0x08, cleanup.detail);
            }
            header.pending = 0;
            catmm_copy(base, &header, (NUINT) sizeof(header));
        }
        if (new_size != header.requested_size) {
            if (catmm_extent_matches<managed>((NUINT) (prefix + header.requested_size), (NUINT) (prefix + new_size))) {
                /* Shrink retains the native storage; clear bytes re-exposed by a
                 * later growth, since they may contain the caller's old data.
                 */
                if (new_size > header.requested_size) {
                    catmm_fill((BYTE*) *memory + header.requested_size, (BYTE) 0, (NUINT) (new_size - header.requested_size));
                }
                header.requested_size = new_size;
                catmm_copy(base, &header, (NUINT) sizeof(header));
            } else {
                CATMMLIB_POINTERW replacement = 0;
                RESULT allocation = catmm_allocate<managed>(&replacement, new_size);
                if (!status_code_is_success(allocation.status)) {
                    return RESULT(allocation.status, context, 0x09, allocation.detail);
                }
                NUINT retained = header.requested_size < new_size ? header.requested_size : new_size;
                catmm_copy(replacement, *memory, retained);
                RESULT release = catmm_extent_free<managed>(base, (NUINT) (prefix + header.requested_size));
                if (!status_code_is_success(release.status)) {
                    CATMMLIB_POINTERW replacementBase = (BYTE*) replacement - prefix;
                    NUINT replacementSize = (NUINT) (prefix + new_size);
                    RESULT cleanup = catmm_extent_free<managed>(replacementBase, replacementSize);
                    if (!status_code_is_success(cleanup.status)) {
                        /* Retain ownership without touching the original payload. No
                         * further mapping is created until this cleanup has succeeded.
                         */
                        header.pending = replacementBase;
                        catmm_copy(base, &header, (NUINT) sizeof(header));
                        STATUS_CODE failureStatus = status_code_is_fatal(release.status) ? release.status : cleanup.status;
                        DETAIL_CODE failureDetail = release.status == cleanup.status ? (DETAIL_CODE) 0
                            : (failureStatus == release.status ? cleanup.status : release.status);
                        return RESULT(failureStatus, context, 0x0A, failureDetail);
                    }
                    return RESULT(release.status, context, 0x0B, release.detail);
                }
                *memory = replacement;
            }
        }
    }
    return RESULT(STATUS_CODE_SUCCESS, context, 0x07, 0);
}

template<BOOL managed>
static RESULT catmm_release(CATMMLIB_POINTERW memory) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x06 : (CONTEXT_CODE) 0x03;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    CATMMLIB_POINTERW base = (BYTE*) memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
    if ((managed || !TARGET_PLATFORM_WIN32) && header.pending != 0) {
        CATMMLIB_MEMORY_HEADER pendingHeader;
        catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
        RESULT cleanup = catmm_extent_free<managed>(header.pending, (NUINT) (prefix + pendingHeader.requested_size));
        if (!status_code_is_success(cleanup.status)) {
            return RESULT(cleanup.status, context, 0x05, cleanup.detail);
        }
        header.pending = 0;
        catmm_copy(base, &header, (NUINT) sizeof(header));
    }
    RESULT release = catmm_extent_free<managed>(base, (NUINT) (prefix + header.requested_size));
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, context, 0x03, release.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, context, 0x04, 0);
}

CATALYST_RESULT catmm_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return catmm_allocate<FALSE>(memory, size);
}

CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, NUINT new_size) {
    return catmm_resize<FALSE>(memory, new_size);
}

CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory) {
    return catmm_release<FALSE>(memory);
}

CATALYST_RESULT catmm_heap_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return catmm_allocate<TRUE>(memory, size);
}

CATALYST_RESULT catmm_heap_realloc(CATMMLIB_POINTERW* memory, NUINT new_size) {
    return catmm_resize<TRUE>(memory, new_size);
}

CATALYST_RESULT catmm_heap_free(CATMMLIB_POINTERW memory) {
    return catmm_release<TRUE>(memory);
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
#if CATMMLIB_FIXED_COPY
        /* Fixed-size compiler copies avoid out-of-line byte loops on x86. */
        __builtin_memcpy(&firstChunk, first, sizeof(firstChunk));
        __builtin_memcpy(&secondChunk, second, sizeof(secondChunk));
#else
        catmm_copy(&firstChunk, first, chunkSize);
        catmm_copy(&secondChunk, second, chunkSize);
#endif
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

void catmm_fill(CATMMLIB_POINTERW memory, CATMMLIB_WORD value, CATALYST_NUINT size) {
    BYTE* output = (BYTE*) memory;
#if CATMMLIB_FIXED_COPY
    const NUINT chunkSize = (NUINT) sizeof(CATMMLIB_COPY_WORD);
    if (size >= chunkSize) {
        CATMMLIB_COPY_WORD chunk;
        BYTE* bytes = (BYTE*) &chunk;
        for (NUINT i = 0; i < chunkSize; ++i) bytes[i] = value;
        do {
            __builtin_memcpy(output, &chunk, sizeof(chunk));
            output += chunkSize;
            size = (NUINT) (size - chunkSize);
        } while (size >= chunkSize);
    }
#endif
    while (size != 0) {
        *output++ = value;
        --size;
    }
}
