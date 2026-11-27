#include "catmmlib.internal.hpp"

/* The slab algorithm is platform-independent. Only native storage and
 * thread yielding use the platform backend. Unsupported address widths or
 * compilers retain native allocation through the same heap API. */
#if TARGET_SIZEOF_ADDRESS >= 2 && ((defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64))) || (defined(__GCC_ATOMIC_CHAR_LOCK_FREE) && __GCC_ATOMIC_CHAR_LOCK_FREE == 2))
#define CATMMLIB_MANAGED_HEAP 1
#include <new>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#else
#define CATMMLIB_MANAGED_HEAP 0
#endif

using namespace catalyst;

/* -------------------------------------------------------------------------------------------------
 * MANAGED HEAP
 * ------------------------------------------------------------------------------------------------- */

#if CATMMLIB_MANAGED_HEAP
/* Small classes share slabs; larger classes use native spans with one or two slots.
 * Slots hold an aligned private header followed by the native extent
 * (including the shared Core prefix).
 * Only partially occupied slabs enter the available list; one empty slab per
 * class is retained separately. Full slabs remain reachable from their slots.
 * Virgin slots are carved lazily.
 */
static const NUINT catmm_slab_max = (NUINT) 32768;
static const BYTE catmm_class_count = (BYTE) 16;

struct CATMMLIB_HEAP_SLAB;
union CATMMLIB_HEAP_SLOT {
    struct {
        CATMMLIB_HEAP_SLAB* owner;
        CATMMLIB_HEAP_SLOT* next;
    } links;
    CATMMLIB_NATIVE_PREFIX alignment;
};

struct CATMMLIB_HEAP_SLAB {
    CATMMLIB_HEAP_SLAB* previous;
    CATMMLIB_HEAP_SLAB* next;
    CATMMLIB_HEAP_SLOT* available;
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
    CATMMLIB_HEAP_SLAB* partial;
    CATMMLIB_HEAP_SLAB* empty;
    BYTE lock;
};

typedef char CATMMLIB_slab_layout_fits[
    (sizeof(CATMMLIB_HEAP_SLAB_PREFIX) + sizeof(CATMMLIB_HEAP_SLOT) +
        catmm_slab_max / (NUINT) 4 <= catmm_slab_max &&
        sizeof(CATMMLIB_HEAP_SLAB_PREFIX) + sizeof(CATMMLIB_HEAP_SLOT) + 1024 <= 4096 &&
        sizeof(CATMMLIB_HEAP_SLOT) % sizeof(CATMMLIB_NATIVE_PREFIX) == 0) ? 1 : -1];

static CATMMLIB_HEAP_CLASS catmm_classes[catmm_class_count];

/* Pad the slab prefix to the same extent granularity as the native prefix.
 * This also preserves 16-byte native heap alignment on Windows x64. */
static const NUINT catmm_slab_prefix = (NUINT) (((sizeof(CATMMLIB_HEAP_SLAB_PREFIX) + sizeof(CATMMLIB_NATIVE_PREFIX) - 1) /
    sizeof(CATMMLIB_NATIVE_PREFIX)) * sizeof(CATMMLIB_NATIVE_PREFIX));

/* Each class protects its own lists and counts with a platform-local lock. */
static void catmm_heap_acquire(CATMMLIB_HEAP_CLASS* pool) {
#if defined(_MSC_VER)
    while (_InterlockedExchange8((volatile char*) &pool->lock, 1) != 0) {
        catmm_platform_yield();
    }
#else
    while (__atomic_exchange_n(&pool->lock, (BYTE) 1, __ATOMIC_ACQUIRE) != 0) {
        while (__atomic_load_n(&pool->lock, __ATOMIC_RELAXED) != 0) {
            catmm_platform_yield();
        }
    }
#endif
}

/* Caller holds the heap lock. */
static void catmm_heap_release(CATMMLIB_HEAP_CLASS* pool) {
#if defined(_MSC_VER)
    _InterlockedExchange8((volatile char*) &pool->lock, 0);
#else
    __atomic_store_n(&pool->lock, (BYTE) 0, __ATOMIC_RELEASE);
#endif
}

