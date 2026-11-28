#include <cstdio>
#include <ctime>
#include <climits>
#include <stddef.h>
#include <string.h>

#if defined(_MSVC_LANG)
    #define PERF_CPP_STANDARD _MSVC_LANG
#else
    #define PERF_CPP_STANDARD __cplusplus
#endif
#if PERF_CPP_STANDARD >= 201103L && PERF_CPP_STANDARD <= 202302L
    #if defined(__has_include)
        #if __has_include(<codecvt>)
            #define PERF_STANDARD_UNICODE 1
        #endif
    #else
        #define PERF_STANDARD_UNICODE 1
    #endif
#endif
#ifndef PERF_STANDARD_UNICODE
    #define PERF_STANDARD_UNICODE 0
#endif
#if PERF_STANDARD_UNICODE
    #include <codecvt>
    #include <cwchar>
#endif

#include "catalyst.hpp"
#include "cattelib.hpp"

// ------------------------------------------------------------
// Baseline: standard C/C++ facilities and contract-specific references.
// ------------------------------------------------------------
/* C90 reference: use the smallest conventional native scalar that holds Unicode. */
#if UINT_MAX >= 0xFFFFFFFFUL
typedef unsigned int NativeScalar;
#else
typedef unsigned long NativeScalar;
#endif

enum { PERF_BYTES = 52, PERF_UTF16_UNITS = PERF_BYTES / 2, PERF_SCALARS = 16 };
enum { PERF_ASCII, PERF_CP1252, PERF_UTF8, PERF_UTF16LE };

typedef struct NativeData {
    NativeScalar scalar;
    const unsigned char* input;
    unsigned char output[PERF_BYTES];
    size_t count;
    unsigned char status;
} NativeData;

enum { OK = 0, PARTIAL = 0x41, INVALID = 0x81, MALFORMED = 0x83, NATIVE_OVERFLOW = 0x90 };

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

#if PERF_STANDARD_UNICODE
// Facets take byte ranges and emit Unicode scalars without allocating
// strings. UTF-16LE is external little-endian bytes, not native wchar_t.
static std::codecvt_utf8<char32_t> standard_utf8;
static std::codecvt_utf16<char32_t, 0x10FFFF, std::little_endian> standard_utf16le;

static const std::codecvt<char32_t, char, std::mbstate_t>& unicode_facet(int encoding) {
    if (encoding == PERF_UTF16LE) return standard_utf16le;
    return standard_utf8;
}

static size_t unicode_scalar_bytes(int encoding, const unsigned char* input) {
    if (encoding == PERF_UTF16LE) {
        NativeScalar first = read16(input);
        return first >= 0xD800 && first <= 0xDBFF ? 4 : 2;
    }
    unsigned char first = *input;
    if (first < 0x80) return 1;
    if (first >= 0xC2 && first <= 0xDF) return 2;
    if (first >= 0xE0 && first <= 0xEF) return 3;
    if (first >= 0xF0 && first <= 0xF4) return 4;
    return 0;
}

static unsigned char standard_encode(int encoding, NativeScalar value,
                                     unsigned char* output, size_t* count) {
    char32_t scalar = (char32_t) value;
    std::mbstate_t state = std::mbstate_t();
    const char32_t* consumed = 0;
    char* begin = (char*) output;
    char* end = begin;
    std::codecvt_base::result result = unicode_facet(encoding).out(
        state, &scalar, &scalar + 1, consumed, begin, begin + 4, end);
    if (result != std::codecvt_base::ok || consumed != &scalar + 1) {
        *count = 0;
        return INVALID;
    }
    *count = (size_t) (end - begin) / (size_t) unit_bytes(encoding);
    return OK;
}

