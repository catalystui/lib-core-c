#ifndef CATTELIB_H
#define CATTELIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATTELIB_EXPORTS)
        #define CATTELIB_API __declspec(dllexport)
    #elif defined(CATTELIB_USE_DLL)
        #define CATTELIB_API __declspec(dllimport)
    #else
        #define CATTELIB_API
    #endif
#else
    #define CATTELIB_API
#endif

/** Encode one codepoint as ASCII, substituting '?' if unrepresentable. */
CATTELIB_API CATALYST_RESULT catte_ascii_encode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw);

/** Decode one readable ASCII unit without validating its range; return the codepoint. */
CATTELIB_API CATALYST_CODEPOINT catte_ascii_decode(CATALYST_ASCII ascii);

/** Return the count of nonzero ASCII units in a valid null-terminated string. */
CATTELIB_API CATALYST_NUINT catte_ascii_length(CATALYST_ASCII ascii);

/** Encode one codepoint as CP1252, substituting '?' if unrepresentable. */
CATTELIB_API CATALYST_RESULT catte_cp1252_encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w);

/** Decode one CP1252 unit, substituting '?' for undefined bytes. */
CATTELIB_API CATALYST_RESULT catte_cp1252_decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint);

/** Return the count of nonzero CP1252 units in a valid null-terminated string. */
CATTELIB_API CATALYST_NUINT catte_cp1252_length(CATALYST_CP1252 cp1252);

/** Encode one codepoint into UTF-8, reporting invalid scalars; output must hold four code units. */
CATTELIB_API CATALYST_RESULT catte_utf8_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced);

/** Decode one UTF-8 scalar; input must hold its full sequence or an accessible terminating NUL. */
CATTELIB_API CATALYST_RESULT catte_utf8_decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed);

/** Count decoded scalars in terminated UTF-8, excluding a leading BOM. */
CATTELIB_API CATALYST_RESULT catte_utf8_length(CATALYST_UTF8 utf8, CATALYST_NUINT* length);

/** Encode one codepoint into UTF-16LE, reporting invalid scalars; output must hold two code units. */
CATTELIB_API CATALYST_RESULT catte_utf16le_encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced);

/** Decode one UTF-16LE scalar; input must hold its full sequence or an accessible terminating NUL unit. */
CATTELIB_API CATALYST_RESULT catte_utf16le_decode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed);

/** Count decoded scalars in terminated UTF-16LE, excluding a leading BOM. */
CATTELIB_API CATALYST_RESULT catte_utf16le_length(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length);

/** Convert a terminated ASCII string to CP1252. Destination must hold source length plus one unit. */
CATTELIB_API CATALYST_RESULT catte_ascii_to_cp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w);

/** Convert a terminated ASCII string to UTF-8. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_ascii_to_utf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);

/** Convert a terminated ASCII string to UTF-16LE. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_ascii_to_utf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);

/** Convert a terminated CP1252 string to ASCII. Destination must hold source length plus one unit. */
CATTELIB_API CATALYST_RESULT catte_cp1252_to_ascii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW ascii);

/** Convert a terminated CP1252 string to UTF-8. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_cp1252_to_utf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);

/** Convert a terminated CP1252 string to UTF-16LE. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_cp1252_to_utf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);

/** Convert a terminated UTF-8 string to ASCII. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf8_to_ascii(CATALYST_UTF8 utf8, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity);

/** Convert a terminated UTF-8 string to CP1252. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf8_to_cp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity);

/** Convert a terminated UTF-8 string to UTF-16LE. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf8_to_utf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity);

/** Convert a terminated UTF-16LE string to ASCII. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf16le_to_ascii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity);

/** Convert a terminated UTF-16LE string to CP1252. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf16le_to_cp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity);

/** Convert a terminated UTF-16LE string to UTF-8. Capacity includes the destination terminator. */
CATTELIB_API CATALYST_RESULT catte_utf16le_to_utf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTELIB_H */
