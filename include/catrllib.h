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

/** Owns contiguous copies of distinct elements, compared bytewise. size is bytes per element; cardinality <= capacity. Initialize before use; do not shallow-copy ownership. */
typedef struct CATRLLIB_SET {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_SET;

/** Owns separate contiguous key/value copies with bytewise unique keys. ksize/vsize are bytes per key/value; cardinality <= capacity. Initialize before use; do not shallow-copy ownership. */
typedef struct CATRLLIB_MAP {
    CATMMLIB_POINTERW keys;
    CATMMLIB_POINTERW values;
    CATALYST_NUINT ksize;
    CATALYST_NUINT vsize;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_MAP;

/** Owns contiguous element copies at indices [0, length). size is bytes per element; length <= capacity. Initialize before use; do not shallow-copy ownership. */
typedef struct CATRLLIB_ARRAY {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
} CATRLLIB_ARRAY;

/** Owns metadata and data Arrays. A caller-supplied resolver interprets their current contents; formats, index domain, output ownership, and failures belong to the resolver. */
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

/** Borrows producer-defined state and a producer function. Chunk type, size, ownership, lifetime, sequencing, buffering, and termination belong to the producer. */
typedef struct CATRLLIB_STREAM CATRLLIB_STREAM;
struct CATRLLIB_STREAM {
    CATMMLIB_POINTERW state;
    CATALYST_RESULT (*next)(
        CATRLLIB_STREAM* stream,
        CATMMLIB_POINTERW* output
    );
};

/* -------------------------------------------------------------------------------------------------
 * C API :: SET OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused set storage without allocation; size must be nonzero bytes per element. Capacity and cardinality become zero. */
CATRLLIB_API void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size);

/** Copy size readable bytes if absent; return TRUE on insertion, FALSE on duplicate. Requires an initialized set and spare capacity for a new element; the entire input extent must be outside the set object and its allocation. */
CATRLLIB_API CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Return TRUE if removed, FALSE if absent; may move the last member into its place. Requires an initialized set and size readable bytes, which may identify a live member; lookup precedes mutation. Capacity is retained. */
CATRLLIB_API CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Compare size readable bytes against an initialized set; input may identify a live member. Return TRUE on membership, FALSE otherwise. */
CATRLLIB_API CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element);

/** Empty an initialized set without releasing storage or changing its capacity; stored pointees are not freed. */
CATRLLIB_API void catrl_set_clear(CATRLLIB_SET* set);

/** Expand an initialized set to exactly capacity elements; smaller/equal requests are no-ops. Overflow or CATMMLIB failure leaves the set unchanged. */
CATRLLIB_API CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity);

/** Shrink an initialized set to max(cardinality, capacity / 2), rounding down. Preserve elements; failure leaves the set unchanged. */
CATRLLIB_API CATALYST_RESULT catrl_set_shrink(CATRLLIB_SET* set);

/** Release an initialized set; success leaves an empty, reusable set with its element size retained. Failure leaves it unchanged; an unallocated set is a no-op. */
CATRLLIB_API CATALYST_RESULT catrl_set_free(CATRLLIB_SET* set);

/* -------------------------------------------------------------------------------------------------
 * C API :: MAP OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused map storage without allocation; ksize/vsize must be nonzero bytes per key/value. Capacity and cardinality become zero. */
CATRLLIB_API void catrl_map_init(CATRLLIB_MAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize);

/** Copy a new key/value pair; return TRUE if inserted, FALSE on duplicate without replacing its value. Requires an initialized map and spare capacity for a new key; the entire readable key/value extents must be outside the map object and both allocations. */
CATRLLIB_API CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);

/** Return TRUE if removed, FALSE if absent; may move the last pair into its place. Requires an initialized map and ksize readable bytes, which may identify a live key; lookup precedes mutation. Capacity is retained. */
CATRLLIB_API CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key);

/** Compare ksize readable key bytes against an initialized map; key may identify a live stored key. Return TRUE on membership, FALSE otherwise. */
CATRLLIB_API CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key);

/** Copy vsize bytes to value when key is present; return TRUE, or FALSE leaving output unchanged. Requires an initialized map and readable key; the entire writable output extent must be outside the map object and both allocations. */
CATRLLIB_API CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value);

/** Replace an existing value; return TRUE, or FALSE without mutation for an absent key. Requires an initialized map and readable key; the entire vsize-byte input extent must be outside the map object and both allocations. */
CATRLLIB_API CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value);

