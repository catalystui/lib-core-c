#ifndef CATRLLIB_H
#define CATRLLIB_H
#include "catalyst.h"
#include "catmmlib.h"
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

/* -------------------------------------------------------------------------------------------------
 * C API :: COLLECTION STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A finite, unordered collection of distinct, fixed-size elements stored contiguously. */
typedef struct CATRLLIB_SET {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_SET;

/** A finite collection of distinct, fixed-size keys, each associated with a fixed-size value, stored contiguously. */
typedef struct CATRLLIB_MAP {
    CATMMLIB_POINTERW keys;
    CATMMLIB_POINTERW values;
    CATALYST_NUINT ksize;
    CATALYST_NUINT vsize;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_MAP;

/** A finite, indexed collection of fixed-size elements stored contiguously. */
typedef struct CATRLLIB_ARRAY {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
} CATRLLIB_ARRAY;

/** A collection of data interpreted through metadata and a resolver function. */
typedef struct CATRLLIB_FILE CATRLLIB_FILE;
struct CATRLLIB_FILE {
    CATRLLIB_ARRAY metadata;
    CATRLLIB_ARRAY data;
    CATALYST_RESULT (*resolver)(
        const CATRLLIB_FILE* file,
        CATALYST_NUINT index,
        CATRLLIB_ARRAY* output
    );
};

/** Stream representation with managed producer state and chunk storage. */
typedef struct CATRLLIB_STREAM CATRLLIB_STREAM;
struct CATRLLIB_STREAM {
    CATMMLIB_POINTERW state;
    CATALYST_NUINT ssize;
    CATALYST_RESULT (*next)(
        CATRLLIB_STREAM* stream,
        CATMMLIB_POINTERW chunk,
        CATALYST_NUINT csize
    );
};

/* -------------------------------------------------------------------------------------------------
 * C API :: SET OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty set with elements of the specified size in bytes. */
CATRLLIB_API void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size);

/** Add an element to the set if it is not already present. Requires sufficient capacity. */
CATRLLIB_API CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Remove an element from the set if present. */
CATRLLIB_API CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Determine whether an element belongs to the set. */
CATRLLIB_API CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Remove all elements from the set without changing its capacity. */
CATRLLIB_API void catrl_set_clear(CATRLLIB_SET* set);

/** Increase the set's capacity to accommodate the specified number of elements. */
CATRLLIB_API CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity);

/** Reduce the set's capacity to accommodate its existing elements. */
CATRLLIB_API CATALYST_RESULT catrl_set_shrink(CATRLLIB_SET* set);

/** Release the resources owned by the set. */
CATRLLIB_API CATALYST_RESULT catrl_set_free(CATRLLIB_SET* set);

/* -------------------------------------------------------------------------------------------------
 * C API :: MAP OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty map with keys and values of the specified sizes in bytes. */
CATRLLIB_API void catrl_map_init(CATRLLIB_MAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize);

/** Add a key-value association if the key is not already present. Requires sufficient capacity. */
CATRLLIB_API CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);

/** Remove a key and its associated value if present. */
CATRLLIB_API CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key);

/** Determine whether a key belongs to the map. */
CATRLLIB_API CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key);

/** Retrieve the value associated with a key if present. */
CATRLLIB_API CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value);

/** Update the value associated with an existing key if present. */
CATRLLIB_API CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);

/** Remove all associations from the map without changing its capacity. */
CATRLLIB_API void catrl_map_clear(CATRLLIB_MAP* map);

/** Increase the map's capacity to accommodate the specified number of associations. */
CATRLLIB_API CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity);

/** Reduce the map's capacity to accommodate its existing associations. */
CATRLLIB_API CATALYST_RESULT catrl_map_shrink(CATRLLIB_MAP* map);

/** Release the resources owned by the map. */
CATRLLIB_API CATALYST_RESULT catrl_map_free(CATRLLIB_MAP* map);

/* -------------------------------------------------------------------------------------------------
 * C API :: ARRAY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty array with elements of the specified size in bytes. */
CATRLLIB_API void catrl_array_init(CATRLLIB_ARRAY* array, CATALYST_NUINT size);

/** Append an element to the array. Requires sufficient capacity. */
CATRLLIB_API void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);

/** Insert an element at the specified index. Requires sufficient capacity. */
CATRLLIB_API void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);

/** Remove the element at the specified index. */
CATRLLIB_API void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index);

/** Determine whether an element belongs to the array. */
CATRLLIB_API CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);

/** Retrieve the element at the specified index. */
CATRLLIB_API void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element);

/** Replace the element at the specified index. */
CATRLLIB_API void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);

/** Remove all elements from the array without changing its capacity. */
CATRLLIB_API void catrl_array_clear(CATRLLIB_ARRAY* array);

/** Increase the array's capacity to accommodate the specified number of elements. */
CATRLLIB_API CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity);

/** Reduce the array's capacity to accommodate its existing elements. */
CATRLLIB_API CATALYST_RESULT catrl_array_shrink(CATRLLIB_ARRAY* array);

/** Release the resources owned by the array. */
CATRLLIB_API CATALYST_RESULT catrl_array_free(CATRLLIB_ARRAY* array);

/* -------------------------------------------------------------------------------------------------
 * C API :: FILE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Initialize a file with metadata, data, and a resolver. */
CATRLLIB_API void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data, CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*));

/* Resolve the data associated with a file index. */
CATRLLIB_API CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output);

/* Release resources owned by the file. */
CATRLLIB_API CATALYST_RESULT catrl_file_free(CATRLLIB_FILE* file);

/* -------------------------------------------------------------------------------------------------
 * C API :: STREAM OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize a stream with ssize bytes of state and a nonzero chunk lifetime. */
CATRLLIB_API CATALYST_RESULT catrl_stream_init(CATRLLIB_STREAM* stream, CATALYST_NUINT ssize, CATALYST_NUINT lifetime, CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW, CATALYST_NUINT));

/** Produce csize bytes into managed storage, retaining the lifetime most recent chunks. */
CATRLLIB_API CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATALYST_NUINT csize, CATMMLIB_POINTERW* output);

/** Release all managed state and chunk allocations. */
CATRLLIB_API CATALYST_RESULT catrl_stream_free(CATRLLIB_STREAM* stream);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
