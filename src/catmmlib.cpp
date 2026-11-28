#include "catmmlib.internal.hpp"

using namespace catalyst;

/* -------------------------------------------------------------------------------------------------
 * MANAGED HEAP
 * ------------------------------------------------------------------------------------------------- */

#if CATMMLIB_MANAGED_HEAP
/* Payload size classes share slabs with bounded backing extents.
 * Slots hold an aligned private header followed by the native extent
 * (including the shared Core prefix).
 * Only partially occupied slabs enter the available list; one empty slab per
 * class is retained separately. Full slabs remain reachable from their slots.
 * Virgin slots are carved lazily.
 */
#if TARGET_SIZEOF_ADDRESS >= 4
static const NUINT catmm_slab_max = (NUINT) 524288;
static const NUINT catmm_payload_max = (NUINT) 65536;
#else
static const NUINT catmm_slab_max = (NUINT) 32768;
static const NUINT catmm_payload_max = (NUINT) 16384;
#endif
static const BYTE catmm_class_count = (BYTE) 16;

/* Store metadata as bytes in native storage, just like allocation headers.
 * Raw addresses identify metadata; only local objects are accessed as structs.
 */
struct CATMMLIB_HEAP_SLOT_HEADER {
    BYTE* owner;
    BYTE* next;
};

union CATMMLIB_HEAP_SLOT {
    CATMMLIB_HEAP_SLOT_HEADER header;
    CATMMLIB_NATIVE_PREFIX alignment;
};

struct CATMMLIB_HEAP_SLAB {
    BYTE* previous;
    BYTE* next;
    BYTE* available;
    BYTE* next_unused;
    NUINT unused;
    NUINT live;
    NUINT extent;
};

union CATMMLIB_HEAP_SLAB_PREFIX {
    CATMMLIB_HEAP_SLAB slab;
    CATMMLIB_HEAP_SLOT alignment;
};

struct CATMMLIB_HEAP_CLASS {
    BYTE* partial;
    BYTE* empty;
    BYTE lock;
};

typedef char CATMMLIB_slab_layout_fits[
    (sizeof(CATMMLIB_HEAP_SLAB_PREFIX) + sizeof(CATMMLIB_HEAP_SLOT) +
        sizeof(CATMMLIB_NATIVE_PREFIX) * 2 <= 4096 &&
        sizeof(CATMMLIB_HEAP_SLOT) % sizeof(CATMMLIB_NATIVE_PREFIX) == 0) ? 1 : -1];

static CATMMLIB_HEAP_CLASS catmm_classes[catmm_class_count];

/* Pad the slab prefix to the same extent granularity as the native prefix.
 * This also preserves 16-byte native heap alignment on Windows x64. */
static const NUINT catmm_slab_prefix = (NUINT) (((sizeof(CATMMLIB_HEAP_SLAB_PREFIX) + sizeof(CATMMLIB_NATIVE_PREFIX) - 1) /
    sizeof(CATMMLIB_NATIVE_PREFIX)) * sizeof(CATMMLIB_NATIVE_PREFIX));

/* Classify the payload without calculating slab geometry on reuse or release. */
static BYTE catmm_heap_class(NUINT payload, NUINT* capacity) {
    if (payload > catmm_payload_max) {
        *capacity = 0;
        return catmm_class_count;
    }
    if (payload <= (NUINT) 8) {
        *capacity = (NUINT) 8;
        return 0;
    }
    /* The class is the bit length of (payload - 1) / 8. Managed
     * heaps use at least two-byte addresses and at most 65536-byte
     * payload classes, so these shifts and the resulting class fit. */
    NUINT remaining = (NUINT) ((payload - (NUINT) 1) >> 3);
    BYTE index = 0;
    if (remaining >= (NUINT) 256) {
        remaining = (NUINT) (remaining >> 8);
        index = 8;
    }
    if (remaining >= (NUINT) 16) {
        remaining = (NUINT) (remaining >> 4);
        index = (BYTE) (index + (BYTE) 4);
    }
    if (remaining >= (NUINT) 4) {
        remaining = (NUINT) (remaining >> 2);
        index = (BYTE) (index + (BYTE) 2);
    }
    if (remaining >= (NUINT) 2) {
        remaining = (NUINT) (remaining >> 1);
        ++index;
    }
    ++index;
    *capacity = (NUINT) ((NUINT) 8 << index);
    return index;
}

