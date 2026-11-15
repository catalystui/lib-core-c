#ifndef CATALYST_H
#define CATALYST_H

#include "cmakecfg.h"
#if defined(TARGET_STDC_VERSION) && (TARGET_STDC_VERSION >= 199901L)
#include "catalyst.c99.h"
#else
#include "catalyst.c90.h"
#endif

#include <float.h>

/* Header-only C conversion helpers, including the configured C90 fallback. */
#if defined(__cplusplus)
    #define CATALYST_CORE_INLINE inline
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define CATALYST_CORE_INLINE static inline
#elif defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
    #define CATALYST_CORE_INLINE static __inline
#else
    #define CATALYST_CORE_INLINE static
#endif

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

/** Byte representations of a bit (0–1) and nibble (0–15). */
typedef CATALYST_BYTE CATALYST_BIT;
typedef CATALYST_BYTE CATALYST_NIBBLE;

#define CATALYST_BIT_MIN    ((CATALYST_BIT) 0)
#define CATALYST_BIT_MAX    ((CATALYST_BIT) 1)
#define CATALYST_NIBBLE_MIN ((CATALYST_NIBBLE) 0)
#define CATALYST_NIBBLE_MAX ((CATALYST_NIBBLE) 15)

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

/** Boolean byte representation: 0 = false, 1 = true. */
typedef CATALYST_BYTE CATALYST_BOOL;
#define CATALYST_TRUE    ((CATALYST_BOOL) 1)
#define CATALYST_FALSE   ((CATALYST_BOOL) 0)

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

/** Four-byte codepoint representation in most-significant-byte-first order. */
typedef struct CATALYST_CODEPOINT {
    CATALYST_BYTE byte0; /* Most significant byte */
    CATALYST_BYTE byte1;
    CATALYST_BYTE byte2;
    CATALYST_BYTE byte3; /* Least significant byte */
} CATALYST_CODEPOINT;

#if CATALYST_SUPPORTS_32BIT
/** Convert between the four stored bytes and a numeric value without validation. */
CATALYST_CORE_INLINE CATALYST_CODEPOINT cat_codepoint_from_uint(CATALYST_UINT value) {
    CATALYST_CODEPOINT codepoint;
    codepoint.byte0 = (CATALYST_BYTE) (value >> 24);
    codepoint.byte1 = (CATALYST_BYTE) (value >> 16);
    codepoint.byte2 = (CATALYST_BYTE) (value >> 8);
    codepoint.byte3 = (CATALYST_BYTE) value;
    return codepoint;
}

CATALYST_CORE_INLINE CATALYST_UINT cat_codepoint_to_uint(CATALYST_CODEPOINT codepoint) {
    return ((CATALYST_UINT) codepoint.byte0 << 24) | ((CATALYST_UINT) codepoint.byte1 << 16) |
           ((CATALYST_UINT) codepoint.byte2 << 8) | (CATALYST_UINT) codepoint.byte3;
}
#endif

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

#if CATALYST_SUPPORTS_16BIT
/** Convert between a UTF-16LE code unit and its numeric value, independent of host byte order. */
CATALYST_CORE_INLINE CATALYST_UTF16LE_CODE_UNIT cat_utf16le_code_unit_from_ushort(CATALYST_USHORT value) {
    CATALYST_UTF16LE_CODE_UNIT codeunit;
    codeunit.lsb = (CATALYST_BYTE) value;
    codeunit.msb = (CATALYST_BYTE) (value >> 8);
    return codeunit;
}

CATALYST_CORE_INLINE CATALYST_USHORT cat_utf16le_code_unit_to_ushort(CATALYST_UTF16LE_CODE_UNIT codeunit) {
    return (CATALYST_USHORT) (((CATALYST_USHORT) codeunit.msb << 8) | (CATALYST_USHORT) codeunit.lsb);
}
#endif
typedef CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LEW;
typedef const CATALYST_UTF16LE_CODE_UNIT* CATALYST_UTF16LE;

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATION STATUS
 * ------------------------------------------------------------------------------------------------- */

/** Eight-bit operation status; the upper two bits identify severity. */
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

/** The four FDEFSPEC severity levels, represented by the upper two status bits. */
typedef CATALYST_BYTE CATALYST_STATUS_CODE_LEVEL;

enum {
    CATALYST_STATUS_CODE_LEVEL_SUCCESS = 0x00,
    CATALYST_STATUS_CODE_LEVEL_WARNING = 0x40,
    CATALYST_STATUS_CODE_LEVEL_ERROR   = 0x80,
    CATALYST_STATUS_CODE_LEVEL_FATAL   = 0xC0
};

#define CATALYST_STATUS_CODE_LEVEL_MASK ((CATALYST_STATUS_CODE) 0xC0)
#define CATALYST_STATUS_CODE_GET_LEVEL(status) ((CATALYST_STATUS_CODE_LEVEL) (((CATALYST_STATUS_CODE) (status)) & CATALYST_STATUS_CODE_LEVEL_MASK))

#define CATALYST_STATUS_CODE_IS_SUCCESS(status) (CATALYST_STATUS_CODE_GET_LEVEL(status) == CATALYST_STATUS_CODE_LEVEL_SUCCESS)
#define CATALYST_STATUS_CODE_IS_WARNING(status) (CATALYST_STATUS_CODE_GET_LEVEL(status) == CATALYST_STATUS_CODE_LEVEL_WARNING)
#define CATALYST_STATUS_CODE_IS_ERROR(status) (CATALYST_STATUS_CODE_GET_LEVEL(status) == CATALYST_STATUS_CODE_LEVEL_ERROR)
#define CATALYST_STATUS_CODE_IS_FATAL(status) (CATALYST_STATUS_CODE_GET_LEVEL(status) == CATALYST_STATUS_CODE_LEVEL_FATAL)

/** Operation-defined result context. */
typedef CATALYST_BYTE CATALYST_CONTEXT_CODE;
/** Function-specific operation or failure stage. */
typedef CATALYST_BYTE CATALYST_OPERATION_CODE;
/** Status-specific result detail. */
typedef CATALYST_BYTE CATALYST_DETAIL_CODE;

/** Ordered result bytes: status, context, operation, and detail. */
typedef struct CATALYST_RESULT {
    CATALYST_STATUS_CODE status;
    CATALYST_CONTEXT_CODE context;
    CATALYST_OPERATION_CODE operation;
    CATALYST_DETAIL_CODE detail;
} CATALYST_RESULT;

#if CATALYST_SUPPORTS_32BIT
/** Convert between ordered result fields and numeric 0xSSCCOODD without validation. */
CATALYST_CORE_INLINE CATALYST_RESULT cat_result_from_uint(CATALYST_UINT value) {
    CATALYST_RESULT result;
    result.status = (CATALYST_STATUS_CODE) (value >> 24);
    result.context = (CATALYST_CONTEXT_CODE) (value >> 16);
    result.operation = (CATALYST_OPERATION_CODE) (value >> 8);
    result.detail = (CATALYST_DETAIL_CODE) value;
    return result;
}

CATALYST_CORE_INLINE CATALYST_UINT cat_result_to_uint(CATALYST_RESULT result) {
    return ((CATALYST_UINT) result.status << 24) | ((CATALYST_UINT) result.context << 16) |
           ((CATALYST_UINT) result.operation << 8) | (CATALYST_UINT) result.detail;
}
#endif

#undef CATALYST_CORE_INLINE

#endif /* CATALYST_H */
