#ifndef CATALYST_C90_H
#define CATALYST_C90_H

#include <limits.h>
#include "catalystcfg.h"

/* -------------------------------------------------------------------------------------------------
 * C API :: NUMERICS
 * ------------------------------------------------------------------------------------------------- */

/* CHAR_BIT */
#if (CHAR_BIT != 8)
    #error "catalyst.h :: Unsupported Platform :: Platform must contain 8-bit bytes (CHAR_BIT == 8)."
#endif

/* BYTE, SBYTE (8-bit types) */
#if (SCHAR_MIN == -128 && SCHAR_MAX == 127 && UCHAR_MAX == 255U)
    typedef unsigned char CATALYST_BYTE;
    typedef signed char CATALYST_SBYTE;

    #define CATALYST_BYTE_MIN  ((CATALYST_BYTE) 0)
    #define CATALYST_BYTE_MAX  ((CATALYST_BYTE) UCHAR_MAX)
    #define CATALYST_SBYTE_MIN ((CATALYST_SBYTE) SCHAR_MIN)
    #define CATALYST_SBYTE_MAX ((CATALYST_SBYTE) SCHAR_MAX)
#else
    #error "catalyst.h :: Unsupported Platform :: Platform must provide fixed-width 8-bit integer types for BYTE and SBYTE."
#endif

/* SHORT, USHORT (16-bit types) */
#if (CATALYST_TARGET_SIZEOF_SHORT == 2 && SHRT_MAX == 0x7FFF && USHRT_MAX == 0xFFFFU && SHRT_MIN + SHRT_MAX == -1)
    #define CATALYST_SUPPORTS_16BIT 1
    typedef signed short CATALYST_SHORT;
    typedef unsigned short CATALYST_USHORT;
    #define CATALYST_SHORT_MIN ((CATALYST_SHORT) SHRT_MIN)
    #define CATALYST_SHORT_MAX ((CATALYST_SHORT) SHRT_MAX)
    #define CATALYST_USHORT_MIN ((CATALYST_USHORT) 0)
    #define CATALYST_USHORT_MAX ((CATALYST_USHORT) USHRT_MAX)
#elif (CATALYST_TARGET_SIZEOF_INT == 2 && INT_MAX == 0x7FFF && UINT_MAX == 0xFFFFU && INT_MIN + INT_MAX == -1)
    #define CATALYST_SUPPORTS_16BIT 1
    typedef signed int CATALYST_SHORT;
    typedef unsigned int CATALYST_USHORT;
    #define CATALYST_SHORT_MIN ((CATALYST_SHORT) INT_MIN)
    #define CATALYST_SHORT_MAX ((CATALYST_SHORT) INT_MAX)
    #define CATALYST_USHORT_MIN ((CATALYST_USHORT) 0)
    #define CATALYST_USHORT_MAX ((CATALYST_USHORT) UINT_MAX)
#elif (CATALYST_TARGET_SIZEOF_LONG == 2 && LONG_MAX == 0x7FFF && ULONG_MAX == 0xFFFFU && LONG_MIN + LONG_MAX == -1)
    #define CATALYST_SUPPORTS_16BIT 1
    typedef signed long CATALYST_SHORT;
    typedef unsigned long CATALYST_USHORT;
    #define CATALYST_SHORT_MIN ((CATALYST_SHORT) LONG_MIN)
    #define CATALYST_SHORT_MAX ((CATALYST_SHORT) LONG_MAX)
    #define CATALYST_USHORT_MIN ((CATALYST_USHORT) 0)
    #define CATALYST_USHORT_MAX ((CATALYST_USHORT) ULONG_MAX)
#else
    #define CATALYST_SUPPORTS_16BIT 0
#endif
#if CATALYST_SUPPORTS_16BIT
    typedef char CATALYST_requires_16bit[(sizeof(CATALYST_SHORT) == 2 && sizeof(CATALYST_USHORT) == 2) ? 1 : -1];
#endif

/* INT, UINT (32-bit types) */
#if (CATALYST_TARGET_SIZEOF_SHORT == 4 && SHRT_MAX == 0x7FFFFFFFL && USHRT_MAX == 0xFFFFFFFFUL && SHRT_MIN + SHRT_MAX == -1)
    #define CATALYST_SUPPORTS_32BIT 1
    typedef signed short CATALYST_INT;
    typedef unsigned short CATALYST_UINT;
    #define CATALYST_INT_MIN ((CATALYST_INT) SHRT_MIN)
    #define CATALYST_INT_MAX ((CATALYST_INT) SHRT_MAX)
    #define CATALYST_UINT_MIN ((CATALYST_UINT) 0)
    #define CATALYST_UINT_MAX ((CATALYST_UINT) USHRT_MAX)
