#include <cstdio>
#include <ctime>
#include <climits>
#include <stddef.h>
#include <string.h>

#include "catalyst.hpp"
#include "cattelib.h"

// ------------------------------------------------------------
// Native-C benchmark: independent C-style reference operations.
// ------------------------------------------------------------
/* C90 reference: use the smallest conventional native scalar that holds Unicode. */
#if UINT_MAX >= 0xFFFFFFFFUL
typedef unsigned int NativeScalar;
#else
typedef unsigned long NativeScalar;
#endif

enum { PERF_BYTES = 52, PERF_UTF16_UNITS = PERF_BYTES / 2 };
enum { PERF_ASCII, PERF_CP1252, PERF_UTF8, PERF_UTF16LE };

typedef struct NativeData {
    NativeScalar scalar;
    const unsigned char* input;
    unsigned char output[PERF_BYTES];
    size_t count;
    unsigned char status;
} NativeData;

enum { OK = 0, PARTIAL = 0x41, INVALID = 0x81, MALFORMED = 0x83, OVERFLOW = 0x90 };

/* Undefined CP1252 extension entries are zero; none maps to U+0000. */
static const unsigned short cp1252_extension[32] = {
    0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
    0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178
};

static int unit_bytes(int encoding) {
    return encoding == PERF_UTF16LE ? 2 : 1;
}

static int terminated(int encoding, const unsigned char* input) {
    return input[0] == 0 && (encoding != PERF_UTF16LE || input[1] == 0);
}

static const unsigned char* skip_bom(int encoding, const unsigned char* input) {
    if (encoding == PERF_UTF8 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF)
        return input + 3;
    if (encoding == PERF_UTF16LE && input[0] == 0xFF && input[1] == 0xFE)
        return input + 2;
    return input;
}

static void write16(unsigned char* output, NativeScalar value) {
    output[0] = (unsigned char) value;
    output[1] = (unsigned char) (value >> 8);
}

static NativeScalar read16(const unsigned char* input) {
    return (NativeScalar) input[0] | ((NativeScalar) input[1] << 8);
}

static unsigned char encode(int encoding, NativeScalar value, unsigned char* output, size_t* count) {
    *count = 1;
    if (encoding == PERF_ASCII) {
        output[0] = (unsigned char) (value < 0x80 ? value : 0x3F);
        return value < 0x80 ? OK : PARTIAL;
    }
    if (encoding == PERF_CP1252) {
        if (value < 0x80 || (value >= 0xA0 && value <= 0xFF)) {
            output[0] = (unsigned char) value;
            return OK;
        }
        switch (value) {
            case 0x20AC: output[0] = 0x80; return OK;
            case 0x201A: output[0] = 0x82; return OK;
            case 0x0192: output[0] = 0x83; return OK;
            case 0x201E: output[0] = 0x84; return OK;
            case 0x2026: output[0] = 0x85; return OK;
            case 0x2020: output[0] = 0x86; return OK;
            case 0x2021: output[0] = 0x87; return OK;
            case 0x02C6: output[0] = 0x88; return OK;
            case 0x2030: output[0] = 0x89; return OK;
            case 0x0160: output[0] = 0x8A; return OK;
            case 0x2039: output[0] = 0x8B; return OK;
            case 0x0152: output[0] = 0x8C; return OK;
            case 0x017D: output[0] = 0x8E; return OK;
            case 0x2018: output[0] = 0x91; return OK;
            case 0x2019: output[0] = 0x92; return OK;
            case 0x201C: output[0] = 0x93; return OK;
            case 0x201D: output[0] = 0x94; return OK;
            case 0x2022: output[0] = 0x95; return OK;
            case 0x2013: output[0] = 0x96; return OK;
            case 0x2014: output[0] = 0x97; return OK;
            case 0x02DC: output[0] = 0x98; return OK;
            case 0x2122: output[0] = 0x99; return OK;
            case 0x0161: output[0] = 0x9A; return OK;
            case 0x203A: output[0] = 0x9B; return OK;
            case 0x0153: output[0] = 0x9C; return OK;
            case 0x017E: output[0] = 0x9E; return OK;
            case 0x0178: output[0] = 0x9F; return OK;
        }
        output[0] = 0x3F;
        return PARTIAL;
    }
    if (value > 0x10FFFFUL || (value >= 0xD800 && value <= 0xDFFF)) {
        *count = 0;
        return INVALID;
    }
    if (encoding == PERF_UTF16LE) {
        if (value < 0x10000UL) {
            write16(output, value);
        } else {
            value -= 0x10000UL;
            write16(output, 0xD800 | (value >> 10));
            write16(output + 2, 0xDC00 | (value & 0x3FF));
            *count = 2;
        }
        return OK;
    }
    if (value < 0x80) {
        output[0] = (unsigned char) value;
    } else if (value < 0x800) {
        output[0] = (unsigned char) (0xC0 | (value >> 6));
        output[1] = (unsigned char) (0x80 | (value & 0x3F));
        *count = 2;
    } else if (value < 0x10000UL) {
        output[0] = (unsigned char) (0xE0 | (value >> 12));
        output[1] = (unsigned char) (0x80 | ((value >> 6) & 0x3F));
        output[2] = (unsigned char) (0x80 | (value & 0x3F));
        *count = 3;
    } else {
        output[0] = (unsigned char) (0xF0 | (value >> 18));
        output[1] = (unsigned char) (0x80 | ((value >> 12) & 0x3F));
        output[2] = (unsigned char) (0x80 | ((value >> 6) & 0x3F));
        output[3] = (unsigned char) (0x80 | (value & 0x3F));
        *count = 4;
    }
    return OK;
}

