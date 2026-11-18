/* Context IDs follow the public CATTELIB declaration order, starting at 0x01.
 * Operation IDs identify individual Result return sites in each method.
 * Dependency failure detail identifies the dependency's reporting site.
 */
#include "catalyst.hpp"
#include "cattelib.h"

using namespace catalyst;

CATALYST_RESULT catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw) {
    // Validate, initialize parameters
    if (asciiw == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x01, 1);
    }
    BYTE codepoint0 = codepoint.byte0;
    BYTE codepoint1 = codepoint.byte1;
    BYTE codepoint2 = codepoint.byte2;
    BYTE codepoint3 = codepoint.byte3;
    if (codepoint0 != 0 || codepoint1 != 0 || codepoint2 != 0 || codepoint3 >= (BYTE) 0x80) {
        // Not a valid ASCII character
        asciiw[0] = (BYTE) '?';
        return RESULT(STATUS_CODE_WARNING_PARTIAL, 0x01, 0x02, 0);
    } else if (codepoint3 == 0) {
        // Handle U+0000 (NUL)
        asciiw[0] = (BYTE) '\0';
        return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x03, 0);
    } else {
        // Maps directly to ASCII
        asciiw[0] = codepoint3;
        return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x04, 0);
    }
}

CATALYST_RESULT catte_ascii_decode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint) {
    // Validate, initialize parameters
    if (ascii == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x01, 0);
    }
    if (codepoint == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x02, 1);
    }

    // ASCII maps directly to the lsb of the codepoint
    BYTE codeunit0 = ascii[0];
    codepoint->byte0 = 0;
    codepoint->byte1 = 0;
    codepoint->byte2 = 0;
    codepoint->byte3 = codeunit0;
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x03, 0);
}

CATALYST_RESULT catte_ascii_length(CATALYST_ASCII ascii, CATALYST_NUINT* length) {
    if (ascii == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 0);
    }
    if (length == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x02, 1);
    }
    NUINT count = 0;
    while (ascii[count] != (BYTE) '\0') {
        if (++count == (NUINT) -1) {
            // Overflow, no null terminator found
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x03, 0x03, 0);
        }
    }
    *length = count;
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x04, 0);
}

CATALYST_RESULT catte_cp1252_encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w) {
    // Validate, initialize parameters
    if (cp1252w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x01, 1);
    }

    BYTE codepoint0 = codepoint.byte0;
    BYTE codepoint1 = codepoint.byte1;
    BYTE codepoint2 = codepoint.byte2;
    BYTE codepoint3 = codepoint.byte3;

    if (codepoint0 == 0 && codepoint1 == 0 && codepoint2 == 0) {
        if (codepoint3 <= (BYTE) 0x7F || codepoint3 >= (BYTE) 0xA0) {
            // U+0000 through U+007F and U+00A0 through U+00FF map directly to CP1252
            cp1252w[0] = codepoint3;
            return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x02, 0);
        }
    }

    if (codepoint0 == 0 && codepoint1 == 0) {
        switch (codepoint2) {
            case (BYTE) 0x01:
                switch (codepoint3) {
                    case (BYTE) 0x52: cp1252w[0] = (BYTE) 0x8C; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x03, 0); // U+0152
                    case (BYTE) 0x53: cp1252w[0] = (BYTE) 0x9C; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x04, 0); // U+0153
                    case (BYTE) 0x60: cp1252w[0] = (BYTE) 0x8A; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x05, 0); // U+0160
                    case (BYTE) 0x61: cp1252w[0] = (BYTE) 0x9A; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x06, 0); // U+0161
                    case (BYTE) 0x78: cp1252w[0] = (BYTE) 0x9F; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x07, 0); // U+0178
                    case (BYTE) 0x7D: cp1252w[0] = (BYTE) 0x8E; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x08, 0); // U+017D
                    case (BYTE) 0x7E: cp1252w[0] = (BYTE) 0x9E; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x09, 0); // U+017E
                    case (BYTE) 0x92: cp1252w[0] = (BYTE) 0x83; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0A, 0); // U+0192
                }
                break;

            case (BYTE) 0x02:
                switch (codepoint3) {
                    case (BYTE) 0xC6: cp1252w[0] = (BYTE) 0x88; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0B, 0); // U+02C6
                    case (BYTE) 0xDC: cp1252w[0] = (BYTE) 0x98; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0C, 0); // U+02DC
                }
                break;

            case (BYTE) 0x20:
                switch (codepoint3) {
                    case (BYTE) 0x13: cp1252w[0] = (BYTE) 0x96; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0D, 0); // U+2013
                    case (BYTE) 0x14: cp1252w[0] = (BYTE) 0x97; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0E, 0); // U+2014
                    case (BYTE) 0x18: cp1252w[0] = (BYTE) 0x91; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x0F, 0); // U+2018
                    case (BYTE) 0x19: cp1252w[0] = (BYTE) 0x92; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x10, 0); // U+2019
                    case (BYTE) 0x1A: cp1252w[0] = (BYTE) 0x82; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x11, 0); // U+201A
                    case (BYTE) 0x1C: cp1252w[0] = (BYTE) 0x93; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x12, 0); // U+201C
                    case (BYTE) 0x1D: cp1252w[0] = (BYTE) 0x94; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x13, 0); // U+201D
                    case (BYTE) 0x1E: cp1252w[0] = (BYTE) 0x84; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x14, 0); // U+201E
                    case (BYTE) 0x20: cp1252w[0] = (BYTE) 0x86; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x15, 0); // U+2020
                    case (BYTE) 0x21: cp1252w[0] = (BYTE) 0x87; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x16, 0); // U+2021
                    case (BYTE) 0x22: cp1252w[0] = (BYTE) 0x95; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x17, 0); // U+2022
                    case (BYTE) 0x26: cp1252w[0] = (BYTE) 0x85; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x18, 0); // U+2026
                    case (BYTE) 0x30: cp1252w[0] = (BYTE) 0x89; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x19, 0); // U+2030
                    case (BYTE) 0x39: cp1252w[0] = (BYTE) 0x8B; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x1A, 0); // U+2039
                    case (BYTE) 0x3A: cp1252w[0] = (BYTE) 0x9B; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x1B, 0); // U+203A
                    case (BYTE) 0xAC: cp1252w[0] = (BYTE) 0x80; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x1C, 0); // U+20AC
                }
                break;

            case (BYTE) 0x21:
                switch (codepoint3) {
                    case (BYTE) 0x22: cp1252w[0] = (BYTE) 0x99; return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x1D, 0); // U+2122
                }
                break;
        }
    }

    // Not a valid CP1252 character
    cp1252w[0] = (BYTE) '?';
    return RESULT(STATUS_CODE_WARNING_PARTIAL, 0x04, 0x1E, 0);
}

