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
 * C API :: COLLECTION EXTENSIONS
 * ------------------------------------------------------------------------------------------------- */

/** A finite, ordered collection of fixed-size elements stored contiguously and retrieved in first-in, first-out (FIFO) order. */
typedef struct CATRLLIB_QUEUE {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
    CATALYST_NUINT head;
} CATRLLIB_QUEUE;

/** A finite, ordered collection of fixed-size elements stored contiguously and retrieved in last-in, first-out (LIFO) order. */
typedef struct CATRLLIB_STACK {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
    CATALYST_NUINT length;
    CATALYST_NUINT capacity;
} CATRLLIB_STACK;

/** A finite collection of fixed-size, reusable element slots stored contiguously, each of which may be independently acquired or released. */
typedef struct CATRLLIB_POOL {
    CATMMLIB_POINTERW data;
    CATMMLIB_POINTERW occupied;
    CATALYST_NUINT size;
    CATALYST_NUINT cardinality;
    CATALYST_NUINT capacity;
} CATRLLIB_POOL;

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

/** Borrow the writable value associated with a readable key, or null if absent. Requires an initialized Map. Reacquire after removal, clear, resize or free attempts; do not release independently. */
CATRLLIB_API CATMMLIB_POINTERW catrl_map_get(CATRLLIB_MAP* map, CATMMLIB_POINTER key);

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

/** Borrow the writable element at a valid index of an initialized Array. Reacquire after insertion, removal, clear, resize or free attempts; do not release independently. */
CATRLLIB_API CATMMLIB_POINTERW catrl_array_get(CATRLLIB_ARRAY* array, CATALYST_NUINT index);

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

/* -------------------------------------------------------------------------------------------------
 * C API :: QUEUE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty queue with elements of the specified size in bytes. */
CATRLLIB_API void catrl_queue_init(CATRLLIB_QUEUE* queue, CATALYST_NUINT size);

/** Append an element to the queue. Requires sufficient capacity. */
CATRLLIB_API void catrl_queue_enqueue(CATRLLIB_QUEUE* queue, CATMMLIB_POINTER element);

/** Remove the first element from the queue and copy it to output if present. */
CATRLLIB_API CATALYST_BOOL catrl_queue_dequeue(CATRLLIB_QUEUE* queue, CATMMLIB_POINTERW output);

/** Borrow the first element in the queue, or null if empty. */
CATRLLIB_API CATMMLIB_POINTERW catrl_queue_peek(CATRLLIB_QUEUE* queue);

/** Remove all elements from the queue without changing its capacity. */
CATRLLIB_API void catrl_queue_clear(CATRLLIB_QUEUE* queue);

/** Increase the queue's capacity to accommodate the specified number of elements. */
CATRLLIB_API CATALYST_RESULT catrl_queue_expand(CATRLLIB_QUEUE* queue, CATALYST_NUINT capacity);

/** Reduce the queue's capacity to accommodate its existing elements. */
CATRLLIB_API CATALYST_RESULT catrl_queue_shrink(CATRLLIB_QUEUE* queue);

/** Release the resources owned by the queue. */
CATRLLIB_API CATALYST_RESULT catrl_queue_free(CATRLLIB_QUEUE* queue);


/* -------------------------------------------------------------------------------------------------
 * C API :: STACK OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty stack with elements of the specified size in bytes. */
CATRLLIB_API void catrl_stack_init(CATRLLIB_STACK* stack, CATALYST_NUINT size);

/** Append an element to the stack. Requires sufficient capacity. */
CATRLLIB_API void catrl_stack_push(CATRLLIB_STACK* stack, CATMMLIB_POINTER element);

/** Remove the last element from the stack and copy it to output if present. */
CATRLLIB_API CATALYST_BOOL catrl_stack_pop(CATRLLIB_STACK* stack, CATMMLIB_POINTERW output);

/** Borrow the last element in the stack, or null if empty. */
CATRLLIB_API CATMMLIB_POINTERW catrl_stack_peek(CATRLLIB_STACK* stack);

/** Remove all elements from the stack without changing its capacity. */
CATRLLIB_API void catrl_stack_clear(CATRLLIB_STACK* stack);

/** Increase the stack's capacity to accommodate the specified number of elements. */
CATRLLIB_API CATALYST_RESULT catrl_stack_expand(CATRLLIB_STACK* stack, CATALYST_NUINT capacity);

/** Reduce the stack's capacity to accommodate its existing elements. */
CATRLLIB_API CATALYST_RESULT catrl_stack_shrink(CATRLLIB_STACK* stack);

/** Release the resources owned by the stack. */
CATRLLIB_API CATALYST_RESULT catrl_stack_free(CATRLLIB_STACK* stack);

/* -------------------------------------------------------------------------------------------------
 * C API :: POOL OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty pool with slots of the specified size in bytes. */
CATRLLIB_API void catrl_pool_init(CATRLLIB_POOL* pool, CATALYST_NUINT size);

/** Acquire an available slot and output its index if one is available. */
CATRLLIB_API CATALYST_BOOL catrl_pool_acquire(CATRLLIB_POOL* pool, CATALYST_NUINT* index);

/** Release an acquired slot at the specified index. */
CATRLLIB_API CATALYST_BOOL catrl_pool_release(CATRLLIB_POOL* pool, CATALYST_NUINT index);

/** Determine whether the slot at the specified index is occupied. */
CATRLLIB_API CATALYST_BOOL catrl_pool_contains(const CATRLLIB_POOL* pool, CATALYST_NUINT index);

/** Borrow the element at the specified occupied index, or null if unavailable. */
CATRLLIB_API CATMMLIB_POINTERW catrl_pool_get(CATRLLIB_POOL* pool, CATALYST_NUINT index);

/** Release all acquired slots without changing the pool's capacity. */
CATRLLIB_API void catrl_pool_clear(CATRLLIB_POOL* pool);

/** Increase the pool's capacity to accommodate the specified number of slots. */
CATRLLIB_API CATALYST_RESULT catrl_pool_expand(CATRLLIB_POOL* pool, CATALYST_NUINT capacity);

/** Reduce the pool's capacity by removing trailing unoccupied slots. */
CATRLLIB_API CATALYST_RESULT catrl_pool_shrink(CATRLLIB_POOL* pool);

/** Release the resources owned by the pool. */
CATRLLIB_API CATALYST_RESULT catrl_pool_free(CATRLLIB_POOL* pool);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATRLLIB_H */