/* Nonzero native extent. Return its class and layout, or class_count for
 * a direct mapping (zero outputs). No mutable heap state is read.
 */
static BYTE catmm_heap_class(NUINT size, NUINT* capacity, NUINT* extent) {
    NUINT current = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    BYTE index = 0;
    for (; index < catmm_class_count && current <= catmm_slab_max / (NUINT) 4; ++index) {
        if (size <= current) {
            *capacity = current;
            NUINT slabSize = (NUINT) 4096;
            while (current > slabSize / (NUINT) 4) slabSize = (NUINT) (slabSize + slabSize);
            *extent = slabSize;
            return index;
        }
        current = (NUINT) (current + current);
    }
    /* Span classes hold one or two blocks of nominal 16, 32 or 64 KiB.
     * Two medium blocks can share one mapping; the largest class retains one.
     */
    const NUINT overhead = (NUINT) (catmm_slab_prefix + (NUINT) sizeof(CATMMLIB_HEAP_SLOT));
    NUINT span = (NUINT) 16384;
    for (BYTE count = 0; count < (BYTE) 3 && index < catmm_class_count; ++count, ++index) {
        NUINT spanCapacity = (NUINT) (span - overhead);
        spanCapacity = (NUINT) (spanCapacity - (NUINT) (spanCapacity % (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX)));
        if (size <= spanCapacity) {
            *capacity = spanCapacity;
            *extent = span;
            if (count < (BYTE) 2 && span <= CATALYST_NUINT_MAX / (NUINT) 2) {
                *extent = (NUINT) (span + span);
            }
            return index;
        }
        if (count == (BYTE) 2 || span > CATALYST_NUINT_MAX / (NUINT) 2) break;
        span = (NUINT) (span + span);
    }
    *capacity = 0;
    *extent = 0;
    return catmm_class_count;
}

/* Caller holds the lock; slab is not in the partial list. */
static void catmm_heap_link(CATMMLIB_HEAP_CLASS* pool, CATMMLIB_HEAP_SLAB* slab) {
    slab->previous = 0;
    slab->next = pool->partial;
    if (pool->partial != 0) pool->partial->previous = slab;
    pool->partial = slab;
}

/* Caller holds the lock; neighbors describe a member of the partial list.
 * Only neighbors are accessed, so this also works after successful unmapping.
 */
static void catmm_heap_unlink(CATMMLIB_HEAP_CLASS* pool, CATMMLIB_HEAP_SLAB* previous, CATMMLIB_HEAP_SLAB* next) {
    if (previous != 0) previous->next = next;
    else pool->partial = next;
    if (next != 0) next->previous = previous;
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
        if (pool->empty != 0 && status_code_is_success(catmm_platform_free(pool->empty, pool->empty->extent).status)) {
            pool->empty = 0;
        }
    }
}
#if defined(_MSC_VER)
} /* anonymous namespace */
#endif
#endif /* CATMMLIB_MANAGED_HEAP */

static BOOL catmm_heap_extent_matches(CATALYST_NUINT old_size, CATALYST_NUINT new_size) {
#if CATMMLIB_MANAGED_HEAP
    NUINT capacity, extent;
    BYTE oldClass = catmm_heap_class(old_size, &capacity, &extent);
    if (oldClass != catmm_class_count) {
        return oldClass == catmm_heap_class(new_size, &capacity, &extent) ? TRUE : FALSE;
    }
#else
    (void) old_size;
    (void) new_size;
#endif
    return FALSE;
}

static RESULT catmm_heap_extent_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
    *memory = 0;
    CATMMLIB_POINTERW base = 0;
    RESULT allocation(STATUS_CODE_SUCCESS, 0x04, 0x02, 0);
