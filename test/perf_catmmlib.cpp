#include <cstdio>
#include <ctime>
#include <climits>
#include <stdlib.h>
#include <string.h>

#include "catalyst.hpp"
#include "catmmlib.h"

// The direct baseline uses exactly the native backend available on the target.
#if TARGET_PLATFORM_WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #undef TRUE
    #undef FALSE
#elif TARGET_PLATFORM_DARWIN
    #include <mach/mach_init.h>
    #include <mach/vm_map.h>
    #include <mach/vm_statistics.h>
#elif TARGET_PLATFORM_LINUX
    #include <sys/mman.h>
#endif

using namespace catalyst;

#if defined(CLOCKS_PER_SEC)

// ------------------------------------------------------------
// Native-C benchmark and Catalyst comparison.
// The common harness owns fixtures, timing and verification.
// ------------------------------------------------------------
typedef catalyst::BOOL (*AllocationOperation)(CATMMLIB_POINTERW*, NUINT, NUINT);

static catalyst::BOOL native_alloc(CATMMLIB_POINTERW* memory, NUINT, NUINT size) {
    *memory = ::calloc(1, (size_t) size);
    return *memory != 0 ? TRUE : FALSE;
}

static catalyst::BOOL native_resize(CATMMLIB_POINTERW* memory, NUINT oldSize, NUINT size) {
    if (*memory == 0) return native_alloc(memory, 0, size);
    if (size == 0) {
        ::free(*memory);
        *memory = 0;
        return TRUE;
    }
    void* resized = ::realloc(*memory, (size_t) size);
    if (resized == 0) return FALSE;
    if (size > oldSize)
        ::memset((unsigned char*) resized + oldSize, 0, (size_t) (size - oldSize));
    *memory = resized;
    return TRUE;
}

static catalyst::BOOL native_free(CATMMLIB_POINTERW* memory, NUINT, NUINT) {
    ::free(*memory);
    *memory = 0;
    return TRUE;
}

// Direct allocation baseline: no C heap or Catalyst allocator metadata.
// The caller supplies the original extent, as the native API requires.
#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_DARWIN || TARGET_PLATFORM_LINUX

static catalyst::BOOL os_alloc(CATMMLIB_POINTERW* memory, NUINT, NUINT size) {
    *memory = 0;
#if TARGET_PLATFORM_WIN32
    *memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T) size);
#elif TARGET_PLATFORM_DARWIN
    vm_address_t address = 0;
    if (vm_allocate(mach_task_self(), &address, (vm_size_t) size, VM_FLAGS_ANYWHERE) != KERN_SUCCESS)
        return FALSE;
    *memory = (void*) address;
    if (*memory != 0) *(volatile unsigned char*) *memory = 0;
#elif TARGET_PLATFORM_LINUX
    void* address = mmap(0, (size_t) size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (address == MAP_FAILED) return FALSE;
    *memory = address;
    if (*memory != 0) *(volatile unsigned char*) *memory = 0;
#endif
    return *memory != 0 ? TRUE : FALSE;
}

static catalyst::BOOL os_free(CATMMLIB_POINTERW* memory, NUINT oldSize, NUINT) {
#if TARGET_PLATFORM_WIN32
    (void) oldSize;
    if (!HeapFree(GetProcessHeap(), 0, *memory)) return FALSE;
#elif TARGET_PLATFORM_DARWIN
    if (vm_deallocate(mach_task_self(), (vm_address_t) *memory, (vm_size_t) oldSize) != KERN_SUCCESS)
        return FALSE;
#elif TARGET_PLATFORM_LINUX
    if (munmap(*memory, (size_t) oldSize) != 0) return FALSE;
#endif
    *memory = 0;
    return TRUE;
}

static catalyst::BOOL os_resize(CATMMLIB_POINTERW* memory, NUINT oldSize, NUINT size) {
    if (*memory == 0) return os_alloc(memory, 0, size);
    if (size == 0) return os_free(memory, oldSize, 0);
    if (size == oldSize) return TRUE;
#if TARGET_PLATFORM_WIN32
    void* replacement = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *memory, (SIZE_T) size);
    if (replacement == 0) return FALSE;
    *memory = replacement;
    return TRUE;
#else
    // Match the mapping-based resize strategy, including Darwin page reuse.
    // No private Core headers or prefix are needed for the native baseline.
#if TARGET_PLATFORM_DARWIN
    if (vm_page_size != 0 &&
        ((vm_size_t) oldSize - 1) / vm_page_size == ((vm_size_t) size - 1) / vm_page_size) {
        if (size > oldSize)
            ::memset((unsigned char*) *memory + oldSize, 0, (size_t) (size - oldSize));
        return TRUE;
    }
