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

/** Opaque collection handles; internal representations are library-owned. */
typedef struct CATALYST_ARRAY CATALYST_ARRAY;
typedef struct CATALYST_SET CATALYST_SET;
typedef struct CATALYST_MAP CATALYST_MAP;
typedef struct CATALYST_STACK CATALYST_STACK;

/**
 * Collection operation conventions:
 * - result may be null. All other pointer arguments must be non-null and
 *   address live handles or readable/writable storage as appropriate.
 * - Sizes are in bytes and must be nonzero. Lengths, counts and indices
 *   count elements or associations; indices start at zero.
 * - Collections copy exactly the configured number of bytes. Referenced
 *   resources remain caller-owned. No semantic equality or deep copy is
 *   performed. Set elements and map keys compare every byte, including
 *   padding; callers supply consistently initialized representations.
 * - Input and output storage must not overlap. Internal storage addresses
 *   are not exposed and no typed alignment of stored bytes is promised.
 * - Create initializes its output handle to null before validation of sizes
 *   and allocation; an existing handle is replaced, not destroyed.
 * - Query outputs are assigned only on success. Recoverable mutation errors
 *   leave logical contents unchanged. Clear retains allocated capacity.
 * - Set and map enumeration order is unspecified and may change on mutation.
 * - Access to a collection during mutation or destruction must be externally
 *   synchronized. Foreign, stale, and incorrectly typed handles are outside
 *   the contract. Destroy consumes a valid handle even on cleanup failure.
 *
 * Results: SUCCESS, SUCCESS_NOOP, ERROR_NOT_FOUND, ERROR_BUFFER_OVERFLOW,
 * ERROR_INVALID_ARGUMENT (detail is the zero-based argument index, including
 * an out-of-bounds index), and ERROR_DEPENDENCY_FAILURE (detail is the
 * allocator status; operation 1 = allocate, 2 = grow, 3 = release).
 * A failed release during growth returns FATAL_INVARIANT_VIOLATION;
 * the collection may no longer be usable. context is zero; operation and
 * detail are zero unless specified above.
 */

/* -------------------------------------------------------------------------------------------------
 * C API :: ARRAY
 * ------------------------------------------------------------------------------------------------- */

/** Create a fixed-length array of zero-filled elements. A zero length is valid. */
CATRLLIB_API void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result);

/** Destroy the array. */
CATRLLIB_API void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result);

/** Copy the element at index into value. */
CATRLLIB_API void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);

/** Replace the element at index. */
CATRLLIB_API void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result);

/** Return the fixed number of elements. */
CATRLLIB_API void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result);


/* -------------------------------------------------------------------------------------------------
 * C API :: SET
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty set. Element equality is byte-for-byte. */
CATRLLIB_API void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result);

/** Destroy the set. */
CATRLLIB_API void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result);

/** Insert a copy of value. An existing equivalent element returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);

/** Remove an equivalent element. An absent element returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result);

/** Return whether an equivalent element exists. */
CATRLLIB_API void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result);

/** Return the number of distinct elements. */
CATRLLIB_API void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result);

/** Copy an element at an enumeration index. */
CATRLLIB_API void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result);

/** Remove all elements, retaining storage. An empty set returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result);


/* -------------------------------------------------------------------------------------------------
 * C API :: MAP
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty map. Key equality is byte-for-byte. */
CATRLLIB_API void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result);

/** Destroy the map. */
CATRLLIB_API void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result);

/** Copy the associated value. An absent key returns ERROR_NOT_FOUND. */
CATRLLIB_API void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result);

/** Insert copies of key and value or replace the value of an existing key. */
CATRLLIB_API void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result);

/** Remove an association. An absent key returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result);

/** Return whether an equivalent key exists. */
CATRLLIB_API void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result);

/** Return the number of associations. */
CATRLLIB_API void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result);

/** Copy the key and value at an enumeration index. */
CATRLLIB_API void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result);

/** Remove all associations, retaining storage. An empty map returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result);


/* -------------------------------------------------------------------------------------------------
 * C API :: STACK
 * ------------------------------------------------------------------------------------------------- */

/** Create an empty last-in, first-out collection. */
CATRLLIB_API void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result);

/** Destroy the stack. */
CATRLLIB_API void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result);

/** Push a copy of value onto the stack. */
CATRLLIB_API void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result);

/** Copy and remove the top element. An empty stack returns ERROR_NOT_FOUND. */
CATRLLIB_API void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);

/** Copy the top element. An empty stack returns ERROR_NOT_FOUND. */
CATRLLIB_API void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result);

/** Return the number of elements. */
CATRLLIB_API void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result);

/** Remove all elements, retaining storage. An empty stack returns SUCCESS_NOOP. */
CATRLLIB_API void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
