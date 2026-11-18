#ifndef CATRLLIB_H
#define CATRLLIB_H
#include "catalyst.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATRLLIB_EXPORTS)
        #define CATRLLIB_API __declspec(dllexport)
    #elif defined(CATRLLIB_USE_DLL)
        #define CATRLLIB_API __declspec(dllimport)
    #else
        #define CATRLLIB_API
    #endif
#else
    #define CATRLLIB_API
#endif

/** Fixed-length array of byte values. */
typedef struct CATALYST_ARRAY CATALYST_ARRAY;
/** Collection of distinct byte values. */
typedef struct CATALYST_SET CATALYST_SET;
/** Collection of byte-keyed value associations. */
typedef struct CATALYST_MAP CATALYST_MAP;
/** Last-in, first-out collection of byte values. */
typedef struct CATALYST_STACK CATALYST_STACK;

/* -------------------------------------------------------------------------------------------------
 * C API :: ARRAY
 * ------------------------------------------------------------------------------------------------- */

/** Create a fixed-length array of zero-filled elements; element_size must be nonzero. A zero length is valid. */
CATRLLIB_API CATALYST_RESULT catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length);

/** Destroy the array and release its storage. */
CATRLLIB_API CATALYST_RESULT catrl_array_destroy(CATALYST_ARRAY* array);

/** Copy the element at a valid index into value. */
CATRLLIB_API void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value);

/** Replace the element at a valid index. */
CATRLLIB_API void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value);

/** Return the fixed number of elements from a valid live handle. */
CATRLLIB_API CATALYST_NUINT catrl_array_length(const CATALYST_ARRAY* array);

/* -------------------------------------------------------------------------------------------------
 * C API :: SET
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty set with nonzero element_size. Element equality is byte-for-byte. */
CATRLLIB_API CATALYST_RESULT catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size);

/** Destroy the set and release its storage. */
CATRLLIB_API CATALYST_RESULT catrl_set_destroy(CATALYST_SET* set);

/** Insert a copy of value. An existing equivalent element returns SUCCESS_NOOP. */
CATRLLIB_API CATALYST_RESULT catrl_set_add(CATALYST_SET* set, const void* value);

/** Remove an equivalent element. Absence is a no-op. */
CATRLLIB_API void catrl_set_remove(CATALYST_SET* set, const void* value);

/** Return whether an equivalent element exists. */
CATRLLIB_API CATALYST_BOOL catrl_set_contains(const CATALYST_SET* set, const void* value);

/** Return the number of distinct elements from a valid live handle. */
CATRLLIB_API CATALYST_NUINT catrl_set_count(const CATALYST_SET* set);

/** Copy an element at a valid unordered enumeration index. */
CATRLLIB_API void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value);

/** Remove all elements, retaining storage. Empty collections are unchanged. */
CATRLLIB_API void catrl_set_clear(CATALYST_SET* set);

/* -------------------------------------------------------------------------------------------------
 * C API :: MAP
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty map with nonzero key_size and value_size. Key equality is byte-for-byte. */
CATRLLIB_API CATALYST_RESULT catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size);

/** Destroy the map and release its storage. */
CATRLLIB_API CATALYST_RESULT catrl_map_destroy(CATALYST_MAP* map);

/** Copy the associated value. An absent key returns ERROR_NOT_FOUND. */
CATRLLIB_API CATALYST_RESULT catrl_map_get(const CATALYST_MAP* map, const void* key, void* value);

/** Insert copies of key and value or replace the value of an existing key. */
CATRLLIB_API CATALYST_RESULT catrl_map_set(CATALYST_MAP* map, const void* key, const void* value);

/** Remove an association. Absence is a no-op. */
CATRLLIB_API void catrl_map_remove(CATALYST_MAP* map, const void* key);

/** Return whether an equivalent key exists. */
CATRLLIB_API CATALYST_BOOL catrl_map_contains(const CATALYST_MAP* map, const void* key);

/** Return the number of associations from a valid live handle. */
CATRLLIB_API CATALYST_NUINT catrl_map_count(const CATALYST_MAP* map);

/** Copy the key and value at a valid unordered enumeration index. */
CATRLLIB_API void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value);

/** Remove all associations, retaining storage. Empty collections are unchanged. */
CATRLLIB_API void catrl_map_clear(CATALYST_MAP* map);

/* -------------------------------------------------------------------------------------------------
 * C API :: STACK
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty last-in, first-out collection with nonzero element_size. */
CATRLLIB_API CATALYST_RESULT catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size);

/** Destroy the stack and release its storage. */
CATRLLIB_API CATALYST_RESULT catrl_stack_destroy(CATALYST_STACK* stack);

/** Push a copy of value onto the stack. */
CATRLLIB_API CATALYST_RESULT catrl_stack_push(CATALYST_STACK* stack, const void* value);

/** Copy and remove the top element. An empty stack returns ERROR_NOT_FOUND. */
CATRLLIB_API CATALYST_RESULT catrl_stack_pop(CATALYST_STACK* stack, void* value);

/** Copy the top element. An empty stack returns ERROR_NOT_FOUND. */
CATRLLIB_API CATALYST_RESULT catrl_stack_peek(const CATALYST_STACK* stack, void* value);

/** Return the number of elements from a valid live handle. */
CATRLLIB_API CATALYST_NUINT catrl_stack_count(const CATALYST_STACK* stack);

/** Remove all elements, retaining storage. Empty collections are unchanged. */
CATRLLIB_API void catrl_stack_clear(CATALYST_STACK* stack);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