#elif (CATALYST_TARGET_SIZEOF_INT == 4 && INT_MAX == 0x7FFFFFFFL && UINT_MAX == 0xFFFFFFFFUL && INT_MIN + INT_MAX == -1)
    #define CATALYST_SUPPORTS_32BIT 1
    typedef signed int CATALYST_INT;
    typedef unsigned int CATALYST_UINT;
    #define CATALYST_INT_MIN ((CATALYST_INT) INT_MIN)
    #define CATALYST_INT_MAX ((CATALYST_INT) INT_MAX)
    #define CATALYST_UINT_MIN ((CATALYST_UINT) 0)
    #define CATALYST_UINT_MAX ((CATALYST_UINT) UINT_MAX)
#elif (CATALYST_TARGET_SIZEOF_LONG == 4 && LONG_MAX == 0x7FFFFFFFL && ULONG_MAX == 0xFFFFFFFFUL && LONG_MIN + LONG_MAX == -1)
    #define CATALYST_SUPPORTS_32BIT 1
    typedef signed long CATALYST_INT;
    typedef unsigned long CATALYST_UINT;
    #define CATALYST_INT_MIN ((CATALYST_INT) LONG_MIN)
    #define CATALYST_INT_MAX ((CATALYST_INT) LONG_MAX)
    #define CATALYST_UINT_MIN ((CATALYST_UINT) 0)
    #define CATALYST_UINT_MAX ((CATALYST_UINT) ULONG_MAX)
#else
    #define CATALYST_SUPPORTS_32BIT 0
#endif
#if CATALYST_SUPPORTS_32BIT
    typedef char CATALYST_requires_32bit[(sizeof(CATALYST_INT) == 4 && sizeof(CATALYST_UINT) == 4) ? 1 : -1];
#endif

/* LONG, ULONG (64-bit types) */
#if (CATALYST_TARGET_SIZEOF_SHORT == 8 && ((SHRT_MAX >> 31) >> 31) == 1L && ((USHRT_MAX >> 31) >> 31) == 3UL && SHRT_MIN + SHRT_MAX == -1)
    #define CATALYST_SUPPORTS_64BIT 1
    typedef signed short CATALYST_LONG;
    typedef unsigned short CATALYST_ULONG;
    #define CATALYST_LONG_MIN ((CATALYST_LONG) SHRT_MIN)
    #define CATALYST_LONG_MAX ((CATALYST_LONG) SHRT_MAX)
    #define CATALYST_ULONG_MIN ((CATALYST_ULONG) 0)
    #define CATALYST_ULONG_MAX ((CATALYST_ULONG) USHRT_MAX)
#elif (CATALYST_TARGET_SIZEOF_INT == 8 && ((INT_MAX >> 31) >> 31) == 1L && ((UINT_MAX >> 31) >> 31) == 3UL && INT_MIN + INT_MAX == -1)
    #define CATALYST_SUPPORTS_64BIT 1
    typedef signed int CATALYST_LONG;
    typedef unsigned int CATALYST_ULONG;
    #define CATALYST_LONG_MIN ((CATALYST_LONG) INT_MIN)
    #define CATALYST_LONG_MAX ((CATALYST_LONG) INT_MAX)
    #define CATALYST_ULONG_MIN ((CATALYST_ULONG) 0)
    #define CATALYST_ULONG_MAX ((CATALYST_ULONG) UINT_MAX)
#elif (CATALYST_TARGET_SIZEOF_LONG == 8 && ((LONG_MAX >> 31) >> 31) == 1L && ((ULONG_MAX >> 31) >> 31) == 3UL && LONG_MIN + LONG_MAX == -1)
    #define CATALYST_SUPPORTS_64BIT 1
    typedef signed long CATALYST_LONG;
    typedef unsigned long CATALYST_ULONG;
    #define CATALYST_LONG_MIN ((CATALYST_LONG) LONG_MIN)
    #define CATALYST_LONG_MAX ((CATALYST_LONG) LONG_MAX)
    #define CATALYST_ULONG_MIN ((CATALYST_ULONG) 0)
    #define CATALYST_ULONG_MAX ((CATALYST_ULONG) ULONG_MAX)
#else
    #define CATALYST_SUPPORTS_64BIT 0
#endif
#if CATALYST_SUPPORTS_64BIT
    typedef char CATALYST_requires_64bit[(sizeof(CATALYST_LONG) == 8 && sizeof(CATALYST_ULONG) == 8) ? 1 : -1];
#endif

#endif /* CATALYST_C90_H */