/* Caller holds the lock; slab is not in the partial list. The caller writes
 * the updated local header back to its storage. */
static void catmm_heap_link(CATMMLIB_HEAP_CLASS* pool, BYTE* address, CATMMLIB_HEAP_SLAB* slab) {
    slab->previous = 0;
    slab->next = pool->partial;
    if (pool->partial != 0) {
        CATMMLIB_HEAP_SLAB next;
        catmm_copy(&next, pool->partial, (NUINT) sizeof(next));
        next.previous = address;
        catmm_copy(pool->partial, &next, (NUINT) sizeof(next));
    }
    pool->partial = address;
}

/* Caller holds the lock; neighbors describe a member of the partial list.
 * Only neighbors are accessed, so this also works after successful unmapping.
 */
static void catmm_heap_unlink(CATMMLIB_HEAP_CLASS* pool, BYTE* previous, BYTE* next) {
    if (previous != 0) {
        CATMMLIB_HEAP_SLAB header;
        catmm_copy(&header, previous, (NUINT) sizeof(header));
        header.next = next;
        catmm_copy(previous, &header, (NUINT) sizeof(header));
    } else {
        pool->partial = next;
    }
    if (next != 0) {
        CATMMLIB_HEAP_SLAB header;
        catmm_copy(&header, next, (NUINT) sizeof(header));
        header.previous = previous;
        catmm_copy(next, &header, (NUINT) sizeof(header));
    }
}

/* Only idle cached slabs are released at library teardown. This is best-effort:
 * there is no public operation or caller to receive a teardown failure.
 * Normal release failures are reported synchronously by heap release.
 */
