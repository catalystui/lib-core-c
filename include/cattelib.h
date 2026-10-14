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

/**
 * Text operation conventions shared by the functions below:
 * - result may be null. Other pointer arguments must address readable input or
 *   writable output storage as appropriate; null arguments return the
 *   function-specific ERROR_INVALID_ARGUMENT detail listed below.
 * - String operations consume null-terminated input. Length excludes the
 *   terminator. Single-codepoint encoders/decoders process one value, including
 *   U+0000, and do not append a separate terminator or skip a BOM.
 * - capacity and produced count destination code units; consumed counts source
 *   code units. ASCII, CP1252, and UTF-8 units are bytes; UTF-16LE units are
 *   two-byte CATALYST_UTF16LE_CODE_UNIT objects.
 * - UTF encoders require space for up to four UTF-8 or two UTF-16LE units.
 *   UTF decoders require readable units for the sequence width selected by
 *   the leading unit, including malformed sequences (up to four UTF-8 units
 *   or two UTF-16LE units). String operations inherit this lookahead requirement.
 *   There is no input buffer-size argument or bounds discovery.
 * - Whole-string conversions include one destination terminator in capacity,
 *   terminate on success/warning, and may leave an unterminated written prefix
 *   on error. They do not roll back output or report partial output counts.
 *   Source/destination overlap and aliasing between output arguments have no
 *   general guarantee; use separate storage.
 * - UTF string length/conversion operations skip a leading UTF-8 EF BB BF or
 *   UTF-16LE FF FE BOM. They do not add an output BOM. Internal U+FEFF is data.
 * - context is always zero. operation and detail are zero on success/warning.
 *   Unless listed otherwise, errors also use operation/detail zero.
 *   Dependency failures use operation 1 for decoding or 2 for encoding, with
 *   the dependency's status byte in detail; its other result fields are lost.
 */
/**
 * Encode one codepoint into one ASCII unit. U+0000 writes a zero unit.
 * Unrepresentable values write '?' and return WARNING_PARTIAL.
 * ERROR_INVALID_ARGUMENT: detail 0 = null asciiw (the output argument).
 */
CATTELIB_API void catteAsciiEncode(const CATALYST_CODEPOINT codepoint, CATALYST_ASCIIW asciiw, CATALYST_RESULT* result);

/**
 * Read one ASCII unit into codepoint, including the zero terminator.
 * The unit is copied numerically without checking its high bit; callers are
 * responsible for supplying valid ASCII when ASCII semantics are required.
 * ERROR_INVALID_ARGUMENT: detail 0 = null ascii, 1 = null codepoint.
 * On success all four codepoint bytes are assigned; null-argument errors
 * leave the codepoint storage unchanged.
 */
CATTELIB_API void catteAsciiDecode(CATALYST_ASCII ascii, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);

/**
 * Count nonzero ASCII units before the terminator, without validating them.
 * ERROR_INVALID_ARGUMENT: detail 0 = null ascii, 1 = null length.
 * ERROR_BUFFER_OVERFLOW: count reaches CATALYST_NUINT_MAX.
 * *length is assigned only on success and is otherwise unchanged.
 */
CATTELIB_API void catteAsciiLength(CATALYST_ASCII ascii, CATALYST_NUINT* length, CATALYST_RESULT* result);

/**
 * Encode one codepoint into one Windows-1252 unit, including U+0000.
 * Unrepresentable values write '?' and return WARNING_PARTIAL.
 * ERROR_INVALID_ARGUMENT: detail 0 = null cp1252w (the output argument).
 */
CATTELIB_API void catteCp1252Encode(const CATALYST_CODEPOINT codepoint, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);

/**
 * Decode one Windows-1252 unit into codepoint, including the terminator.
 * Undefined bytes 81, 8D, 8F, 90, and 9D (hex) produce U+003F ('?') and
 * WARNING_PARTIAL. Other bytes use the Windows-1252 mapping.
 * ERROR_INVALID_ARGUMENT: detail 0 = null cp1252, 1 = null codepoint.
 * Null-argument errors leave the codepoint storage unchanged.
 */
CATTELIB_API void catteCp1252Decode(CATALYST_CP1252 cp1252, CATALYST_CODEPOINT* codepoint, CATALYST_RESULT* result);

/**
 * Count nonzero CP1252 units before the terminator without decoding them.
 * Undefined Windows-1252 bytes count as one unit and do not cause a warning.
 * ERROR_INVALID_ARGUMENT: detail 0 = null cp1252, 1 = null length.
 * ERROR_BUFFER_OVERFLOW: count reaches CATALYST_NUINT_MAX.
 * *length is assigned only on success and is otherwise unchanged.
 */