CATALYST_RESULT catte_cp1252_decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint) {
    // Validate, initialize parameters
    if (cp1252 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x01, 0);
    }
    if (codepoint == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x02, 1);
    }

    BYTE codeunit0 = cp1252[0];
    codepoint->byte0 = 0;
    codepoint->byte1 = 0;
    codepoint->byte2 = 0;
    codepoint->byte3 = codeunit0;

    if (codeunit0 <= (BYTE) 0x7F || codeunit0 >= (BYTE) 0xA0) {
        // U+0000 through U+007F and U+00A0 through U+00FF map directly from CP1252
        return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x03, 0);
    }

    switch (codeunit0) {
        case (BYTE) 0x80: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0xAC; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x04, 0); // U+20AC
        case (BYTE) 0x82: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x1A; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x05, 0); // U+201A
        case (BYTE) 0x83: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x92; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x06, 0); // U+0192
        case (BYTE) 0x84: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x1E; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x07, 0); // U+201E
        case (BYTE) 0x85: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x26; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x08, 0); // U+2026
        case (BYTE) 0x86: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x20; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x09, 0); // U+2020
        case (BYTE) 0x87: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x21; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0A, 0); // U+2021
        case (BYTE) 0x88: codepoint->byte2 = (BYTE) 0x02; codepoint->byte3 = (BYTE) 0xC6; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0B, 0); // U+02C6
        case (BYTE) 0x89: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x30; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0C, 0); // U+2030
        case (BYTE) 0x8A: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x60; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0D, 0); // U+0160
        case (BYTE) 0x8B: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x39; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0E, 0); // U+2039
        case (BYTE) 0x8C: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x52; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x0F, 0); // U+0152
        case (BYTE) 0x8E: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x7D; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x10, 0); // U+017D
        case (BYTE) 0x91: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x18; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x11, 0); // U+2018
        case (BYTE) 0x92: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x19; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x12, 0); // U+2019
        case (BYTE) 0x93: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x1C; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x13, 0); // U+201C
        case (BYTE) 0x94: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x1D; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x14, 0); // U+201D
        case (BYTE) 0x95: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x22; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x15, 0); // U+2022
        case (BYTE) 0x96: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x13; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x16, 0); // U+2013
        case (BYTE) 0x97: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x14; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x17, 0); // U+2014
        case (BYTE) 0x98: codepoint->byte2 = (BYTE) 0x02; codepoint->byte3 = (BYTE) 0xDC; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x18, 0); // U+02DC
        case (BYTE) 0x99: codepoint->byte2 = (BYTE) 0x21; codepoint->byte3 = (BYTE) 0x22; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x19, 0); // U+2122
        case (BYTE) 0x9A: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x61; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x1A, 0); // U+0161
        case (BYTE) 0x9B: codepoint->byte2 = (BYTE) 0x20; codepoint->byte3 = (BYTE) 0x3A; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x1B, 0); // U+203A
        case (BYTE) 0x9C: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x53; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x1C, 0); // U+0153
        case (BYTE) 0x9E: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x7E; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x1D, 0); // U+017E
        case (BYTE) 0x9F: codepoint->byte2 = (BYTE) 0x01; codepoint->byte3 = (BYTE) 0x78; return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x1E, 0); // U+0178
    }

    // Undefined CP1252 code unit
    codepoint->byte0 = 0;
    codepoint->byte1 = 0;
    codepoint->byte2 = 0;
    codepoint->byte3 = (BYTE) '?';
    return RESULT(STATUS_CODE_WARNING_PARTIAL, 0x05, 0x1F, 0);
}

