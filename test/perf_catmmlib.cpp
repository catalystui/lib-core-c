#include <cstdio>
#include <ctime>
#include <climits>
#include <stdlib.h>
#include <string.h>

#include "catalyst.hpp"
#include "catmmlib.h"

using namespace catalyst;

#if defined(CLOCKS_PER_SEC)

// ------------------------------------------------------------
// Native-C benchmark and Catalyst comparison.
// The common harness owns fixtures, timing and verification.
// ------------------------------------------------------------
typedef BOOL (*AllocationOperation)(CATMMLIB_POINTERW*, NUINT, NUINT);

static BOOL native_alloc(CATMMLIB_POINTERW* memory, NUINT, NUINT size) {
    *memory = ::calloc(1, (size_t) size);
    return *memory != 0 ? TRUE : FALSE;
}

static BOOL native_resize(CATMMLIB_POINTERW* memory, NUINT oldSize, NUINT size) {
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

static BOOL native_free(CATMMLIB_POINTERW* memory, NUINT, NUINT) {
    ::free(*memory);
    *memory = 0;
    return TRUE;
}

#define ALLOCATOR(name, prefix) \
    static BOOL name##_alloc(CATMMLIB_POINTERW* memory, NUINT, NUINT size) { \
        return prefix##_alloc(memory, size).status == STATUS_CODE_SUCCESS ? TRUE : FALSE; \
    } \
    static BOOL name##_resize(CATMMLIB_POINTERW* memory, NUINT, NUINT size) { \
        return prefix##_realloc(memory, size).status == STATUS_CODE_SUCCESS ? TRUE : FALSE; \
    } \
    static BOOL name##_free(CATMMLIB_POINTERW* memory, NUINT, NUINT) { \
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
static const Allocator directAllocator = { direct_alloc, direct_resize, direct_free };
static const Allocator heapAllocator = { heap_alloc, heap_resize, heap_free };