/** Empty an initialized map without releasing its key/value storage or changing capacity; stored pointees are not freed. */
CATRLLIB_API void catrl_map_clear(CATRLLIB_MAP* map);

/** Expand an initialized map to exactly capacity pairs; smaller/equal requests are no-ops. Failure preserves pairs and logical capacity, but a successful key resize can invalidate key aliases. */
CATRLLIB_API CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity);

/** Shrink an initialized map to max(cardinality, capacity / 2). Preserve pairs; if value resizing fails after key resizing, the smaller logical capacity remains. */
CATRLLIB_API CATALYST_RESULT catrl_map_shrink(CATRLLIB_MAP* map);

/** Release keys then values of an initialized map. Successful key release empties it and sets capacity to zero; a later failure retains values for retry. Sizes remain reusable. */
CATRLLIB_API CATALYST_RESULT catrl_map_free(CATRLLIB_MAP* map);

/* -------------------------------------------------------------------------------------------------
 * C API :: ARRAY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused array storage without allocation; size must be nonzero bytes per element. Capacity and length become zero. */
CATRLLIB_API void catrl_array_init(CATRLLIB_ARRAY* array, CATALYST_NUINT size);

/** Append a copy of size readable bytes without allocation. Requires an initialized array and length < capacity; the entire input extent must be outside the array object and its allocation. */
CATRLLIB_API void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);

/** Insert a copy after shifting the suffix right. Requires an initialized array, index <= length, and length < capacity; the entire size-byte input extent must be outside the array object and its allocation, since the shift may overwrite an aliased source. */
CATRLLIB_API void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);

/** Remove an element and shift the suffix left; retain capacity. Requires an initialized array and index < length. */
CATRLLIB_API void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index);

/** Compare size readable bytes against an initialized array; input may identify a live stored element. Return TRUE on membership, FALSE otherwise. */
CATRLLIB_API CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element);

/** Copy size bytes to output. Requires an initialized array and index < length; the entire writable output extent must be outside the array object and its allocation. */
CATRLLIB_API void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element);

/** Replace one element with a copy of size readable bytes. Requires an initialized array and index < length; the entire input extent must be outside the array object and its allocation. */
CATRLLIB_API void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element);

/** Empty an initialized array without releasing storage or changing capacity; stored pointees are not freed. */
CATRLLIB_API void catrl_array_clear(CATRLLIB_ARRAY* array);

/** Expand an initialized array to exactly capacity elements; smaller/equal requests are no-ops. Overflow or CATMMLIB failure leaves the array unchanged. */
CATRLLIB_API CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity);

/** Shrink an initialized array to max(length, capacity / 2), rounding down. Preserve elements and order; failure leaves the array unchanged. */
CATRLLIB_API CATALYST_RESULT catrl_array_shrink(CATRLLIB_ARRAY* array);

/** Release an initialized array; success leaves an empty, reusable array with its element size retained. Failure leaves it unchanged; an unallocated array is a no-op. */
CATRLLIB_API CATALYST_RESULT catrl_array_free(CATRLLIB_ARRAY* array);

/* -------------------------------------------------------------------------------------------------
 * C API :: FILE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Transfer ownership of two distinct initialized Arrays into unused file storage without allocation, emptying the source Arrays. Requires separate objects and a non-null resolver; source sizes are retained. */
CATRLLIB_API void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data, CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*));

/** Call the initialized file resolver once with current metadata/data. Requires a writable output Array object outside the file and its allocations; resolver-defined output data may borrow file storage. Index validity and output/failure behavior belong to the resolver. */
CATRLLIB_API CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output);

/** Release metadata then data Arrays. Failure retains unreleased resources for retry; earlier releases remain effective. Success clears resolver; referenced objects are not freed. */
CATRLLIB_API CATALYST_RESULT catrl_file_free(CATRLLIB_FILE* file);

/* -------------------------------------------------------------------------------------------------
 * C API :: STREAM OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Bind stream state and producer without allocation or ownership transfer. Requires a non-null producer, which defines state ownership, lifetime, and cleanup; state may be null if the producer permits it. */
CATRLLIB_API void catrl_stream_init(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW state, CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW*));

/** Call the initialized producer once, passing a valid output pointer slot. Output points to a producer-defined chunk; all output, state, termination, and failure behavior belongs to the producer. */
CATRLLIB_API CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW* output);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