#if defined(_MSC_VER)
namespace {
struct CATMMLIB_HEAP_LIFETIME {
    ~CATMMLIB_HEAP_LIFETIME();
};
static CATMMLIB_HEAP_LIFETIME catmm_heap_lifetime;
CATMMLIB_HEAP_LIFETIME::~CATMMLIB_HEAP_LIFETIME() {
#else
static void __attribute__((destructor)) catmm_heap_dispose() {
#endif
    /* Library teardown requires quiescent callers; never wait on a thread
     * that the operating system may already have terminated. */
    for (BYTE index = 0; index < catmm_class_count; ++index) {
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        if (pool->empty != 0) {
            CATMMLIB_HEAP_SLAB slab;
            catmm_copy(&slab, pool->empty, (NUINT) sizeof(slab));
            if (status_code_is_success(catmm_platform_free(pool->empty, slab.extent).status)) {
                pool->empty = 0;
            }
        }
    }
}
#if defined(_MSC_VER)
} /* anonymous namespace */
#endif
#endif /* CATMMLIB_MANAGED_HEAP */

/* overwrite bytes at the beginning of the payload will be filled by the
 * caller before the replacement can become public; ordinary allocations use zero. */
static RESULT catmm_heap_extent_alloc(CATMMLIB_POINTERW* memory, NUINT size, NUINT* backing_size, NUINT overwrite) {
    *memory = 0;
    CATMMLIB_POINTERW base = 0;
    RESULT allocation(STATUS_CODE_SUCCESS, 0x04, 0x02, 0);
#if CATMMLIB_MANAGED_HEAP
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    NUINT capacity;
    BYTE index = catmm_heap_class((NUINT) (size - prefix), &capacity);
    if (index != catmm_class_count) {
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        catmm_platform_lock(&pool->lock);
        BYTE* address = pool->partial;
        BOOL wasPartial = address != 0 ? TRUE : FALSE;
        CATMMLIB_HEAP_SLAB slab;
        if (address == 0) {
            address = pool->empty;
            if (address != 0) pool->empty = 0;
        }
        if (address != 0) {
            catmm_copy(&slab, address, (NUINT) sizeof(slab));
        } else {
            NUINT slotCapacity = (NUINT) (((capacity + prefix + prefix - (NUINT) 1) / prefix) * prefix);
            NUINT stride = (NUINT) (slotCapacity + (NUINT) sizeof(CATMMLIB_HEAP_SLOT));
            NUINT slots = (NUINT) ((catmm_slab_max - catmm_slab_prefix) / stride);
            NUINT limit = (NUINT) 64;
#if TARGET_SIZEOF_ADDRESS >= 4
            /* Small payloads dominate composite creation. Retain one larger
             * slab rather than repeatedly map/unmap many small slabs.
             * Keep both the existing extent bound and the narrow-target policy. */
            if (capacity <= (NUINT) 32) limit = (NUINT) 1024;
#endif
            if (slots > limit) slots = limit;
            NUINT extent = (NUINT) (catmm_slab_prefix + stride * slots);
            if (extent < (NUINT) 4096) extent = (NUINT) 4096;
            allocation = catmm_platform_alloc(&base, extent);
            if (status_code_is_success(allocation.status)) {
                address = (BYTE*) base;
                slab.previous = 0;
                slab.next = 0;
                slab.available = 0;
                slab.next_unused = address + catmm_slab_prefix;
                slab.unused = (NUINT) ((extent - catmm_slab_prefix) /
                    stride);
                slab.live = 0;
                slab.extent = extent;
            }
        }
        BOOL reused = FALSE;
        if (address != 0) {
            BYTE* slot = slab.available;
            CATMMLIB_HEAP_SLOT_HEADER slotHeader;
            if (slot != 0) {
                catmm_copy(&slotHeader, slot, (NUINT) sizeof(slotHeader));
                slab.available = slotHeader.next;
                reused = TRUE;
            } else {
                slot = slab.next_unused;
                slotHeader.owner = address;
                slotHeader.next = 0;
                catmm_copy(slot, &slotHeader, (NUINT) sizeof(slotHeader));
                NUINT slotCapacity = (NUINT) (((capacity + prefix + prefix - (NUINT) 1) / prefix) * prefix);
                slab.next_unused += (NUINT) (sizeof(CATMMLIB_HEAP_SLOT) + slotCapacity);
                --slab.unused;
            }
            ++slab.live;
            if (slab.available == 0 && slab.unused == 0) {
                if (wasPartial) catmm_heap_unlink(pool, slab.previous, slab.next);
            } else if (!wasPartial) {
                catmm_heap_link(pool, address, &slab);
            }
            catmm_copy(address, &slab, (NUINT) sizeof(slab));
            base = slot + sizeof(CATMMLIB_HEAP_SLOT);
            *backing_size = capacity;
        }
        catmm_platform_unlock(&pool->lock);
        /* The live slot now belongs exclusively to this allocation. Clear
         * reused storage outside the lock, before publishing the pointer.
         */
        NUINT payload = (NUINT) (size - prefix);
        if (reused && overwrite < payload) {
            catmm_fill((BYTE*) base + prefix + overwrite, (BYTE) 0, (NUINT) (payload - overwrite));
        }
    } else
#endif
    {
        (void) overwrite;
        allocation = catmm_platform_alloc(&base, size);
    }
    if (!status_code_is_success(allocation.status)) {
        return RESULT(allocation.status, 0x04, 0x01, allocation.detail);
    }
    *memory = base;
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x02, 0);
}