#if CATMMLIB_MANAGED_HEAP
    NUINT capacity, extent;
    BYTE index = catmm_heap_class(size, &capacity, &extent);
    if (index != catmm_class_count) {
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        catmm_heap_acquire(pool);
        CATMMLIB_HEAP_SLAB* slab = pool->partial;
        BOOL wasPartial = slab != 0 ? TRUE : FALSE;
        if (slab == 0) {
            slab = pool->empty;
            if (slab != 0) {
                pool->empty = 0;
            } else {
                allocation = catmm_platform_alloc(&base, extent);
                if (status_code_is_success(allocation.status)) {
                    /* Construct the slab in native storage without allocating again. */
                    CATMMLIB_HEAP_SLAB_PREFIX* prefix = new (base) CATMMLIB_HEAP_SLAB_PREFIX;
                    slab = &prefix->slab;
                    slab->previous = 0;
                    slab->next = 0;
                    slab->available = 0;
                    slab->next_unused = (BYTE*) base + catmm_slab_prefix;
                    slab->unused = (NUINT) ((extent - catmm_slab_prefix) /
                        (NUINT) (sizeof(CATMMLIB_HEAP_SLOT) + capacity));
                    slab->live = 0;
                    slab->extent = extent;
                }
            }
        }
        BOOL reused = FALSE;
        if (slab != 0) {
            CATMMLIB_HEAP_SLOT* slot = slab->available;
            if (slot != 0) {
                slab->available = slot->links.next;
                reused = TRUE;
            } else {
                slot = new (slab->next_unused) CATMMLIB_HEAP_SLOT;
                slot->links.owner = slab;
                slab->next_unused += (NUINT) (sizeof(CATMMLIB_HEAP_SLOT) + capacity);
                --slab->unused;
            }
            ++slab->live;
            if (slab->available == 0 && slab->unused == 0) {
                if (wasPartial) catmm_heap_unlink(pool, slab->previous, slab->next);
            } else if (!wasPartial) {
                catmm_heap_link(pool, slab);
            }
            base = (BYTE*) slot + sizeof(CATMMLIB_HEAP_SLOT);
        }
        catmm_heap_release(pool);
        /* The live slot now belongs exclusively to this allocation. Clear
         * reused storage outside the lock, before publishing the pointer.
         */
        if (reused) catmm_fill(base, (BYTE) 0, size);
    } else
#endif
    {
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
    NUINT capacity, extent;
    BYTE index = catmm_heap_class(size, &capacity, &extent);
    if (index != catmm_class_count) {
        CATMMLIB_HEAP_SLOT* slot = (CATMMLIB_HEAP_SLOT*) ((BYTE*) memory - sizeof(CATMMLIB_HEAP_SLOT));
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        catmm_heap_acquire(pool);
        CATMMLIB_HEAP_SLAB* slab = slot->links.owner;
        BOOL wasPartial = slab->available != 0 || slab->unused != 0 ? TRUE : FALSE;
        if (slab->live == (NUINT) 1 && pool->empty != 0) {
            /* Reclaim a surplus empty slab. Do not modify the live allocation
             * or its list membership until native release succeeds.
             */
            CATMMLIB_HEAP_SLAB* previous = slab->previous;
            CATMMLIB_HEAP_SLAB* next = slab->next;
            release = catmm_platform_free(slab, slab->extent);
            if (status_code_is_success(release.status) && wasPartial) {
                catmm_heap_unlink(pool, previous, next);
            }
        } else {
            slot->links.next = slab->available;
            slab->available = slot;
            --slab->live;
            if (slab->live == 0) {
                if (wasPartial) catmm_heap_unlink(pool, slab->previous, slab->next);
                pool->empty = slab;
            } else if (!wasPartial) {
                catmm_heap_link(pool, slab);
            }
        }
        catmm_heap_release(pool);
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

static RESULT catmm_allocate(CATMMLIB_POINTERW* memory, NUINT size, BOOL managed) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x04 : (CONTEXT_CODE) 0x01;
    *memory = 0;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    if (size > (NUINT) (CATALYST_NUINT_MAX - prefix)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x03, 0);
    }
    CATMMLIB_POINTERW base = 0;
    RESULT allocation = managed
        ? catmm_heap_extent_alloc(&base, (NUINT) (prefix + size))
        : catmm_platform_alloc(&base, (NUINT) (prefix + size));
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

static RESULT catmm_release(CATMMLIB_POINTERW memory, BOOL managed) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x06 : (CONTEXT_CODE) 0x03;
    const NUINT prefix = (NUINT) sizeof(CATMMLIB_NATIVE_PREFIX);
    CATMMLIB_POINTERW base = (BYTE*) memory - prefix;
    CATMMLIB_MEMORY_HEADER header;
    catmm_copy(&header, base, (NUINT) sizeof(header));
    if ((managed || !TARGET_PLATFORM_WIN32) && header.pending != 0) {
        CATMMLIB_MEMORY_HEADER pendingHeader;
        catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
        NUINT pendingSize = (NUINT) (prefix + pendingHeader.requested_size);
        RESULT cleanup = managed ? catmm_heap_extent_free(header.pending, pendingSize)
            : catmm_platform_free(header.pending, pendingSize);
        if (!status_code_is_success(cleanup.status)) {
            return RESULT(cleanup.status, context, 0x05, cleanup.detail);
        }
        header.pending = 0;
        catmm_copy(base, &header, (NUINT) sizeof(header));
    }
    NUINT extent = (NUINT) (prefix + header.requested_size);
    RESULT release = managed ? catmm_heap_extent_free(base, extent) : catmm_platform_free(base, extent);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, context, 0x03, release.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, context, 0x04, 0);
}