#endif
    void* replacement = 0;
    if (!os_alloc(&replacement, 0, size)) return FALSE;
    ::memcpy(replacement, *memory, (size_t) (oldSize < size ? oldSize : size));
    if (!os_free(memory, oldSize, 0)) {
        // Attempt to release the unused replacement; report any failure.
        os_free(&replacement, size, 0);
        return FALSE;
    }
    *memory = replacement;
    return TRUE;
#endif
}

#if TARGET_PLATFORM_WIN32
static const char* osAllocName = "HeapAlloc";
static const char* osResizeName = "HeapReAlloc";
static const char* osFreeName = "HeapFree";
#elif TARGET_PLATFORM_DARWIN
static const char* osAllocName = "vm_allocate";
static const char* osResizeName = "native page reuse / allocate-copy-free";
static const char* osFreeName = "vm_deallocate";
#elif TARGET_PLATFORM_LINUX
static const char* osAllocName = "mmap";
static const char* osResizeName = "native allocate-copy-free";
static const char* osFreeName = "munmap";
#endif
#endif

#define ALLOCATOR(name, prefix) \
    static catalyst::BOOL name##_alloc(CATMMLIB_POINTERW* memory, NUINT, NUINT size) { \
        return prefix##_alloc(memory, size).status == STATUS_CODE_SUCCESS ? TRUE : FALSE; \
    } \
    static catalyst::BOOL name##_resize(CATMMLIB_POINTERW* memory, NUINT, NUINT size) { \
        return prefix##_realloc(memory, size).status == STATUS_CODE_SUCCESS ? TRUE : FALSE; \
    } \
    static catalyst::BOOL name##_free(CATMMLIB_POINTERW* memory, NUINT, NUINT) { \
        if (prefix##_free(*memory).status != STATUS_CODE_SUCCESS) return FALSE; \
        *memory = 0; \
        return TRUE; \
    }
ALLOCATOR(direct, catmm)
ALLOCATOR(heap, catmm_heap)
#undef ALLOCATOR

struct Allocator {
    AllocationOperation allocate;
    AllocationOperation resize;
    AllocationOperation release;
};

static const Allocator nativeAllocator = { native_alloc, native_resize, native_free };
#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_DARWIN || TARGET_PLATFORM_LINUX
static const Allocator osAllocator = { os_alloc, os_resize, os_free };
#endif
static const Allocator directAllocator = { direct_alloc, direct_resize, direct_free };
static const Allocator heapAllocator = { heap_alloc, heap_resize, heap_free };

struct MemoryData {
    catalyst::BYTE* first;
    catalyst::BYTE* second;
    NUINT size;
    SBYTE result;
};

static void native_copy(MemoryData* d) { ::memcpy(d->first, d->second, (size_t) d->size); }
static void compare_copy(MemoryData* d) { catmm_copy(d->first, d->second, d->size); }
static void native_copy_unaligned(MemoryData* d) { ::memcpy(d->first + 1, d->second + 1, (size_t) d->size); }
static void compare_copy_unaligned(MemoryData* d) { catmm_copy(d->first + 1, d->second + 1, d->size); }
static void native_move_separate(MemoryData* d) { ::memmove(d->first, d->second, (size_t) d->size); }
static void compare_move_separate(MemoryData* d) { catmm_move(d->first, d->second, d->size); }
static void native_move_right(MemoryData* d) { ::memmove(d->first + 1, d->first, (size_t) d->size); }
static void compare_move_right(MemoryData* d) { catmm_move(d->first + 1, d->first, d->size); }
static void native_move_left(MemoryData* d) { ::memmove(d->first, d->first + 1, (size_t) d->size); }
static void compare_move_left(MemoryData* d) { catmm_move(d->first, d->first + 1, d->size); }
static void native_compare(MemoryData* d) {
    int result = ::memcmp(d->first, d->second, (size_t) d->size);
    d->result = result < 0 ? -1 : result > 0 ? 1 : 0;
}
static void compare_compare(MemoryData* d) { d->result = catmm_compare(d->first, d->second, d->size); }
static void native_fill(MemoryData* d) { ::memset(d->first, 0xA5, (size_t) d->size); }
static void compare_fill(MemoryData* d) { catmm_fill(d->first, 0xA5, d->size); }
static void native_zero(MemoryData* d) { ::memset(d->first, 0, (size_t) d->size); }
static void compare_zero(MemoryData* d) { catmm_fill(d->first, 0, d->size); }

enum MemoryKind {
    COPY, COPY_UNALIGNED, MOVE_SEPARATE, MOVE_RIGHT, MOVE_LEFT,
    COMPARE_EQUAL, COMPARE_FIRST, COMPARE_LAST, FILL, ZERO
};

struct MemoryBenchmark {
    const char* method;
    const char* workload;
    MemoryKind kind;
    void (*native)(MemoryData*);
    void (*comparison)(MemoryData*);
};