static unsigned char decode(int encoding, const unsigned char* input, NativeScalar* scalar, size_t* count) {
    NativeScalar value;
    NativeScalar low;
    size_t n, i;
    *scalar = 0;
    *count = 0;
    if (encoding == PERF_UTF16LE) {
        value = read16(input);
        if (value >= 0xDC00 && value <= 0xDFFF) return MALFORMED;
        if (value >= 0xD800 && value <= 0xDBFF) {
            low = read16(input + 2);
            if (low < 0xDC00 || low > 0xDFFF) return MALFORMED;
            value = 0x10000UL + ((value - 0xD800) << 10) + (low - 0xDC00);
            *count = 2;
        } else {
            *count = 1;
        }
        *scalar = value;
        return OK;
    }
    value = input[0];
    if (encoding == PERF_ASCII || value < 0x80) {
        *scalar = value;
        *count = 1;
        return OK;
    }
    if (encoding == PERF_CP1252) {
        if (value >= 0xA0) {
            *scalar = value;
        } else {
            *scalar = cp1252_extension[value - 0x80];
            if (*scalar == 0) {
                *scalar = 0x3F;
                *count = 1;
                return PARTIAL;
            }
        }
        *count = 1;
        return OK;
    }
    if (value >= 0xC2 && value <= 0xDF) {
        n = 2; value &= 0x1F;
    } else if (value >= 0xE0 && value <= 0xEF) {
        n = 3; value &= 0x0F;
    } else if (value >= 0xF0 && value <= 0xF4) {
        n = 4; value &= 0x07;
    } else {
        return MALFORMED;
    }
    for (i = 1; i < n; ++i) {
        if (input[i] < 0x80 || input[i] > 0xBF) return MALFORMED;
        value = (value << 6) | (input[i] & 0x3F);
    }
    if ((n == 3 && value < 0x800) || (n == 4 && value < 0x10000UL) ||
        (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFFUL)
        return MALFORMED;
    *scalar = value;
    *count = n;
    return OK;
}

static unsigned char length(int encoding, const unsigned char* input, size_t* count) {
    NativeScalar value;
    size_t consumed, total = 0;
    unsigned char status;
    if (encoding == PERF_ASCII || encoding == PERF_CP1252) {
        *count = strlen((const char*) input);
        return OK;
    }
    *count = 0;
    input = skip_bom(encoding, input);
    while (!terminated(encoding, input)) {
        status = decode(encoding, input, &value, &consumed);
        if (status != OK) return status;
        input += consumed * unit_bytes(encoding);
        ++total;
    }
    *count = total;
    return OK;
}

static unsigned char convert(int from, int to, const unsigned char* input, unsigned char* output) {
    unsigned char status = OK, step, encoded[4];
    size_t consumed, produced, written = 0, bytes;
    NativeScalar value;
    const size_t width = (size_t) unit_bytes(to);
    /* Neither single-byte to single-byte API requires a capacity argument. */
    if ((from == PERF_ASCII && to == PERF_CP1252) ||
        (from == PERF_CP1252 && to == PERF_ASCII)) {
        while (*input) {
            if (*input < 0x80) *output++ = *input++;
            else {
                *output++ = 0x3F;
                ++input;
                status = PARTIAL;
            }
        }
        *output = 0;
        return status;
    }
    input = skip_bom(from, input);
    while (!terminated(from, input)) {
        /* ASCII conversions check the input byte and substitute as CATTELIB does. */
        if (from == PERF_ASCII && input[0] >= 0x80) {
            value = 0x3F;
            consumed = 1;
            step = PARTIAL;
        } else {
            step = decode(from, input, &value, &consumed);
        }
        if (step == MALFORMED) return step;
        if (step == PARTIAL) status = PARTIAL;
        step = encode(to, value, encoded, &produced);
        if (step == PARTIAL) status = PARTIAL;
        else if (step != OK) return step;
        bytes = produced * width;
        if (bytes > PERF_BYTES - written - width) return OVERFLOW;
        memcpy(output + written, encoded, bytes);
        written += bytes;
        input += consumed * unit_bytes(from);
    }
    memset(output + written, 0, width);
    return status;
}

#define DEFINE_ENCODING(name, encoding) \
    static void native_##name##_encode(NativeData* data) { \
        data->status = encode(encoding, data->scalar, data->output, &data->count); \
    } \
    static void native_##name##_decode(NativeData* data) { \
        data->status = decode(encoding, data->input, &data->scalar, &data->count); \
    } \
    static void native_##name##_length(NativeData* data) { \
        data->status = length(encoding, data->input, &data->count); \
    }