static RESULT catmm_resize(CATMMLIB_POINTERW* memory, NUINT new_size, BOOL managed) {
    const CONTEXT_CODE context = managed ? (CONTEXT_CODE) 0x05 : (CONTEXT_CODE) 0x02;
    if (*memory == 0) {
        RESULT allocation = catmm_allocate(memory, new_size, managed);
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
            catmm_copy(base, &header, (NUINT) sizeof(header));
            *memory = (BYTE*) base + prefix;
        }
    } else
#endif
    {
        if (header.pending != 0) {
            CATMMLIB_MEMORY_HEADER pendingHeader;
            catmm_copy(&pendingHeader, header.pending, (NUINT) sizeof(pendingHeader));
            NUINT pendingSize = (NUINT) (prefix + pendingHeader.requested_size);
            RESULT cleanup = managed ? catmm_heap_extent_free(header.pending, pendingSize)
                : catmm_platform_free(header.pending, pendingSize);
            if (!status_code_is_success(cleanup.status)) {
                return RESULT(cleanup.status, context, 0x08, cleanup.detail);
            }
            header.pending = 0;
            catmm_copy(base, &header, (NUINT) sizeof(header));
        }
        if (new_size != header.requested_size) {
            NUINT oldExtent = (NUINT) (prefix + header.requested_size);
            NUINT newExtent = (NUINT) (prefix + new_size);
            BOOL matches = managed ? catmm_heap_extent_matches(oldExtent, newExtent) : FALSE;
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
                header.requested_size = new_size;
                catmm_copy(base, &header, (NUINT) sizeof(header));
            } else {
                CATMMLIB_POINTERW replacement = 0;
                RESULT allocation = catmm_allocate(&replacement, new_size, managed);
                if (!status_code_is_success(allocation.status)) {
                    return RESULT(allocation.status, context, 0x09, allocation.detail);
                }
                NUINT retained = header.requested_size < new_size ? header.requested_size : new_size;
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
    return catmm_allocate(memory, size, FALSE);
}

CATALYST_RESULT catmm_realloc(CATMMLIB_POINTERW* memory, NUINT new_size) {
    return catmm_resize(memory, new_size, FALSE);
}

CATALYST_RESULT catmm_free(CATMMLIB_POINTERW memory) {
    return catmm_release(memory, FALSE);
}

CATALYST_RESULT catmm_heap_alloc(CATMMLIB_POINTERW* memory, NUINT size) {
    return catmm_allocate(memory, size, TRUE);
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

/* Use fixed-size compiler copies only on known native ABIs. The byte loops
 * remain the complete implementation on other targets. */
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__CHERI__) && !defined(__CHERI_PURE_CAPABILITY__) && (defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__))
#define CATMMLIB_BLOCK_COPY 1
#else
#define CATMMLIB_BLOCK_COPY 0
#endif

