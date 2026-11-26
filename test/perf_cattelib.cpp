#include <cstdio>
#include <ctime>
#include <climits>

#include "catalyst.hpp"
#include "cattelib.h"
#include "perf_cattelib_native.h"

using namespace catalyst;

struct CatalystData {
    CODEPOINT scalar;
    const BYTE* input;
    UTF16LE input16;
    BYTE output[PERF_BYTES];
    UTF16LE_CODE_UNIT output16[PERF_UTF16_UNITS];
    NUINT count;
    STATUS_CODE status;
};

// ------------------------------------------------------------
// Catalyst comparison: only Core types and CATTELIB operations.
// ------------------------------------------------------------
static void compare_ascii_encode(CatalystData* d) {
    d->status = catte_ascii_encode(d->scalar, d->output).status;
    d->count = 1;
}
static void compare_ascii_decode(CatalystData* d) {
    d->scalar = catte_ascii_decode(d->input);
    d->count = 1;
    d->status = STATUS_CODE_SUCCESS;
}
static void compare_ascii_length(CatalystData* d) {
    d->count = catte_ascii_length(d->input);
    d->status = STATUS_CODE_SUCCESS;
}
static void compare_cp1252_encode(CatalystData* d) {
    d->status = catte_cp1252_encode(d->scalar, d->output).status;
    d->count = 1;
}
static void compare_cp1252_decode(CatalystData* d) {
    d->status = catte_cp1252_decode(d->input, &d->scalar).status;
    d->count = 1;
}
static void compare_cp1252_length(CatalystData* d) {
    d->count = catte_cp1252_length(d->input);
    d->status = STATUS_CODE_SUCCESS;
}

#define COMPARE_UNICODE(name, input, output) \
    static void compare_##name##_encode(CatalystData* d) { \
        d->status = catte_##name##_encode(d->scalar, d->output, &d->count).status; \
    } \
    static void compare_##name##_decode(CatalystData* d) { \
        d->status = catte_##name##_decode(d->input, &d->scalar, &d->count).status; \
    } \
    static void compare_##name##_length(CatalystData* d) { \
        d->status = catte_##name##_length(d->input, &d->count).status; \
    }
#define COMPARE_CONVERSION(from, to, input, output, capacity) \
    static void compare_##from##_to_##to(CatalystData* d) { \
        d->status = catte_##from##_to_##to(d->input, d->output, capacity).status; \
    }
COMPARE_UNICODE(utf8, input, output)
COMPARE_UNICODE(utf16le, input16, output16)

