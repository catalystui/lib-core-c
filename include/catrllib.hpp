#ifndef CATRLLIB_HPP
#define CATRLLIB_HPP
#include "catmmlib.hpp"
#include "catrllib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: COLLECTION STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** Alias of the C collection type; retains its ownership and lifetime contracts. */
typedef ::CATRLLIB_SET SET;
/** Alias of the C collection type; retains its ownership and lifetime contracts. */
typedef ::CATRLLIB_MAP MAP;
/** Alias of the C collection type; retains its ownership and lifetime contracts. */
typedef ::CATRLLIB_ARRAY ARRAY;
/** Alias of the C collection type; retains its ownership and lifetime contracts. */
typedef ::CATRLLIB_FILE FILE;
/** Alias of the C collection type; retains its ownership and lifetime contracts. */
typedef ::CATRLLIB_STREAM STREAM;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: SET OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused set storage without allocation; size must be nonzero bytes per element. Capacity and cardinality become zero. */
inline void set_init(SET* set, NUINT size) {
    ::catrl_set_init(set, size);
}

/** Copy size readable bytes if absent; return TRUE on insertion, FALSE on duplicate. Requires an initialized set, spare capacity for a new element, and input outside owned storage. */
inline BOOL set_add(SET* set, POINTER element) {
    return ::catrl_set_add(set, element);
}

/** Return TRUE if removed, FALSE if absent; may replace the removed element with the last. Requires an initialized set and size readable bytes; capacity is retained. */
inline BOOL set_remove(SET* set, POINTER element) {
    return ::catrl_set_remove(set, element);
}

/** Compare size readable bytes against an initialized set; return TRUE on membership, FALSE otherwise. */
inline BOOL set_contains(const SET* set, POINTER element) {
    return ::catrl_set_contains(set, element);
}

/** Empty an initialized set without releasing storage or changing its capacity; stored pointees are not freed. */
inline void set_clear(SET* set) {
    ::catrl_set_clear(set);
}

/** Expand an initialized set to exactly capacity elements; smaller/equal requests are no-ops. Overflow or CATMMLIB failure leaves the set unchanged. */
inline RESULT set_expand(SET* set, NUINT capacity) {
    return ::catrl_set_expand(set, capacity);
}

/** Shrink an initialized set to max(cardinality, capacity / 2), rounding down. Preserve elements; failure leaves the set unchanged. */
inline RESULT set_shrink(SET* set) {
    return ::catrl_set_shrink(set);
}