/* Copy forward when no unread source byte can be overwritten. */
static void catmm_copy_forward(BYTE* output, const BYTE* input, NUINT size) {
#if CATMMLIB_BLOCK_COPY
    while (size >= (NUINT) 32) {
        BYTE chunk[32];
        __builtin_memcpy(chunk, input, sizeof(chunk));
        __builtin_memcpy(output, chunk, sizeof(chunk));
        output += (NUINT) 32;
        input += (NUINT) 32;
        size = (NUINT) (size - (NUINT) 32);
    }
#endif
    while (size != 0) {
        *output++ = *input++;
        --size;
    }
}

void catmm_copy(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (destination == source) return;
    catmm_copy_forward((BYTE*) destination, (const BYTE*) source, size);
}

void catmm_move(CATMMLIB_POINTERW destination, CATMMLIB_POINTER source, CATALYST_NUINT size) {
    if (size == 0 || destination == source) return;
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    BOOL backward = FALSE;
#if TARGET_SIZEOF_ADDRESS >= 4 && !defined(__CHERI__) && !defined(__CHERI_PURE_CAPABILITY__) && (defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64))
    /* These native ABIs have flat byte addresses. Subtract only in ascending
     * order; computing the end address could overflow the representation. */
    const CATMMLIB_ADDRESS outputAddress = (CATMMLIB_ADDRESS) destination;
    const CATMMLIB_ADDRESS inputAddress = (CATMMLIB_ADDRESS) source;
    backward = outputAddress > inputAddress && (CATMMLIB_ADDRESS) (outputAddress - inputAddress) < (CATMMLIB_ADDRESS) size;
#else
    /* On other representations, equality within the source extent avoids
     * ordering unrelated pointers or assuming a flat address space. */
    NUINT offset = size;
    const BYTE* candidate = input;
    while (offset != 0) {
        if (output == candidate) {
            backward = TRUE;
            break;
        }
        ++candidate;
        --offset;
    }
#endif
    if (!backward) {
        catmm_copy_forward(output, input, size);
        return;
    }
    output += size;
    input += size;
#if CATMMLIB_BLOCK_COPY
    while (size >= (NUINT) 16) {
        BYTE chunk[16];
        output -= (NUINT) 16;
        input -= (NUINT) 16;
        __builtin_memcpy(chunk, input, sizeof(chunk));
        __builtin_memcpy(output, chunk, sizeof(chunk));
        size = (NUINT) (size - (NUINT) 16);
    }
#endif
    while (size != 0) {
        *--output = *--input;
        --size;
    }
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
    while (size >= (NUINT) width) {
        CATMMLIB_COMPARE_WORD firstChunk, secondChunk;
        __builtin_memcpy(&firstChunk, first, sizeof(firstChunk));
        __builtin_memcpy(&secondChunk, second, sizeof(secondChunk));
        // Equality is independent of byte order; mismatches use the byte loop.
        if (firstChunk != secondChunk) break;
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
#if CATMMLIB_BLOCK_COPY && defined(__OPTIMIZE_SIZE__)
    if (size >= (NUINT) 32) {
        BYTE chunk[32];
        for (BYTE i = 0; i < (BYTE) 32; ++i) chunk[i] = value;
        do {
            __builtin_memcpy(output, chunk, sizeof(chunk));
            output += (NUINT) 32;
            size = (NUINT) (size - (NUINT) 32);
        } while (size >= (NUINT) 32);
    }
#endif
    while (size != 0) {
        *output++ = value;
        --size;
    }
}