#define DEFINE_CONVERSION(from, to, source, destination) \
    static void native_##from##_to_##to(NativeData* data) { \
        data->status = convert(source, destination, data->input, data->output); \
    }

DEFINE_ENCODING(ascii, PERF_ASCII)
DEFINE_ENCODING(cp1252, PERF_CP1252)
DEFINE_ENCODING(utf8, PERF_UTF8)
DEFINE_ENCODING(utf16le, PERF_UTF16LE)
DEFINE_CONVERSION(ascii, cp1252, PERF_ASCII, PERF_CP1252)
DEFINE_CONVERSION(ascii, utf8, PERF_ASCII, PERF_UTF8)
DEFINE_CONVERSION(ascii, utf16le, PERF_ASCII, PERF_UTF16LE)
DEFINE_CONVERSION(cp1252, ascii, PERF_CP1252, PERF_ASCII)
DEFINE_CONVERSION(cp1252, utf8, PERF_CP1252, PERF_UTF8)
DEFINE_CONVERSION(cp1252, utf16le, PERF_CP1252, PERF_UTF16LE)
DEFINE_CONVERSION(utf8, ascii, PERF_UTF8, PERF_ASCII)
DEFINE_CONVERSION(utf8, cp1252, PERF_UTF8, PERF_CP1252)
DEFINE_CONVERSION(utf8, utf16le, PERF_UTF8, PERF_UTF16LE)
DEFINE_CONVERSION(utf16le, ascii, PERF_UTF16LE, PERF_ASCII)
DEFINE_CONVERSION(utf16le, cp1252, PERF_UTF16LE, PERF_CP1252)
DEFINE_CONVERSION(utf16le, utf8, PERF_UTF16LE, PERF_UTF8)

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
    { "Supplementary / undef.", {0, 1, 0xF6, 0}, 0x44, 0x81,
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
    std::printf(" %c%7lu.%lu%%", comparison < native ? '-' : '+', percent, tenth);
}

struct Summary {
    BYTE faster;
    BYTE similar;
    BYTE slower;
    BYTE review;
    BYTE shortSamples;
    BYTE unavailable;
    BYTE failed;
};

static void report_unavailable(const Workload& work, Summary& summary) {
    std::printf("  %-22s %14s %14s %12s\n", work.name, "-", "-", "no timing");
    ++summary.unavailable;
}