/** Release an initialized set; success leaves an empty, reusable set with its element size retained. Failure leaves it unchanged; an unallocated set is a no-op. */
inline RESULT set_free(SET* set) {
    return ::catrl_set_free(set);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: MAP OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused map storage without allocation; ksize/vsize must be nonzero bytes per key/value. Capacity and cardinality become zero. */
inline void map_init(MAP* map, NUINT ksize, NUINT vsize) {
    ::catrl_map_init(map, ksize, vsize);
}

/** Copy a new key/value pair; return TRUE if inserted, FALSE on duplicate without replacing its value. Requires an initialized map, readable key/value extents outside its storage, and spare capacity for a new key. */
inline BOOL map_add(MAP* map, POINTER key, POINTER value) {
    return ::catrl_map_add(map, key, value);
}

/** Return TRUE if removed, FALSE if absent; may move the last pair into its place. Requires an initialized map and ksize readable key bytes; capacity is retained. */
inline BOOL map_remove(MAP* map, POINTER key) {
    return ::catrl_map_remove(map, key);
}

/** Compare ksize readable key bytes against an initialized map; return TRUE on membership, FALSE otherwise. */
inline BOOL map_contains(const MAP* map, POINTER key) {
    return ::catrl_map_contains(map, key);
}

/** Copy vsize bytes to value when key is present; return TRUE, or FALSE leaving output unchanged. Requires an initialized map, readable key, and writable output outside its owned storage. */
inline BOOL map_get(const MAP* map, POINTER key, POINTERW value) {
    return ::catrl_map_get(map, key, value);
}

/** Replace an existing value; return TRUE, or FALSE without mutation for an absent key. Requires an initialized map and readable key/value extents; value must be outside owned storage. */
inline BOOL map_set(MAP* map, POINTER key, POINTER value) {
    return ::catrl_map_set(map, key, value);
}

/** Empty an initialized map without releasing its key/value storage or changing capacity; stored pointees are not freed. */
inline void map_clear(MAP* map) {
    ::catrl_map_clear(map);
}

/** Expand an initialized map to exactly capacity pairs; smaller/equal requests are no-ops. Failure preserves pairs and logical capacity, but a successful key resize can invalidate key aliases. */
inline RESULT map_expand(MAP* map, NUINT capacity) {
    return ::catrl_map_expand(map, capacity);
}

/** Shrink an initialized map to max(cardinality, capacity / 2). Preserve pairs; if value resizing fails after key resizing, the smaller logical capacity remains. */
inline RESULT map_shrink(MAP* map) {
    return ::catrl_map_shrink(map);
}

/** Release keys then values of an initialized map. Successful key release empties it and sets capacity to zero; a later failure retains values for retry. Sizes remain reusable. */
inline RESULT map_free(MAP* map) {
    return ::catrl_map_free(map);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: ARRAY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused array storage without allocation; size must be nonzero bytes per element. Capacity and length become zero. */
inline void array_init(ARRAY* array, NUINT size) {
    ::catrl_array_init(array, size);
}

/** Append a copy of size readable bytes. Requires an initialized array, length < capacity, and input outside owned storage; no allocation occurs. */
inline void array_add(ARRAY* array, POINTER element) {
    ::catrl_array_add(array, element);
}

/** Insert a copy and shift the suffix right. Requires an initialized array, index <= length, length < capacity, and size readable bytes outside owned storage. */
inline void array_insert(ARRAY* array, NUINT index, POINTER element) {
    ::catrl_array_insert(array, index, element);
}

/** Remove an element and shift the suffix left; retain capacity. Requires an initialized array and index < length. */
inline void array_remove(ARRAY* array, NUINT index) {
    ::catrl_array_remove(array, index);
}

/** Compare size readable bytes against an initialized array; return TRUE on membership, FALSE otherwise. */
inline BOOL array_contains(const ARRAY* array, POINTER element) {
    return ::catrl_array_contains(array, element);
}

/** Copy size bytes to writable output outside owned storage. Requires an initialized array and index < length. */
inline void array_get(const ARRAY* array, NUINT index, POINTERW element) {
    ::catrl_array_get(array, index, element);
}

/** Replace one element with a copy of size readable bytes outside owned storage. Requires an initialized array and index < length. */
inline void array_set(ARRAY* array, NUINT index, POINTER element) {
    ::catrl_array_set(array, index, element);
}

/** Empty an initialized array without releasing storage or changing capacity; stored pointees are not freed. */
inline void array_clear(ARRAY* array) {
    ::catrl_array_clear(array);
}

/** Expand an initialized array to exactly capacity elements; smaller/equal requests are no-ops. Overflow or CATMMLIB failure leaves the array unchanged. */
inline RESULT array_expand(ARRAY* array, NUINT capacity) {
    return ::catrl_array_expand(array, capacity);
}

/** Shrink an initialized array to max(length, capacity / 2), rounding down. Preserve elements and order; failure leaves the array unchanged. */
inline RESULT array_shrink(ARRAY* array) {
    return ::catrl_array_shrink(array);
}

/** Release an initialized array; success leaves an empty, reusable array with its element size retained. Failure leaves it unchanged; an unallocated array is a no-op. */
inline RESULT array_free(ARRAY* array) {
    return ::catrl_array_free(array);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: FILE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Move two distinct initialized Arrays into unused file storage without allocation, emptying the source Arrays. Requires separate objects and a non-null resolver; source sizes are retained. */
inline void file_init(FILE* file, ARRAY* metadata, ARRAY* data, ::CATALYST_RESULT (*resolver)(const FILE*, NUINT, ARRAY*)) {
    ::catrl_file_init(file, metadata, data, resolver);
}

/** Call the initialized file resolver once with current metadata/data. Requires valid separate output storage and resolver preconditions; index validity and output/failure behavior are resolver-defined. */
inline RESULT file_get(const FILE* file, NUINT index, ARRAY* output) {
    return ::catrl_file_get(file, index, output);
}

/** Release metadata then data Arrays. Failure retains unreleased resources for retry; earlier releases remain effective. Success clears resolver; referenced objects are not freed. */
inline RESULT file_free(FILE* file) {
    return ::catrl_file_free(file);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: STREAM OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize stream storage without allocation or taking state ownership. Requires a non-null producer; state may be null if the producer permits it. */
inline void stream_init(STREAM* stream, POINTERW state, ::CATALYST_RESULT (*next)(STREAM*, POINTERW*)) {
    ::catrl_stream_init(stream, state, next);
}

/** Call the initialized producer once, passing a valid output pointer slot. Output points to a producer-defined chunk; all output, state, termination, and failure behavior belongs to the producer. */
inline RESULT stream_next(STREAM* stream, POINTERW* output) {
    return ::catrl_stream_next(stream, output);
}

} /* namespace catalyst */
#endif /* CATRLLIB_HPP */
