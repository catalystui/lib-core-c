/* Independent native C reference for the portable performance executable. */
#include "perf_cattelib_native.h"
#include <string.h>

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
    void native_##name##_encode(NativeData* data) { \
        data->status = encode(encoding, data->scalar, data->output, &data->count); \
    } \
    void native_##name##_decode(NativeData* data) { \
        data->status = decode(encoding, data->input, &data->scalar, &data->count); \
    } \
    void native_##name##_length(NativeData* data) { \
        data->status = length(encoding, data->input, &data->count); \
    }
#define DEFINE_CONVERSION(from, to, source, destination) \
    void native_##from##_to_##to(NativeData* data) { \
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
