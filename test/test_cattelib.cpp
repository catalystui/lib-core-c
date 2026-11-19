#include <cstdio>

#include "catalyst.hpp"
#include "cattelib.hpp"

using namespace catalyst;

struct Cp1252EncodeTest {
    const char* name;
    BYTE byte0;
    BYTE byte1;
    BYTE byte2;
    BYTE byte3;
    BYTE expectedCodeunit;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
};

struct Cp1252DecodeTest {
    const char* name;
    BYTE codeunit;
    BYTE expectedByte0;
    BYTE expectedByte1;
    BYTE expectedByte2;
    BYTE expectedByte3;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
};

struct Utf8EncodeTest {
    const char* name;
    BYTE byte0;
    BYTE byte1;
    BYTE byte2;
    BYTE byte3;
    BYTE expected0;
    BYTE expected1;
    BYTE expected2;
    BYTE expected3;
    NUINT expectedProduced;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
};

struct Utf8DecodeTest {
    const char* name;
    BYTE input0;
    BYTE input1;
    BYTE input2;
    BYTE input3;
    BYTE expectedByte0;
    BYTE expectedByte1;
    BYTE expectedByte2;
    BYTE expectedByte3;
    NUINT expectedConsumed;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
    DETAIL_CODE expectedDetail;
};

struct Utf16leEncodeTest {
    const char* name;
    BYTE byte0;
    BYTE byte1;
    BYTE byte2;
    BYTE byte3;
    BYTE expected0msb;
    BYTE expected0lsb;
    BYTE expected1msb;
    BYTE expected1lsb;
    NUINT expectedProduced;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
};

struct Utf16leDecodeTest {
    const char* name;
    BYTE input0msb;
    BYTE input0lsb;
    BYTE input1msb;
    BYTE input1lsb;
    BYTE expectedByte0;
    BYTE expectedByte1;
    BYTE expectedByte2;
    BYTE expectedByte3;
    NUINT expectedConsumed;
    STATUS_CODE expectedStatus;
    OPERATION_CODE expectedOperation;
};

