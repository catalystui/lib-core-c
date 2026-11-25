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

/* Caller holds the class lock; slab is live. */
static BOOL catmm_heap_has_space(const CATMMLIB_HEAP_SLAB* slab) {
    return slab->available != 0 || slab->unused != 0 ? TRUE : FALSE;
}

/* Base is a fresh mapping of the selected class layout. Placement new starts
 * private POD lifetimes without allocating memory or touching virgin slots.
 */
static CATMMLIB_HEAP_SLAB* catmm_heap_init(CATMMLIB_POINTERW base, NUINT capacity, NUINT extent) {
    CATMMLIB_HEAP_SLAB_PREFIX* prefix = new (base) CATMMLIB_HEAP_SLAB_PREFIX;
    CATMMLIB_HEAP_SLAB* slab = &prefix->slab;
    slab->previous = 0;
    slab->next = 0;
    slab->available = 0;
    slab->next_unused = (BYTE*) base + catmm_slab_prefix;
    slab->unused = (NUINT) ((extent - catmm_slab_prefix) /
        (NUINT) (sizeof(CATMMLIB_HEAP_SLOT) + capacity));
    slab->live = 0;
    slab->extent = extent;
    return slab;
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

CATALYST_BOOL catmm_heap_extent_matches(CATALYST_NUINT old_size, CATALYST_NUINT new_size) {
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

CATALYST_RESULT catmm_heap_extent_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size) {
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
                if (status_code_is_success(allocation.status)) slab = catmm_heap_init(base, capacity, extent);
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
            if (!catmm_heap_has_space(slab)) {
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

CATALYST_RESULT catmm_heap_extent_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size) {
    RESULT release(STATUS_CODE_SUCCESS, 0x06, 0x02, 0);
#if CATMMLIB_MANAGED_HEAP
    NUINT capacity, extent;
    BYTE index = catmm_heap_class(size, &capacity, &extent);
    if (index != catmm_class_count) {
        CATMMLIB_HEAP_SLOT* slot = (CATMMLIB_HEAP_SLOT*) ((BYTE*) memory - sizeof(CATMMLIB_HEAP_SLOT));
        CATMMLIB_HEAP_CLASS* pool = &catmm_classes[index];
        catmm_heap_acquire(pool);
        CATMMLIB_HEAP_SLAB* slab = slot->links.owner;
        BOOL wasPartial = catmm_heap_has_space(slab);
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
