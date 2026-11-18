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
/** Finite indexed byte source originating from persistent storage. */
typedef struct CATALYST_FILE CATALYST_FILE;
/** Sequential producer of byte chunks. */
typedef struct CATALYST_STREAM CATALYST_STREAM;

/** Evaluate one valid zero-based index in a persistent source. */
typedef CATALYST_RESULT (*CATALYST_FILE_READER)(void* state, CATALYST_NUINT index, CATALYST_BYTE* value);
/** Produce the next borrowed chunk, or SUCCESS_NOOP at permanent exhaustion. */
typedef CATALYST_RESULT (*CATALYST_STREAM_READER)(void* state, const CATALYST_BYTE** chunk, CATALYST_NUINT* length);

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
 * C API :: FILE
 * ------------------------------------------------------------------------------------------------- */

/** Bind a fixed-length persistent byte source. The provider remains caller-owned. */
CATRLLIB_API CATALYST_RESULT catrl_file_create(CATALYST_FILE** file, void* state, CATALYST_FILE_READER reader, CATALYST_NUINT length);

/** Release the handle, not the underlying persistent source. */
CATRLLIB_API CATALYST_RESULT catrl_file_destroy(CATALYST_FILE* file);

/** Evaluate the persistent byte source at an index. No cursor is advanced. */
CATRLLIB_API CATALYST_RESULT catrl_file_get(const CATALYST_FILE* file, CATALYST_NUINT index, CATALYST_BYTE* value);

/** Return the fixed length of the indexed source in bytes. */
CATRLLIB_API CATALYST_NUINT catrl_file_length(const CATALYST_FILE* file);

/* -------------------------------------------------------------------------------------------------
 * C API :: STREAM
 * ------------------------------------------------------------------------------------------------- */

/** Bind a sequential byte producer without retrieving a chunk. */
CATRLLIB_API CATALYST_RESULT catrl_stream_create(CATALYST_STREAM** stream, void* state, CATALYST_STREAM_READER reader);

/** Release the handle without destroying its producer or freeing borrowed chunks. */
CATRLLIB_API CATALYST_RESULT catrl_stream_destroy(CATALYST_STREAM* stream);

/** Retrieve exactly the next chunk from the producer. SUCCESS returns a chunk, which may be empty; SUCCESS_NOOP returns null and zero at permanent exhaustion. Later calls after exhaustion do not invoke the producer. On any other result, outputs are unchanged; the producer may have advanced and previously borrowed bytes may no longer be valid. */
CATRLLIB_API CATALYST_RESULT catrl_stream_next(CATALYST_STREAM* stream, const CATALYST_BYTE** chunk, CATALYST_NUINT* length);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
