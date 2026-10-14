#ifndef CATALYST_H
#define CATALYST_H

/**
 * Foundational C representations for CatalystUI (FDEFSPEC).
 * cmakecfg.h supplies target capabilities from the build configuration.
 * TARGET_STDC_VERSION selects the C99 fixed-width types or the C90 fallback;
 * C++ inclusion uses the same configured representation. Optional numeric
 * types exist only when their CATALYST_SUPPORTS_* macro is nonzero.
 * These are value representations, not runtime-validating containers.
 */
#include "cmakecfg.h"
#if defined(TARGET_STDC_VERSION) && (TARGET_STDC_VERSION >= 199901L)
#include "catalyst.c99.h"
#else
#include "catalyst.c90.h"
#endif

#include <float.h>

/* CONSTEXPR Support */
#if defined(__cplusplus)
    #if __cplusplus >= 201103L
        #define CATALYST_CONSTEXPR constexpr
    #else
        #define CATALYST_CONSTEXPR
    #endif
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: NUMERICS
 * ------------------------------------------------------------------------------------------------- */

#if (FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MIN_EXP == -125 && FLT_MAX_EXP == 128)
    #define CATALYST_SUPPORTS_SINGLE 1
    typedef char CATALYST_requires_float_4_bytes[(sizeof(float) == 4) ? 1 : -1]; /* Compile-time check for 32-bit float */
    typedef float CATALYST_SINGLE;
#else
    #define CATALYST_SUPPORTS_SINGLE 0
#endif

#if (FLT_RADIX == 2 && DBL_MANT_DIG == 53 && DBL_MIN_EXP == -1021 && DBL_MAX_EXP == 1024)
    #define CATALYST_SUPPORTS_DOUBLE 1
    typedef char CATALYST_requires_double_8_bytes[(sizeof(double) == 8) ? 1 : -1]; /* Compile-time check for 64-bit double */
    typedef double CATALYST_DOUBLE;
#else
    #define CATALYST_SUPPORTS_DOUBLE 0
#endif

/**
 * Boolean values use one byte: 0 is false and 1 is true.
 * The typedef does not restrict other byte values; callers supply 0 or 1.
 */
typedef CATALYST_BYTE CATALYST_BOOL;
#define CATALYST_TRUE    ((CATALYST_BOOL) 1)
#define CATALYST_FALSE   ((CATALYST_BOOL) 0)

/**
 * NINT/NUINT select signed/unsigned integer types with the configured pointer
 * width. They represent native-sized counts and sizes; they do not encode or
 * normalize pointers, particularly on segmented targets.
 */
#ifndef TARGET_SIZEOF_VOID_P
    #error "catalyst.h :: Unsupported Platform :: Unable to determine pointer size, TARGET_SIZEOF_VOID_P was not defined."
#endif
#if TARGET_SIZEOF_VOID_P == 1
    typedef CATALYST_SBYTE  CATALYST_NINT;
    typedef CATALYST_BYTE   CATALYST_NUINT;

    #define CATALYST_NINT_MIN  CATALYST_SBYTE_MIN
    #define CATALYST_NINT_MAX  CATALYST_SBYTE_MAX
    #define CATALYST_NUINT_MIN CATALYST_BYTE_MIN
    #define CATALYST_NUINT_MAX CATALYST_BYTE_MAX
#elif TARGET_SIZEOF_VOID_P == 2
    #if CATALYST_SUPPORTS_16BIT
        typedef CATALYST_SHORT  CATALYST_NINT;
        typedef CATALYST_USHORT CATALYST_NUINT;

        #define CATALYST_NINT_MIN  CATALYST_SHORT_MIN
        #define CATALYST_NINT_MAX  CATALYST_SHORT_MAX
        #define CATALYST_NUINT_MIN CATALYST_USHORT_MIN
        #define CATALYST_NUINT_MAX CATALYST_USHORT_MAX
    #else
        #error "catalyst.h :: Unsupported Platform :: Compiler specifies 16-bit pointers but no 16-bit integer type exists."
    #endif
#elif TARGET_SIZEOF_VOID_P == 4
    #if CATALYST_SUPPORTS_32BIT
        typedef CATALYST_INT    CATALYST_NINT;
        typedef CATALYST_UINT   CATALYST_NUINT;

        #define CATALYST_NINT_MIN  CATALYST_INT_MIN
        #define CATALYST_NINT_MAX  CATALYST_INT_MAX
        #define CATALYST_NUINT_MIN CATALYST_UINT_MIN
        #define CATALYST_NUINT_MAX CATALYST_UINT_MAX
    #else
        #error "catalyst.h :: Unsupported Platform :: Compiler specifies 32-bit pointers but no 32-bit integer type exists."
    #endif