static void benchmark_case(const Benchmark& benchmark, const Workload& work, BYTE pairs, Summary& summary) {
    Input input;
    NativeData native;
    CatalystData comparison;
    prepare(benchmark, work, pairs, input, native, comparison);
    benchmark.native(&native);
    benchmark.comparison(&comparison);
    if (!matches(benchmark, native, comparison)) {
        std::printf("  %-22s FAIL: output or status mismatch\n", work.name);
        ++summary.failed;
        return;
    }

    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long target = frequency / 40; // Aim for 25 ms per sample.
    if (target < 20) target = 20;
    unsigned long minimum = frequency / 1000; // Short but useful: at least 1 ms and 20 ticks.
    if (minimum < 20) minimum = 20;
    unsigned long budget = frequency / 4; // Avoid excessive calibration on slower targets.
    if (budget < target) budget = target;
    unsigned long calls = 16;
    unsigned long n = 0, c = 0;
    const unsigned long maxCalls = 67108864UL;

    // Calibrate the pair with the same call count. A zero reading at a small
    // batch is normal; keep growing until the clock advances or a limit is hit.
    for (;;) {
        if (!measure(benchmark.native, native, calls, &n) ||
            !measure(benchmark.comparison, comparison, calls, &c)) {
            report_unavailable(work, summary);
            return;
        }
        if ((n >= target && c >= target) || calls == maxCalls ||
            n >= budget || c >= budget) break;
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
            report_unavailable(work, summary);
            return;
        }
        if (!matches(benchmark, native, comparison)) {
            std::printf("  %-22s FAIL: timed output or status mismatch\n", work.name);
            ++summary.failed;
            return;
        }
    }

    n = median(nativeSamples);
    c = median(catalystSamples);
    if (n == 0 || c == 0) {
        report_unavailable(work, summary);
        return;
    }

    std::printf("  %-22s", work.name);
    print_nanoseconds(n, calls);
    print_nanoseconds(c, calls);
    print_difference(n, c);

    // A short nonzero sample is informative, but exclude very short samples
    // from the aggregate classification rather than counting them as failures.
    if (n < minimum || c < minimum) {
        std::printf(" *");
        ++summary.shortSamples;
    } else {
        unsigned long delta = c >= n ? c - n : n - c;
        if (delta <= n / 20) ++summary.similar;
        else if (c < n) ++summary.faster;
        else {
            ++summary.slower;
            if (delta >= n / 5 + (n % 5 != 0 ? 1 : 0)) ++summary.review;
        }
    }
    std::printf("\n");
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATTELIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATTELIB Performance (Release recommended)\n");
    std::printf("Native-C is the benchmark. Negative = faster, positive = slower.\n");
    std::printf("Median of 3 paired samples; time per call (16 scalars per string).\n");
    std::printf("Both paths validate output and consume status; Native-C has no diagnostic metadata.\n");
    std::printf("Displayed decimal places are estimates, not clock precision.\n");
    std::printf("\n  %-22s %14s %14s %12s\n", "Workload", "Native-C", "Catalyst", "Difference");
    std::printf("  -----------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
    for (BYTE i = 0; i < sizeof(benchmarks) / sizeof(benchmarks[0]); ++i) {
        std::printf("\n%s\n", benchmarks[i].name);
        for (BYTE j = 0; j < sizeof(workloads) / sizeof(workloads[0]); ++j) {
            benchmark_case(benchmarks[i], workloads[j], 8, summary);
        }
    }

    std::printf("\nPerformance summary\n");
    std::printf("  Faster (>5%%):                  %3u\n", (unsigned int) summary.faster);
    std::printf("  Comparable (within 5%%):        %3u\n", (unsigned int) summary.similar);
    std::printf("  Slower (>5%%):                  %3u\n", (unsigned int) summary.slower);
    std::printf("    At least 20%% slower:          %3u (review)\n", (unsigned int) summary.review);
    std::printf("  Short samples (*):             %3u\n", (unsigned int) summary.shortSamples);
    std::printf("  Timing unavailable:            %3u\n", (unsigned int) summary.unavailable);
    std::printf("  Verification failures:         %3u\n", (unsigned int) summary.failed);
    if (summary.shortSamples != 0)
        std::printf("  * Less than 1 ms or 20 ticks; excluded from performance classification.\n");
    if (summary.failed != 0) return 1;
    std::printf("\nAll CATTELIB performance comparisons verified.\n");
    return summary.unavailable != 0 ? 2 : 0;
#else
    std::printf("CATTELIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