struct MemoryData {
    BYTE* first;
    BYTE* second;
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

static BOOL elapsed(std::clock_t start, unsigned long* ticks) {
    const std::clock_t end = std::clock();
    if (start == (std::clock_t) -1 || start < 0 ||
        end == (std::clock_t) -1 || end < start) return FALSE;
    if (end - start > ULONG_MAX) return FALSE;
    *ticks = (unsigned long) (end - start);
    return TRUE;
}

static BYTE pattern(NUINT index) {
    return (BYTE) (index % (NUINT) 251);
}

static void prepare_memory(MemoryData& d, MemoryKind kind) {
    for (NUINT i = 0; i < d.size + (NUINT) 2; ++i) {
        d.first[i] = pattern(i);
        d.second[i] = pattern(i);
        if (kind == COPY || kind == COPY_UNALIGNED || kind == MOVE_SEPARATE)
            d.first[i] = (BYTE) (pattern(i) ^ (BYTE) 0xFF);
    }
    if (kind == COMPARE_FIRST) d.second[0] ^= (BYTE) 0x80;
    if (kind == COMPARE_LAST) d.second[d.size - (NUINT) 1] ^= (BYTE) 0x80;
    d.result = 0;
}

static BOOL verify_memory(const MemoryData& d, MemoryKind kind, unsigned long calls) {
    SBYTE expectedResult = 0;
    for (NUINT i = 0; i < d.size + (NUINT) 2; ++i) {
        BYTE first = pattern(i), second = pattern(i);
        if (kind == COPY || kind == MOVE_SEPARATE) {
            if (i >= d.size) first ^= (BYTE) 0xFF;
        } else if (kind == COPY_UNALIGNED) {
            if (i == 0 || i > d.size) first ^= (BYTE) 0xFF;
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
            second ^= (BYTE) 0x80;
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

static Measurement measure_memory(MemoryTrial& trial, unsigned long calls, unsigned long* ticks) {
    prepare_memory(trial.data, trial.kind);
    void (*volatile operation)(MemoryData*) = trial.operation;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; i < calls; ++i) {
        operation(&trial.data);
        observedResult = trial.data.result;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (!verify_memory(trial.data, trial.kind, calls)) return MEASURE_FAIL;
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

static BOOL verify_allocation(CATMMLIB_POINTERW memory, NUINT oldSize, NUINT newSize, BOOL full) {
    if (newSize == 0) return memory == 0 ? TRUE : FALSE;
    if (memory == 0) return FALSE;
    const BYTE* bytes = (const BYTE*) memory;
    NUINT retained = oldSize < newSize ? oldSize : newSize;
    if (full) {
        for (NUINT i = 0; i < newSize; ++i)
            if (bytes[i] != (i < retained ? (BYTE) 0xA5 : (BYTE) 0)) return FALSE;
    } else {
        // Check both ends of each retained/new extent on every timed batch.
        if (retained != 0 && (bytes[0] != 0xA5 || bytes[retained - (NUINT) 1] != 0xA5)) return FALSE;
        if (newSize > retained && (bytes[retained] != 0 || bytes[newSize - (NUINT) 1] != 0)) return FALSE;
    }
    return TRUE;
}

static Measurement measure_allocation(AllocationTrial& trial, unsigned long calls, unsigned long* ticks) {
    const Allocator& allocator = *trial.allocator;
    AllocationOperation selected = trial.kind == ALLOCATE ? allocator.allocate :
        trial.kind == RELEASE ? allocator.release : allocator.resize;
    AllocationOperation volatile operation = selected;
    BYTE batchLimit = BATCH_CAPACITY;
#if TARGET_SIZEOF_ADDRESS >= 4
    // Bound live large allocations to 256 KiB (not counting allocator metadata).
    NUINT extent = trial.oldSize > trial.newSize ? trial.oldSize : trial.newSize;
    while (batchLimit > 1 && extent > (NUINT) 262144 / batchLimit) batchLimit /= 2;
#endif
    *ticks = 0;
    unsigned long completed = 0;
    while (completed < calls) {
        CATMMLIB_POINTERW blocks[BATCH_CAPACITY] = { 0 };
        BYTE count = calls - completed < batchLimit ? (BYTE) (calls - completed) : batchLimit;
        BOOL ok = TRUE;
        if (trial.oldSize != 0) {
            for (BYTE i = 0; i < count; ++i) {
                if (!allocator.allocate(&blocks[i], 0, trial.oldSize)) { ok = FALSE; break; }
                for (NUINT j = 0; j < trial.oldSize; ++j) ((BYTE*) blocks[i])[j] = 0xA5;
            }
        }
        unsigned long batchTicks = 0;
        BOOL clockOk = TRUE;
        if (ok) {
            const std::clock_t start = std::clock();
            for (BYTE i = 0; i < count; ++i) {
                if (!operation(&blocks[i], trial.oldSize, trial.newSize)) { ok = FALSE; break; }
                observedResult = 0;
            }
            clockOk = elapsed(start, &batchTicks);
            if (ok) {
                for (BYTE i = 0; i < count; ++i) {
                    if (!verify_allocation(blocks[i], trial.oldSize, trial.newSize, calls == 1 ? TRUE : FALSE))
                        ok = FALSE;
                }
            }
        }
        for (BYTE i = 0; i < count; ++i) {
            if (blocks[i] != 0 && !allocator.release(&blocks[i], 0, 0)) ok = FALSE;
        }
        if (!ok) return MEASURE_FAIL;
        if (!clockOk || batchTicks > ULONG_MAX - *ticks) return MEASURE_CLOCK;
        *ticks += batchTicks;
        completed += count;
    }
    return MEASURE_OK;
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
    for (BYTE i = 0; i < 10; ++i) {
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
    for (BYTE i = 0; i < 12; ++i) {
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
    BYTE digits = 1;
    for (unsigned long remaining = percent; remaining >= 10; remaining /= 10) ++digits;
    for (BYTE i = digits + 4; i < 12; ++i) std::putchar(' ');
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
                           Measurement (*measure)(Trial&, unsigned long, unsigned long*),
                           unsigned long maxCalls, Summary& summary) {
    unsigned long n = 0, c = 0;
    Measurement result = measure(native, 1, &n);
    if (result == MEASURE_OK) result = measure(comparison, 1, &c);
    if (result != MEASURE_OK) { report_failure(label, result, summary); return; }

    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long target = frequency / 40;
    if (target < 20) target = 20;
    unsigned long minimum = frequency / 1000;
    if (minimum < 20) minimum = 20;
    unsigned long budget = frequency / 4;
    if (budget < target) budget = target;
    unsigned long calls = 16;

    for (;;) {
        const std::clock_t calibrationStart = std::clock();
        result = measure(native, calls, &n);
        if (result == MEASURE_OK) result = measure(comparison, calls, &c);
        if (result != MEASURE_OK) { report_failure(label, result, summary); return; }
        unsigned long calibrationTicks = 0;
        if (!elapsed(calibrationStart, &calibrationTicks)) {
            report_failure(label, MEASURE_CLOCK, summary);
            return;
        }
        if ((n >= target && c >= target) || calls == maxCalls ||
            n >= budget || c >= budget || calibrationTicks >= budget) break;
        calls *= 2;
    }
    unsigned long nativeSamples[3], catalystSamples[3];
    for (BYTE sample = 0; sample < 3; ++sample) {
        if (sample == 1) {
            result = measure(comparison, calls, &catalystSamples[sample]);
            if (result == MEASURE_OK) result = measure(native, calls, &nativeSamples[sample]);
        } else {
            result = measure(native, calls, &nativeSamples[sample]);
            if (result == MEASURE_OK) result = measure(comparison, calls, &catalystSamples[sample]);
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
}

static void benchmark_allocations(const Allocator& allocator, BOOL managed, Summary& summary) {
    for (BYTE b = 0; b < sizeof(allocationBenchmarks) / sizeof(allocationBenchmarks[0]); ++b) {
        const AllocationBenchmark& benchmark = allocationBenchmarks[b];
        if (benchmark.kind == ALLOCATE)
            std::printf("\n%s (calloc)\n", managed ? "catmm_heap_alloc" : "catmm_alloc");
        else if (benchmark.kind == GROW)
            std::printf("\n%s (realloc + zero growth)\n", managed ? "catmm_heap_realloc" : "catmm_realloc");
        else if (benchmark.kind == RELEASE)
            std::printf("\n%s (free)\n", managed ? "catmm_heap_free" : "catmm_free");
        for (BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
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
            AllocationTrial native = { &nativeAllocator, benchmark.kind, oldSize, newSize };
            AllocationTrial comparison = { &allocator, benchmark.kind, oldSize, newSize };
            // Limit repeated allocation setup and page churn; timing is batched.
            benchmark_case(label, native, comparison, measure_allocation, 1048576UL, summary);
        }
    }
}

static void benchmark_memory(Summary& summary) {
    const char* previous = 0;
    for (BYTE b = 0; b < sizeof(memoryBenchmarks) / sizeof(memoryBenchmarks[0]); ++b) {
        const MemoryBenchmark& benchmark = memoryBenchmarks[b];
        if (previous == 0 || ::strcmp(previous, benchmark.method) != 0) {
            std::printf("\n%s\n", benchmark.method);
            previous = benchmark.method;
        }
        for (BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]) + 2; ++i) {
            NUINT size = i < 2 ? (NUINT) i : sizes[i - 2];
            if (size == 0 && (benchmark.kind == COMPARE_FIRST || benchmark.kind == COMPARE_LAST)) continue;
            char label[40];
            std::sprintf(label, "%lu B (%s)", (unsigned long) size, benchmark.workload);
            // Common fixture storage is not part of either timed implementation.
            // Reuse it sequentially so small-address targets need only two buffers.
            BYTE* first = (BYTE*) ::malloc((size_t) (size + (NUINT) 2));
            BYTE* second = (BYTE*) ::malloc((size_t) (size + (NUINT) 2));
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
    std::printf("Time per call: median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("Allocation: calloc/realloc with zero growth/free vs direct and managed Catalyst allocators.\n");
    std::printf("Direct allocation may use OS mappings; the C allocator normally reuses heap storage.\n");
    std::printf("Allocation timing excludes fixture setup, cleanup and verification; includes batch clock overhead.\n");
    std::printf("Allocation contents are fully checked in preflight and sampled in timed batches.\n");
    std::printf("Memory: standard C memcpy/memmove/memcmp/memset vs CATMMLIB.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-28s %14s %14s %12s  %s\n", "Workload", "Native-C", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
    benchmark_allocations(directAllocator, FALSE, summary);
    benchmark_allocations(heapAllocator, TRUE, summary);
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
