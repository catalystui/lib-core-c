#ifndef CATTRLIB_H
#define CATTRLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATTRLIB_EXPORTS)
        #define CATTRLIB_API __declspec(dllexport)
    #elif defined(CATTRLIB_USE_DLL)
        #define CATTRLIB_API __declspec(dllimport)
    #else
        #define CATTRLIB_API
    #endif
#else
    #define CATTRLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: THREADING STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------
 * C API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTRLIB_H */