static void compare_ascii_to_cp1252(CatalystData* d) {
    d->status = catte_ascii_to_cp1252(d->input, d->output).status;
}
static void compare_cp1252_to_ascii(CatalystData* d) {
    d->status = catte_cp1252_to_ascii(d->input, d->output).status;
}
COMPARE_CONVERSION(ascii, utf8, input, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(ascii, utf16le, input, output16, (NUINT) PERF_UTF16_UNITS)
COMPARE_CONVERSION(cp1252, utf8, input, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(cp1252, utf16le, input, output16, (NUINT) PERF_UTF16_UNITS)
COMPARE_CONVERSION(utf8, ascii, input, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(utf8, cp1252, input, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(utf8, utf16le, input, output16, (NUINT) PERF_UTF16_UNITS)
COMPARE_CONVERSION(utf16le, ascii, input16, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(utf16le, cp1252, input16, output, (NUINT) PERF_BYTES)
COMPARE_CONVERSION(utf16le, utf8, input16, output, (NUINT) PERF_BYTES)

#undef COMPARE_UNICODE
#undef COMPARE_CONVERSION

// ------------------------------------------------------------
// Identical logical workloads and independent output verification.
// ------------------------------------------------------------
struct Benchmark {
    const char* name;
    BYTE source;
    BYTE destination;
    void (*native)(NativeData*);
    void (*comparison)(CatalystData*);
};

#define BENCH(name, source, destination) \
    { "catte_" #name, source, destination, native_##name, compare_##name }
#define ENCODING(name, encoding) \
    BENCH(name##_encode, encoding, encoding), \
    BENCH(name##_decode, encoding, encoding), \
    BENCH(name##_length, encoding, encoding)
static const Benchmark benchmarks[] = {
    ENCODING(ascii, PERF_ASCII),
    ENCODING(cp1252, PERF_CP1252),
    ENCODING(utf8, PERF_UTF8),
    ENCODING(utf16le, PERF_UTF16LE),
    BENCH(ascii_to_cp1252, PERF_ASCII, PERF_CP1252),
    BENCH(ascii_to_utf8, PERF_ASCII, PERF_UTF8),
    BENCH(ascii_to_utf16le, PERF_ASCII, PERF_UTF16LE),
    BENCH(cp1252_to_ascii, PERF_CP1252, PERF_ASCII),
    BENCH(cp1252_to_utf8, PERF_CP1252, PERF_UTF8),
    BENCH(cp1252_to_utf16le, PERF_CP1252, PERF_UTF16LE),
    BENCH(utf8_to_ascii, PERF_UTF8, PERF_ASCII),
    BENCH(utf8_to_cp1252, PERF_UTF8, PERF_CP1252),
    BENCH(utf8_to_utf16le, PERF_UTF8, PERF_UTF16LE),
    BENCH(utf16le_to_ascii, PERF_UTF16LE, PERF_ASCII),
    BENCH(utf16le_to_cp1252, PERF_UTF16LE, PERF_CP1252),
    BENCH(utf16le_to_utf8, PERF_UTF16LE, PERF_UTF8)
};
#undef BENCH
#undef ENCODING

struct Workload {
    const char* name;
    BYTE scalar[4];
    BYTE ascii;
    BYTE cp1252;
    BYTE utf8[4];
    BYTE utf8Count;
    BYTE utf16[4];
    BYTE utf16Count;
};

static const Workload workloads[] = {
    { "ASCII", {0, 0, 0, 0x41}, 0x41, 0x41,
        {0x41, 0, 0, 0}, 1, {0x41, 0, 0, 0}, 1 },
    { "Latin-1", {0, 0, 0, 0xE9}, 0x42, 0xE9,
        {0xC3, 0xA9, 0, 0}, 2, {0xE9, 0, 0, 0}, 1 },
    { "CP1252 extension", {0, 0, 0x20, 0xAC}, 0x43, 0x80,
        {0xE2, 0x82, 0xAC, 0}, 3, {0xAC, 0x20, 0, 0}, 1 },
    { "Supplementary / undefined CP1252", {0, 1, 0xF6, 0}, 0x44, 0x81,
        {0xF0, 0x9F, 0x98, 0x80}, 4, {0x3D, 0xD8, 0, 0xDE}, 2 }
};

struct Input {
    BYTE bytes[PERF_BYTES];
    UTF16LE_CODE_UNIT units[PERF_UTF16_UNITS];
};

static void prepare(const Benchmark& benchmark, const Workload& work, BYTE pairs,
                    Input& input, NativeData& native, CatalystData& comparison) {
    NUINT position = 0;
    // Strings alternate the representative scalar with ASCII 'x'.
    for (BYTE i = 0; i < pairs; ++i) {
        if (benchmark.source == PERF_UTF16LE) {
            for (BYTE j = 0; j < work.utf16Count; ++j) {
                input.units[position++] = UTF16LE_CODE_UNIT(work.utf16[j * 2 + 1], work.utf16[j * 2]);
            }
            input.units[position++] = UTF16LE_CODE_UNIT(0, 0x78);
        } else {
            if (benchmark.source == PERF_UTF8) {
                for (BYTE j = 0; j < work.utf8Count; ++j) input.bytes[position++] = work.utf8[j];
            } else {
                input.bytes[position++] = benchmark.source == PERF_ASCII ? work.ascii : work.cp1252;
            }
            input.bytes[position++] = 0x78;
        }
    }
    if (benchmark.source == PERF_UTF16LE) {
        input.units[position] = UTF16LE_CODE_UNIT(0, 0);
        native.input = (const unsigned char*) input.units;
    } else {
        input.bytes[position] = 0;
        native.input = input.bytes;
    }
    comparison.input = input.bytes;
    comparison.input16 = input.units;
    comparison.scalar = CODEPOINT(work.scalar[0], work.scalar[1], work.scalar[2], work.scalar[3]);
    // Native representation is prepared outside the timed region.
    native.scalar = ((NativeScalar) work.scalar[0] << 24) |
        ((NativeScalar) work.scalar[1] << 16) | ((NativeScalar) work.scalar[2] << 8) | work.scalar[3];
    native.count = 0;
    native.status = 0;
    comparison.count = 0;
    comparison.status = 0;
    for (BYTE i = 0; i < PERF_BYTES; ++i) {
        native.output[i] = 0xA5;
        comparison.output[i] = 0xA5;
    }
    for (BYTE i = 0; i < PERF_UTF16_UNITS; ++i)
        comparison.output16[i] = UTF16LE_CODE_UNIT(0xA5, 0xA5);
}

static BOOL matches(const Benchmark& benchmark, const NativeData& native, const CatalystData& comparison) {
    if (native.status != comparison.status || native.count != comparison.count) return FALSE;
    if ((BYTE) (native.scalar >> 24) != comparison.scalar.byte0 ||
        (BYTE) (native.scalar >> 16) != comparison.scalar.byte1 ||
        (BYTE) (native.scalar >> 8) != comparison.scalar.byte2 ||
        (BYTE) native.scalar != comparison.scalar.byte3) return FALSE;
    for (BYTE i = 0; i < PERF_BYTES; ++i) {
        BYTE actual = benchmark.destination == PERF_UTF16LE
            ? ((i & 1) ? comparison.output16[i / 2].msb : comparison.output16[i / 2].lsb)
            : comparison.output[i];
        if (native.output[i] != actual) return FALSE;
    }
    return TRUE;
}

// ------------------------------------------------------------
// Portable timing harness. Native clock/counter types are confined
// to the executable, not the Catalyst comparison or Core library.
// ------------------------------------------------------------
#if defined(CLOCKS_PER_SEC)
static volatile BYTE observedStatus = 0;

template<typename Data>
static BOOL measure(void (*function)(Data*), Data& data, unsigned long calls, unsigned long* ticks) {
    // The volatile indirect call prevents LTO from removing repeated operations.
    void (*volatile operation)(Data*) = function;
    const std::clock_t start = std::clock();
    if (start == (std::clock_t) -1 || start < 0) return FALSE;
    for (unsigned long i = 0; i < calls; ++i) {
        operation(&data);
        observedStatus = (BYTE) data.status;
    }
    const std::clock_t end = std::clock();
    if (end == (std::clock_t) -1 || end < start) return FALSE;
    if (end - start > ULONG_MAX) return FALSE;
    *ticks = (unsigned long) (end - start);
    return TRUE;
}

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

/* Absolute time per call, with integer arithmetic confined to reporting. */
static void print_nanoseconds(unsigned long ticks, unsigned long calls) {
    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long value = ticks / frequency, remainder = ticks % frequency;
    for (BYTE i = 0; i < 9; ++i) {
        unsigned long digit = decimal_digit(&remainder, frequency);
        if (value > (ULONG_MAX - digit) / 10) {
            std::printf(" %12s", "n/a");
            return;
        }
        value = value * 10 + digit;
    }
    if (value < calls) std::printf(" %12s", "<1");
    else std::printf(" %12lu", value / calls);
}

/* Print an integer percent without requiring floating point or overflowing. */
static void print_ratio(unsigned long native, unsigned long comparison) {
    unsigned long whole = comparison / native;
    unsigned long remainder = comparison % native;
    unsigned long fraction = decimal_digit(&remainder, native) * 10;
    fraction += decimal_digit(&remainder, native);
    if (whole <= (ULONG_MAX - fraction) / 100)
        std::printf("%lu%%", whole * 100 + fraction);
    else
        std::printf("out of range");
}

static BOOL benchmark_case(const Benchmark& benchmark, const Workload& work, BYTE pairs, BOOL* unresolved) {
    Input input;
    NativeData native;
    CatalystData comparison;
    prepare(benchmark, work, pairs, input, native, comparison);
    benchmark.native(&native);
    benchmark.comparison(&comparison);
    if (!matches(benchmark, native, comparison)) {
        std::printf("FAIL: %s / %s (output or status mismatch)\n", benchmark.name, work.name);
        return FALSE;
    }
    unsigned long target = (unsigned long) CLOCKS_PER_SEC / 40; // about 25 ms
    if (target < 20) target = 20;
    unsigned long calls = 16;
    unsigned long n = 0, c = 0;
    const unsigned long maxCalls = 4194304UL;
    // Shared calibrated iteration count; limit slow cases and coarse clocks.
    for (;;) {
        if (!measure(benchmark.native, native, calls, &n) ||
            !measure(benchmark.comparison, comparison, calls, &c)) {
            std::printf("SKIP: %s / %s (clock unavailable or wrapped)\n", benchmark.name, work.name);
            *unresolved = TRUE;
            return TRUE;
        }
        if (n == 0 && c == 0 && calls >= 65536UL) break;
        if ((n >= target && c >= target) || calls == maxCalls ||
            n >= (unsigned long) CLOCKS_PER_SEC || c >= (unsigned long) CLOCKS_PER_SEC) break;
        calls *= 2;
    }
    unsigned long nativeSamples[3], catalystSamples[3];
    for (BYTE sample = 0; sample < 3; ++sample) {
        BOOL ok;
        if (sample == 1) {
            ok = measure(benchmark.comparison, comparison, calls, &catalystSamples[sample]) &&
                 measure(benchmark.native, native, calls, &nativeSamples[sample]);
        } else {
            ok = measure(benchmark.native, native, calls, &nativeSamples[sample]) &&
                 measure(benchmark.comparison, comparison, calls, &catalystSamples[sample]);
        }
        if (!ok) {
            std::printf("SKIP: %s / %s (clock unavailable or wrapped)\n", benchmark.name, work.name);
            *unresolved = TRUE;
            return TRUE;
        }
        if (!matches(benchmark, native, comparison)) {
            std::printf("FAIL: %s / %s (timed output or status mismatch)\n", benchmark.name, work.name);
            return FALSE;
        }
    }
    n = median(nativeSamples);
    c = median(catalystSamples);
    std::printf("  %-34s %8lu %12lu %12lu", work.name, calls, n, c);
    if (n >= target && c >= target) {
        print_nanoseconds(n, calls);
        print_nanoseconds(c, calls);
        std::printf("  ");
        print_ratio(n, c);
    } else {
        std::printf(" %12s %12s  insufficient resolution", "-", "-");
        *unresolved = TRUE;
    }
    std::printf("\n");
    return TRUE;
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATTELIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATTELIB performance (Release recommended)\n");
    std::printf("Native-C = benchmark (100%%); Catalyst = comparison. Lower is faster.\n");
    std::printf("Median of 3 paired samples; %lu clock ticks/second; no I/O in timed regions.\n",
        (unsigned long) CLOCKS_PER_SEC);
    std::printf("Both paths consume status and validate output; Native-C has no diagnostic metadata.\n");
    std::printf("Scalar cases use the first unit/scalar; strings use 16 scalars.\n");
    std::printf("Ticks are total measured time; ns/call is rounded down (not timer precision).\n");
    std::printf("  %-34s %8s %12s %12s %12s %12s  %s\n", "Workload", "Calls", "Native ticks", "Cat. ticks", "Native ns/op", "Cat. ns/op", "Relative");
    BOOL failed = FALSE, unresolved = FALSE;
    for (BYTE i = 0; i < sizeof(benchmarks) / sizeof(benchmarks[0]); ++i) {
        std::printf("\n%s\n", benchmarks[i].name);
        for (BYTE j = 0; j < sizeof(workloads) / sizeof(workloads[0]); ++j) {
            if (!benchmark_case(benchmarks[i], workloads[j], 8, &unresolved)) failed = TRUE;
        }
    }
    if (failed) {
        std::printf("\nCATTELIB performance verification failed.\n");
        return 1;
    }
    if (unresolved) {
        std::printf("\nSome measurements could not be resolved by this clock.\n");
        return 2;
    }
    std::printf("\nAll CATTELIB performance comparisons verified.\n");
    return 0;
#else
    std::printf("CATTELIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