static const MemoryBenchmark memoryBenchmarks[] = {
    { "catmm_copy", "aligned", COPY, native_copy, compare_copy },
    { "catmm_copy", "unaligned", COPY_UNALIGNED, native_copy_unaligned, compare_copy_unaligned },
    { "catmm_move", "separate", MOVE_SEPARATE, native_move_separate, compare_move_separate },
    { "catmm_move", "overlap right", MOVE_RIGHT, native_move_right, compare_move_right },
    { "catmm_move", "overlap left", MOVE_LEFT, native_move_left, compare_move_left },
    { "catmm_compare", "equal", COMPARE_EQUAL, native_compare, compare_compare },
    { "catmm_compare", "first differs", COMPARE_FIRST, native_compare, compare_compare },
    { "catmm_compare", "last differs", COMPARE_LAST, native_compare, compare_compare },
    { "catmm_fill", "nonzero", FILL, native_fill, compare_fill },
    { "catmm_fill", "zero", ZERO, native_zero, compare_zero }
};

enum AllocationKind { ALLOCATE, GROW, SHRINK, SAME_SIZE, RESIZE_NULL, RESIZE_ZERO, RELEASE };

struct AllocationBenchmark {
    const char* workload;
    AllocationKind kind;
};

static const AllocationBenchmark allocationBenchmarks[] = {
    { "allocate", ALLOCATE }, { "grow", GROW }, { "shrink", SHRINK },
    { "same size", SAME_SIZE }, { "from null", RESIZE_NULL },
    { "to zero", RESIZE_ZERO }, { "free", RELEASE }
};

// Small workloads are always representable; larger extents are selected
// by the address-width ABI, not assumed processor arithmetic support.
static const NUINT sizes[] = {
    8, 32,
#if TARGET_SIZEOF_ADDRESS >= 2
    128, 1024,
#endif
#if TARGET_SIZEOF_ADDRESS >= 4
    4096, 65536,
#endif
};

// ------------------------------------------------------------
// Independent verification and timing of memory operations.
// ------------------------------------------------------------
enum Measurement { MEASURE_OK, MEASURE_CLOCK, MEASURE_FAIL };
static volatile SBYTE observedResult = 0;

static catalyst::BOOL elapsed(std::clock_t start, unsigned long* ticks) {
    const std::clock_t end = std::clock();
    if (start == (std::clock_t) -1 || start < 0 ||
        end == (std::clock_t) -1 || end < start) return FALSE;
    if (end - start > ULONG_MAX) return FALSE;
    *ticks = (unsigned long) (end - start);
    return TRUE;
}

static catalyst::BYTE pattern(NUINT index) {
    return (catalyst::BYTE) (index % (NUINT) 251);
}

static void prepare_memory(MemoryData& d, MemoryKind kind) {
    for (NUINT i = 0; i < d.size + (NUINT) 2; ++i) {
        d.first[i] = pattern(i);
        d.second[i] = pattern(i);
        if (kind == COPY || kind == COPY_UNALIGNED || kind == MOVE_SEPARATE)
            d.first[i] = (catalyst::BYTE) (pattern(i) ^ (catalyst::BYTE) 0xFF);
    }
    if (kind == COMPARE_FIRST) d.second[0] ^= (catalyst::BYTE) 0x80;
    if (kind == COMPARE_LAST) d.second[d.size - (NUINT) 1] ^= (catalyst::BYTE) 0x80;
    d.result = 0;
}

static catalyst::BOOL verify_memory(const MemoryData& d, MemoryKind kind, unsigned long calls) {
    SBYTE expectedResult = 0;
    for (NUINT i = 0; i < d.size + (NUINT) 2; ++i) {
        catalyst::BYTE first = pattern(i), second = pattern(i);
        if (kind == COPY || kind == MOVE_SEPARATE) {
            if (i >= d.size) first ^= (catalyst::BYTE) 0xFF;
        } else if (kind == COPY_UNALIGNED) {
            if (i == 0 || i > d.size) first ^= (catalyst::BYTE) 0xFF;
        } else if (kind == MOVE_RIGHT && i <= d.size) {
            first = calls >= (unsigned long) i ? pattern(0) : pattern((NUINT) (i - (NUINT) calls));
        } else if (kind == MOVE_LEFT && i < d.size) {
            first = calls >= (unsigned long) (d.size - i) ? pattern(d.size) : pattern((NUINT) (i + (NUINT) calls));
        } else if (kind == FILL && i < d.size) {
            first = 0xA5;
        } else if (kind == ZERO && i < d.size) {
            first = 0;
        } else if ((kind == COMPARE_FIRST && i == 0) ||
                   (kind == COMPARE_LAST && i == d.size - (NUINT) 1)) {
            second ^= (catalyst::BYTE) 0x80;
            expectedResult = first < second ? (SBYTE) -1 : (SBYTE) 1;
        }
        if (d.first[i] != first || d.second[i] != second) return FALSE;
    }
    return d.result == expectedResult ? TRUE : FALSE;
}