static unsigned char standard_decode(int encoding, const unsigned char* input,
                                     NativeScalar* scalar, size_t* count) {
    size_t bytes = unicode_scalar_bytes(encoding, input);
    *scalar = 0;
    *count = 0;
    if (bytes == 0) return MALFORMED;
    char32_t value = 0;
    std::mbstate_t state = std::mbstate_t();
    const char* begin = (const char*) input;
    const char* consumed = begin;
    char32_t* end = &value;
    std::codecvt_base::result result = unicode_facet(encoding).in(
        state, begin, begin + bytes, consumed, &value, &value + 1, end);
    if (result != std::codecvt_base::ok || consumed != begin + bytes ||
        end != &value + 1 || value > 0x10FFFFUL ||
        (value >= 0xD800 && value <= 0xDFFF)) return MALFORMED;
    *scalar = (NativeScalar) value;
    *count = bytes / (size_t) unit_bytes(encoding);
    return OK;
}

// Input extents are valid, terminated benchmark strings of at most 16 scalars.
static size_t unicode_bytes(int encoding, const unsigned char* input) {
    if (encoding == PERF_UTF8) return strlen((const char*) input);
    const unsigned char* end = input;
    while (!terminated(encoding, end)) end += 2;
    return (size_t) (end - input);
}

static unsigned char standard_read_string(int encoding, const unsigned char* input,
                                          char32_t* scalars, size_t* count) {
    const char* begin = (const char*) input;
    const char* end = begin + unicode_bytes(encoding, input);
    const char* consumed = begin;
    char32_t* produced = scalars;
    std::mbstate_t state = std::mbstate_t();
    std::codecvt_base::result result = unicode_facet(encoding).in(
        state, begin, end, consumed, scalars, scalars + PERF_SCALARS, produced);
    *count = 0;
    if (result != std::codecvt_base::ok || consumed != end) return MALFORMED;
    for (const char32_t* p = scalars; p != produced; ++p)
        if (*p > 0x10FFFFUL || (*p >= 0xD800 && *p <= 0xDFFF)) return MALFORMED;
    *count = (size_t) (produced - scalars);
    return OK;
}
#endif

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
#if PERF_STANDARD_UNICODE
    return standard_encode(encoding, value, output, count);
#else
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
#endif
}

static unsigned char decode(int encoding, const unsigned char* input, NativeScalar* scalar, size_t* count) {
    NativeScalar value;
    NativeScalar low;
    size_t n, i;
    *scalar = 0;
    *count = 0;
#if PERF_STANDARD_UNICODE
    if (encoding == PERF_UTF8 || encoding == PERF_UTF16LE)
        return standard_decode(encoding, input, scalar, count);
#endif
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
#if PERF_STANDARD_UNICODE
    char32_t scalars[PERF_SCALARS];
    return standard_read_string(encoding, input, scalars, count);
#else
    while (!terminated(encoding, input)) {
        status = decode(encoding, input, &value, &consumed);
        if (status != OK) return status;
        input += consumed * unit_bytes(encoding);
        ++total;
    }
    *count = total;
    return OK;
#endif
}