CATALYST_RESULT catte_cp1252_length(CATALYST_CP1252 cp1252, CATALYST_NUINT* length) {
    if (cp1252 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x01, 0);
    }
    if (length == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x02, 1);
    }

    NUINT count = 0;
    while (cp1252[count] != (BYTE) '\0') {
        if (++count == (NUINT) -1) {
            // Overflow, no null terminator found
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x03, 0);
        }
    }

    *length = count;
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x04, 0);
}


CATALYST_RESULT catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced) {
    // Validate, initialize parameters
    if (utf8w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 1);
    }
    if (produced == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x02, 2);
    }
    *produced = 0;

    // Prepare code units
    BYTE codepoint0 = codepoint.byte0;
    BYTE codepoint1 = codepoint.byte1;
    BYTE codepoint2 = codepoint.byte2;
    BYTE codepoint3 = codepoint.byte3;

    // 1-byte ASCII
    if (codepoint0 == 0 && codepoint1 == 0 && codepoint2 == 0 && codepoint3 < (BYTE) 0x80u) {
        utf8w[0] = codepoint3;
        *produced = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
    }

    // 2-byte sequence
    if (codepoint0 == 0 && codepoint1 == 0 && codepoint2 <= (BYTE) 0x07u) {
        if (codepoint2 == 0 && codepoint3 < (BYTE) 0x80u) {
            // Overlong encoding
            return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x07, 0x04, 0);
        }
        BYTE t0 = (BYTE) (((codepoint2 & (BYTE) 0x07u) << 2) | (codepoint3 >> 6));
        BYTE t1 = (BYTE) (codepoint3 & (BYTE) 0x3Fu);
        utf8w[0] = (BYTE) ((BYTE) 0xC0u | t0);
        utf8w[1] = (BYTE) ((BYTE) 0x80u | t1);
        *produced = 2;
        return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x05, 0);
    }

    // 3-byte sequence
    if (codepoint0 == 0 && codepoint1 == 0) {
        if (codepoint2 < (BYTE) 0x08u) {
            // Overlong encoding
            return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x07, 0x06, 0);
        }
        if (codepoint2 >= (BYTE) 0xD8u && codepoint2 <= (BYTE) 0xDFu) {
            // Invalid surrogate codepoint
            return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x07, 0);
        }
        BYTE t0 = (BYTE) (codepoint2 >> 4);
        BYTE t1 = (BYTE) (((codepoint2 & (BYTE) 0x0Fu) << 2) | (codepoint3 >> 6));
        BYTE t2 = (BYTE) (codepoint3 & (BYTE) 0x3Fu);
        utf8w[0] = (BYTE) ((BYTE) 0xE0u | t0);
        utf8w[1] = (BYTE) ((BYTE) 0x80u | t1);
        utf8w[2] = (BYTE) ((BYTE) 0x80u | t2);
        *produced = 3;
        return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x08, 0);
    }

    // 4-byte sequence
    if (codepoint0 == 0 && codepoint1 != 0 && codepoint1 <= (BYTE) 0x10u) {
        BYTE t0 = (BYTE) ((codepoint1 & (BYTE) 0x1Cu) >> 2);
        BYTE t1 = (BYTE) (((codepoint1 & (BYTE) 0x03u) << 4) | (codepoint2 >> 4));
        BYTE t2 = (BYTE) (((codepoint2 & (BYTE) 0x0Fu) << 2) | (codepoint3 >> 6));
        BYTE t3 = (BYTE) (codepoint3 & (BYTE) 0x3Fu);
        utf8w[0] = (BYTE) ((BYTE) 0xF0u | t0);
        utf8w[1] = (BYTE) ((BYTE) 0x80u | t1);
        utf8w[2] = (BYTE) ((BYTE) 0x80u | t2);
        utf8w[3] = (BYTE) ((BYTE) 0x80u | t3);
        *produced = 4;
        return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x09, 0);
    }

    // Invalid codepoint
    return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x0A, 0);
}