#elif TARGET_SIZEOF_VOID_P == 8
    #if CATALYST_SUPPORTS_64BIT
        typedef CATALYST_LONG   CATALYST_NINT;
        typedef CATALYST_ULONG  CATALYST_NUINT;

        #define CATALYST_NINT_MIN  CATALYST_LONG_MIN
        #define CATALYST_NINT_MAX  CATALYST_LONG_MAX
        #define CATALYST_NUINT_MIN CATALYST_ULONG_MIN
        #define CATALYST_NUINT_MAX CATALYST_ULONG_MAX
    #else
        #error "catalyst.h :: Unsupported Platform :: Compiler specifies 64-bit pointers but no 64-bit integer type exists."
    #endif
#else
    #error "catalyst.h :: Unsupported Platform :: Unsupported pointer size specified by compiler."
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: TEXT ENCODING
 * ------------------------------------------------------------------------------------------------- */

typedef CATALYST_BYTE CATALYST_TEXT_ENCODING;

enum {
    CATALYST_TEXT_ENCODING_UNKNOWN = 0x0,
    CATALYST_TEXT_ENCODING_ASCII   = 0x1,
    CATALYST_TEXT_ENCODING_CP1252  = 0x2,
    CATALYST_TEXT_ENCODING_UTF8    = 0x3,
    CATALYST_TEXT_ENCODING_UTF16LE = 0x4
};

/**
 * Four-byte, most-significant-byte-first codepoint representation, independent
 * of host integer byte order. Unicode codepoints range through U+10FFFF;
 * UTF encoders accept scalar values and reject U+D800..U+DFFF surrogates.
 * The representation itself can hold other values without validating them.
 */
typedef struct CATALYST_CODEPOINT {
    CATALYST_BYTE byte0; /* Most significant byte */
    CATALYST_BYTE byte1;
    CATALYST_BYTE byte2;
    CATALYST_BYTE byte3; /* Least significant byte */
} CATALYST_CODEPOINT;

/**
 * Encoded string pointers address null-terminated code-unit sequences.
 * The W suffix denotes writable storage; unsuffixed pointers are read-only.
 * ASCII uses byte storage for 7-bit values. CP1252 and UTF-8 use byte units.
 * UTF-16LE stores each unit explicitly as low byte followed by high byte.
 * The pointer aliases carry no ownership, length, or capacity information.
 */
typedef CATALYST_BYTE CATALYST_ASCII_CODE_UNIT;
typedef CATALYST_ASCII_CODE_UNIT* CATALYST_ASCIIW;
typedef const CATALYST_ASCII_CODE_UNIT* CATALYST_ASCII;

typedef CATALYST_BYTE CATALYST_CP1252_CODE_UNIT;
typedef CATALYST_CP1252_CODE_UNIT* CATALYST_CP1252W;
typedef const CATALYST_CP1252_CODE_UNIT* CATALYST_CP1252;

typedef CATALYST_BYTE CATALYST_UTF8_CODE_UNIT;
typedef CATALYST_UTF8_CODE_UNIT* CATALYST_UTF8W;
typedef const CATALYST_UTF8_CODE_UNIT* CATALYST_UTF8;

typedef struct CATALYST_UTF16LE_CODE_UNIT {
    CATALYST_BYTE lsb; /* Low byte of the UTF-16 code unit; first byte in UTF-16LE storage. */
    CATALYST_BYTE msb; /* High byte of the UTF-16 code unit; second byte in UTF-16LE storage. */
} CATALYST_UTF16LE_CODE_UNIT;
typedef CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LEW;
typedef const CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LE;

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATION STATUS
 * ------------------------------------------------------------------------------------------------- */

/**
 * Status is an eight-bit FDEFSPEC code. Its upper two bits select severity:
 * 00 = success, 01 = warning, 10 = recoverable error, 11 = fatal error.
 * Unlisted values are reserved; severity predicates classify those values too.
 * WARNING_PARTIAL means completion with incomplete or substituted output;
 * ERROR_DEPENDENCY_FAILURE identifies a failed dependency through detail.
 */
typedef CATALYST_BYTE CATALYST_STATUS_CODE;

enum {
    /* Operation completed successfully. */
    CATALYST_STATUS_CODE_SUCCESS      = 0x00,
    /* Operation succeeded without performing an action. */
    CATALYST_STATUS_CODE_SUCCESS_NOOP = 0x01,