struct MemoryTrial {
    MemoryData data;
    MemoryKind kind;
    void (*operation)(MemoryData*);
};

static Measurement measure_memory(MemoryTrial& trial, unsigned long calls, unsigned long* ticks, catalyst::BOOL verify) {
    prepare_memory(trial.data, trial.kind);
    void (*volatile operation)(MemoryData*) = trial.operation;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; i < calls; ++i) {
        operation(&trial.data);
        observedResult = trial.data.result;
    }
    catalyst::BOOL clockOk = elapsed(start, ticks);
    if (verify && !verify_memory(trial.data, trial.kind, calls)) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

// ------------------------------------------------------------
// Isolated allocation operations, with bounded live batches.
// Setup, verification and cleanup are outside the timed region.
// ------------------------------------------------------------
#if TARGET_SIZEOF_ADDRESS >= 4
    enum { BATCH_CAPACITY = 64 };
#elif TARGET_SIZEOF_ADDRESS >= 2
    enum { BATCH_CAPACITY = 8 };
#else
    enum { BATCH_CAPACITY = 2 };
#endif

struct AllocationTrial {
    const Allocator* allocator;
    AllocationKind kind;
    NUINT oldSize;
    NUINT newSize;
};

static catalyst::BOOL verify_allocation(CATMMLIB_POINTERW memory, NUINT oldSize, NUINT newSize, catalyst::BOOL full) {
    if (newSize == 0) return memory == 0 ? TRUE : FALSE;
    if (memory == 0) return FALSE;
    const catalyst::BYTE* bytes = (const catalyst::BYTE*) memory;
    NUINT retained = oldSize < newSize ? oldSize : newSize;
    if (full) {
        for (NUINT i = 0; i < newSize; ++i)
            if (bytes[i] != (i < retained ? (catalyst::BYTE) 0xA5 : (catalyst::BYTE) 0)) return FALSE;
    } else {
        // Check both ends of each retained/new extent on every timed batch.
        if (retained != 0 && (bytes[0] != 0xA5 || bytes[retained - (NUINT) 1] != 0xA5)) return FALSE;
        if (newSize > retained && (bytes[retained] != 0 || bytes[newSize - (NUINT) 1] != 0)) return FALSE;
    }
    return TRUE;
}

static Measurement measure_allocation(AllocationTrial& trial, unsigned long calls, unsigned long* ticks, catalyst::BOOL verify) {
    const Allocator& allocator = *trial.allocator;
    AllocationOperation selected = trial.kind == ALLOCATE ? allocator.allocate :
        trial.kind == RELEASE ? allocator.release : allocator.resize;
    AllocationOperation volatile operation = selected;
    catalyst::BYTE batchLimit = BATCH_CAPACITY;
#if TARGET_SIZEOF_ADDRESS >= 4
    // Bound live large allocations to 256 KiB (not counting allocator metadata).
    NUINT extent = trial.oldSize > trial.newSize ? trial.oldSize : trial.newSize;
    while (batchLimit > 1 && extent > (NUINT) 262144 / batchLimit) batchLimit /= 2;
#endif
    *ticks = 0;
    unsigned long completed = 0;
    while (completed < calls) {
        CATMMLIB_POINTERW blocks[BATCH_CAPACITY] = { 0 };
        catalyst::BYTE count = calls - completed < batchLimit ? (catalyst::BYTE) (calls - completed) : batchLimit;
        catalyst::BOOL ok = TRUE;
        if (trial.oldSize != 0) {
            for (catalyst::BYTE i = 0; i < count; ++i) {
                if (!allocator.allocate(&blocks[i], 0, trial.oldSize)) { ok = FALSE; break; }
                ::memset(blocks[i], 0xA5, (size_t) trial.oldSize);
            }
        }
        catalyst::BYTE succeeded = 0;
        unsigned long batchTicks = 0;
        catalyst::BOOL clockOk = TRUE;
        if (ok) {
            const std::clock_t start = std::clock();
            for (catalyst::BYTE i = 0; i < count; ++i) {
                if (!operation(&blocks[i], trial.oldSize, trial.newSize)) { ok = FALSE; break; }
                observedResult = 0;
                ++succeeded;
            }
            clockOk = elapsed(start, &batchTicks);
            if (ok && verify) {
                for (catalyst::BYTE i = 0; i < count; ++i) {
                    if (!verify_allocation(blocks[i], trial.oldSize, trial.newSize, calls == 1 ? TRUE : FALSE))
                        ok = FALSE;
                }
            }
        }
        for (catalyst::BYTE i = 0; i < count; ++i) {
            // Completed operations own the new extent; unprocessed or failed
            // operations still own the old extent. Mapping APIs require it.
            NUINT extent = i < succeeded ? trial.newSize : trial.oldSize;
            if (blocks[i] != 0 && !allocator.release(&blocks[i], extent, 0)) ok = FALSE;
        }
        if (!ok) return MEASURE_FAIL;
        if (!clockOk || batchTicks > ULONG_MAX - *ticks) return MEASURE_CLOCK;
        *ticks += batchTicks;
        completed += count;
    }
    return MEASURE_OK;
}