CATALYST_RESULT catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed) {
    // Validate, initialize parameters
    if (utf8 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 0);
    }
    if (codepoint == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x02, 1);
    }
    if (consumed == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x03, 2);
    }
    *consumed = 0;

    // Zero out the codepoint
    BYTE codeunit0 = utf8[0];
    codepoint->byte0 = 0;
    codepoint->byte1 = 0;
    codepoint->byte2 = 0;
    codepoint->byte3 = 0;

    // NUL '\0' character
    if (codeunit0 == (BYTE) '\0') {
        *consumed = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x04, 0);
    }

    // 1-byte ASCII
    if (codeunit0 < (BYTE) 0x80) {
        codepoint->byte3 = codeunit0;
        *consumed = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x05, 0);
    }

    // 2-byte sequence
    if (codeunit0 >= (BYTE) 0xC2u && codeunit0 <= (BYTE) 0xDFu) {
        BYTE codeunit1 = utf8[1];
        if (codeunit1 == 0) {
            // Unexpected end of input
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x06, 0);
        }
        if (codeunit1 < (BYTE) 0x80u || codeunit1 > (BYTE) 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x07, 0);
        }
        BYTE t0 = (BYTE) (codeunit0 & (BYTE) 0x1Fu);
        BYTE t1 = (BYTE) (codeunit1 & (BYTE) 0x3Fu);
        codepoint->byte2 = (BYTE) (t0 >> 2);
        codepoint->byte3 = (BYTE) (((t0 & (BYTE) 0x03u) << 6) | t1);
        *consumed = 2;
        return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x08, 0);
    }

    // 3-byte sequence
    if (codeunit0 >= (BYTE) 0xE0u && codeunit0 <= (BYTE) 0xEFu) {
        BYTE codeunit1 = utf8[1];
        BYTE codeunit2 = utf8[2];
        if (codeunit1 == 0 || codeunit2 == 0) {
            // Unexpected end of input
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x09, codeunit1 == 0 ? 1 : 2);
        }
        if (codeunit1 < (BYTE) 0x80u || codeunit1 > (BYTE) 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0A, 0);
        }
        if (codeunit2 < (BYTE) 0x80u || codeunit2 > (BYTE) 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0B, 0);
        }
        if (codeunit0 == (BYTE) 0xE0u && codeunit1 < (BYTE) 0xA0u) {
            // Overlong encoding
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0C, 0);
        }
        if (codeunit0 == (BYTE) 0xEDu && codeunit1 >= (BYTE) 0xA0u) {
            // Invalid surrogate codepoint
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0D, 0);
        }
        BYTE t0 = (BYTE) (codeunit0 & 0x0Fu);
        BYTE t1 = (BYTE) (codeunit1 & 0x3Fu);
        BYTE t2 = (BYTE) (codeunit2 & 0x3Fu);
        codepoint->byte2 = (BYTE) ((t0 << 4) | (t1 >> 2));
        codepoint->byte3 = (BYTE) (((t1 & 0x03u) << 6) | t2);
        *consumed = 3;
        return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x0E, 0);
    }

    // 4-byte sequence
    if (codeunit0 >= (BYTE) 0xF0u && codeunit0 <= (BYTE) 0xF4u) {
        BYTE codeunit1 = utf8[1];
        BYTE codeunit2 = utf8[2];
        BYTE codeunit3 = utf8[3];
        if (codeunit1 == 0 || codeunit2 == 0 || codeunit3 == 0) {
            // Unexpected end of input
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x0F, codeunit1 == 0 ? 1 : (codeunit2 == 0 ? 2 : 3));
        }
        if (codeunit1 < 0x80u || codeunit1 > 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x10, 0);
        }
        if (codeunit2 < 0x80u || codeunit2 > 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x11, 0);
        }
        if (codeunit3 < 0x80u || codeunit3 > 0xBFu) {
            // Invalid continuation byte
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x12, 0);
        }
        if (codeunit0 == 0xF0u && codeunit1 < 0x90u) {
            // Overlong encoding
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x13, 0);
        }
        if (codeunit0 == 0xF4u && codeunit1 > 0x8Fu) {
            // Codepoints above U+10FFFF
            return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x14, 0);
        }
        BYTE t0 = (BYTE) (codeunit0 & (BYTE) 0x07u);
        BYTE t1 = (BYTE) (codeunit1 & (BYTE) 0x3Fu);
        BYTE t2 = (BYTE) (codeunit2 & (BYTE) 0x3Fu);
        BYTE t3 = (BYTE) (codeunit3 & (BYTE) 0x3Fu);
        codepoint->byte0 = 0;
        codepoint->byte1 = (BYTE) ((t0 << 2) | (t1 >> 4));
        codepoint->byte2 = (BYTE) (((t1 & (BYTE) 0x0Fu) << 4) | (t2 >> 2));
        codepoint->byte3 = (BYTE) (((t2 & (BYTE) 0x03u) << 6) | t3);
        *consumed = 4;
        return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x15, 0); // Valid 4-byte sequence
    }

    // Invalid leading code unit
    return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x08, 0x16, 0);
}