static unsigned char convert(int from, int to, const unsigned char* input, unsigned char* output) {
    unsigned char status = OK, step, encoded[4];
    size_t consumed, produced, written = 0, bytes;
    NativeScalar value;
    const size_t width = (size_t) unit_bytes(to);
    // All-ASCII strings have identical byte representations in these
    // encodings. Validate once and use the C library's bulk copy.
    if ((from == PERF_ASCII && (to == PERF_CP1252 || to == PERF_UTF8)) ||
        (from == PERF_CP1252 && to == PERF_ASCII)) {
        size_t length = strlen((const char*) input);
        size_t i = 0;
        while (i < length && input[i] < 0x80) ++i;
        if (i == length) {
            if (to == PERF_UTF8 && length >= PERF_BYTES) return NATIVE_OVERFLOW;
            memcpy(output, input, length + 1);
            return OK;
        }
    }
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
#if PERF_STANDARD_UNICODE
    // Decode/encode entire Unicode strings with one library call per side.
    // ASCII/CP1252 substitution still follows the explicit Core contract.
    if (from == PERF_UTF8 || from == PERF_UTF16LE || to == PERF_UTF8 || to == PERF_UTF16LE) {
        char32_t scalars[PERF_SCALARS];
        size_t count = 0;
        if (from == PERF_UTF8 || from == PERF_UTF16LE) {
            step = standard_read_string(from, input, scalars, &count);
            if (step != OK) return step;
        } else {
            while (*input != 0) {
                if (count == PERF_SCALARS) return NATIVE_OVERFLOW;
                step = decode(from, input, &value, &consumed);
                if (from == PERF_ASCII && value >= 0x80) { value = 0x3F; step = PARTIAL; }
                if (step == PARTIAL) status = PARTIAL;
                else if (step != OK) return step;
                scalars[count++] = (char32_t) value;
                input += consumed;
            }
        }
        if (to == PERF_UTF8 || to == PERF_UTF16LE) {
            std::mbstate_t state = std::mbstate_t();
            const char32_t* consumedScalars = scalars;
            char* begin = (char*) output;
            char* end = begin;
            std::codecvt_base::result result = unicode_facet(to).out(
                state, scalars, scalars + count, consumedScalars,
                begin, begin + PERF_BYTES - width, end);
            if (result != std::codecvt_base::ok || consumedScalars != scalars + count)
                return result == std::codecvt_base::partial ? NATIVE_OVERFLOW : INVALID;
            memset(end, 0, width);
            return status;
        }
        if (count >= PERF_BYTES) return NATIVE_OVERFLOW;
        for (size_t i = 0; i < count; ++i) {
            step = encode(to, (NativeScalar) scalars[i], output + i, &produced);
            if (step == PARTIAL) status = PARTIAL;
            else if (step != OK) return step;
        }
        output[count] = 0;
        return status;
    }
#endif
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
        if (bytes > PERF_BYTES - written - width) return NATIVE_OVERFLOW;
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
    d->status = catalyst::ascii_encode(d->scalar, d->output).status;
    d->count = 1;
}
static void compare_ascii_decode(CatalystData* d) {
    d->scalar = catalyst::ascii_decode(d->input);
    d->count = 1;
    d->status = STATUS_CODE_SUCCESS;
}
static void compare_ascii_length(CatalystData* d) {
    d->count = catalyst::ascii_length(d->input);
    d->status = STATUS_CODE_SUCCESS;
}
static void compare_cp1252_encode(CatalystData* d) {
    d->status = catalyst::cp1252_encode(d->scalar, d->output).status;
    d->count = 1;
}
static void compare_cp1252_decode(CatalystData* d) {
    d->status = catalyst::cp1252_decode(d->input, &d->scalar).status;
    d->count = 1;
}
static void compare_cp1252_length(CatalystData* d) {
    d->count = catalyst::cp1252_length(d->input);
    d->status = STATUS_CODE_SUCCESS;
}

#define COMPARE_UNICODE(name, input, output) \
    static void compare_##name##_encode(CatalystData* d) { \
        d->status = catalyst::name##_encode(d->scalar, d->output, &d->count).status; \
    } \
    static void compare_##name##_decode(CatalystData* d) { \
        d->status = catalyst::name##_decode(d->input, &d->scalar, &d->count).status; \
    } \
    static void compare_##name##_length(CatalystData* d) { \
        d->status = catalyst::name##_length(d->input, &d->count).status; \
    }
#define COMPARE_CONVERSION(from, to, input, output, capacity) \
    static void compare_##from##_to_##to(CatalystData* d) { \
        d->status = catalyst::from##_to_##to(d->input, d->output, capacity).status; \
    }
COMPARE_UNICODE(utf8, input, output)
COMPARE_UNICODE(utf16le, input16, output16)

static void compare_ascii_to_cp1252(CatalystData* d) {
    d->status = catalyst::ascii_to_cp1252(d->input, d->output).status;
}
static void compare_cp1252_to_ascii(CatalystData* d) {
    d->status = catalyst::cp1252_to_ascii(d->input, d->output).status;
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
    BOOL scalarInput;
    void (*native)(NativeData*);
    void (*comparison)(CatalystData*);
};