static RESULT catmm_heap_extent_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size) {
    RESULT release(STATUS_CODE_SUCCESS, 0x06, 0x02, 0);
#if CATMMLIB_MANAGED_HEAP
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    NUINT capacity;
    BYTE index = catmm_heap_class((NUINT) (size - prefix), &capacity);
    if (index != catmm_class_count) {
        BYTE* slot = (BYTE*) memory - sizeof(CATMMLIB_HEAP_SLOT);
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        catmm_platform_lock(&pool->lock);
        CATMMLIB_HEAP_SLOT_HEADER slotHeader;
        catmm_copy(&slotHeader, slot, (NUINT) sizeof(slotHeader));
        BYTE* address = slotHeader.owner;
        CATMMLIB_HEAP_SLAB slab;
        catmm_copy(&slab, address, (NUINT) sizeof(slab));
        BOOL wasPartial = slab.available != 0 || slab.unused != 0 ? TRUE : FALSE;
        if (slab.live == (NUINT) 1 && pool->empty != 0) {
            /* Reclaim a surplus empty slab. Do not modify the live allocation
             * or its list membership until native release succeeds.
             */
            release = catmm_platform_free(address, slab.extent);
            if (status_code_is_success(release.status) && wasPartial) {
                catmm_heap_unlink(pool, slab.previous, slab.next);
            }
        } else {
            slotHeader.next = slab.available;
            catmm_copy(slot, &slotHeader, (NUINT) sizeof(slotHeader));
            slab.available = slot;
            --slab.live;
            if (slab.live == 0) {
                if (wasPartial) catmm_heap_unlink(pool, slab.previous, slab.next);
                pool->empty = address;
            } else if (!wasPartial) {
                catmm_heap_link(pool, address, &slab);
            }
            catmm_copy(address, &slab, (NUINT) sizeof(slab));
        }
        catmm_platform_unlock(&pool->lock);
    } else
#endif
    {
        release = catmm_platform_free(memory, size);
    }
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x06, 0x01, release.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x02, 0);
}

/* -------------------------------------------------------------------------------------------------
 * MEMORY ALLOCATION & MANAGEMENT
 * ------------------------------------------------------------------------------------------------- */

/* Contexts: native alloc/realloc/free 01/02/03; heap alloc/realloc/free 04/05/06. Every reporting site has a distinct
 * operation within its context. Native backend sites reserve 01/02 (and alloc
 * 06/07 for page-rounding overflow/unsupported page size). Shared resize 0A
 * reports cleanup failure (or a fatal original-release failure); detail is the
 * other failure's status when different, zero otherwise.
 * Shared operations are assigned below.
 */

/* overwrite is at most size and nonzero only for private resize replacements. */
static RESULT catmm_allocate(CATMMLIB_POINTERW* memory, NUINT size, BOOL managed, NUINT overwrite) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x04 : (CONTEXT_CODE) 0x01;
    *memory = 0;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    if (size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x03, 0);
    }
    CATMMLIB_POINTERW base = 0;
    NUINT backingSize = size;
    RESULT allocation = managed
        ? catmm_heap_extent_alloc(&base, (NUINT) (prefix + size), &backingSize, overwrite)
        : catmm_platform_alloc(&base, (NUINT) (prefix + size));
    if (!status_code_is_success(allocation.status)) {
        return RESULT(allocation.status, context, 0x04, allocation.detail);
    }
    CATMMLIB_MEMORY_HEADER header;
    header.requested_size = size;
    header.backing_size = backingSize;
    header.pending = 0;
    catmm_copy(base, &header, (NUINT) sizeof(header));
    BYTE* payload = (BYTE*) base + prefix;
    *memory = payload;
    return RESULT(STATUS_CODE_SUCCESS, context, 0x05, 0);
}

static RESULT catmm_release(CATMMLIB_POINTERW memory, BOOL managed) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x06 : (CONTEXT_CODE) 0x03;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    CATMMLIB_POINTERW base = (BYTE*) memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
    if ((managed || !TARGET_PLATFORM_WIN32) && header.pending != 0) {
        CATMMLIB_MEMORY_HEADER pendingHeader;
        catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
        NUINT pendingSize = (NUINT) (prefix + pendingHeader.backing_size);
        RESULT cleanup = managed ? catmm_heap_extent_free(header.pending, pendingSize)
            : catmm_platform_free(header.pending, pendingSize);
        if (!status_code_is_success(cleanup.status)) {
            return RESULT(cleanup.status, context, 0x05, cleanup.detail);
        }
        header.pending = 0;
        catmm_copy(base, &header, (NUINT) sizeof(header));
    }
    NUINT extent = (NUINT) (prefix + header.backing_size);
    RESULT release = managed ? catmm_heap_extent_free(base, extent) : catmm_platform_free(base, extent);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, context, 0x03, release.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, context, 0x04, 0);
}

