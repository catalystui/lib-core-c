#ifndef CATTELIB_HPP
#define CATTELIB_HPP
#include "catalyst.hpp"
#include "cattelib.h"
/**
 * C++ namespace wrappers for cattelib.h. Buffer sizes, code-unit counts,
 * termination, lookahead, replacement, and result behavior match the C API.
 */
namespace catalyst {

inline void ascii_encode(const CODEPOINT codepoint, ASCIIW asciiw, RESULT* result) {
    ::catte_ascii_encode(codepoint, asciiw, result);
}

inline void ascii_decode(ASCII ascii, CODEPOINT* codepoint, RESULT* result) {
    ::catte_ascii_decode(ascii, codepoint, result);
}

inline void ascii_length(ASCII ascii, NUINT* length, RESULT* result) {
    ::catte_ascii_length(ascii, length, result);
}

inline void cp1252_encode(const CODEPOINT codepoint, CP1252W cp1252w, RESULT* result) {
    ::catte_cp1252_encode(codepoint, cp1252w, result);
}

inline void cp1252_decode(CP1252 cp1252, CODEPOINT* codepoint, RESULT* result) {
    ::catte_cp1252_decode(cp1252, codepoint, result);
}

inline void cp1252_length(CP1252 cp1252, NUINT* length, RESULT* result) {
    ::catte_cp1252_length(cp1252, length, result);
}

inline void utf8_encode(const CODEPOINT codepoint, UTF8W utf8w, NUINT* produced, RESULT* result) {
    ::catte_utf8_encode(codepoint, utf8w, produced, result);
}

inline void utf8_decode(UTF8 utf8, CODEPOINT* codepoint, NUINT* consumed, RESULT* result) {
    ::catte_utf8_decode(utf8, codepoint, consumed, result);
}

inline void utf8_length(UTF8 utf8, NUINT* length, RESULT* result) {
    ::catte_utf8_length(utf8, length, result);
}

inline void utf16le_encode(const CODEPOINT codepoint, UTF16LEW utf16lew, NUINT* produced, RESULT* result) {
    ::catte_utf16le_encode(codepoint, utf16lew, produced, result);
}

inline void utf16le_decode(UTF16LE utf16le, CODEPOINT* codepoint, NUINT* consumed, RESULT* result) {
    ::catte_utf16le_decode(utf16le, codepoint, consumed, result);
}

inline void utf16le_length(UTF16LE utf16le, NUINT* length, RESULT* result) {
    ::catte_utf16le_length(utf16le, length, result);
}

inline void ascii_to_cp1252(ASCII ascii, CP1252W cp1252w, RESULT* result) {
    ::catte_ascii_to_cp1252(ascii, cp1252w, result);
}

inline void ascii_to_utf8(ASCII ascii, UTF8W utf8w, NUINT capacity, RESULT* result) {
    ::catte_ascii_to_utf8(ascii, utf8w, capacity, result);
}

inline void ascii_to_utf16le(ASCII ascii, UTF16LEW utf16lew, NUINT capacity, RESULT* result) {
    ::catte_ascii_to_utf16le(ascii, utf16lew, capacity, result);
}

inline void cp1252_to_ascii(CP1252 cp1252, ASCIIW ascii, RESULT* result) {
    ::catte_cp1252_to_ascii(cp1252, ascii, result);
}

inline void cp1252_to_utf8(CP1252 cp1252, UTF8W utf8w, NUINT capacity, RESULT* result) {
    ::catte_cp1252_to_utf8(cp1252, utf8w, capacity, result);
}

inline void cp1252_to_utf16le(CP1252 cp1252, UTF16LEW utf16lew, NUINT capacity, RESULT* result) {
    ::catte_cp1252_to_utf16le(cp1252, utf16lew, capacity, result);
}

inline void utf8_to_ascii(UTF8 utf8, ASCIIW ascii, NUINT capacity, RESULT* result) {
    ::catte_utf8_to_ascii(utf8, ascii, capacity, result);
}

inline void utf8_to_cp1252(UTF8 utf8, CP1252W cp1252w, NUINT capacity, RESULT* result) {
    ::catte_utf8_to_cp1252(utf8, cp1252w, capacity, result);
}

inline void utf8_to_utf16le(UTF8 utf8, UTF16LEW utf16lew, NUINT capacity, RESULT* result) {
    ::catte_utf8_to_utf16le(utf8, utf16lew, capacity, result);
}

inline void utf16le_to_ascii(UTF16LE utf16le, ASCIIW ascii, NUINT capacity, RESULT* result) {
    ::catte_utf16le_to_ascii(utf16le, ascii, capacity, result);
}

inline void utf16le_to_cp1252(UTF16LE utf16le, CP1252W cp1252w, NUINT capacity, RESULT* result) {
    ::catte_utf16le_to_cp1252(utf16le, cp1252w, capacity, result);
}

inline void utf16le_to_utf8(UTF16LE utf16le, UTF8W utf8w, NUINT capacity, RESULT* result) {
    ::catte_utf16le_to_utf8(utf16le, utf8w, capacity, result);
}

} /* namespace catalyst */
#endif /* CATTELIB_HPP */
