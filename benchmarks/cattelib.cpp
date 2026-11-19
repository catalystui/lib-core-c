// Host-only performance and equivalence experiment, not a Core API.
#include <cstdio>
#include <ctime>
#include "catalyst.hpp"
#include "cattelib.h"
using namespace catalyst;

static volatile UINT sink = 0;
static const NUINT SCALARS = 1024;
static const NUINT REPEAT = 1024 * 1024;
static const NUINT STRING_REPEAT = 3000;
static CODEPOINT points[SCALARS];
static BYTE utf8_units[SCALARS][4];
static UTF16LE_CODE_UNIT utf16_units[SCALARS][2];
static BYTE utf8[SCALARS * 4 + 1];
static UTF16LE_CODE_UNIT utf16[SCALARS * 2 + 1];
static BYTE cp1252[SCALARS + 1];
static BYTE output8[SCALARS * 4 + 1];
static UTF16LE_CODE_UNIT output16[SCALARS * 2 + 1];

static void prepare(BOOL mixed) {
    NUINT n8 = 0, n16 = 0;
    for (NUINT i = 0; i < SCALARS; ++i) {
        UINT v = (UINT) 0x20 + (UINT) (i % 95);
        if (mixed) {
            switch (i % 4) {
                case 1: v = (UINT) 0x80 + (UINT) (i % 0x780); break;
                case 2: v = (UINT) 0x800 + (UINT) (i % 0xC000); break;
                case 3: v = (UINT) 0x10000 + (UINT) ((i * 911) % 0x100000); break;
            }
        }
        points[i] = v;
        NUINT p8 = 0, p16 = 0;
        catte_utf8_encode(points[i], utf8_units[i], &p8);
        catte_utf16le_encode(points[i], utf16_units[i], &p16);
        for (NUINT j = 0; j < p8; ++j) utf8[n8++] = utf8_units[i][j];
        for (NUINT j = 0; j < p16; ++j) utf16[n16++] = utf16_units[i][j];
        cp1252[i] = mixed ? (BYTE) (0x80u + (i % 32)) : (BYTE) v;
    }
    utf8[n8] = 0;
    utf16[n16] = UTF16LE_CODE_UNIT(0, 0);
    cp1252[SCALARS] = 0;
}

static const BYTE mapping[32][4] = {
    {0x20, 0xAC, 0x04, 0x1C},
    {0, 0x3F, 0x1F, 0},
    {0x20, 0x1A, 0x05, 0x11},
    {0x01, 0x92, 0x06, 0x0A},
    {0x20, 0x1E, 0x07, 0x14},
    {0x20, 0x26, 0x08, 0x18},
    {0x20, 0x20, 0x09, 0x15},
    {0x20, 0x21, 0x0A, 0x16},
    {0x02, 0xC6, 0x0B, 0x0B},
    {0x20, 0x30, 0x0C, 0x19},
    {0x01, 0x60, 0x0D, 0x05},
    {0x20, 0x39, 0x0E, 0x1A},
    {0x01, 0x52, 0x0F, 0x03},
    {0, 0x3F, 0x1F, 0},
    {0x01, 0x7D, 0x10, 0x08},
    {0, 0x3F, 0x1F, 0},
    {0, 0x3F, 0x1F, 0},
    {0x20, 0x18, 0x11, 0x0F},
    {0x20, 0x19, 0x12, 0x10},
    {0x20, 0x1C, 0x13, 0x12},
    {0x20, 0x1D, 0x14, 0x13},
    {0x20, 0x22, 0x15, 0x17},
    {0x20, 0x13, 0x16, 0x0D},
    {0x20, 0x14, 0x17, 0x0E},
    {0x02, 0xDC, 0x18, 0x0C},
    {0x21, 0x22, 0x19, 0x1D},
    {0x01, 0x61, 0x1A, 0x06},
    {0x20, 0x3A, 0x1B, 0x1B},
    {0x01, 0x53, 0x1C, 0x04},
    {0, 0x3F, 0x1F, 0},
    {0x01, 0x7E, 0x1D, 0x09},
    {0x01, 0x78, 0x1E, 0x07}
};

static __attribute__((noinline)) CATALYST_RESULT table_decode(CATALYST_CP1252 input, CATALYST_CODEPOINT* cp) {
    BYTE b = input[0];
    cp->byte0 = 0; cp->byte1 = 0; cp->byte2 = 0;
    if (b < 0x80 || b >= 0xA0) {
        cp->byte3 = b;
        return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x03, 0);
    }
    const BYTE* row = mapping[b - (BYTE) 0x80];
    cp->byte2 = row[0]; cp->byte3 = row[1];
    return RESULT(row[3] ? STATUS_CODE_SUCCESS : STATUS_CODE_WARNING_PARTIAL, 0x05, row[2], 0);
}
static __attribute__((noinline)) CATALYST_RESULT table_encode(CATALYST_CODEPOINT cp, CATALYST_CP1252W out) {
    if (cp.byte0 == 0 && cp.byte1 == 0 && cp.byte2 == 0 && (cp.byte3 < 0x80 || cp.byte3 >= 0xA0)) {
        out[0] = cp.byte3;
        return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x02, 0);
    }
    if (cp.byte0 == 0 && cp.byte1 == 0) {
        for (BYTE i = 0; i < 32; ++i) {
            if (mapping[i][3] && cp.byte2 == mapping[i][0] && cp.byte3 == mapping[i][1]) {
                out[0] = (BYTE) (0x80 + i);
                return RESULT(STATUS_CODE_SUCCESS, 0x04, mapping[i][3], 0);
            }
        }
    }
    out[0] = 0x3F;
    return RESULT(STATUS_CODE_WARNING_PARTIAL, 0x04, 0x1E, 0);
}
static int check_tables() {
    BYTE b = 0;
    do {
        CODEPOINT x, y;
        BYTE a = 0, c = 0;
        RESULT r = catte_cp1252_decode(&b, &x);
        RESULT t = table_decode(&b, &y);
        if ((UINT) x != (UINT) y || (UINT) r != (UINT) t) return 1;
        r = catte_cp1252_encode(x, &a);
        t = table_encode(x, &c);
        if (a != c || (UINT) r != (UINT) t) return 1;
    } while (++b);
    return 0;
}