CATTELIB_API void catteCp1252Length(CATALYST_CP1252 cp1252, CATALYST_NUINT* length, CATALYST_RESULT* result);

/**
 * Encode one Unicode scalar value into one to four UTF-8 units.
 * U+0000 produces one zero unit. No additional terminator is written.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf8w, 1 = null produced.
 * After pointer validation, *produced is zero until success. Encoding errors
 * leave the destination unchanged and use ERROR with detail 0:
 * operation 1 = overlong encoding branch; 2 = surrogate U+D800..U+DFFF;
 * 3 = codepoint above U+10FFFF. Operation 1 is retained by the implementation,
 * but earlier branches handle the values that would reach this condition.
 */
CATTELIB_API void catteUtf8Encode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF8W utf8w, CATALYST_NUINT* produced, CATALYST_RESULT* result);

/**
 * Decode one UTF-8 sequence into a Unicode scalar value.
 * U+0000 consumes one unit; U+FEFF is decoded normally, including a BOM.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf8, 1 = null codepoint,
 * 2 = null consumed. These errors leave output storage unchanged.
 * After pointer validation, codepoint and *consumed are zeroed. Decode errors
 * leave them zero and use ERROR with detail 0:
 * operation 1 = zero unit before sequence completion; 2 = invalid continuation;
 * 3 = overlong sequence; 4 = surrogate; 5 = codepoint above U+10FFFF;
 * 6 = invalid leading unit (including C0/C1, stray continuations, and F5..FF).
 * On success, *consumed is the sequence's one-to-four-unit width.
 */
CATTELIB_API void catteUtf8Decode(CATALYST_UTF8 utf8, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);

/**
 * Count Unicode scalar values in a terminated UTF-8 string, skipping a
 * leading BOM. This counts decoded codepoints, not grapheme clusters or bytes.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf8, 1 = null length.
 * After pointer validation, *length is zero until complete success.
 * ERROR with operation 1 and detail 0 = UTF-8 decoding failed; the underlying
 * decode result is not propagated. ERROR_BUFFER_OVERFLOW = count reaches
 * CATALYST_NUINT_MAX, with operation/detail zero.
 */
CATTELIB_API void catteUtf8Length(CATALYST_UTF8 utf8, CATALYST_NUINT* length, CATALYST_RESULT* result);

/**
 * Encode one Unicode scalar value into one or two UTF-16LE units.
 * U+0000 produces one zero unit. No additional terminator is written.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf16lew, 1 = null produced.
 * After pointer validation, *produced is zero until success. Encoding errors
 * leave the destination unchanged and use ERROR with detail 0:
 * operation 1 = codepoint above U+10FFFF; 2 = surrogate U+D800..U+DFFF.
 */
CATTELIB_API void catteUtf16leEncode(const CATALYST_CODEPOINT codepoint, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT* produced, CATALYST_RESULT* result);

/**
 * Decode one UTF-16LE scalar value, consuming one unit or a surrogate pair.
 * U+0000 consumes one unit; U+FEFF is decoded normally, including a BOM.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf16le, 1 = null codepoint,
 * 2 = null consumed. These errors leave output storage unchanged.
 * After pointer validation, codepoint and *consumed are zeroed. Decode errors
 * leave them zero and use ERROR with detail 0:
 * operation 1 = low surrogate without a preceding high surrogate;
 * 2 = high surrogate not followed by a low surrogate (including a terminator).
 */
CATTELIB_API void catteUtf16leDecode(CATALYST_UTF16LE utf16le, CATALYST_CODEPOINT* codepoint, CATALYST_NUINT* consumed, CATALYST_RESULT* result);

/**
 * Count Unicode scalar values in terminated UTF-16LE, skipping a leading
 * BOM. A surrogate pair counts as one; this is not a grapheme or byte count.
 * ERROR_INVALID_ARGUMENT: detail 0 = null utf16le, 1 = null length.
 * After pointer validation, *length is zero until complete success.
 * ERROR with operation 1 and detail 0 = UTF-16LE decoding failed; the underlying
 * decode result is not propagated. ERROR_BUFFER_OVERFLOW = count reaches
 * CATALYST_NUINT_MAX, with operation/detail zero.
 */
CATTELIB_API void catteUtf16leLength(CATALYST_UTF16LE utf16le, CATALYST_NUINT* length, CATALYST_RESULT* result);