static RESULT catmm_resize(CATMMLIB_POINTERW* memory, NUINT new_size, BOOL managed) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x05 : (CONTEXT_CODE) 0x02;
    if (*memory == 0) {
        RESULT allocation = catmm_allocate(memory, new_size, managed, 0);
        return RESULT(allocation.status, context, 0x03, allocation.detail);
    }
    if (new_size == 0) {
        RESULT release = catmm_release(*memory, managed);
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
            header.backing_size = new_size;
            catmm_copy(base, &header, (NUINT) sizeof(header));
            *memory = (BYTE*) base + prefix;
        }
    } else
#endif
    {
        if (header.pending != 0) {
            CATMMLIB_MEMORY_HEADER pendingHeader;
            catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
            NUINT pendingSize = (NUINT) (prefix + pendingHeader.backing_size);
            RESULT cleanup = managed ? catmm_heap_extent_free(header.pending, pendingSize)
                : catmm_platform_free(header.pending, pendingSize);
            if (!status_code_is_success(cleanup.status)) {
                return RESULT(cleanup.status, context, 0x08, cleanup.detail);
            }
            header.pending = 0;
            catmm_copy(base, &header, (NUINT) sizeof(header));
        }
        if (new_size != header.requested_size) {
            NUINT oldExtent = (NUINT) (prefix + header.backing_size);
            NUINT newExtent = (NUINT) (prefix + new_size);
            BOOL matches = managed && new_size <= header.backing_size &&
                (new_size >= header.backing_size / (NUINT) 2 || header.backing_size <= (NUINT) 8) ? TRUE : FALSE;
#if TARGET_PLATFORM_DARWIN
            if (!managed) matches = catmm_platform_extent_matches(oldExtent, newExtent);
#endif
            if (matches) {
                /* Shrink retains the native storage; clear bytes re-exposed by a
                 * later growth, since they may contain the caller's old data.
                 */
                if (new_size > header.requested_size) {
                    catmm_fill((BYTE*) *memory + header.requested_size, (BYTE) 0, (NUINT) (new_size - header.requested_size));
                }
                if (managed) {
                    /* Capacity and pending ownership are unchanged. */
                    catmm_copy(base, &new_size, (NUINT) sizeof(new_size));
                } else {
                    header.requested_size = new_size;
                    header.backing_size = new_size;
                    catmm_copy(base, &header, (NUINT) sizeof(header));
                }
#if TARGET_PLATFORM_DARWIN
            } else if (!managed && new_size < header.requested_size) {
                RESULT shrink = catmm_platform_shrink(base, oldExtent, newExtent);
                if (!status_code_is_success(shrink.status)) {
                    return RESULT(shrink.status, context, 0x06, shrink.detail);
                }
                header.requested_size = new_size;
                header.backing_size = new_size;
                catmm_copy(base, &header, (NUINT) sizeof(header));
            } else if (!managed && catmm_platform_try_grow(base, oldExtent, newExtent)) {
                header.requested_size = new_size;
                header.backing_size = new_size;
                catmm_copy(base, &header, (NUINT) sizeof(header));
#endif
            } else {
                CATMMLIB_POINTERW replacement = 0;
                NUINT retained = header.requested_size < new_size ? header.requested_size : new_size;
                RESULT allocation = catmm_allocate(&replacement, new_size, managed, retained);
                if (!status_code_is_success(allocation.status)) {
                    return RESULT(allocation.status, context, 0x09, allocation.detail);
                }
                catmm_copy(replacement, *memory, retained);
                RESULT release = managed ? catmm_heap_extent_free(base, oldExtent)
                    : catmm_platform_free(base, oldExtent);
                if (!status_code_is_success(release.status)) {
                    CATMMLIB_POINTERW replacementBase = (BYTE*) replacement - prefix;
                    NUINT replacementSize = (NUINT) (prefix + new_size);
                    RESULT cleanup = managed ? catmm_heap_extent_free(replacementBase, replacementSize)
                        : catmm_platform_free(replacementBase, replacementSize);
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

CATALYST_RESULT catmm_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return catmm_allocate(memory, size, FALSE, 0);
}

CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, NUINT new_size) {
    return catmm_resize(memory, new_size, FALSE);
}

CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory) {
    return catmm_release(memory, FALSE);
}

CATALYST_RESULT catmm_heap_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return catmm_allocate(memory, size, TRUE, 0);
}

