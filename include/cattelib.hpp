#ifndef CATTELIB_HPP
#define CATTELIB_HPP
#include "catalyst.hpp"
#include "cattelib.h"
namespace catalyst {

inline RESULT ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw) {
    return ::catte_ascii_encode(codepoint, asciiw);
}

inline CODEPOINT ascii_decode(ASCII ascii) {
    return ::catte_ascii_decode(ascii);
}

inline NUINT ascii_length(ASCII ascii) {
    return ::catte_ascii_length(ascii);
}

inline RESULT cp1252_encode(const CODEPOINT codepoint, CP1252W cp1252w) {
    return ::catte_cp1252_encode(codepoint, cp1252w);
}

inline RESULT cp1252_decode(CP1252 cp1252, CODEPOINT* codepoint) {
    return ::catte_cp1252_decode(cp1252, codepoint);
}

inline NUINT cp1252_length(CP1252 cp1252) {
    return ::catte_cp1252_length(cp1252);
}

inline RESULT utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced) {
    return ::catte_utf8_encode(codepoint, utf8w, produced);
}

inline RESULT utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed) {
    return ::catte_utf8_decode(utf8, codepoint, consumed);
}

inline RESULT utf8_length(UTF8 utf8, NUINT* length) {
    return ::catte_utf8_length(utf8, length);
}

inline RESULT utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced) {
    return ::catte_utf16le_encode(codepoint, utf16lew, produced);
}

inline RESULT utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed) {
    return ::catte_utf16le_decode(utf16le, codepoint, consumed);
}

inline RESULT utf16le_length(UTF16LE utf16le, NUINT* length) {
    return ::catte_utf16le_length(utf16le, length);
}

inline RESULT ascii_to_cp1252(ASCII ascii, CP1252W cp1252w) {
    return ::catte_ascii_to_cp1252(ascii, cp1252w);
}

inline RESULT ascii_to_utf8(ASCII ascii, UTF8W utf8w, NUINT capacity) {
    return ::catte_ascii_to_utf8(ascii, utf8w, capacity);
}

inline RESULT ascii_to_utf16le(ASCII ascii, UTF16LEW utf16lew, NUINT capacity) {
    return ::catte_ascii_to_utf16le(ascii, utf16lew, capacity);
}

inline RESULT cp1252_to_ascii(CP1252 cp1252, ASCIIW ascii) {
    return ::catte_cp1252_to_ascii(cp1252, ascii);
}

inline RESULT cp1252_to_utf8(CP1252 cp1252, UTF8W utf8w, NUINT capacity) {
    return ::catte_cp1252_to_utf8(cp1252, utf8w, capacity);
}

inline RESULT cp1252_to_utf16le(CP1252 cp1252, UTF16LEW utf16lew, NUINT capacity) {
    return ::catte_cp1252_to_utf16le(cp1252, utf16lew, capacity);
}

inline RESULT utf8_to_ascii(UTF8 utf8, ASCIIW ascii, NUINT capacity) {
    return ::catte_utf8_to_ascii(utf8, ascii, capacity);
}

inline RESULT utf8_to_cp1252(UTF8 utf8, CP1252W cp1252w, NUINT capacity) {
    return ::catte_utf8_to_cp1252(utf8, cp1252w, capacity);
}

inline RESULT utf8_to_utf16le(UTF8 utf8, UTF16LEW utf16lew, NUINT capacity) {
    return ::catte_utf8_to_utf16le(utf8, utf16lew, capacity);
}

inline RESULT utf16le_to_ascii(UTF16LE utf16le, ASCIIW ascii, NUINT capacity) {
    return ::catte_utf16le_to_ascii(utf16le, ascii, capacity);
}

inline RESULT utf16le_to_cp1252(UTF16LE utf16le, CP1252W cp1252w, NUINT capacity) {
    return ::catte_utf16le_to_cp1252(utf16le, cp1252w, capacity);
}

inline RESULT utf16le_to_utf8(UTF16LE utf16le, UTF8W utf8w, NUINT capacity) {
    return ::catte_utf16le_to_utf8(utf16le, utf8w, capacity);
}

} /* namespace catalyst */
#endif /* CATTELIB_HPP */