/**
 * Convert a terminated ASCII string to terminated CP1252.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * No capacity parameter: destination must hold source length plus one unit.
 * ERROR_BUFFER_OVERFLOW: index reaches CATALYST_NUINT_MAX.
 * Source bytes above 7F (hex) become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteAsciiToCp1252(CATALYST_ASCII ascii, CATALYST_CP1252W cp1252w, CATALYST_RESULT* result);

/**
 * Convert a terminated ASCII string to terminated UTF-8.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * capacity is the number of writable UTF-8 units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * Source bytes above 7F (hex) become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteAsciiToUtf8(CATALYST_ASCII ascii, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated ASCII string to terminated UTF-16LE.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * capacity is the number of writable UTF-16LE units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * Source bytes above 7F (hex) become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteAsciiToUtf16le(CATALYST_ASCII ascii, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated CP1252 string to terminated ASCII.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * No capacity parameter: destination must hold source length plus one unit.
 * ERROR_BUFFER_OVERFLOW: index reaches CATALYST_NUINT_MAX.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteCp1252Decode failed;
 * operation 2 = catteAsciiEncode failed. detail contains its status byte.
 * Undefined CP1252 bytes become '?' with WARNING_PARTIAL.
 * Values unrepresentable in the destination become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteCp1252ToAscii(CATALYST_CP1252 cp1252, CATALYST_ASCIIW ascii, CATALYST_RESULT* result);

/**
 * Convert a terminated CP1252 string to terminated UTF-8.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * capacity is the number of writable UTF-8 units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteCp1252Decode failed;
 * operation 2 = catteUtf8Encode failed. detail contains its status byte.
 * Undefined CP1252 bytes become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteCp1252ToUtf8(CATALYST_CP1252 cp1252, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated CP1252 string to terminated UTF-16LE.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * capacity is the number of writable UTF-16LE units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteCp1252Decode failed;
 * operation 2 = catteUtf16leEncode failed. detail contains its status byte.
 * Undefined CP1252 bytes become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteCp1252ToUtf16le(CATALYST_CP1252 cp1252, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-8 string to terminated ASCII.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable ASCII units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf8Decode failed;
 * operation 2 = catteAsciiEncode failed. detail contains its status byte.
 * Values unrepresentable in the destination become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteUtf8ToAscii(CATALYST_UTF8 utf8, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-8 string to terminated CP1252.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable CP1252 units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf8Decode failed;
 * operation 2 = catteCp1252Encode failed. detail contains its status byte.
 * Values unrepresentable in the destination become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteUtf8ToCp1252(CATALYST_UTF8 utf8, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-8 string to terminated UTF-16LE.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable UTF-16LE units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf8Decode failed;
 * operation 2 = catteUtf16leEncode failed. detail contains its status byte.
 */
CATTELIB_API void catteUtf8ToUtf16le(CATALYST_UTF8 utf8, CATALYST_UTF16LEW utf16lew, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-16LE string to terminated ASCII.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable ASCII units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf16leDecode failed;
 * operation 2 = catteAsciiEncode failed. detail contains its status byte.
 * Values unrepresentable in the destination become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteUtf16leToAscii(CATALYST_UTF16LE utf16le, CATALYST_ASCIIW ascii, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-16LE string to terminated CP1252.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable CP1252 units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf16leDecode failed;
 * operation 2 = catteCp1252Encode failed. detail contains its status byte.
 * Values unrepresentable in the destination become '?' with WARNING_PARTIAL.
 */
CATTELIB_API void catteUtf16leToCp1252(CATALYST_UTF16LE utf16le, CATALYST_CP1252W cp1252w, CATALYST_NUINT capacity, CATALYST_RESULT* result);

/**
 * Convert a terminated UTF-16LE string to terminated UTF-8.
 * ERROR_INVALID_ARGUMENT: detail 0 = null source, 1 = null destination.
 * Skips the source leading BOM; no destination BOM is added.
 * capacity is the number of writable UTF-8 units, including the terminator.
 * ERROR_BUFFER_OVERFLOW: capacity is zero or cannot hold the next encoded
 * value plus a terminator. Previously written output is not rolled back.
 * ERROR_DEPENDENCY_FAILURE: operation 1 = catteUtf16leDecode failed;
 * operation 2 = catteUtf8Encode failed. detail contains its status byte.
 */
CATTELIB_API void catteUtf16leToUtf8(CATALYST_UTF16LE utf16le, CATALYST_UTF8W utf8w, CATALYST_NUINT capacity, CATALYST_RESULT* result);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTELIB_H */