CATALYST_RESULT catmm_heap_realloc(CATMMLIB_POINTERW* memory, NUINT new_size) {
    return catmm_resize(memory, new_size, TRUE);
}

CATALYST_RESULT catmm_heap_free(CATMMLIB_POINTERW memory) {
    return catmm_release(memory, TRUE);
}

/* -------------------------------------------------------------------------------------------------
 * MEMORY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* The configured target must have flat byte addresses. Block sizes derive
 * from the address width, not the processor name. The byte loops remain the
 * complete implementation when compiler block copies are unavailable. */
#if defined(__GNUC__) || defined(__clang__)
#define CATMMLIB_BLOCK_COPY 1
#define CATMMLIB_BLOCK_SIZE (TARGET_SIZEOF_ADDRESS * 4)
#define CATMMLIB_BLOCK_ALIGNMENT (TARGET_SIZEOF_ADDRESS * 2)
#define CATMMLIB_LARGE_BLOCK_SIZE (CATMMLIB_BLOCK_SIZE * 2)
#define CATMMLIB_BLOCK_THRESHOLD (CATMMLIB_BLOCK_SIZE * 8)
#else
#define CATMMLIB_BLOCK_COPY 0
#endif

#if CATMMLIB_BLOCK_COPY
/* Read all units before writing. Fixed-size units let compilers retain the
 * transfer in registers instead of spilling a single large temporary. */
static inline void catmm_copy_block(BYTE* output, const BYTE* input, BOOL large) {
    BYTE first[CATMMLIB_BLOCK_ALIGNMENT], second[CATMMLIB_BLOCK_ALIGNMENT];
    __builtin_memcpy(first, input, sizeof(first));
    __builtin_memcpy(second, input + (NUINT) CATMMLIB_BLOCK_ALIGNMENT, sizeof(second));
    if (large) {
        BYTE third[CATMMLIB_BLOCK_ALIGNMENT], fourth[CATMMLIB_BLOCK_ALIGNMENT];
        __builtin_memcpy(third, input + (NUINT) CATMMLIB_BLOCK_SIZE, sizeof(third));
        __builtin_memcpy(fourth, input + (NUINT) CATMMLIB_BLOCK_SIZE + (NUINT) CATMMLIB_BLOCK_ALIGNMENT, sizeof(fourth));
        __builtin_memcpy(output + (NUINT) CATMMLIB_BLOCK_SIZE, third, sizeof(third));
        __builtin_memcpy(output + (NUINT) CATMMLIB_BLOCK_SIZE + (NUINT) CATMMLIB_BLOCK_ALIGNMENT, fourth, sizeof(fourth));
    }
    __builtin_memcpy(output, first, sizeof(first));
    __builtin_memcpy(output + (NUINT) CATMMLIB_BLOCK_ALIGNMENT, second, sizeof(second));
}
/* Read both ends before writing either end. A valid extent between Width
 * and twice Width bytes is covered even when the source and destination
 * overlap in either direction. The duplicated middle bytes agree. */
template<NUINT Width>
static inline void catmm_copy_ends(BYTE* output, const BYTE* input, NUINT size) {
    BYTE first[Width], last[Width];
    NUINT offset = (NUINT) (size - Width);
    __builtin_memcpy(first, input, Width);
    __builtin_memcpy(last, input + offset, Width);
    __builtin_memcpy(output, first, Width);
    __builtin_memcpy(output + offset, last, Width);
}

/* Valid extents of at most 32 bytes. Constant-width compiler copies avoid
 * byte loops and preserve arbitrary overlap without an address comparison. */