int main() {
    int failed = 0;
    RESULT result;

    // ------------------------------------------------------------
    // ASCII Encode
    // ------------------------------------------------------------
    {
        CODEPOINT codepoint(0, 0, 0, (BYTE) 'A');
        BYTE output[1] = { 0 };

        result = ascii_encode(codepoint, output);

        if (
            output[0] != (BYTE) 'A' ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x04 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_encode U+0041\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_encode U+0041 -> 'A'\n");
        }
    }

    {
        CODEPOINT codepoint(0, 0, 0, (BYTE) 0x80);
        BYTE output[1] = { 0 };

        result = ascii_encode(codepoint, output);

        if (
            output[0] != (BYTE) 0x3F ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x01 || result.operation != 0x02 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_encode U+0080\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_encode U+0080 -> '?'\n");
        }
    }

    // ------------------------------------------------------------
    // ASCII Decode
    // ------------------------------------------------------------
    {
        BYTE input[1] = { (BYTE) 'Z' };
        CODEPOINT codepoint(0xFF, 0xFF, 0xFF, 0xFF);

        codepoint = ascii_decode(input);

        if (
            codepoint.byte0 != 0 ||
            codepoint.byte1 != 0 ||
            codepoint.byte2 != 0 ||
            codepoint.byte3 != (BYTE) 'Z'
        ) {
            std::printf("FAIL: catte_ascii_decode 'Z'\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_decode 'Z' -> U+005A\n");
        }
    }

    // ------------------------------------------------------------
    // ASCII Length
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'H', 'e', 'l', 'l', 'o', '\0' };
        NUINT length = 0;

        length = ascii_length(input);

        if (
            length != 5
        ) {
            std::printf("FAIL: catte_ascii_length \"Hello\"\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_length \"Hello\" -> 5\n");
        }
    }

    // ------------------------------------------------------------
    // CP1252 Encode
    // ------------------------------------------------------------
    {
        Cp1252EncodeTest tests[] = {
            { "U+0000", 0x00, 0x00, 0x00, 0x00, 0x00, STATUS_CODE_SUCCESS, 0x02 },
            { "U+0041", 0x00, 0x00, 0x00, 0x41, 0x41, STATUS_CODE_SUCCESS, 0x02 },
            { "U+007F", 0x00, 0x00, 0x00, 0x7F, 0x7F, STATUS_CODE_SUCCESS, 0x02 },
            { "U+00A0", 0x00, 0x00, 0x00, 0xA0, 0xA0, STATUS_CODE_SUCCESS, 0x02 },
            { "U+00FF", 0x00, 0x00, 0x00, 0xFF, 0xFF, STATUS_CODE_SUCCESS, 0x02 },

            { "U+20AC", 0x00, 0x00, 0x20, 0xAC, 0x80, STATUS_CODE_SUCCESS, 0x1C },
            { "U+201A", 0x00, 0x00, 0x20, 0x1A, 0x82, STATUS_CODE_SUCCESS, 0x11 },
            { "U+0192", 0x00, 0x00, 0x01, 0x92, 0x83, STATUS_CODE_SUCCESS, 0x0A },
            { "U+201E", 0x00, 0x00, 0x20, 0x1E, 0x84, STATUS_CODE_SUCCESS, 0x14 },
            { "U+2026", 0x00, 0x00, 0x20, 0x26, 0x85, STATUS_CODE_SUCCESS, 0x18 },
            { "U+2020", 0x00, 0x00, 0x20, 0x20, 0x86, STATUS_CODE_SUCCESS, 0x15 },
            { "U+2021", 0x00, 0x00, 0x20, 0x21, 0x87, STATUS_CODE_SUCCESS, 0x16 },
            { "U+02C6", 0x00, 0x00, 0x02, 0xC6, 0x88, STATUS_CODE_SUCCESS, 0x0B },
            { "U+2030", 0x00, 0x00, 0x20, 0x30, 0x89, STATUS_CODE_SUCCESS, 0x19 },
            { "U+0160", 0x00, 0x00, 0x01, 0x60, 0x8A, STATUS_CODE_SUCCESS, 0x05 },
            { "U+2039", 0x00, 0x00, 0x20, 0x39, 0x8B, STATUS_CODE_SUCCESS, 0x1A },
            { "U+0152", 0x00, 0x00, 0x01, 0x52, 0x8C, STATUS_CODE_SUCCESS, 0x03 },
            { "U+017D", 0x00, 0x00, 0x01, 0x7D, 0x8E, STATUS_CODE_SUCCESS, 0x08 },

            { "U+2018", 0x00, 0x00, 0x20, 0x18, 0x91, STATUS_CODE_SUCCESS, 0x0F },
            { "U+2019", 0x00, 0x00, 0x20, 0x19, 0x92, STATUS_CODE_SUCCESS, 0x10 },
            { "U+201C", 0x00, 0x00, 0x20, 0x1C, 0x93, STATUS_CODE_SUCCESS, 0x12 },
            { "U+201D", 0x00, 0x00, 0x20, 0x1D, 0x94, STATUS_CODE_SUCCESS, 0x13 },
            { "U+2022", 0x00, 0x00, 0x20, 0x22, 0x95, STATUS_CODE_SUCCESS, 0x17 },
            { "U+2013", 0x00, 0x00, 0x20, 0x13, 0x96, STATUS_CODE_SUCCESS, 0x0D },
            { "U+2014", 0x00, 0x00, 0x20, 0x14, 0x97, STATUS_CODE_SUCCESS, 0x0E },
            { "U+02DC", 0x00, 0x00, 0x02, 0xDC, 0x98, STATUS_CODE_SUCCESS, 0x0C },
            { "U+2122", 0x00, 0x00, 0x21, 0x22, 0x99, STATUS_CODE_SUCCESS, 0x1D },
            { "U+0161", 0x00, 0x00, 0x01, 0x61, 0x9A, STATUS_CODE_SUCCESS, 0x06 },
            { "U+203A", 0x00, 0x00, 0x20, 0x3A, 0x9B, STATUS_CODE_SUCCESS, 0x1B },
            { "U+0153", 0x00, 0x00, 0x01, 0x53, 0x9C, STATUS_CODE_SUCCESS, 0x04 },
            { "U+017E", 0x00, 0x00, 0x01, 0x7E, 0x9E, STATUS_CODE_SUCCESS, 0x09 },
            { "U+0178", 0x00, 0x00, 0x01, 0x78, 0x9F, STATUS_CODE_SUCCESS, 0x07 },

            { "U+0080 invalid", 0x00, 0x00, 0x00, 0x80, '?', STATUS_CODE_WARNING_PARTIAL, 0x1E },
            { "U+0100 invalid", 0x00, 0x00, 0x01, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1E },
            { "U+1F600 invalid", 0x00, 0x01, 0xF6, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1E }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            CODEPOINT codepoint;
            BYTE output[1];

            codepoint.byte0 = tests[i].byte0;
            codepoint.byte1 = tests[i].byte1;
            codepoint.byte2 = tests[i].byte2;
            codepoint.byte3 = tests[i].byte3;
            output[0] = 0;

            result = cp1252_encode(codepoint, output);

            if (
                output[0] != tests[i].expectedCodeunit ||
                result.status != tests[i].expectedStatus || result.context != 0x04 || result.operation != tests[i].expectedOperation || result.detail != 0
            ) {
                std::printf("FAIL: catte_cp1252_encode %s\n", tests[i].name);
                std::printf("  expected output=0x%02X status=0x%02X\n", tests[i].expectedCodeunit, tests[i].expectedStatus);
                std::printf("  actual   output=0x%02X status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    output[0], result.status, result.context, result.operation, result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_cp1252_encode %s -> 0x%02X\n", tests[i].name, output[0]);
            }
        }
    }

    // ------------------------------------------------------------
    // CP1252 Decode
    // ------------------------------------------------------------
    {
        Cp1252DecodeTest tests[] = {
            { "0x00", 0x00, 0x00, 0x00, 0x00, 0x00, STATUS_CODE_SUCCESS, 0x03 },
            { "0x41", 0x41, 0x00, 0x00, 0x00, 0x41, STATUS_CODE_SUCCESS, 0x03 },
            { "0x7F", 0x7F, 0x00, 0x00, 0x00, 0x7F, STATUS_CODE_SUCCESS, 0x03 },
            { "0xA0", 0xA0, 0x00, 0x00, 0x00, 0xA0, STATUS_CODE_SUCCESS, 0x03 },
            { "0xFF", 0xFF, 0x00, 0x00, 0x00, 0xFF, STATUS_CODE_SUCCESS, 0x03 },

            { "0x80", 0x80, 0x00, 0x00, 0x20, 0xAC, STATUS_CODE_SUCCESS, 0x04 },
            { "0x82", 0x82, 0x00, 0x00, 0x20, 0x1A, STATUS_CODE_SUCCESS, 0x05 },
            { "0x83", 0x83, 0x00, 0x00, 0x01, 0x92, STATUS_CODE_SUCCESS, 0x06 },
            { "0x84", 0x84, 0x00, 0x00, 0x20, 0x1E, STATUS_CODE_SUCCESS, 0x07 },
            { "0x85", 0x85, 0x00, 0x00, 0x20, 0x26, STATUS_CODE_SUCCESS, 0x08 },
            { "0x86", 0x86, 0x00, 0x00, 0x20, 0x20, STATUS_CODE_SUCCESS, 0x09 },
            { "0x87", 0x87, 0x00, 0x00, 0x20, 0x21, STATUS_CODE_SUCCESS, 0x0A },
            { "0x88", 0x88, 0x00, 0x00, 0x02, 0xC6, STATUS_CODE_SUCCESS, 0x0B },
            { "0x89", 0x89, 0x00, 0x00, 0x20, 0x30, STATUS_CODE_SUCCESS, 0x0C },
            { "0x8A", 0x8A, 0x00, 0x00, 0x01, 0x60, STATUS_CODE_SUCCESS, 0x0D },
            { "0x8B", 0x8B, 0x00, 0x00, 0x20, 0x39, STATUS_CODE_SUCCESS, 0x0E },
            { "0x8C", 0x8C, 0x00, 0x00, 0x01, 0x52, STATUS_CODE_SUCCESS, 0x0F },
            { "0x8E", 0x8E, 0x00, 0x00, 0x01, 0x7D, STATUS_CODE_SUCCESS, 0x10 },

            { "0x91", 0x91, 0x00, 0x00, 0x20, 0x18, STATUS_CODE_SUCCESS, 0x11 },
            { "0x92", 0x92, 0x00, 0x00, 0x20, 0x19, STATUS_CODE_SUCCESS, 0x12 },
            { "0x93", 0x93, 0x00, 0x00, 0x20, 0x1C, STATUS_CODE_SUCCESS, 0x13 },
            { "0x94", 0x94, 0x00, 0x00, 0x20, 0x1D, STATUS_CODE_SUCCESS, 0x14 },
            { "0x95", 0x95, 0x00, 0x00, 0x20, 0x22, STATUS_CODE_SUCCESS, 0x15 },
            { "0x96", 0x96, 0x00, 0x00, 0x20, 0x13, STATUS_CODE_SUCCESS, 0x16 },
            { "0x97", 0x97, 0x00, 0x00, 0x20, 0x14, STATUS_CODE_SUCCESS, 0x17 },
            { "0x98", 0x98, 0x00, 0x00, 0x02, 0xDC, STATUS_CODE_SUCCESS, 0x18 },
            { "0x99", 0x99, 0x00, 0x00, 0x21, 0x22, STATUS_CODE_SUCCESS, 0x19 },
            { "0x9A", 0x9A, 0x00, 0x00, 0x01, 0x61, STATUS_CODE_SUCCESS, 0x1A },
            { "0x9B", 0x9B, 0x00, 0x00, 0x20, 0x3A, STATUS_CODE_SUCCESS, 0x1B },
            { "0x9C", 0x9C, 0x00, 0x00, 0x01, 0x53, STATUS_CODE_SUCCESS, 0x1C },
            { "0x9E", 0x9E, 0x00, 0x00, 0x01, 0x7E, STATUS_CODE_SUCCESS, 0x1D },
            { "0x9F", 0x9F, 0x00, 0x00, 0x01, 0x78, STATUS_CODE_SUCCESS, 0x1E },

            { "0x81 undefined", 0x81, 0x00, 0x00, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1F },
            { "0x8D undefined", 0x8D, 0x00, 0x00, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1F },
            { "0x8F undefined", 0x8F, 0x00, 0x00, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1F },
            { "0x90 undefined", 0x90, 0x00, 0x00, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1F },
            { "0x9D undefined", 0x9D, 0x00, 0x00, 0x00, '?', STATUS_CODE_WARNING_PARTIAL, 0x1F }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            BYTE input[1];
            CODEPOINT codepoint;

            input[0] = tests[i].codeunit;
            codepoint.byte0 = 0xFF;
            codepoint.byte1 = 0xFF;
            codepoint.byte2 = 0xFF;
            codepoint.byte3 = 0xFF;

            result = cp1252_decode(input, &codepoint);

            if (
                codepoint.byte0 != tests[i].expectedByte0 ||
                codepoint.byte1 != tests[i].expectedByte1 ||
                codepoint.byte2 != tests[i].expectedByte2 ||
                codepoint.byte3 != tests[i].expectedByte3 ||
                result.status != tests[i].expectedStatus || result.context != 0x05 || result.operation != tests[i].expectedOperation || result.detail != 0
            ) {
                std::printf("FAIL: catte_cp1252_decode %s\n", tests[i].name);
                std::printf("  expected codepoint=%02X %02X %02X %02X status=0x%02X\n",
                    tests[i].expectedByte0,
                    tests[i].expectedByte1,
                    tests[i].expectedByte2,
                    tests[i].expectedByte3,
                    tests[i].expectedStatus);
                std::printf("  actual   codepoint=%02X %02X %02X %02X status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3,
                    result.status,
                    result.context,
                    result.operation,
                    result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_cp1252_decode %s -> U+%02X%02X%02X%02X\n",
                    tests[i].name,
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3);
            }
        }
    }


    // ------------------------------------------------------------
    // CP1252 Length
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'H', 'e', (BYTE) 0x80, (BYTE) 0xA0, '!', '\0' };
        NUINT length = 0;

        length = cp1252_length(input);

        if (
            length != 5
        ) {
            std::printf("FAIL: catte_cp1252_length mixed string\n");
            std::printf("  actual length=%lu\n", (unsigned long) length);
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_length mixed string -> 5\n");
        }
    }

    {
        BYTE input[] = { '\0' };
        NUINT length = 123;

        length = cp1252_length(input);

        if (
            length != 0
        ) {
            std::printf("FAIL: catte_cp1252_length empty string\n");
            std::printf("  actual length=%lu\n", (unsigned long) length);
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_length empty string -> 0\n");
        }
    }

    {
        BYTE input[] = { 'A', '\0', 'B', '\0' };
        NUINT length = 0;

        length = cp1252_length(input);

        if (
            length != 1
        ) {
            std::printf("FAIL: catte_cp1252_length stops at NUL\n");
            std::printf("  actual length=%lu\n", (unsigned long) length);
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_length stops at NUL -> 1\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-8 Encode
    // ------------------------------------------------------------
    {
        Utf8EncodeTest tests[] = {
            { "U+0000", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x03 },
            { "U+0041", 0x00, 0x00, 0x00, 0x41, 0x41, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x03 },
            { "U+007F", 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x03 },

            { "U+0080", 0x00, 0x00, 0x00, 0x80, 0xC2, 0x80, 0x00, 0x00, 2, STATUS_CODE_SUCCESS, 0x05 },
            { "U+00A2", 0x00, 0x00, 0x00, 0xA2, 0xC2, 0xA2, 0x00, 0x00, 2, STATUS_CODE_SUCCESS, 0x05 },
            { "U+07FF", 0x00, 0x00, 0x07, 0xFF, 0xDF, 0xBF, 0x00, 0x00, 2, STATUS_CODE_SUCCESS, 0x05 },

            { "U+0800", 0x00, 0x00, 0x08, 0x00, 0xE0, 0xA0, 0x80, 0x00, 3, STATUS_CODE_SUCCESS, 0x08 },
            { "U+20AC", 0x00, 0x00, 0x20, 0xAC, 0xE2, 0x82, 0xAC, 0x00, 3, STATUS_CODE_SUCCESS, 0x08 },
            { "U+D7FF", 0x00, 0x00, 0xD7, 0xFF, 0xED, 0x9F, 0xBF, 0x00, 3, STATUS_CODE_SUCCESS, 0x08 },
            { "U+E000", 0x00, 0x00, 0xE0, 0x00, 0xEE, 0x80, 0x80, 0x00, 3, STATUS_CODE_SUCCESS, 0x08 },

            { "U+1F600 emoji", 0x00, 0x01, 0xF6, 0x00, 0xF0, 0x9F, 0x98, 0x80, 4, STATUS_CODE_SUCCESS, 0x09 },
            { "U+2070E CJK", 0x00, 0x02, 0x07, 0x0E, 0xF0, 0xA0, 0x9C, 0x8E, 4, STATUS_CODE_SUCCESS, 0x09 },
            { "U+10FFFF max", 0x00, 0x10, 0xFF, 0xFF, 0xF4, 0x8F, 0xBF, 0xBF, 4, STATUS_CODE_SUCCESS, 0x09 },

            { "U+D800 surrogate", 0x00, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07 },
            { "U+110000 invalid", 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A },
            { "0x01000000 invalid", 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            CODEPOINT codepoint;
            BYTE output[4];
            NUINT produced;

            codepoint.byte0 = tests[i].byte0;
            codepoint.byte1 = tests[i].byte1;
            codepoint.byte2 = tests[i].byte2;
            codepoint.byte3 = tests[i].byte3;

            output[0] = 0;
            output[1] = 0;
            output[2] = 0;
            output[3] = 0;
            produced = 123;

            result = utf8_encode(codepoint, output, &produced);

            if (
                output[0] != tests[i].expected0 ||
                output[1] != tests[i].expected1 ||
                output[2] != tests[i].expected2 ||
                output[3] != tests[i].expected3 ||
                produced != tests[i].expectedProduced ||
                result.status != tests[i].expectedStatus || result.context != 0x07 || result.operation != tests[i].expectedOperation || result.detail != 0
            ) {
                std::printf("FAIL: catte_utf8_encode %s\n", tests[i].name);
                std::printf("  expected output=%02X %02X %02X %02X produced=%lu status=0x%02X operation=0x%02X\n",
                    tests[i].expected0,
                    tests[i].expected1,
                    tests[i].expected2,
                    tests[i].expected3,
                    (unsigned long) tests[i].expectedProduced,
                    tests[i].expectedStatus,
                    tests[i].expectedOperation);
                std::printf("  actual   output=%02X %02X %02X %02X produced=%lu status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    output[0],
                    output[1],
                    output[2],
                    output[3],
                    (unsigned long) produced,
                    result.status,
                    result.context,
                    result.operation,
                    result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_utf8_encode %s -> %02X %02X %02X %02X\n",
                    tests[i].name,
                    output[0],
                    output[1],
                    output[2],
                    output[3]);
            }
        }
    }


    // ------------------------------------------------------------
    // UTF-8 Decode
    // ------------------------------------------------------------
    {
        Utf8DecodeTest tests[] = {
            { "NUL", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x04, 0x00 },
            { "U+0041", 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 1, STATUS_CODE_SUCCESS, 0x05, 0x00 },
            { "U+007F", 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 1, STATUS_CODE_SUCCESS, 0x05, 0x00 },

            { "U+0080", 0xC2, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 2, STATUS_CODE_SUCCESS, 0x08, 0x00 },
            { "U+00A2", 0xC2, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 2, STATUS_CODE_SUCCESS, 0x08, 0x00 },
            { "U+07FF", 0xDF, 0xBF, 0x00, 0x00, 0x00, 0x00, 0x07, 0xFF, 2, STATUS_CODE_SUCCESS, 0x08, 0x00 },

            { "U+0800", 0xE0, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x08, 0x00, 3, STATUS_CODE_SUCCESS, 0x0E, 0x00 },
            { "U+20AC", 0xE2, 0x82, 0xAC, 0x00, 0x00, 0x00, 0x20, 0xAC, 3, STATUS_CODE_SUCCESS, 0x0E, 0x00 },
            { "U+D7FF", 0xED, 0x9F, 0xBF, 0x00, 0x00, 0x00, 0xD7, 0xFF, 3, STATUS_CODE_SUCCESS, 0x0E, 0x00 },
            { "U+E000", 0xEE, 0x80, 0x80, 0x00, 0x00, 0x00, 0xE0, 0x00, 3, STATUS_CODE_SUCCESS, 0x0E, 0x00 },

            { "U+1F600 emoji", 0xF0, 0x9F, 0x98, 0x80, 0x00, 0x01, 0xF6, 0x00, 4, STATUS_CODE_SUCCESS, 0x15, 0x00 },
            { "U+2070E CJK", 0xF0, 0xA0, 0x9C, 0x8E, 0x00, 0x02, 0x07, 0x0E, 4, STATUS_CODE_SUCCESS, 0x15, 0x00 },
            { "U+10FFFF max", 0xF4, 0x8F, 0xBF, 0xBF, 0x00, 0x10, 0xFF, 0xFF, 4, STATUS_CODE_SUCCESS, 0x15, 0x00 },

            { "2-byte unexpected end", 0xC2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x06, 0x00 },
            { "2-byte invalid continuation", 0xC2, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x07, 0x00 },
            { "3-byte unexpected end", 0xE2, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x09, 0x02 },
            { "3-byte invalid continuation", 0xE2, 0x28, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0A, 0x00 },
            { "3-byte second continuation invalid", 0xE2, 0x82, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0B, 0x00 },
            { "3-byte first continuation missing", 0xE2, 0x00, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x09, 0x01 },
            { "3-byte overlong", 0xE0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0C, 0x00 },
            { "surrogate U+D800", 0xED, 0xA0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0D, 0x00 },
            { "4-byte unexpected end", 0xF0, 0x9F, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0F, 0x03 },
            { "4-byte invalid continuation", 0xF0, 0x28, 0x98, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x10, 0x00 },
            { "4-byte second continuation invalid", 0xF0, 0x9F, 0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x11, 0x00 },
            { "4-byte third continuation invalid", 0xF0, 0x9F, 0x98, 0x20, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x12, 0x00 },
            { "4-byte first continuation missing", 0xF0, 0x00, 0x98, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0F, 0x01 },
            { "4-byte second continuation missing", 0xF0, 0x9F, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0F, 0x02 },
            { "4-byte overlong", 0xF0, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x13, 0x00 },
            { "above U+10FFFF", 0xF4, 0x90, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x14, 0x00 },
            { "invalid leading continuation", 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x16, 0x00 },
            { "invalid leading 0xFF", 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x16, 0x00 }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            BYTE input[4];
            CODEPOINT codepoint;
            NUINT consumed;

            input[0] = tests[i].input0;
            input[1] = tests[i].input1;
            input[2] = tests[i].input2;
            input[3] = tests[i].input3;

            codepoint.byte0 = 0xFF;
            codepoint.byte1 = 0xFF;
            codepoint.byte2 = 0xFF;
            codepoint.byte3 = 0xFF;
            consumed = 123;

            result = utf8_decode(input, &codepoint, &consumed);

            if (
                codepoint.byte0 != tests[i].expectedByte0 ||
                codepoint.byte1 != tests[i].expectedByte1 ||
                codepoint.byte2 != tests[i].expectedByte2 ||
                codepoint.byte3 != tests[i].expectedByte3 ||
                consumed != tests[i].expectedConsumed ||
                result.status != tests[i].expectedStatus || result.context != 0x08 || result.operation != tests[i].expectedOperation || result.detail != tests[i].expectedDetail
            ) {
                std::printf("FAIL: catte_utf8_decode %s\n", tests[i].name);
                std::printf("  expected codepoint=%02X %02X %02X %02X consumed=%lu status=0x%02X operation=0x%02X\n",
                    tests[i].expectedByte0,
                    tests[i].expectedByte1,
                    tests[i].expectedByte2,
                    tests[i].expectedByte3,
                    (unsigned long) tests[i].expectedConsumed,
                    tests[i].expectedStatus,
                    tests[i].expectedOperation);
                std::printf("  actual   codepoint=%02X %02X %02X %02X consumed=%lu status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3,
                    (unsigned long) consumed,
                    result.status,
                    result.context,
                    result.operation,
                    result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_utf8_decode %s -> U+%02X%02X%02X%02X\n",
                    tests[i].name,
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3);
            }
        }
    }



    // ------------------------------------------------------------
    // UTF-8 Length
    // ------------------------------------------------------------
    {
        BYTE input[] = {
            'A',
            (BYTE) 0xC2u, (BYTE) 0xA2u,
            (BYTE) 0xE2u, (BYTE) 0x82u, (BYTE) 0xACu,
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            (BYTE) 0xF0u, (BYTE) 0xA0u, (BYTE) 0x9Cu, (BYTE) 0x8Eu,
            '\0'
        };
        NUINT length = 0;

        result = utf8_length(input, &length);

        if (
            length != 5 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x09 || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf8_length mixed string\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_length mixed string -> 5\n");
        }
    }

    {
        BYTE input[] = {
            (BYTE) 0xEFu, (BYTE) 0xBBu, (BYTE) 0xBFu,
            'A',
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            '\0'
        };
        NUINT length = 0;

        result = utf8_length(input, &length);

        if (
            length != 2 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x09 || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf8_length BOM + emoji\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_length BOM + emoji -> 2\n");
        }
    }

    {
        BYTE input[] = {
            'A',
            (BYTE) 0xE0u, (BYTE) 0x80u, (BYTE) 0x80u,
            '\0'
        };
        NUINT length = 123;

        result = utf8_length(input, &length);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x09 || result.operation != 0x03 || result.detail != 0x0C
        ) {
            std::printf("FAIL: catte_utf8_length invalid UTF-8 sequence\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X detail=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_length invalid UTF-8 sequence -> error operation 3\n");
        }
    }


    // ------------------------------------------------------------
    // UTF-16LE Encode
    // ------------------------------------------------------------
    {
        Utf16leEncodeTest tests[] = {
            { "U+0000", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },
            { "U+0041", 0x00, 0x00, 0x00, 0x41, 0x00, 0x41, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },
            { "U+00A2", 0x00, 0x00, 0x00, 0xA2, 0x00, 0xA2, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },
            { "U+20AC", 0x00, 0x00, 0x20, 0xAC, 0x20, 0xAC, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },
            { "U+D7FF", 0x00, 0x00, 0xD7, 0xFF, 0xD7, 0xFF, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },
            { "U+E000", 0x00, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x05 },

            { "U+1F600 emoji", 0x00, 0x01, 0xF6, 0x00, 0xD8, 0x3D, 0xDE, 0x00, 2, STATUS_CODE_SUCCESS, 0x06 },
            { "U+2070E CJK", 0x00, 0x02, 0x07, 0x0E, 0xD8, 0x41, 0xDF, 0x0E, 2, STATUS_CODE_SUCCESS, 0x06 },
            { "U+10FFFF max", 0x00, 0x10, 0xFF, 0xFF, 0xDB, 0xFF, 0xDF, 0xFF, 2, STATUS_CODE_SUCCESS, 0x06 },

            { "U+D800 surrogate", 0x00, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04 },
            { "U+DFFF surrogate", 0x00, 0x00, 0xDF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04 },
            { "U+110000 invalid", 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03 },
            { "0x01000000 invalid", 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03 }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            CODEPOINT codepoint;
            UTF16LE_CODE_UNIT output[2];
            NUINT produced;

            codepoint.byte0 = tests[i].byte0;
            codepoint.byte1 = tests[i].byte1;
            codepoint.byte2 = tests[i].byte2;
            codepoint.byte3 = tests[i].byte3;

            output[0].msb = 0;
            output[0].lsb = 0;
            output[1].msb = 0;
            output[1].lsb = 0;
            produced = 123;

            result = utf16le_encode(codepoint, output, &produced);

            if (
                output[0].msb != tests[i].expected0msb ||
                output[0].lsb != tests[i].expected0lsb ||
                output[1].msb != tests[i].expected1msb ||
                output[1].lsb != tests[i].expected1lsb ||
                produced != tests[i].expectedProduced ||
                result.status != tests[i].expectedStatus || result.context != 0x0A || result.operation != tests[i].expectedOperation || result.detail != 0
            ) {
                std::printf("FAIL: catte_utf16le_encode %s\n", tests[i].name);
                std::printf("  expected output=%02X%02X %02X%02X produced=%lu status=0x%02X operation=0x%02X\n",
                    tests[i].expected0msb,
                    tests[i].expected0lsb,
                    tests[i].expected1msb,
                    tests[i].expected1lsb,
                    (unsigned long) tests[i].expectedProduced,
                    tests[i].expectedStatus,
                    tests[i].expectedOperation);
                std::printf("  actual   output=%02X%02X %02X%02X produced=%lu status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    output[0].msb,
                    output[0].lsb,
                    output[1].msb,
                    output[1].lsb,
                    (unsigned long) produced,
                    result.status,
                    result.context,
                    result.operation,
                    result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_utf16le_encode %s -> %02X%02X %02X%02X\n",
                    tests[i].name,
                    output[0].msb,
                    output[0].lsb,
                    output[1].msb,
                    output[1].lsb);
            }
        }
    }


    // ------------------------------------------------------------
    // UTF-16LE Decode
    // ------------------------------------------------------------
    {
        Utf16leDecodeTest tests[] = {
            { "NUL", 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 1, STATUS_CODE_SUCCESS, 0x04 },
            { "U+0041", 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 1, STATUS_CODE_SUCCESS, 0x06 },
            { "U+00A2", 0x00, 0xA2, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA2, 1, STATUS_CODE_SUCCESS, 0x06 },
            { "U+20AC", 0x20, 0xAC, 0x00, 0x00, 0x00, 0x00, 0x20, 0xAC, 1, STATUS_CODE_SUCCESS, 0x06 },
            { "U+D7FF", 0xD7, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xD7, 0xFF, 1, STATUS_CODE_SUCCESS, 0x06 },
            { "U+E000", 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 1, STATUS_CODE_SUCCESS, 0x06 },

            { "U+1F600 emoji", 0xD8, 0x3D, 0xDE, 0x00, 0x00, 0x01, 0xF6, 0x00, 2, STATUS_CODE_SUCCESS, 0x08 },
            { "U+2070E CJK", 0xD8, 0x41, 0xDF, 0x0E, 0x00, 0x02, 0x07, 0x0E, 2, STATUS_CODE_SUCCESS, 0x08 },
            { "U+10FFFF max", 0xDB, 0xFF, 0xDF, 0xFF, 0x00, 0x10, 0xFF, 0xFF, 2, STATUS_CODE_SUCCESS, 0x08 },

            { "low surrogate alone", 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x05 },
            { "high then BMP invalid", 0xD8, 0x3D, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x07 },
            { "high then high invalid", 0xD8, 0x3D, 0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x07 }
        };

        unsigned int testCount = (unsigned int) (sizeof(tests) / sizeof(tests[0]));
        unsigned int i;

        for (i = 0; i < testCount; ++i) {
            UTF16LE_CODE_UNIT input[2];
            CODEPOINT codepoint;
            NUINT consumed;

            input[0].msb = tests[i].input0msb;
            input[0].lsb = tests[i].input0lsb;
            input[1].msb = tests[i].input1msb;
            input[1].lsb = tests[i].input1lsb;

            codepoint.byte0 = 0xFF;
            codepoint.byte1 = 0xFF;
            codepoint.byte2 = 0xFF;
            codepoint.byte3 = 0xFF;
            consumed = 123;

            result = utf16le_decode(input, &codepoint, &consumed);

            if (
                codepoint.byte0 != tests[i].expectedByte0 ||
                codepoint.byte1 != tests[i].expectedByte1 ||
                codepoint.byte2 != tests[i].expectedByte2 ||
                codepoint.byte3 != tests[i].expectedByte3 ||
                consumed != tests[i].expectedConsumed ||
                result.status != tests[i].expectedStatus || result.context != 0x0B || result.operation != tests[i].expectedOperation || result.detail != 0
            ) {
                std::printf("FAIL: catte_utf16le_decode %s\n", tests[i].name);
                std::printf("  expected codepoint=%02X %02X %02X %02X consumed=%lu status=0x%02X operation=0x%02X\n",
                    tests[i].expectedByte0,
                    tests[i].expectedByte1,
                    tests[i].expectedByte2,
                    tests[i].expectedByte3,
                    (unsigned long) tests[i].expectedConsumed,
                    tests[i].expectedStatus,
                    tests[i].expectedOperation);
                std::printf("  actual   codepoint=%02X %02X %02X %02X consumed=%lu status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3,
                    (unsigned long) consumed,
                    result.status,
                    result.context,
                    result.operation,
                    result.detail);
                failed++;
            } else {
                std::printf("PASS: catte_utf16le_decode %s -> U+%02X%02X%02X%02X\n",
                    tests[i].name,
                    codepoint.byte0,
                    codepoint.byte1,
                    codepoint.byte2,
                    codepoint.byte3);
            }
        }
    }



    // ------------------------------------------------------------
    // UTF-16LE Length
    // ------------------------------------------------------------
    {
        UTF16LE_CODE_UNIT input[8];
        NUINT length = 0;

        input[0].msb = 0x00; input[0].lsb = 0x41; // U+0041
        input[1].msb = 0xD8; input[1].lsb = 0x3D; // U+1F600 high
        input[2].msb = 0xDE; input[2].lsb = 0x00; // U+1F600 low
        input[3].msb = 0xD8; input[3].lsb = 0x41; // U+2070E high
        input[4].msb = 0xDF; input[4].lsb = 0x0E; // U+2070E low
        input[5].msb = 0x20; input[5].lsb = 0xAC; // U+20AC
        input[6].msb = 0x00; input[6].lsb = 0x00; // NUL
        input[7].msb = 0x00; input[7].lsb = 0x00;

        result = utf16le_length(input, &length);

        if (
            length != 4 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x0C || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf16le_length mixed string\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_length mixed string -> 4\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[5];
        NUINT length = 0;

        input[0].msb = 0xFE; input[0].lsb = 0xFF; // BOM U+FEFF
        input[1].msb = 0x00; input[1].lsb = 0x41; // U+0041
        input[2].msb = 0xD8; input[2].lsb = 0x3D; // U+1F600 high
        input[3].msb = 0xDE; input[3].lsb = 0x00; // U+1F600 low
        input[4].msb = 0x00; input[4].lsb = 0x00; // NUL

        result = utf16le_length(input, &length);

        if (
            length != 2 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x0C || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf16le_length BOM + emoji\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_length BOM + emoji -> 2\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        NUINT length = 123;

        input[0].msb = 0x00; input[0].lsb = 0x41;
        input[1].msb = 0xD8; input[1].lsb = 0x3D; // high surrogate without valid low surrogate
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_length(input, &length);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x0C || result.operation != 0x03 || result.detail != 0x07
        ) {
            std::printf("FAIL: catte_utf16le_length invalid UTF-16LE sequence\n");
            std::printf("  actual length=%lu status=0x%02X operation=0x%02X detail=0x%02X\n",
                (unsigned long) length,
                result.status,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_length invalid UTF-16LE sequence -> error operation 3\n");
        }
    }


    // ------------------------------------------------------------
    // ASCII -> CP1252
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', 'B', 'C', '\0' };
        BYTE output[4];

        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;

        result = ascii_to_cp1252(input, output);

        if (
            output[0] != 'A' ||
            output[1] != 'B' ||
            output[2] != 'C' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x0D || result.operation != 0x04 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_cp1252 ASCII direct\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_cp1252 ASCII direct\n");
        }
    }

    {
        BYTE input[] = { 'A', (BYTE) 0x80u, 'B', '\0' };
        BYTE output[4];

        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;

        result = ascii_to_cp1252(input, output);

        if (
            output[0] != 'A' ||
            output[1] != '?' ||
            output[2] != 'B' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x0D || result.operation != 0x04 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_cp1252 invalid ASCII fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_cp1252 invalid ASCII fallback -> '?'\n");
        }
    }


    // ------------------------------------------------------------
    // ASCII -> UTF-8
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', 'B', 'C', '\0' };
        BYTE output[4];

        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;

        result = ascii_to_utf8(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != 'B' ||
            output[2] != 'C' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x0E || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_utf8 ASCII direct\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf8 ASCII direct\n");
        }
    }

    {
        BYTE input[] = { 'A', (BYTE) 0x80u, 'B', '\0' };
        BYTE output[4];

        result = ascii_to_utf8(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != '?' ||
            output[2] != 'B' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x0E || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_utf8 invalid ASCII fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf8 invalid ASCII fallback -> '?'\n");
        }
    }

    {
        BYTE input[] = { 'A', 'B', '\0' };
        BYTE output[2];

        result = ascii_to_utf8(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0E || result.operation != 0x04 || result.detail != 0) {
            std::printf("FAIL: catte_ascii_to_utf8 buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf8 buffer overflow\n");
        }
    }

    {
        BYTE input[] = { 'A', '\0' };
        BYTE output[2];


        result = ascii_to_utf8(input, output, 0);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0E || result.operation != 0x03 || result.detail != 0) {
            std::printf("FAIL: catte_ascii_to_utf8 zero capacity\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf8 zero capacity -> buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // ASCII -> UTF-16LE
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', 'B', '\0' };
        UTF16LE_CODE_UNIT output[3];

        result = ascii_to_utf16le(input, output, 3);

        if (
            output[0].msb != 0x00 || output[0].lsb != 'A' ||
            output[1].msb != 0x00 || output[1].lsb != 'B' ||
            output[2].msb != 0x00 || output[2].lsb != 0x00 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x0F || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_utf16le ASCII direct\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf16le ASCII direct\n");
        }
    }

    {
        BYTE input[] = { 'A', (BYTE) 0x80u, '\0' };
        UTF16LE_CODE_UNIT output[3];

        result = ascii_to_utf16le(input, output, 3);

        if (
            output[0].msb != 0x00 || output[0].lsb != 'A' ||
            output[1].msb != 0x00 || output[1].lsb != '?' ||
            output[2].msb != 0x00 || output[2].lsb != 0x00 ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x0F || result.operation != 0x05 || result.detail != 0
        ) {
            std::printf("FAIL: catte_ascii_to_utf16le invalid ASCII fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf16le invalid ASCII fallback -> '?'\n");
        }
    }

    {
        BYTE input[] = { 'A', 'B', '\0' };
        UTF16LE_CODE_UNIT output[2];

        result = ascii_to_utf16le(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0F || result.operation != 0x04 || result.detail != 0) {
            std::printf("FAIL: catte_ascii_to_utf16le buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_ascii_to_utf16le buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // CP1252 -> ASCII
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', (BYTE) 0x80u, (BYTE) 0x81u, '\0' };
        BYTE output[4];

        result = cp1252_to_ascii(input, output);

        if (
            output[0] != 'A' ||
            output[1] != '?' ||
            output[2] != '?' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x10 || result.operation != 0x06 || result.detail != 0
        ) {
            std::printf("FAIL: catte_cp1252_to_ascii fallback behavior\n");
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_to_ascii fallback behavior\n");
        }
    }


    // ------------------------------------------------------------
    // CP1252 -> UTF-8
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', (BYTE) 0x80u, (BYTE) 0x81u, '\0' };
        BYTE output[8];

        result = cp1252_to_utf8(input, output, 8);

        if (
            output[0] != 'A' ||
            output[1] != (BYTE) 0xE2u ||
            output[2] != (BYTE) 0x82u ||
            output[3] != (BYTE) 0xACu ||
            output[4] != '?' ||
            output[5] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x11 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_cp1252_to_utf8 euro + undefined fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_to_utf8 euro + undefined fallback\n");
        }
    }

    {
        BYTE input[] = { (BYTE) 0x80u, '\0' };
        BYTE output[3];

        result = cp1252_to_utf8(input, output, 3);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x11 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_cp1252_to_utf8 buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_to_utf8 buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // CP1252 -> UTF-16LE
    // ------------------------------------------------------------
    {
        BYTE input[] = { 'A', (BYTE) 0x80u, (BYTE) 0x81u, '\0' };
        UTF16LE_CODE_UNIT output[4];

        result = cp1252_to_utf16le(input, output, 4);

        if (
            output[0].msb != 0x00 || output[0].lsb != 'A' ||
            output[1].msb != 0x20 || output[1].lsb != 0xAC ||
            output[2].msb != 0x00 || output[2].lsb != '?' ||
            output[3].msb != 0x00 || output[3].lsb != 0x00 ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x12 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_cp1252_to_utf16le euro + undefined fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_to_utf16le euro + undefined fallback\n");
        }
    }

    {
        BYTE input[] = { 'A', 'B', '\0' };
        UTF16LE_CODE_UNIT output[2];

        result = cp1252_to_utf16le(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x12 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_cp1252_to_utf16le buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_cp1252_to_utf16le buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-8 -> ASCII
    // ------------------------------------------------------------
    {
        BYTE input[] = {
            (BYTE) 0xEFu, (BYTE) 0xBBu, (BYTE) 0xBFu,
            'A',
            (BYTE) 0xE2u, (BYTE) 0x82u, (BYTE) 0xACu,
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            '\0'
        };
        BYTE output[4];

        result = utf8_to_ascii(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != '?' ||
            output[2] != '?' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x13 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf8_to_ascii BOM + fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_ascii BOM + fallback\n");
        }
    }

    {
        BYTE input[] = { 'A', 'B', '\0' };
        BYTE output[2];

        result = utf8_to_ascii(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x13 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf8_to_ascii buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_ascii buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-8 -> CP1252
    // ------------------------------------------------------------
    {
        BYTE input[] = {
            (BYTE) 0xEFu, (BYTE) 0xBBu, (BYTE) 0xBFu,
            'A',
            (BYTE) 0xE2u, (BYTE) 0x82u, (BYTE) 0xACu,
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            '\0'
        };
        BYTE output[4];

        result = utf8_to_cp1252(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != (BYTE) 0x80u ||
            output[2] != '?' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x14 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf8_to_cp1252 BOM + euro + fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_cp1252 BOM + euro + fallback\n");
        }
    }

    {
        BYTE input[] = { 'A', 'B', '\0' };
        BYTE output[2];

        result = utf8_to_cp1252(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x14 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf8_to_cp1252 buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_cp1252 buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-8 -> UTF-16LE
    // ------------------------------------------------------------
    {
        BYTE input[] = {
            (BYTE) 0xEFu, (BYTE) 0xBBu, (BYTE) 0xBFu,
            'A',
            (BYTE) 0xE2u, (BYTE) 0x82u, (BYTE) 0xACu,
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            (BYTE) 0xF0u, (BYTE) 0xA0u, (BYTE) 0x9Cu, (BYTE) 0x8Eu,
            '\0'
        };
        UTF16LE_CODE_UNIT output[7];

        result = utf8_to_utf16le(input, output, 7);

        if (
            output[0].msb != 0x00 || output[0].lsb != 'A' ||
            output[1].msb != 0x20 || output[1].lsb != 0xAC ||
            output[2].msb != 0xD8 || output[2].lsb != 0x3D ||
            output[3].msb != 0xDE || output[3].lsb != 0x00 ||
            output[4].msb != 0xD8 || output[4].lsb != 0x41 ||
            output[5].msb != 0xDF || output[5].lsb != 0x0E ||
            output[6].msb != 0x00 || output[6].lsb != 0x00 ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x15 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf8_to_utf16le BOM + mixed Unicode\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_utf16le BOM + mixed Unicode\n");
        }
    }

    {
        BYTE input[] = {
            (BYTE) 0xF0u, (BYTE) 0x9Fu, (BYTE) 0x98u, (BYTE) 0x80u,
            '\0'
        };
        UTF16LE_CODE_UNIT output[2];

        result = utf8_to_utf16le(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x15 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf8_to_utf16le buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_utf16le buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-16LE -> ASCII
    // ------------------------------------------------------------
    {
        UTF16LE_CODE_UNIT input[6];
        BYTE output[4];

        input[0].msb = 0xFE; input[0].lsb = 0xFF; // BOM
        input[1].msb = 0x00; input[1].lsb = 'A';
        input[2].msb = 0x20; input[2].lsb = 0xAC;
        input[3].msb = 0xD8; input[3].lsb = 0x3D;
        input[4].msb = 0xDE; input[4].lsb = 0x00;
        input[5].msb = 0x00; input[5].lsb = 0x00;

        result = utf16le_to_ascii(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != '?' ||
            output[2] != '?' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x16 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf16le_to_ascii BOM + fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_ascii BOM + fallback\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[2];

        input[0].msb = 0x00; input[0].lsb = 'A';
        input[1].msb = 0x00; input[1].lsb = 'B';
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_ascii(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x16 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf16le_to_ascii buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_ascii buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-16LE -> CP1252
    // ------------------------------------------------------------
    {
        UTF16LE_CODE_UNIT input[6];
        BYTE output[4];

        input[0].msb = 0xFE; input[0].lsb = 0xFF; // BOM
        input[1].msb = 0x00; input[1].lsb = 'A';
        input[2].msb = 0x20; input[2].lsb = 0xAC;
        input[3].msb = 0xD8; input[3].lsb = 0x3D;
        input[4].msb = 0xDE; input[4].lsb = 0x00;
        input[5].msb = 0x00; input[5].lsb = 0x00;

        result = utf16le_to_cp1252(input, output, 4);

        if (
            output[0] != 'A' ||
            output[1] != (BYTE) 0x80u ||
            output[2] != '?' ||
            output[3] != '\0' ||
            result.status != STATUS_CODE_WARNING_PARTIAL || result.context != 0x17 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf16le_to_cp1252 BOM + euro + fallback\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_cp1252 BOM + euro + fallback\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[2];

        input[0].msb = 0x00; input[0].lsb = 'A';
        input[1].msb = 0x00; input[1].lsb = 'B';
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_cp1252(input, output, 2);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x17 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf16le_to_cp1252 buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_cp1252 buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // UTF-16LE -> UTF-8
    // ------------------------------------------------------------
    {
        UTF16LE_CODE_UNIT input[7];
        BYTE output[13];

        input[0].msb = 0xFE; input[0].lsb = 0xFF; // BOM
        input[1].msb = 0x00; input[1].lsb = 'A';
        input[2].msb = 0x20; input[2].lsb = 0xAC;
        input[3].msb = 0xD8; input[3].lsb = 0x3D;
        input[4].msb = 0xDE; input[4].lsb = 0x00;
        input[5].msb = 0xD8; input[5].lsb = 0x41;
        input[6].msb = 0xDF; input[6].lsb = 0x0E;

        // Need terminator after the CJK pair
        {
            UTF16LE_CODE_UNIT terminated[8];
            unsigned int i;

            for (i = 0; i < 7; ++i) {
                terminated[i] = input[i];
            }
            terminated[7].msb = 0x00;
            terminated[7].lsb = 0x00;

            result = utf16le_to_utf8(terminated, output, 13);
        }

        if (
            output[0] != 'A' ||
            output[1] != (BYTE) 0xE2u ||
            output[2] != (BYTE) 0x82u ||
            output[3] != (BYTE) 0xACu ||
            output[4] != (BYTE) 0xF0u ||
            output[5] != (BYTE) 0x9Fu ||
            output[6] != (BYTE) 0x98u ||
            output[7] != (BYTE) 0x80u ||
            output[8] != (BYTE) 0xF0u ||
            output[9] != (BYTE) 0xA0u ||
            output[10] != (BYTE) 0x9Cu ||
            output[11] != (BYTE) 0x8Eu ||
            output[12] != '\0' ||
            result.status != STATUS_CODE_SUCCESS || result.context != 0x18 || result.operation != 0x07 || result.detail != 0
        ) {
            std::printf("FAIL: catte_utf16le_to_utf8 BOM + mixed Unicode\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_utf8 BOM + mixed Unicode\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[4];

        input[0].msb = 0xD8; input[0].lsb = 0x3D;
        input[1].msb = 0xDE; input[1].lsb = 0x00;
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_utf8(input, output, 4);

        if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x18 || result.operation != 0x06 || result.detail != 0) {
            std::printf("FAIL: catte_utf16le_to_utf8 buffer overflow\n");
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_utf8 buffer overflow\n");
        }
    }

    // ------------------------------------------------------------
    // Conversion dependency failure behavior
    // ------------------------------------------------------------

    {
        BYTE input[] = {
            (BYTE) 0xE0u, (BYTE) 0x80u, (BYTE) 0x80u,
            '\0'
        };
        BYTE output[4];

        result = utf8_to_ascii(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x13 || result.operation != 0x04 || result.detail != 0x0C
        ) {
            std::printf("FAIL: catte_utf8_to_ascii dependency failure from UTF-8 decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_ascii dependency failure from UTF-8 decode\n");
        }
    }

    {
        BYTE input[] = {
            (BYTE) 0xE0u, (BYTE) 0x80u, (BYTE) 0x80u,
            '\0'
        };
        BYTE output[4];

        result = utf8_to_cp1252(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x14 || result.operation != 0x04 || result.detail != 0x0C
        ) {
            std::printf("FAIL: catte_utf8_to_cp1252 dependency failure from UTF-8 decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_cp1252 dependency failure from UTF-8 decode\n");
        }
    }

    {
        BYTE input[] = {
            (BYTE) 0xE0u, (BYTE) 0x80u, (BYTE) 0x80u,
            '\0'
        };
        UTF16LE_CODE_UNIT output[4];

        result = utf8_to_utf16le(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x15 || result.operation != 0x04 || result.detail != 0x0C
        ) {
            std::printf("FAIL: catte_utf8_to_utf16le dependency failure from UTF-8 decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf8_to_utf16le dependency failure from UTF-8 decode\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[4];

        input[0].msb = 0xD8; input[0].lsb = 0x3D; // high surrogate
        input[1].msb = 0x00; input[1].lsb = 'A';  // invalid low surrogate
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_ascii(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x16 || result.operation != 0x04 || result.detail != 0x07
        ) {
            std::printf("FAIL: catte_utf16le_to_ascii dependency failure from UTF-16LE decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_ascii dependency failure from UTF-16LE decode\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[4];

        input[0].msb = 0xD8; input[0].lsb = 0x3D; // high surrogate
        input[1].msb = 0x00; input[1].lsb = 'A';  // invalid low surrogate
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_cp1252(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x17 || result.operation != 0x04 || result.detail != 0x07
        ) {
            std::printf("FAIL: catte_utf16le_to_cp1252 dependency failure from UTF-16LE decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_cp1252 dependency failure from UTF-16LE decode\n");
        }
    }

    {
        UTF16LE_CODE_UNIT input[3];
        BYTE output[4];

        input[0].msb = 0xD8; input[0].lsb = 0x3D; // high surrogate
        input[1].msb = 0x00; input[1].lsb = 'A';  // invalid low surrogate
        input[2].msb = 0x00; input[2].lsb = 0x00;

        result = utf16le_to_utf8(input, output, 4);

        if (
            result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x18 || result.operation != 0x04 || result.detail != 0x07
        ) {
            std::printf("FAIL: catte_utf16le_to_utf8 dependency failure from UTF-16LE decode\n");
            std::printf("  actual status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
                result.status,
                result.context,
                result.operation,
                result.detail);
            failed++;
        } else {
            std::printf("PASS: catte_utf16le_to_utf8 dependency failure from UTF-16LE decode\n");
        }
    }

    {
        CODEPOINT codepoint(0, 0, 0, 0);
        BYTE output[1] = { 0xFF };
        result = ascii_encode(codepoint, output);
        if (output[0] != 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x03 || result.detail != 0) failed++;
        output[0] = 0xFF;
        ascii_encode(codepoint, output);
        if (output[0] != 0) failed++;
    }

    {
        BYTE truncated3[] = { (BYTE) 0xE2, 0 };
        BYTE truncated4[] = { (BYTE) 0xF0, 0 };
        BYTE truncated4second[] = { (BYTE) 0xF0, (BYTE) 0x9F, 0 };
        CODEPOINT codepoint;
        NUINT consumed = 99;
        NUINT length = 99;
        result = utf8_decode(truncated3, &codepoint, &consumed);
        if (consumed != 0 || result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x08 || result.operation != 0x09 || result.detail != 1) failed++;
        result = utf8_decode(truncated4, &codepoint, &consumed);
        if (consumed != 0 || result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x08 || result.operation != 0x0F || result.detail != 1) failed++;
        result = utf8_length(truncated4second, &length);
        if (length != 0 || result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x09 || result.operation != 0x03 || result.detail != 0x0F) failed++;
        UTF16LE_CODE_UNIT output[4];
        result = utf8_to_utf16le(truncated4second, output, 4);
        if (result.status != STATUS_CODE_ERROR_MALFORMED_INPUT || result.context != 0x15 || result.operation != 0x04 || result.detail != 0x0F) failed++;
    }

    // Verify the public representations and C/C++ UTF-16LE array identity.
    {
        CODEPOINT codepoint(0x00, 0x10, 0xFF, 0xFF);
        RESULT result(0x83, 0x08, 0x0F, 0x02);
        UTF16LE_CODE_UNIT units[2];
        CATALYST_UTF16LEW writable = units;
        writable[0].lsb = 0x34;
        writable[0].msb = 0x12;
        writable[1].lsb = 0x78;
        writable[1].msb = 0x56;
        const BYTE* bytes = (const BYTE*) units;
        if (sizeof(CATALYST_CODEPOINT) != 4 || sizeof(CATALYST_RESULT) != 4 ||
            sizeof(UTF16LE_CODE_UNIT) != 2 || bytes[0] != 0x34 || bytes[1] != 0x12 ||
            bytes[2] != 0x78 || bytes[3] != 0x56 || sizeof(NUINT) != sizeof(void*) ||
            CATALYST_SBYTE_MIN != -128 || CATALYST_SBYTE_MAX != 127) failed++;
#if CATALYST_SUPPORTS_16BIT
        if (sizeof(USHORT) != 2 || CATALYST_SHORT_MIN != -32768L ||
            (USHORT) units[0] != (USHORT) 0x1234u) failed++;
#endif
#if CATALYST_SUPPORTS_32BIT
        if (sizeof(UINT) != 4 || CATALYST_INT_MIN != (-2147483647L - 1L) ||
            (UINT) codepoint != (UINT) 0x0010FFFFu ||
            (UINT) result != (UINT) 0x83080F02u) failed++;
        result = (UINT) 0x40120304u;
        if (result.status != 0x40 || result.context != 0x12 ||
            result.operation != 0x03 || result.detail != 0x04) failed++;
#endif
#if CATALYST_SUPPORTS_64BIT
        if (sizeof(ULONG) != 8 || (ULONG) CATALYST_LONG_MAX != (CATALYST_ULONG_MAX >> 1) ||
            CATALYST_LONG_MIN + CATALYST_LONG_MAX != -1) failed++;
#endif
    }

    // ------------------------------------------------------------
    // Done
    // ------------------------------------------------------------
    if (failed != 0) {
        std::printf("\n%d CATTELIB test(s) failed.\n", failed);
        return 1;
    }

    std::printf("\nAll CATTELIB tests passed.\n");
    return 0;
}