CATALYST_RESULT catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length) {
    // Validate, initialize parameters
    if (utf8 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x01, 0);
    }
    if (length == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x02, 1);
    }
    *length = 0;

    // Skip BOM if present
    if (utf8[0] == (BYTE) 0xEFu && utf8[1] == (BYTE) 0xBBu && utf8[2] == (BYTE) 0xBFu) {
        utf8 += 3;
    }

    NUINT count = 0;
    while (*utf8) {
        CODEPOINT codepoint;
        NUINT consumed;
        RESULT decodeResult;
        decodeResult = catte_utf8_decode(utf8, &codepoint, &consumed);
        if (!status_code_is_success(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x09, 0x03, decodeResult.operation);
        }
        if (++count == (NUINT) -1) {
            // Overflow, no null terminator found
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x09, 0x04, 0);
        }
        utf8 += consumed;
    }
    *length = count;
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x05, 0);
}


CATALYST_RESULT catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced) {
    // Validate, initialize parameters
    if (utf16lew == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 1);
    }
    if (produced == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x02, 2);
    }
    *produced = 0;

    // Prepare code units
    BYTE codepoint0 = codepoint.byte0;
    BYTE codepoint1 = codepoint.byte1;
    BYTE codepoint2 = codepoint.byte2;
    BYTE codepoint3 = codepoint.byte3;

    // Validate codepoint is within Unicode range U+0000 to U+10FFFF
    if (codepoint0 != 0 || codepoint1 > (BYTE) 0x10u) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x03, 0); // Invalid codepoint above U+10FFFF
    }

    // Single UTF-16 code unit
    if (codepoint1 == 0) {
        if (codepoint2 >= (BYTE) 0xD8u && codepoint2 <= (BYTE) 0xDFu) {
            return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x04, 0); // Invalid surrogate codepoint
        }
        utf16lew[0].msb = codepoint2;
        utf16lew[0].lsb = codepoint3;
        *produced = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x05, 0);
    }

    // Surrogate pair
    BYTE x2 = (BYTE) (codepoint1 - (BYTE) 0x01u);
    BYTE x1 = codepoint2;
    BYTE x0 = codepoint3;

    // High surrogate: 0xD800 + ((codepoint - 0x10000) >> 10)
    BYTE hs_hi2 = (BYTE) (x2 >> 2);
    BYTE hs_lo8 = (BYTE) (((x2 & (BYTE) 0x03u) << 6) | (x1 >> 2));

    // Low surrogate: 0xDC00 + ((codepoint - 0x10000) & 0x03FF)
    BYTE ls_hi2 = (BYTE) (x1 & (BYTE) 0x03u);
    BYTE ls_lo8 = x0;

    utf16lew[0].msb = (BYTE) ((BYTE) 0xD8u | hs_hi2);
    utf16lew[0].lsb = hs_lo8;
    utf16lew[1].msb = (BYTE) ((BYTE) 0xDCu | ls_hi2);
    utf16lew[1].lsb = ls_lo8;
    *produced = 2;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x06, 0);
}