// ------------------------------------------------------------
// Warm heap lifecycles: keep one persistent allocation at a time.
// Time complete operation pairs rather than one clock call per allocation.
// ------------------------------------------------------------
enum WarmKind { WARM_LIFETIME, WARM_NULL_ZERO, WARM_RESIZE, WARM_SAME };

struct WarmTrial {
    const Allocator* allocator;
    WarmKind kind;
    NUINT small;
    NUINT large;
};

static Measurement measure_warm(WarmTrial& trial, unsigned long calls, unsigned long* ticks, catalyst::BOOL verify) {
    const Allocator& allocator = *trial.allocator;
    AllocationOperation volatile allocate = trial.kind == WARM_NULL_ZERO ? allocator.resize : allocator.allocate;
    AllocationOperation volatile release = trial.kind == WARM_NULL_ZERO ? allocator.resize : allocator.release;
    AllocationOperation volatile resize = allocator.resize;
    CATMMLIB_POINTERW memory = 0;
    NUINT owned = 0;
    catalyst::BOOL ok = TRUE;
    const catalyst::BOOL lifetime = trial.kind == WARM_LIFETIME || trial.kind == WARM_NULL_ZERO ? TRUE : FALSE;
    if (!lifetime) {
        ok = allocator.allocate(&memory, 0, trial.small);
        if (ok) {
            owned = trial.small;
            ::memset(memory, 0xA5, (size_t) owned);
        }
    }
    // Full byte verification is done in the one-cycle preflight. The measured
    // samples use the same workload but only check outcomes and the endpoints.
    const catalyst::BOOL full = verify && calls == 1 ? TRUE : FALSE;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        if (lifetime) {
            if (!allocate(&memory, 0, trial.small)) { ok = FALSE; break; }
            owned = trial.small;
            if (memory == 0) { ok = FALSE; break; }
            // Check representative bytes, then dirty them to exercise reuse.
            if (!verify_allocation(memory, 0, owned, full)) { ok = FALSE; break; }
            ((volatile unsigned char*) memory)[0] = 0xA5;
            ((volatile unsigned char*) memory)[owned - (NUINT) 1] = 0xA5;
            if (!release(&memory, owned, 0)) { ok = FALSE; break; }
            owned = 0;
            if (memory != 0) { ok = FALSE; break; }
        } else if (trial.kind == WARM_RESIZE) {
            if (!resize(&memory, owned, trial.large)) { ok = FALSE; break; }
            if (!verify_allocation(memory, owned, trial.large, full)) {
                owned = trial.large;
                ok = FALSE;
                break;
            }
            owned = trial.large;
            ((volatile unsigned char*) memory)[owned - (NUINT) 1] = 0xA5;
            if (!resize(&memory, owned, trial.small)) { ok = FALSE; break; }
            owned = trial.small;
            if (!verify_allocation(memory, owned, owned, full)) { ok = FALSE; break; }
        } else {
            if (!resize(&memory, owned, owned)) { ok = FALSE; break; }
        }
        observedResult = 0;
    }
    catalyst::BOOL clockOk = elapsed(start, ticks);
    if (ok && !lifetime && verify && !verify_allocation(memory, owned, owned, TRUE)) ok = FALSE;
    if (memory != 0 && !allocator.release(&memory, owned, 0)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

// ------------------------------------------------------------
// Portable calibration and output, matching perf_cattelib.
// Clock and reporting arithmetic is native and outside Core.
// ------------------------------------------------------------
static unsigned long median(unsigned long* values) {
    unsigned long a = values[0], b = values[1], c = values[2];
    if (a > b) { unsigned long t = a; a = b; b = t; }
    if (b > c) { unsigned long t = b; b = c; c = t; }
    return a > b ? a : b;
}

/* Convert a fractional remainder into one decimal digit without overflow. */
static unsigned long decimal_digit(unsigned long* remainder, unsigned long divisor) {
    unsigned long original = *remainder, carry = 0, digit = 0;
    for (catalyst::BYTE i = 0; i < 10; ++i) {
        if (carry >= divisor - original) {
            carry -= divisor - original;
            ++digit;
        } else {
            carry += original;
        }
    }
    *remainder = carry;
    return digit;
}

/* Print ns/call to three decimal places without floating point or large products.
 * Divide the decimal digits of elapsed seconds by the call count as they are
 * generated. maxCalls below ensures carry * 10 fits even a 32-bit unsigned long.
 */
static void print_nanoseconds(unsigned long ticks, unsigned long calls) {
    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long seconds = ticks / frequency;
    unsigned long remainder = ticks % frequency;
    unsigned long value = seconds / calls, carry = seconds % calls;
    unsigned long fraction = 0;
    for (catalyst::BYTE i = 0; i < 12; ++i) {
        carry = carry * 10 + decimal_digit(&remainder, frequency);
        unsigned long digit = carry / calls;
        carry %= calls;
        if (i < 9) {
            if (value > (ULONG_MAX - digit) / 10) {
                std::printf(" %14s", "out of range");
                return;
            }
            value = value * 10 + digit;
        } else {
            fraction = fraction * 10 + digit;
        }
    }
    if (value == 0 && fraction == 0) std::printf(" %14s", "<0.001 ns");
    else std::printf(" %7lu.%03lu ns", value, fraction);
}

/* Report the change in execution time relative to Native-C, not speedup. */
static void print_difference(unsigned long native, unsigned long comparison) {
    unsigned long delta = comparison >= native ? comparison - native : native - comparison;
    unsigned long whole = delta / native, remainder = delta % native;
    unsigned long percent = decimal_digit(&remainder, native) * 10;
    percent += decimal_digit(&remainder, native);
    unsigned long tenth = decimal_digit(&remainder, native);
    if (whole > (ULONG_MAX - percent) / 100) {
        std::printf(" %12s", "out of range");
        return;
    }
    percent += whole * 100;
    // Pad the complete signed percentage, not the digits after the sign.
    catalyst::BYTE digits = 1;
    for (unsigned long remaining = percent; remaining >= 10; remaining /= 10) ++digits;
    for (catalyst::BYTE i = digits + 4; i < 12; ++i) std::putchar(' ');
    std::printf(" %c%lu.%lu%%", comparison < native ? '-' : '+', percent, tenth);
}


struct Summary {
    unsigned long faster, similar, slower, review, shortSamples, unavailable, failed;
};

static void report_failure(const char* label, Measurement result, Summary& summary) {
    if (result == MEASURE_CLOCK) {
        std::printf("  %-28s %14s %14s %12s  No timing\n", label, "-", "-", "-");
        ++summary.unavailable;
    } else {
        std::printf("  %-28s FAIL: operation, fixture or verification failed\n", label);
        ++summary.failed;
    }
}

template<typename Trial>
static void benchmark_case(const char* label, Trial& native, Trial& comparison,
                           Measurement (*measure)(Trial&, unsigned long, unsigned long*, catalyst::BOOL),
                           unsigned long maxCalls, Summary& summary) {
    unsigned long n = 0, c = 0;
    Measurement result = measure(native, 1, &n, TRUE);
    if (result == MEASURE_OK) result = measure(comparison, 1, &c, TRUE);
    if (result != MEASURE_OK) { report_failure(label, result, summary); return; }

    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long target = frequency / 100; // 10 ms per path.
    if (target < 20) target = 20;
    unsigned long minimum = frequency / 1000;
    if (minimum < 20) minimum = 20;
    unsigned long budget = frequency / 10; // 100 ms cumulative calibration CPU budget.
    if (budget < target) budget = target;
    unsigned long sampleBudget = frequency / 10; // Bound full setup + timing + cleanup per pair.
    if (sampleBudget < target) sampleBudget = target;
    unsigned long calls = 16;
    const std::clock_t calibrationStart = std::clock();

    for (;;) {
        const std::clock_t attemptStart = std::clock();
        result = measure(native, calls, &n, FALSE);
        if (result == MEASURE_OK) result = measure(comparison, calls, &c, FALSE);
        if (result != MEASURE_OK) { report_failure(label, result, summary); return; }
        unsigned long totalTicks = 0, attemptTicks = 0;
        if (!elapsed(calibrationStart, &totalTicks) || !elapsed(attemptStart, &attemptTicks)) {
            report_failure(label, MEASURE_CLOCK, summary);
            return;
        }
        unsigned long faster = n < c ? n : c;
        if (faster >= target || calls >= maxCalls || totalTicks >= budget ||
            attemptTicks >= sampleBudget) break;

        // Estimate a bounded multiplier from the last observation. No
        // unchecked multiplication, and zero-tick readings grow by eight.
        unsigned long factor = faster == 0 ? 8 : target / faster;
        if (factor < 2) factor = 2;
        if (factor > 16) factor = 16;
        if (attemptTicks != 0) {
            unsigned long remaining = budget - totalTicks;
            unsigned long allowed = remaining < sampleBudget ? remaining : sampleBudget;
            unsigned long affordable = allowed / attemptTicks;
            if (affordable < 2) break;
            if (factor > affordable) factor = affordable;
        }
        if (factor > maxCalls / calls) calls = maxCalls;
        else calls *= factor;
    }
    unsigned long nativeSamples[3], catalystSamples[3];
    for (catalyst::BYTE sample = 0; sample < 3; ++sample) {
        if (sample == 1) {
            result = measure(comparison, calls, &catalystSamples[sample], TRUE);
            if (result == MEASURE_OK) result = measure(native, calls, &nativeSamples[sample], TRUE);
        } else {
            result = measure(native, calls, &nativeSamples[sample], TRUE);
            if (result == MEASURE_OK) result = measure(comparison, calls, &catalystSamples[sample], TRUE);
        }
        if (result != MEASURE_OK) { report_failure(label, result, summary); return; }
    }
    n = median(nativeSamples);
    c = median(catalystSamples);
    if (n == 0 || c == 0) { report_failure(label, MEASURE_CLOCK, summary); return; }
    std::printf("  %-28s", label);
    print_nanoseconds(n, calls);
    print_nanoseconds(c, calls);
    print_difference(n, c);
    if (n < minimum || c < minimum) {
        std::printf("  Short sample *");
        ++summary.shortSamples;
    } else {
        unsigned long delta = c >= n ? c - n : n - c;
        if (delta <= n / 20) {
            ++summary.similar;
            std::printf("  Comparable");
        } else if (c < n) {
            ++summary.faster;
            std::printf("  Faster");
        } else {
            ++summary.slower;
            if (delta >= n / 5 + (n % 5 != 0 ? 1 : 0)) {
                ++summary.review;
                std::printf("  Review");
            } else std::printf("  Slower");
        }
    }
    std::printf("\n");
    std::fflush(stdout);
}

static void benchmark_allocations(const Allocator& baseline, const Allocator& allocator, catalyst::BOOL managed,
                                  const char* allocName, const char* resizeName, const char* freeName,
                                  Summary& summary) {
    for (catalyst::BYTE b = 0; b < sizeof(allocationBenchmarks) / sizeof(allocationBenchmarks[0]); ++b) {
        const AllocationBenchmark& benchmark = allocationBenchmarks[b];
        if (!managed && benchmark.kind != ALLOCATE && benchmark.kind != GROW &&
            benchmark.kind != SHRINK && benchmark.kind != RELEASE) continue;
        if (benchmark.kind == ALLOCATE)
            std::printf("\n%s (%s)\n", managed ? "catmm_heap_alloc" : "catmm_alloc", allocName);
        else if (benchmark.kind == GROW)
            std::printf("\n%s (%s)\n", managed ? "catmm_heap_realloc" : "catmm_realloc", resizeName);
        else if (benchmark.kind == RELEASE)
            std::printf("\n%s (%s)\n", managed ? "catmm_heap_free" : "catmm_free", freeName);
        for (catalyst::BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            // Direct OS calls need only a small and a large representative extent.
            if (!managed && i != 0 && i != sizeof(sizes) / sizeof(sizes[0]) - 1) continue;
            NUINT size = sizes[i], oldSize = size, newSize = size;
            if (benchmark.kind == ALLOCATE || benchmark.kind == RESIZE_NULL) oldSize = 0;
            else if (benchmark.kind == GROW) oldSize = (NUINT) (size / (NUINT) 2);
            else if (benchmark.kind == SHRINK) newSize = (NUINT) (size / (NUINT) 2);
            else if (benchmark.kind == RELEASE || benchmark.kind == RESIZE_ZERO) newSize = 0;
            char label[40];
            if (benchmark.kind == ALLOCATE || benchmark.kind == RELEASE)
                std::sprintf(label, "%lu bytes", (unsigned long) size);
            else
                std::sprintf(label, "%lu -> %lu B (%s)", (unsigned long) oldSize,
                    (unsigned long) newSize, benchmark.workload);
            AllocationTrial native = { &baseline, benchmark.kind, oldSize, newSize };
            AllocationTrial comparison = { &allocator, benchmark.kind, oldSize, newSize };
            // Limit repeated allocation setup and page churn; timing is batched.
            benchmark_case(label, native, comparison, measure_allocation, 1048576UL, summary);
        }
    }
}

static void benchmark_warm_heap(Summary& summary) {
    static const char* labels[] = {
        "catmm_heap_alloc + catmm_heap_free (calloc + free)",
        "catmm_heap_realloc from null + to zero (calloc + free)",
        "catmm_heap_realloc grow + shrink (realloc + zero growth)",
        "catmm_heap_realloc same size (realloc)"
    };
    for (catalyst::BYTE kind = 0; kind < 4; ++kind) {
        std::printf("\n%s\n", labels[kind]);
        for (catalyst::BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
            NUINT size = sizes[i];
            NUINT small = kind == WARM_RESIZE ? (NUINT) (size / (NUINT) 2) : size;
            char label[40];
            if (kind == WARM_RESIZE)
                std::sprintf(label, "%lu -> %lu -> %lu B", (unsigned long) small,
                    (unsigned long) size, (unsigned long) small);
            else
                std::sprintf(label, "%lu bytes", (unsigned long) size);
            WarmTrial native = { &nativeAllocator, (WarmKind) kind, small, size };
            WarmTrial comparison = { &heapAllocator, (WarmKind) kind, small, size };
            benchmark_case(label, native, comparison, measure_warm, 1048576UL, summary);
        }
    }
}

static void benchmark_memory(Summary& summary) {
    const char* previous = 0;
    for (catalyst::BYTE b = 0; b < sizeof(memoryBenchmarks) / sizeof(memoryBenchmarks[0]); ++b) {
        const MemoryBenchmark& benchmark = memoryBenchmarks[b];
        if (previous == 0 || ::strcmp(previous, benchmark.method) != 0) {
            std::printf("\n%s\n", benchmark.method);
            previous = benchmark.method;
        }
        for (catalyst::BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]) + 2; ++i) {
            NUINT size = i < 2 ? (NUINT) i : sizes[i - 2];
            if (size == 0 && (benchmark.kind == COMPARE_FIRST || benchmark.kind == COMPARE_LAST)) continue;
            char label[40];
            std::sprintf(label, "%lu B (%s)", (unsigned long) size, benchmark.workload);
            // Common fixture storage is not part of either timed implementation.
            // Reuse it sequentially so small-address targets need only two buffers.
            catalyst::BYTE* first = (catalyst::BYTE*) ::malloc((size_t) (size + (NUINT) 2));
            catalyst::BYTE* second = (catalyst::BYTE*) ::malloc((size_t) (size + (NUINT) 2));
            if (first == 0 || second == 0) {
                report_failure(label, MEASURE_FAIL, summary);
            } else {
                MemoryTrial native = { { first, second, size, 0 }, benchmark.kind, benchmark.native };
                MemoryTrial comparison = { { first, second, size, 0 }, benchmark.kind, benchmark.comparison };
                benchmark_case(label, native, comparison, measure_memory, 67108864UL, summary);
            }
            ::free(first);
            ::free(second);
        }
    }
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATMMLIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATMMLIB Performance (Release recommended)\n");
    std::printf("Native-C is the benchmark. Lower time is better.\n");
    std::printf("Difference: +10%% means Catalyst takes 10%% longer; -10%% means 10%% less time.\n");
    std::printf("Time per operation or pair: median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("Direct allocation: two representative sizes; mapping baseline touches its first page.\n");
    std::printf("Managed allocation: C heap (calloc/realloc + zero growth/free) vs CATMMLIB heap.\n");
    std::printf("Direct/burst timing excludes setup, cleanup and verification; includes batch clock overhead.\n");
    std::printf("Warm heap timings include both operations in each pair and identical endpoint checks.\n");
    std::printf("Full allocation contents checked in preflight; measured batches sample endpoints.\n");
    std::printf("Memory: standard C memcpy/memmove/memcmp/memset vs CATMMLIB.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-28s %14s %14s %12s  %s\n", "Workload", "Native-C", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_DARWIN || TARGET_PLATFORM_LINUX
    std::printf("\nDirect allocation (native OS baseline)\n");
    benchmark_allocations(osAllocator, directAllocator, FALSE, osAllocName, osResizeName, osFreeName, summary);
#else
    std::printf("\nDirect allocation: no native OS baseline available on this target.\n");
    summary.unavailable += 8;
#endif
    std::printf("\nManaged allocation (warm reuse; one persistent block, standard C heap baseline)\n");
    std::printf("Sizes outside managed size classes still use native backing allocation (including 64 KiB payloads).\n");
    benchmark_warm_heap(summary);
    std::printf("\nManaged allocation (burst; up to %u live blocks, standard C heap baseline)\n", (unsigned int) BATCH_CAPACITY);
    benchmark_allocations(nativeAllocator, heapAllocator, TRUE, "calloc", "realloc + zero growth", "free", summary);
    std::printf("\nMemory operations (standard C baseline)\n");
    benchmark_memory(summary);
    std::printf("\nPerformance summary\n");
    std::printf("  Faster (>5%%):                  %3lu\n", summary.faster);
    std::printf("  Comparable (within 5%%):        %3lu\n", summary.similar);
    std::printf("  Slower (>5%%):                  %3lu\n", summary.slower);
    std::printf("    At least 20%% slower:          %3lu (review)\n", summary.review);
    std::printf("  Short samples (*):             %3lu\n", summary.shortSamples);
    std::printf("  Timing unavailable:            %3lu\n", summary.unavailable);
    std::printf("  Verification failures:         %3lu\n", summary.failed);
    if (summary.shortSamples != 0)
        std::printf("  * Less than 1 ms or 20 ticks; excluded from performance classification.\n");
    if (summary.failed != 0) return 1;
    std::printf("\nAll CATMMLIB performance comparisons verified.\n");
    return summary.unavailable != 0 ? 2 : 0;
#else
    std::printf("CATMMLIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