extern "C" CATALYST_RESULT inspect_result(CATALYST_BYTE detail) {
    return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0F, detail);
}
extern "C" CATALYST_UINT inspect_packed_result(CATALYST_BYTE detail) {
    return (UINT) RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0F, detail);
}

static double run(int mode) {
    const NUINT repeat = mode < 4 || mode >= 14 ? REPEAT : STRING_REPEAT;
    UINT checksum = 0;
    std::clock_t start = std::clock();
    for (NUINT i = 0; i < repeat; ++i) {
        NUINT index = i % SCALARS;
        CODEPOINT decoded;
        NUINT n = 0;
        RESULT r;
        switch (mode) {
            case 0: r = catte_utf8_encode(points[index], output8, &n); checksum += output8[0] + output8[n-1] + (UINT) n; break;
            case 1: r = catte_utf8_decode(utf8_units[index], &decoded, &n); checksum += (UINT) decoded + (UINT) n; break;
            case 2: r = catte_utf16le_encode(points[index], output16, &n); checksum += output16[0].lsb + output16[n-1].msb + (UINT) n; break;
            case 3: r = catte_utf16le_decode(utf16_units[index], &decoded, &n); checksum += (UINT) decoded + (UINT) n; break;
            case 4: r = catte_utf8_length(utf8, &n); checksum += (UINT) n; break;
            case 5: r = catte_utf16le_length(utf16, &n); checksum += (UINT) n; break;
            case 6: r = catte_utf8_to_utf16le(utf8, output16, SCALARS*2+1); checksum += output16[index].lsb; break;
            case 7: r = catte_utf16le_to_utf8(utf16, output8, SCALARS*4+1); checksum += output8[index]; break;
            case 8: r = catte_utf8_to_ascii(utf8, output8, SCALARS+1); checksum += output8[index]; break;
            case 9: r = catte_utf16le_to_ascii(utf16, output8, SCALARS+1); checksum += output8[index]; break;
            case 10: r = catte_utf8_to_cp1252(utf8, output8, SCALARS+1); checksum += output8[index]; break;
            case 11: r = catte_utf16le_to_cp1252(utf16, output8, SCALARS+1); checksum += output8[index]; break;
            case 12: r = catte_cp1252_to_utf8(cp1252, output8, SCALARS*4+1); checksum += output8[index]; break;
            case 13: r = catte_cp1252_to_utf16le(cp1252, output16, SCALARS*2+1); checksum += output16[index].lsb; break;
            case 14: r = catte_cp1252_decode(cp1252+index, &decoded); checksum += (UINT) decoded; break;
            case 15: r = table_decode(cp1252+index, &decoded); checksum += (UINT) decoded; break;
            case 16: r = catte_cp1252_encode(points[index], output8); checksum += output8[0]; break;
            default: r = table_encode(points[index], output8); checksum += output8[0]; break;
        }
        checksum += (UINT) r;
    }
    double seconds = (double) (std::clock() - start) / (double) CLOCKS_PER_SEC;
    sink = checksum;
    return seconds * 1e9 / (double) repeat;
}

int main() {
    if (check_tables()) return 1;
    const char* names[] = {"utf8_encode","utf8_decode","utf16_encode","utf16_decode","utf8_length","utf16_length",
        "utf8_to_utf16","utf16_to_utf8","utf8_to_ascii","utf16_to_ascii","utf8_to_cp1252","utf16_to_cp1252",
        "cp1252_to_utf8","cp1252_to_utf16","cp1252_decode_switch","cp1252_decode_table","cp1252_encode_switch","cp1252_encode_table"};
    for (BYTE mix = 0; mix <= 1; ++mix) {
        prepare((BOOL) mix);
        for (int mode = 0; mode < 18; ++mode) {
            if (mode >= 14 && mix == 0) continue;
            double a = run(mode), b = run(mode), c = run(mode);
            if (a > b) { double t=a; a=b; b=t; }
            if (b > c) { double t=b; b=c; c=t; }
            if (a > b) b=a;
            std::printf("BENCH %s %s %.3f ns/call checksum=%08lX\n", mix ? "mixed" : "ascii",
                names[mode], b, (unsigned long) sink);
        }
    }
    return 0;
}