CATALYST_RESULT catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed) {
    // Validate, initialize parameters
    if (utf16le == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x01, 0);
    }
    if (codepoint == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x02, 1);
    }
    if (consumed == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x03, 2);
    }
    *consumed = 0;

    // Zero out the codepoint
    BYTE codeunit0 = utf16le[0].msb;
    BYTE codeunit1 = utf16le[0].lsb;
    codepoint->byte0 = 0;
    codepoint->byte1 = 0;
    codepoint->byte2 = 0;
    codepoint->byte3 = 0;

    // NUL '\0' character
    if (codeunit0 == 0 && codeunit1 == 0) {
        *consumed = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x04, 0);
    }

    // Low surrogate without preceding high surrogate
    if (codeunit0 >= (BYTE) 0xDCu && codeunit0 <= (BYTE) 0xDFu) {
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0B, 0x05, 0);
    }

    // Not a surrogate pair, single code unit
    if (codeunit0 < (BYTE) 0xD8u || codeunit0 > (BYTE) 0xDBu) {
        codepoint->byte2 = codeunit0;
        codepoint->byte3 = codeunit1;
        *consumed = 1;
        return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x06, 0);
    }

    // Surrogate pair, must be followed by a low surrogate
    BYTE codeunit2 = utf16le[1].msb;
    BYTE codeunit3 = utf16le[1].lsb;

    if (!(codeunit2 >= (BYTE) 0xDCu && codeunit2 <= (BYTE) 0xDFu)) {
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0x0B, 0x07, 0);
    }

    // Extract the 20-bit codepoint from the surrogate pair
    BYTE hs_hi = (BYTE) (codeunit0 & (BYTE) 0x03u);
    BYTE hs_lo = codeunit1;
    BYTE ls_hi = (BYTE) (codeunit2 & (BYTE) 0x03u);
    BYTE ls_lo = codeunit3;

    BYTE p0 = ls_lo;
    BYTE p1 = (BYTE) (((hs_lo & (BYTE) 0x3Fu) << 2) | ls_hi);
    BYTE p2 = (BYTE) (((hs_hi & (BYTE) 0x03u) << 2) | (hs_lo >> 6));

    codepoint->byte0 = 0;
    codepoint->byte1 = (BYTE) (p2 + (BYTE) 0x01u);
    codepoint->byte2 = p1;
    codepoint->byte3 = p0;
    *consumed = 2;
    return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x08, 0);
}


CATALYST_RESULT catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length) {
    // Validate, initialize parameters
    if (utf16le == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x01, 0);
    }
    if (length == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x02, 1);
    }
    *length = 0;

    // Skip BOM if present
    if (utf16le[0].msb == (BYTE) 0xFEu && utf16le[0].lsb == (BYTE) 0xFFu) {
        utf16le += 1;
    }

    NUINT count = 0;
    while (utf16le[0].msb != 0 || utf16le[0].lsb != 0) {
        CODEPOINT codepoint;
        NUINT consumed;
        RESULT decodeResult;
        decodeResult = catte_utf16le_decode(utf16le, &codepoint, &consumed);
        if (!status_code_is_success(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x0C, 0x03, decodeResult.operation); // Invalid UTF-16LE sequence
        }
        if (++count == (NUINT) -1) {
            // Overflow, no null terminator found
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0C, 0x04, 0);
        }
        utf16le += consumed;
    }
    *length = count;
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x05, 0);
}

CATALYST_RESULT catte_ascii_to_cp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w) {
    // Validate, initialize parameters
    if (ascii == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0D, 0x01, 0);
    }
    if (cp1252w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0D, 0x02, 1);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT index = 0;

    while (ascii[index] != (BYTE) '\0') {
        if (ascii[index] < (BYTE) 0x80u) {
            cp1252w[index] = ascii[index];
        } else {
            cp1252w[index] = (BYTE) '?';
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (++index == (NUINT) -1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0D, 0x03, 0);
        }
    }

    cp1252w[index] = (BYTE) '\0';
    return RESULT(status, 0x0D, 0x04, 0);
}

CATALYST_RESULT catte_ascii_to_utf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (ascii == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x01, 0);
    }
    if (utf8w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0E, 0x03, 0);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT read = 0;
    NUINT write = 0;

    while (ascii[read] != (BYTE) '\0') {
        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0E, 0x04, 0);
        }

        if (ascii[read] < (BYTE) 0x80u) {
            utf8w[write] = ascii[read];
        } else {
            utf8w[write] = (BYTE) '?';
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        ++read;
        ++write;
    }

    utf8w[write] = (BYTE) '\0';
    return RESULT(status, 0x0E, 0x05, 0);
}

CATALYST_RESULT catte_ascii_to_utf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (ascii == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0F, 0x01, 0);
    }
    if (utf16lew == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0F, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0F, 0x03, 0);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT read = 0;
    NUINT write = 0;

    while (ascii[read] != (BYTE) '\0') {
        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0F, 0x04, 0);
        }

        utf16lew[write].msb = 0;

        if (ascii[read] < (BYTE) 0x80u) {
            utf16lew[write].lsb = ascii[read];
        } else {
            utf16lew[write].lsb = (BYTE) '?';
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        ++read;
        ++write;
    }

    utf16lew[write].msb = 0;
    utf16lew[write].lsb = 0;
    return RESULT(status, 0x0F, 0x05, 0);
}


CATALYST_RESULT catte_cp1252_to_ascii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW asciiw) {
    // Validate, initialize parameters
    if (cp1252 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x01, 0);
    }
    if (asciiw == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x02, 1);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT index = 0;

    while (cp1252[index] != (BYTE) '\0') {
        CODEPOINT codepoint;
        BYTE output[1];
        RESULT decodeResult;
        RESULT encodeResult;

        decodeResult = catte_cp1252_decode(cp1252 + index, &codepoint);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x10, 0x03, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_ascii_encode(codepoint, output);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x10, 0x04, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        asciiw[index] = output[0];

        if (++index == (NUINT) -1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x05, 0);
        }
    }

    asciiw[index] = (BYTE) '\0';
    return RESULT(status, 0x10, 0x06, 0);
}