static inline void catmm_copy_short(BYTE* output, const BYTE* input, NUINT size) {
    if (size >= (NUINT) 16) {
        catmm_copy_ends<(NUINT) 16>(output, input, size);
    } else if (size >= (NUINT) 8) {
        catmm_copy_ends<(NUINT) 8>(output, input, size);
    } else if (size >= (NUINT) 4) {
        catmm_copy_ends<(NUINT) 4>(output, input, size);
    } else if (size >= (NUINT) 2) {
        catmm_copy_ends<(NUINT) 2>(output, input, size);
    } else if (size != 0) {
        *output = *input;
    }
}
#endif

/* Copy forward when no unread source byte can be overwritten. */
static inline void catmm_copy_forward(BYTE* output, const BYTE* input, NUINT size) {
#if CATMMLIB_BLOCK_COPY
    /* Fixed-size compiler copies permit unaligned input/output. Avoid byte
     * alignment prologues for the short and medium shifts used by collections. */
    if (size >= (NUINT) CATMMLIB_BLOCK_THRESHOLD) {
        while (size >= (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2)) {
            catmm_copy_block(output, input, TRUE);
            catmm_copy_block(output + (NUINT) CATMMLIB_LARGE_BLOCK_SIZE,
                input + (NUINT) CATMMLIB_LARGE_BLOCK_SIZE, TRUE);
            output += (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2);
            input += (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2);
            size = (NUINT) (size - (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2));
        }
    }
    while (size >= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE) {
        catmm_copy_block(output, input, TRUE);
        output += (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
        input += (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
        size = (NUINT) (size - (NUINT) CATMMLIB_LARGE_BLOCK_SIZE);
    }
    while (size >= (NUINT) CATMMLIB_BLOCK_SIZE) {
        catmm_copy_block(output, input, FALSE);
        output += (NUINT) CATMMLIB_BLOCK_SIZE;
        input += (NUINT) CATMMLIB_BLOCK_SIZE;
        size = (NUINT) (size - (NUINT) CATMMLIB_BLOCK_SIZE);
    }
    while (size > (NUINT) 32) {
        *output++ = *input++;
        --size;
    }
    catmm_copy_short(output, input, size);
#else
    while (size != 0) {
        *output++ = *input++;
        --size;
    }
#endif
}

void catmm_copy(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (destination == source) return;
    catmm_copy_forward((BYTE*) destination, (const BYTE*) source, size);
}

void catmm_move(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (size == 0 || destination == source) return;
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
#if CATMMLIB_BLOCK_COPY
    if (size <= (NUINT) 32) {
        catmm_copy_short(output, input, size);
        return;
    }
#endif
    /* Supported targets use flat byte addresses. Subtract only in ascending
     * order; computing the end address could overflow the representation. */
    const CATMMLIB_ADDRESS outputAddress = (CATMMLIB_ADDRESS) destination;
    const CATMMLIB_ADDRESS inputAddress = (CATMMLIB_ADDRESS) source;
    BOOL backward = outputAddress > inputAddress && (CATMMLIB_ADDRESS) (outputAddress - inputAddress) < (CATMMLIB_ADDRESS) size;
    if (!backward) {
        catmm_copy_forward(output, input, size);
        return;
    }
    output += size;
    input += size;
#if CATMMLIB_BLOCK_COPY
    /* Process the higher block first so overlapping right shifts cannot
     * overwrite the next block's unread source bytes. */
    if (size >= (NUINT) CATMMLIB_BLOCK_THRESHOLD) {
        while (size >= (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2)) {
            output -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
            input -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
            catmm_copy_block(output, input, TRUE);
            output -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
            input -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
            catmm_copy_block(output, input, TRUE);
            size = (NUINT) (size - (NUINT) (CATMMLIB_LARGE_BLOCK_SIZE * 2));
        }
    }
    while (size >= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE) {
        output -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
        input -= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
        catmm_copy_block(output, input, TRUE);
        size = (NUINT) (size - (NUINT) CATMMLIB_LARGE_BLOCK_SIZE);
    }
    while (size >= (NUINT) CATMMLIB_BLOCK_SIZE) {
        output -= (NUINT) CATMMLIB_BLOCK_SIZE;
        input -= (NUINT) CATMMLIB_BLOCK_SIZE;
        catmm_copy_block(output, input, FALSE);
        size = (NUINT) (size - (NUINT) CATMMLIB_BLOCK_SIZE);
    }
    while (size > (NUINT) 32) {
        *--output = *--input;
        --size;
    }
    catmm_copy_short(output - size, input - size, size);
#else
    while (size != 0) {
        *--output = *--input;
        --size;
    }
#endif
}

CATALYST_SBYTE catmm_compare(CATMMLIB_POINTER memory1, CATMMLIB_POINTER memory2, CATALYST_NUINT size) {
    const BYTE* first = (const BYTE*) memory1;
    const BYTE* second = (const BYTE*) memory2;
    if (memory1 == memory2) return (SBYTE) 0;
#if CATMMLIB_BLOCK_COPY
#if CATALYST_SUPPORTS_64BIT
    typedef ULONG CATMMLIB_COMPARE_WORD;
#elif CATALYST_SUPPORTS_32BIT
    typedef UINT CATMMLIB_COMPARE_WORD;
#elif CATALYST_SUPPORTS_16BIT
    typedef USHORT CATMMLIB_COMPARE_WORD;
#endif
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT || CATALYST_SUPPORTS_16BIT
    const BYTE width = (BYTE) sizeof(CATMMLIB_COMPARE_WORD);
    const BYTE block = (BYTE) (width * (BYTE) 4);
    while (size >= (NUINT) block) {
        CATMMLIB_COMPARE_WORD firstChunk[4], secondChunk[4];
        __builtin_memcpy(firstChunk, first, sizeof(firstChunk));
        __builtin_memcpy(secondChunk, second, sizeof(secondChunk));
        if (((firstChunk[0] ^ secondChunk[0]) | (firstChunk[1] ^ secondChunk[1]) |
            (firstChunk[2] ^ secondChunk[2]) | (firstChunk[3] ^ secondChunk[3])) != 0) break;
        first += block;
        second += block;
        size = (NUINT) (size - (NUINT) block);
    }
    while (size >= (NUINT) width) {
        CATMMLIB_COMPARE_WORD firstChunk, secondChunk;
        __builtin_memcpy(&firstChunk, first, sizeof(firstChunk));
        __builtin_memcpy(&secondChunk, second, sizeof(secondChunk));
        // Equality is independent of byte order; resolve mismatches in byte order.
        if (firstChunk != secondChunk) {
#if CATALYST_SUPPORTS_64BIT && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            firstChunk = (CATMMLIB_COMPARE_WORD) __builtin_bswap64(firstChunk);
            secondChunk = (CATMMLIB_COMPARE_WORD) __builtin_bswap64(secondChunk);
            return firstChunk < secondChunk ? (SBYTE) -1 : (SBYTE) 1;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            return firstChunk < secondChunk ? (SBYTE) -1 : (SBYTE) 1;
#endif
#endif
            break;
        }
        first += width;
        second += width;
        size = (NUINT) (size - (NUINT) width);
    }
#endif
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
#if CATMMLIB_BLOCK_COPY
    if (size >= (NUINT) CATMMLIB_BLOCK_SIZE) {
        BYTE chunk[CATMMLIB_BLOCK_SIZE];
        for (BYTE i = 0; i < (BYTE) CATMMLIB_BLOCK_SIZE; ++i) chunk[i] = value;
        while (size >= (NUINT) CATMMLIB_LARGE_BLOCK_SIZE) {
            __builtin_memcpy(output, chunk, sizeof(chunk));
            __builtin_memcpy(output + (NUINT) CATMMLIB_BLOCK_SIZE, chunk, sizeof(chunk));
            output += (NUINT) CATMMLIB_LARGE_BLOCK_SIZE;
            size = (NUINT) (size - (NUINT) CATMMLIB_LARGE_BLOCK_SIZE);
        }
        if (size >= (NUINT) CATMMLIB_BLOCK_SIZE) {
            __builtin_memcpy(output, chunk, sizeof(chunk));
            output += (NUINT) CATMMLIB_BLOCK_SIZE;
            size = (NUINT) (size - (NUINT) CATMMLIB_BLOCK_SIZE);
        }
    }
#endif
    while (size != 0) {
        *output++ = value;
        --size;
    }
}