    /* Operation completed with an unexpected or undesirable result. */
    CATALYST_STATUS_CODE_WARNING            = 0x40,
    /* Operation completed with incomplete or substituted output. */
    CATALYST_STATUS_CODE_WARNING_PARTIAL    = 0x41,
    /* Operation used behavior that may be removed in the future. */
    CATALYST_STATUS_CODE_WARNING_DEPRECATED = 0x42,

    /* Operation failed with a recoverable error. */
    CATALYST_STATUS_CODE_ERROR                      = 0x80,
    /* Invalid argument; detail identifies the argument as documented by the API. */
    CATALYST_STATUS_CODE_ERROR_INVALID_ARGUMENT     = 0x81,
    /* Inconsistent, corrupted, or invalid state. */
    CATALYST_STATUS_CODE_ERROR_INVALID_STATE        = 0x82,
    /* Required input contains malformed or invalid data. */
    CATALYST_STATUS_CODE_ERROR_MALFORMED_INPUT      = 0x83,
    /* A required resource denied access. */
    CATALYST_STATUS_CODE_ERROR_ACCESS_DENIED        = 0x84,
    /* Requested behavior has not been implemented. */
    CATALYST_STATUS_CODE_ERROR_NOT_IMPLEMENTED      = 0x85,
    /* Target system/configuration does not support the request. */
    CATALYST_STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED = 0x86,
    /* A required timeout occurred before completion. */
    CATALYST_STATUS_CODE_ERROR_TIMEOUT              = 0x87,
    /* A required resource does not exist. */
    CATALYST_STATUS_CODE_ERROR_NOT_FOUND            = 0x88,
    /* An external event interrupted the operation. */
    CATALYST_STATUS_CODE_ERROR_INTERRUPTED          = 0x89,
    /* A dependency failed; detail contains its status byte. */
    CATALYST_STATUS_CODE_ERROR_DEPENDENCY_FAILURE   = 0x8A,
    /* Required data exceeds the available buffer or representable extent. */
    CATALYST_STATUS_CODE_ERROR_BUFFER_OVERFLOW      = 0x90,
    /* Memory allocation was unsuccessful. */
    CATALYST_STATUS_CODE_ERROR_ALLOCATION_FAILED    = 0x91,
    /* An input/output error occurred. */
    CATALYST_STATUS_CODE_ERROR_IO_ERROR             = 0xA0,

    /* Operation failed with an unrecoverable error. */
    CATALYST_STATUS_CODE_FATAL                      = 0xC0,
    /* A fundamental invariant was violated. */
    CATALYST_STATUS_CODE_FATAL_INVARIANT_VIOLATION  = 0xC1
};

#define CATALYST_STATUS_CODE_LEVEL_MASK ((CATALYST_STATUS_CODE) 0xC0)

#define CATALYST_STATUS_CODE_IS_SUCCESS(status) ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_SUCCESS)
#define CATALYST_STATUS_CODE_IS_WARNING(status) ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_WARNING)
#define CATALYST_STATUS_CODE_IS_ERROR(status) ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_ERROR)
#define CATALYST_STATUS_CODE_IS_FATAL(status) ((((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK) == CATALYST_STATUS_CODE_FATAL)

/**
 * Result metadata uses byte-sized codes. context must be zero on success;
 * otherwise its meaning is operation-defined. operation identifies a
 * function-specific operation or failure stage. detail is interpreted by
 * status first, then context, then operation; undocumented values are unknown.
 * FDEFSPEC uses INVALID_ARGUMENT detail for an argument index and
 * DEPENDENCY_FAILURE detail for the dependency's status. Consult the API's
 * per-function documentation for actual detail values and output behavior.
 */
typedef CATALYST_BYTE CATALYST_CONTEXT_CODE;
typedef CATALYST_BYTE CATALYST_OPERATION_CODE;
typedef CATALYST_BYTE CATALYST_DETAIL_CODE;

/**
 * Four ordered result bytes: status, context, operation, detail.
 * These field names define the C representation; there is no host-endian
 * integer cast contract. The C++ RESULT helper explicitly packs/unpacks a
 * numeric 0xSSCCOODD value when a 32-bit unsigned type is available.
 * Automatic C aggregate storage must be initialized by the caller.
 */
typedef struct CATALYST_RESULT {
    CATALYST_STATUS_CODE status;
    CATALYST_CONTEXT_CODE context;
    CATALYST_OPERATION_CODE operation;
    CATALYST_DETAIL_CODE detail;
} CATALYST_RESULT;

#endif /* CATALYST_H */