CATALYST_RESULT catte_cp1252_to_utf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (cp1252 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x01, 0);
    }
    if (utf8w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x11, 0x03, 0);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT read = 0;
    NUINT write = 0;

    while (cp1252[read] != (BYTE) '\0') {
        CODEPOINT codepoint;
        BYTE output[4];
        NUINT produced;
        RESULT decodeResult;
        RESULT encodeResult;
        NUINT i;

        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;

        decodeResult = catte_cp1252_decode(cp1252 + read, &codepoint);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x11, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_utf8_encode(codepoint, output, &produced);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x11, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || produced >= (capacity - write)) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x11, 0x06, 0);
        }

        for (i = 0; i < produced; ++i) {
            utf8w[write + i] = output[i];
        }

        ++read;
        write += produced;
    }

    utf8w[write] = (BYTE) '\0';
    return RESULT(status, 0x11, 0x07, 0);
}


CATALYST_RESULT catte_cp1252_to_utf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (cp1252 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x01, 0);
    }
    if (utf16lew == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x12, 0x03, 0);
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT read = 0;
    NUINT write = 0;

    while (cp1252[read] != (BYTE) '\0') {
        CODEPOINT codepoint;
        UTF16LE_CODE_UNIT output[2];
        NUINT produced;
        RESULT decodeResult;
        RESULT encodeResult;
        NUINT i;

        output[0].msb = 0;
        output[0].lsb = 0;
        output[1].msb = 0;
        output[1].lsb = 0;

        decodeResult = catte_cp1252_decode(cp1252 + read, &codepoint);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x12, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_utf16le_encode(codepoint, output, &produced);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x12, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || produced >= (capacity - write)) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x12, 0x06, 0);
        }

        for (i = 0; i < produced; ++i) {
            utf16lew[write + i] = output[i];
        }

        ++read;
        write += produced;
    }

    utf16lew[write].msb = 0;
    utf16lew[write].lsb = 0;
    return RESULT(status, 0x12, 0x07, 0);
}


CATALYST_RESULT catte_utf8_to_ascii(CATALYST_UTF8 utf8, CATALYST_ASCIIW asciiw, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf8 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x01, 0);
    }
    if (asciiw == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x13, 0x03, 0);
    }

    // Skip BOM if present
    if (utf8[0] == (BYTE) 0xEFu && utf8[1] == (BYTE) 0xBBu && utf8[2] == (BYTE) 0xBFu) {
        utf8 += 3;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf8[0] != (BYTE) '\0') {
        CODEPOINT codepoint;
        BYTE output[1];
        NUINT consumed;
        RESULT decodeResult;
        RESULT encodeResult;

        decodeResult = catte_utf8_decode(utf8, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x13, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_ascii_encode(codepoint, output);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x13, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x13, 0x06, 0);
        }

        asciiw[write] = output[0];
        utf8 += consumed;
        ++write;
    }

    asciiw[write] = (BYTE) '\0';
    return RESULT(status, 0x13, 0x07, 0);
}


CATALYST_RESULT catte_utf8_to_cp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf8 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x01, 0);
    }
    if (cp1252w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x14, 0x03, 0);
    }

    // Skip BOM if present
    if (utf8[0] == (BYTE) 0xEFu && utf8[1] == (BYTE) 0xBBu && utf8[2] == (BYTE) 0xBFu) {
        utf8 += 3;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf8[0] != (BYTE) '\0') {
        CODEPOINT codepoint;
        BYTE output[1];
        NUINT consumed;
        RESULT decodeResult;
        RESULT encodeResult;

        decodeResult = catte_utf8_decode(utf8, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x14, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_cp1252_encode(codepoint, output);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x14, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x14, 0x06, 0);
        }

        cp1252w[write] = output[0];
        utf8 += consumed;
        ++write;
    }

    cp1252w[write] = (BYTE) '\0';
    return RESULT(status, 0x14, 0x07, 0);
}