#define BENCH(name, source, destination, scalarInput) \
    { "catte_" #name, source, destination, scalarInput, native_##name, compare_##name }
#define ENCODING(name, encoding) \
    BENCH(name##_encode, encoding, encoding, TRUE), \
    BENCH(name##_decode, encoding, encoding, FALSE), \
    BENCH(name##_length, encoding, encoding, FALSE)
static const Benchmark benchmarks[] = {
    ENCODING(ascii, PERF_ASCII),
    ENCODING(cp1252, PERF_CP1252),
    ENCODING(utf8, PERF_UTF8),
    ENCODING(utf16le, PERF_UTF16LE),
    BENCH(ascii_to_cp1252, PERF_ASCII, PERF_CP1252, FALSE),
    BENCH(ascii_to_utf8, PERF_ASCII, PERF_UTF8, FALSE),
    BENCH(ascii_to_utf16le, PERF_ASCII, PERF_UTF16LE, FALSE),
    BENCH(cp1252_to_ascii, PERF_CP1252, PERF_ASCII, FALSE),
    BENCH(cp1252_to_utf8, PERF_CP1252, PERF_UTF8, FALSE),
    BENCH(cp1252_to_utf16le, PERF_CP1252, PERF_UTF16LE, FALSE),
    BENCH(utf8_to_ascii, PERF_UTF8, PERF_ASCII, FALSE),
    BENCH(utf8_to_cp1252, PERF_UTF8, PERF_CP1252, FALSE),
    BENCH(utf8_to_utf16le, PERF_UTF8, PERF_UTF16LE, FALSE),
    BENCH(utf16le_to_ascii, PERF_UTF16LE, PERF_ASCII, FALSE),
    BENCH(utf16le_to_cp1252, PERF_UTF16LE, PERF_CP1252, FALSE),
    BENCH(utf16le_to_utf8, PERF_UTF16LE, PERF_UTF8, FALSE)
};
#undef BENCH
#undef ENCODING

struct Workload {
    const char* scalarName;
    const char* asciiName;
    const char* cp1252Name;
    BYTE scalar[4];
    BYTE ascii;
    BYTE cp1252;
    BYTE utf8[4];
    BYTE utf8Count;
    BYTE utf16[4];
    BYTE utf16Count;
};

static const Workload workloads[] = {
    { "A (ASCII)", "A (ASCII)", "A (ASCII)", {0, 0, 0, 0x41}, 0x41, 0x41,
        {0x41, 0, 0, 0}, 1, {0x41, 0, 0, 0}, 1 },
    { "U+00E9 (Latin-1)", "B (ASCII)", "U+00E9 (Latin-1)", {0, 0, 0, 0xE9}, 0x42, 0xE9,
        {0xC3, 0xA9, 0, 0}, 2, {0xE9, 0, 0, 0}, 1 },
    { "U+20AC (Euro)", "C (ASCII)", "0x80 (Euro)", {0, 0, 0x20, 0xAC}, 0x43, 0x80,
        {0xE2, 0x82, 0xAC, 0}, 3, {0xAC, 0x20, 0, 0}, 1 },
    { "U+1F600 (Emoji)", "D (ASCII)", "0x81 (Undefined)", {0, 1, 0xF6, 0}, 0x44, 0x81,
        {0xF0, 0x9F, 0x98, 0x80}, 4, {0x3D, 0xD8, 0, 0xDE}, 2 }
};

static const char* workload_name(const Benchmark& benchmark, const Workload& work) {
    if (benchmark.scalarInput) return work.scalarName;
    if (benchmark.source == PERF_ASCII) return work.asciiName;
    if (benchmark.source == PERF_CP1252) return work.cp1252Name;
    return work.scalarName;
}

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

static BOOL elapsed(std::clock_t start, unsigned long* ticks) {
    const std::clock_t end = std::clock();
    if (start == (std::clock_t) -1 || start < 0 ||
        end == (std::clock_t) -1 || end < start) return FALSE;
    if (end - start > ULONG_MAX) return FALSE;
    *ticks = (unsigned long) (end - start);
    return TRUE;
}

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
    return elapsed(start, ticks);
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
    // Pad the complete signed percentage, not the digits after the sign.
    BYTE digits = 1;
    for (unsigned long remaining = percent; remaining >= 10; remaining /= 10) ++digits;
    for (BYTE i = digits + 4; i < 12; ++i) std::putchar(' ');
    std::printf(" %c%lu.%lu%%", comparison < native ? '-' : '+', percent, tenth);
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

static void report_unavailable(const char* name, Summary& summary) {
    std::printf("  %-22s %14s %14s %12s\n", name, "-", "-", "no timing");
    ++summary.unavailable;
}

static void benchmark_case(const Benchmark& benchmark, const Workload& work, BYTE pairs, Summary& summary) {
    const char* name = workload_name(benchmark, work);
    Input input;
    NativeData native;
    CatalystData comparison;
    prepare(benchmark, work, pairs, input, native, comparison);
    benchmark.native(&native);
    benchmark.comparison(&comparison);
    if (!matches(benchmark, native, comparison)) {
        std::printf("  %-22s FAIL: output or status mismatch\n", name);
        ++summary.failed;
        return;
    }

    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long target = frequency / 100; // Aim for 10 ms per path.
    if (target < 20) target = 20;
    unsigned long minimum = frequency / 1000;
    if (minimum < 20) minimum = 20;
    unsigned long budget = frequency / 10; // 100 ms cumulative calibration CPU budget.
    if (budget < target) budget = target;
    unsigned long calls = 16;
    unsigned long n = 0, c = 0;
    const unsigned long maxCalls = 67108864UL;
    const std::clock_t calibrationStart = std::clock();

    // Estimate the required shared call count rather than repeatedly doubling.
    for (;;) {
        const std::clock_t attemptStart = std::clock();
        if (!measure(benchmark.native, native, calls, &n) ||
            !measure(benchmark.comparison, comparison, calls, &c)) {
            report_unavailable(name, summary);
            return;
        }
        unsigned long totalTicks = 0, attemptTicks = 0;
        if (!elapsed(calibrationStart, &totalTicks) || !elapsed(attemptStart, &attemptTicks)) {
            report_unavailable(name, summary);
            return;
        }
        unsigned long faster = n < c ? n : c;
        if (faster >= target || calls >= maxCalls || totalTicks >= budget ||
            attemptTicks >= budget) break;

        // Bound the estimate, including zero-tick samples and coarse clocks.
        unsigned long factor = faster == 0 ? 8 : target / faster;
        if (factor < 2) factor = 2;
        if (factor > 16) factor = 16;
        if (attemptTicks != 0) {
            unsigned long affordable = (budget - totalTicks) / attemptTicks;
            if (affordable < 2) break;
            if (factor > affordable) factor = affordable;
        }
        if (factor > maxCalls / calls) calls = maxCalls;
        else calls *= factor;
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
            report_unavailable(name, summary);
            return;
        }
        if (!matches(benchmark, native, comparison)) {
            std::printf("  %-22s FAIL: timed output or status mismatch\n", name);
            ++summary.failed;
            return;
        }
    }

    n = median(nativeSamples);
    c = median(catalystSamples);
    if (n == 0 || c == 0) {
        report_unavailable(name, summary);
        return;
    }

    std::printf("  %-22s", name);
    print_nanoseconds(n, calls);
    print_nanoseconds(c, calls);
    print_difference(n, c);

    // A short nonzero sample is informative, but exclude very short samples
    // from the aggregate classification rather than counting them as failures.
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
            } else {
                std::printf("  Slower");
            }
        }
    }
    std::printf("\n");
    std::fflush(stdout);
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATTELIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATTELIB Performance (Release recommended)\n");
    std::printf("Standard library / reference is the baseline. Lower time is better.\n");
    std::printf("Difference: +10%% means Catalyst takes 10%% longer; -10%% means 10%% less time.\n");
    std::printf("Time per call: median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
#if PERF_STANDARD_UNICODE
    std::printf("UTF-8/UTF-16LE baseline: standard C++ codecvt facets; ASCII/CP1252: C library and byte mapping.\n");
#else
    std::printf("UTF-8/UTF-16LE baseline: C-style reference (Unicode facets unavailable); ASCII/CP1252: C library and byte mapping.\n");
#endif
    std::printf("Encode/decode: one character. Length/conversion: 8 repetitions of the named input and x (16 characters).\n");
    std::printf("Both paths validate output and consume status; baseline has no diagnostic metadata.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-22s %14s %14s %12s  %s\n", "Input", "Baseline", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------\n");
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