CATALYST_RESULT catte_utf8_to_utf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf8 == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x01, 0);
    }
    if (utf16lew == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x15, 0x03, 0);
    }

    // Skip BOM if present
    if (utf8[0] == (BYTE) 0xEFu && utf8[1] == (BYTE) 0xBBu && utf8[2] == (BYTE) 0xBFu) {
        utf8 += 3;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf8[0] != (BYTE) '\0') {
        CODEPOINT codepoint;
        UTF16LE_CODE_UNIT output[2];
        NUINT consumed;
        NUINT produced;
        RESULT decodeResult;
        RESULT encodeResult;
        NUINT i;

        output[0].msb = 0;
        output[0].lsb = 0;
        output[1].msb = 0;
        output[1].lsb = 0;

        decodeResult = catte_utf8_decode(utf8, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x15, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_utf16le_encode(codepoint, output, &produced);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x15, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || produced >= (capacity - write)) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x15, 0x06, 0);
        }

        for (i = 0; i < produced; ++i) {
            utf16lew[write + i] = output[i];
        }

        utf8 += consumed;
        write += produced;
    }

    utf16lew[write].msb = 0;
    utf16lew[write].lsb = 0;
    return RESULT(status, 0x15, 0x07, 0);
}


CATALYST_RESULT catte_utf16le_to_ascii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW asciiw, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf16le == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x16, 0x01, 0);
    }
    if (asciiw == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x16, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x16, 0x03, 0);
    }

    // Skip BOM if present
    if (utf16le[0].msb == (BYTE) 0xFEu && utf16le[0].lsb == (BYTE) 0xFFu) {
        utf16le += 1;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf16le[0].msb != 0 || utf16le[0].lsb != 0) {
        CODEPOINT codepoint;
        BYTE output[1];
        NUINT consumed;
        RESULT decodeResult;
        RESULT encodeResult;

        decodeResult = catte_utf16le_decode(utf16le, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x16, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_ascii_encode(codepoint, output);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x16, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x16, 0x06, 0);
        }

        asciiw[write] = output[0];
        utf16le += consumed;
        ++write;
    }

    asciiw[write] = (BYTE) '\0';
    return RESULT(status, 0x16, 0x07, 0);
}


CATALYST_RESULT catte_utf16le_to_cp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf16le == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x01, 0);
    }
    if (cp1252w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x17, 0x03, 0);
    }

    // Skip BOM if present
    if (utf16le[0].msb == (BYTE) 0xFEu && utf16le[0].lsb == (BYTE) 0xFFu) {
        utf16le += 1;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf16le[0].msb != 0 || utf16le[0].lsb != 0) {
        CODEPOINT codepoint;
        BYTE output[1];
        NUINT consumed;
        RESULT decodeResult;
        RESULT encodeResult;

        decodeResult = catte_utf16le_decode(utf16le, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x17, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_cp1252_encode(codepoint, output);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x17, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || (capacity - write) <= 1) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x17, 0x06, 0);
        }

        cp1252w[write] = output[0];
        utf16le += consumed;
        ++write;
    }

    cp1252w[write] = (BYTE) '\0';
    return RESULT(status, 0x17, 0x07, 0);
}


CATALYST_RESULT catte_utf16le_to_utf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity) {
    // Validate, initialize parameters
    if (utf16le == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x18, 0x01, 0);
    }
    if (utf8w == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x18, 0x02, 1);
    }
    if (capacity == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x18, 0x03, 0);
    }

    // Skip BOM if present
    if (utf16le[0].msb == (BYTE) 0xFEu && utf16le[0].lsb == (BYTE) 0xFFu) {
        utf16le += 1;
    }

    STATUS_CODE status = STATUS_CODE_SUCCESS;
    NUINT write = 0;

    while (utf16le[0].msb != 0 || utf16le[0].lsb != 0) {
        CODEPOINT codepoint;
        BYTE output[4];
        NUINT consumed;
        NUINT produced;
        RESULT decodeResult;
        RESULT encodeResult;
        NUINT i;

        output[0] = 0;
        output[1] = 0;
        output[2] = 0;
        output[3] = 0;

        decodeResult = catte_utf16le_decode(utf16le, &codepoint, &consumed);
        if (status_code_is_error(decodeResult.status) || status_code_is_fatal(decodeResult.status)) {
            return RESULT(decodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : decodeResult.status, 0x18, 0x04, decodeResult.operation);
        }
        if (status_code_is_warning(decodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        encodeResult = catte_utf8_encode(codepoint, output, &produced);
        if (status_code_is_error(encodeResult.status) || status_code_is_fatal(encodeResult.status)) {
            return RESULT(encodeResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : encodeResult.status, 0x18, 0x05, encodeResult.operation);
        }
        if (status_code_is_warning(encodeResult.status)) {
            status = STATUS_CODE_WARNING_PARTIAL;
        }

        if (write >= capacity || produced >= (capacity - write)) {
            return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x18, 0x06, 0);
        }

        for (i = 0; i < produced; ++i) {
            utf8w[write + i] = output[i];
        }

        utf16le += consumed;
        write += produced;
    }

    utf8w[write] = (BYTE) '\0';
    return RESULT(status, 0x18, 0x07, 0);
}
