#include "catmmlib.hpp"
#include "catrllib.h"

using namespace catalyst;

/* Contexts follow public declaration order (including non-reporting methods).
 * Set expand/shrink/free: 06/07/08; Map expand/shrink/free: 10/11/12;
 * Array expand/shrink/free: 1B/1C/1D; File get/free: 1F/20; Stream init/next/free: 21/22/23.
 *
 * Reporting sites:
 * Single-buffer expand: 01 no-op, 02 extent overflow, 03 resize failure, 04 success.
 * Single-buffer shrink: 01 no-op, 02 resize failure, 03 success.
 * Single-buffer free: 01 no-op, 02 release failure, 03 success.
 * Map expand: 01 no-op, 02 key overflow, 03 value overflow, 04 key resize failure, 05 value resize failure, 06 success.
 * Map shrink: 01 no-op, 02 key resize failure, 03 value resize failure, 04 success.
 * Map free: 01 key release failure, 02 value release failure, 03 no-op, 04 success.
 * File free: 01 no-op, 02 metadata release failure, 03 data release failure, 04 success.
 * File get: 01 callback result. Stream result sites are assigned with its implementation below.
 * Dependency status/detail are retained except where Stream optimization failures are reported as warnings. Callback context/operation are replaced; Stream producer argument indices are mapped at its boundary.
 */

/* Checked allocation extents, spare insertion slots, and valid direct indices bound count/offset arithmetic without relying on wider promotions. */

/* Nonzero size, count * size readable data bytes when count is nonzero, and size readable element bytes. Return count for absence; extents were checked when capacity was established. */
static NUINT catrl_find(POINTER data, NUINT count, NUINT size, POINTER element) {
    if (count == 0) return 0;
    const BYTE* current = (const BYTE*) data;
    const BYTE* target = (const BYTE*) element;
    const BYTE first = target[0];
    NUINT index = 0;
    if (size == (NUINT) 1) {
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT
        if (count >= (NUINT) 32) {
            if (current[0] == first) return 0;
#if CATALYST_SUPPORTS_64BIT
            typedef ULONG CATRL_SEARCH_WORD;
            const CATRL_SEARCH_WORD ones = (CATRL_SEARCH_WORD) (CATALYST_ULONG_MAX / (ULONG) CATALYST_BYTE_MAX);
#else
            typedef UINT CATRL_SEARCH_WORD;
            const CATRL_SEARCH_WORD ones = (CATRL_SEARCH_WORD) (CATALYST_UINT_MAX / (UINT) CATALYST_BYTE_MAX);
#endif
            const CATRL_SEARCH_WORD high = (CATRL_SEARCH_WORD) (ones << 7);
            const CATRL_SEARCH_WORD pattern = (CATRL_SEARCH_WORD) (ones * (CATRL_SEARCH_WORD) first);
            const NUINT width = (NUINT) sizeof(CATRL_SEARCH_WORD);
            /* Detect zero bytes after XOR. Locate the match in the same
             * word, not a second traversal of the remaining collection. */
            while ((NUINT) (count - index) >= width) {
                CATRL_SEARCH_WORD word;
#if defined(__GNUC__) || defined(__clang__)
                __builtin_memcpy(&word, current, sizeof(word));
#else
                for (BYTE i = 0; i < (BYTE) sizeof(word); ++i) ((BYTE*) &word)[i] = current[i];
#endif
                word ^= pattern;
                CATRL_SEARCH_WORD matches = (CATRL_SEARCH_WORD) ((word - ones) & ~word & high);
                if (matches != 0) {
                    /* Borrow propagation may mark higher bytes, but never
                     * changes the lowest match on little-endian targets. */
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if CATALYST_SUPPORTS_64BIT && defined(__SIZEOF_LONG_LONG__) && __SIZEOF_LONG_LONG__ == 8
                    return (NUINT) (index + (NUINT) ((NUINT) __builtin_ctzll(matches) / (NUINT) 8));
#elif !CATALYST_SUPPORTS_64BIT && CATALYST_SUPPORTS_32BIT && defined(__SIZEOF_INT__) && __SIZEOF_INT__ == 4
                    return (NUINT) (index + (NUINT) ((NUINT) __builtin_ctz(matches) / (NUINT) 8));
#endif
#endif
                    /* On any other representation, compare the candidate
                     * bytes in memory order. */
                    for (BYTE i = 0; i < (BYTE) sizeof(word); ++i) {
                        if (current[i] == first) return (NUINT) (index + (NUINT) i);
                    }
                }
                current += width;
                index = (NUINT) (index + width);
            }
        }
#endif
        while ((NUINT) (count - index) >= (NUINT) 4) {
            if (current[0] == first) return index;
            if (current[1] == first) return (NUINT) (index + (NUINT) 1);
            if (current[2] == first) return (NUINT) (index + (NUINT) 2);
            if (current[3] == first) return (NUINT) (index + (NUINT) 3);
            current += 4;
            index = (NUINT) (index + (NUINT) 4);
        }
        while (index < count) {
            if (*current++ == first) return index;
            ++index;
        }
        return count;
    }
    while (index < count) {
        if (current[0] == first && catmm_compare(current + 1, target + 1, (NUINT) (size - (NUINT) 1)) == 0) return index;
        current += size;
        ++index;
    }
    return count;
}

/* Store one small element without a general-purpose transfer call. All input
 * and output bytes obey the same non-overlap contract as catmm_copy. */
static inline void catrl_store(POINTERW destination, POINTER source, NUINT size) {
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    if (size == (NUINT) 1) {
        output[0] = input[0];
    } else if (size == (NUINT) 8) {
        for (BYTE i = 0; i < (BYTE) 8; ++i) output[i] = input[i];
    } else {
        catmm_copy(destination, source, size);
    }
}

/* -------------------------------------------------------------------------------------------------
 * C API :: SET OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_set_init(CATRLLIB_SET* set, CATALYST_NUINT size) {
    set->data = 0;
    set->size = size;
    set->cardinality = 0;
    set->capacity = 0;
}

CATALYST_BOOL catrl_set_add(CATRLLIB_SET* set, CATMMLIB_POINTER element) {
    if (catrl_find(set->data, set->cardinality, set->size, element) != set->cardinality) return FALSE;
    catrl_store((BYTE*) set->data + (NUINT) (set->cardinality * set->size), element, set->size);
    ++set->cardinality;
    return TRUE;
}

CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element) {
    NUINT index = catrl_find(set->data, set->cardinality, set->size, element);
    if (index == set->cardinality) return FALSE;
    --set->cardinality;
    if (index != set->cardinality) {
        catrl_store((BYTE*) set->data + (NUINT) (index * set->size), (const BYTE*) set->data + (NUINT) (set->cardinality * set->size), set->size);
    }
    return TRUE;
}

CATALYST_BOOL catrl_set_contains(const CATRLLIB_SET* set, CATMMLIB_POINTER element) {
    return catrl_find(set->data, set->cardinality, set->size, element) != set->cardinality ? TRUE : FALSE;
}

void catrl_set_clear(CATRLLIB_SET* set) {
    set->cardinality = 0;
}

CATALYST_RESULT catrl_set_expand(CATRLLIB_SET* set, CATALYST_NUINT capacity) {
    if (capacity <= set->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x06, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / set->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x02, 0);
    }
    RESULT resize = set->data == 0
        ? catmm_heap_alloc(&set->data, (NUINT) (capacity * set->size))
        : catmm_heap_realloc(&set->data, (NUINT) (capacity * set->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x06, 0x03, resize.detail);
    }
    set->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x04, 0);
}

CATALYST_RESULT catrl_set_shrink(CATRLLIB_SET* set) {
    NUINT capacity = (NUINT) (set->capacity / (NUINT) 2);
    if (capacity < set->cardinality) capacity = set->cardinality;
    if (capacity == set->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01, 0);
    }
    RESULT resize = catmm_heap_realloc(&set->data, (NUINT) (capacity * set->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x07, 0x02, resize.detail);
    }
    set->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_RESULT catrl_set_free(CATRLLIB_SET* set) {
    if (set->data == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x08, 0x01, 0);
    }
    RESULT release = catmm_heap_free(set->data);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x08, 0x02, release.detail);
    }
    set->data = 0;
    set->cardinality = 0;
    set->capacity = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x03, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: MAP OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_map_init(CATRLLIB_MAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize) {
    map->keys = 0;
    map->values = 0;
    map->ksize = ksize;
    map->vsize = vsize;
    map->cardinality = 0;
    map->capacity = 0;
}

CATALYST_BOOL catrl_map_add(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value) {
    if (catrl_find(map->keys, map->cardinality, map->ksize, key) != map->cardinality) return FALSE;
    catrl_store((BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), key, map->ksize);
    catrl_store((BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), value, map->vsize);
    ++map->cardinality;
    return TRUE;
}

CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return FALSE;
    --map->cardinality;
    if (index != map->cardinality) {
        catrl_store((BYTE*) map->keys + (NUINT) (index * map->ksize), (const BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), map->ksize);
        catrl_store((BYTE*) map->values + (NUINT) (index * map->vsize), (const BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), map->vsize);
    }
    return TRUE;
}

CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key) {
    return catrl_find(map->keys, map->cardinality, map->ksize, key) != map->cardinality ? TRUE : FALSE;
}

CATMMLIB_POINTERW catrl_map_get(CATRLLIB_MAP* map, CATMMLIB_POINTER key) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return 0;
    return (BYTE*) map->values + (NUINT) (index * map->vsize);
}

CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return FALSE;
    catrl_store((BYTE*) map->values + (NUINT) (index * map->vsize), value, map->vsize);
    return TRUE;
}

void catrl_map_clear(CATRLLIB_MAP* map) {
    map->cardinality = 0;
}

CATALYST_RESULT catrl_map_expand(CATRLLIB_MAP* map, CATALYST_NUINT capacity) {
    if (capacity <= map->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / map->ksize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x02, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / map->vsize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x03, 0);
    }
    RESULT keys = map->keys == 0
        ? catmm_heap_alloc(&map->keys, (NUINT) (capacity * map->ksize))
        : catmm_heap_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x10, 0x04, keys.detail);
    }
    /* Commit the logical capacity only when both buffers can hold it. If the
     * second resize fails, the larger key buffer is still owned by the map.
     */
    RESULT values = map->values == 0
        ? catmm_heap_alloc(&map->values, (NUINT) (capacity * map->vsize))
        : catmm_heap_realloc(&map->values, (NUINT) (capacity * map->vsize));
    if (!status_code_is_success(values.status)) {
        return RESULT(values.status, 0x10, 0x05, values.detail);
    }
    map->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x10, 0x06, 0);
}

CATALYST_RESULT catrl_map_shrink(CATRLLIB_MAP* map) {
    NUINT capacity = (NUINT) (map->capacity / (NUINT) 2);
    if (capacity < map->cardinality) capacity = map->cardinality;
    if (capacity == map->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x11, 0x01, 0);
    }
    RESULT keys = catmm_heap_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x11, 0x02, keys.detail);
    }
    /* Publish the smaller safe capacity before the second fallible resize.
     * Both buffers still contain every live pair on either outcome.
     */
    map->capacity = capacity;
    RESULT values = catmm_heap_realloc(&map->values, (NUINT) (capacity * map->vsize));
    if (!status_code_is_success(values.status)) {
        return RESULT(values.status, 0x11, 0x03, values.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x11, 0x04, 0);
}

CATALYST_RESULT catrl_map_free(CATRLLIB_MAP* map) {
    if (map->keys == 0 && map->values == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x12, 0x03, 0);
    }
    if (map->keys != 0) {
        RESULT keys = catmm_heap_free(map->keys);
        if (!status_code_is_success(keys.status)) {
            return RESULT(keys.status, 0x12, 0x01, keys.detail);
        }
        map->keys = 0;
        map->cardinality = 0;
        map->capacity = 0;
    }
    if (map->values != 0) {
        RESULT values = catmm_heap_free(map->values);
        if (!status_code_is_success(values.status)) {
            return RESULT(values.status, 0x12, 0x02, values.detail);
        }
        map->values = 0;
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x12, 0x04, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: ARRAY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_array_init(CATRLLIB_ARRAY* array, CATALYST_NUINT size) {
    array->data = 0;
    array->size = size;
    array->length = 0;
    array->capacity = 0;
}

void catrl_array_add(CATRLLIB_ARRAY* array, CATMMLIB_POINTER element) {
    catrl_store((BYTE*) array->data + (NUINT) (array->length * array->size), element, array->size);
    ++array->length;
}

void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element) {
    /* The shift precedes the input copy; element is outside this Array's storage. */
    BYTE* destination = (BYTE*) array->data + (NUINT) (index * array->size);
    if (index < array->length) {
        catmm_move(destination + array->size, destination, (NUINT) ((array->length - index) * array->size));
    }
    catrl_store(destination, element, array->size);
    ++array->length;
}

void catrl_array_remove(CATRLLIB_ARRAY* array, CATALYST_NUINT index) {
    --array->length;
    if (index < array->length) {
        BYTE* destination = (BYTE*) array->data + (NUINT) (index * array->size);
        catmm_move(destination, destination + array->size, (NUINT) ((array->length - index) * array->size));
    }
}

CATALYST_BOOL catrl_array_contains(const CATRLLIB_ARRAY* array, CATMMLIB_POINTER element) {
    return catrl_find(array->data, array->length, array->size, element) != array->length ? TRUE : FALSE;
}

CATMMLIB_POINTERW catrl_array_get(CATRLLIB_ARRAY* array, CATALYST_NUINT index) {
    return (BYTE*) array->data + (NUINT) (index * array->size);
}

void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element) {
    catrl_store((BYTE*) array->data + (NUINT) (index * array->size), element, array->size);
}

void catrl_array_clear(CATRLLIB_ARRAY* array) {
    array->length = 0;
}

CATALYST_RESULT catrl_array_expand(CATRLLIB_ARRAY* array, CATALYST_NUINT capacity) {
    if (capacity <= array->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x1B, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / array->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1B, 0x02, 0);
    }
    RESULT resize = array->data == 0
        ? catmm_heap_alloc(&array->data, (NUINT) (capacity * array->size))
        : catmm_heap_realloc(&array->data, (NUINT) (capacity * array->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x1B, 0x03, resize.detail);
    }
    array->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x1B, 0x04, 0);
}

CATALYST_RESULT catrl_array_shrink(CATRLLIB_ARRAY* array) {
    NUINT capacity = (NUINT) (array->capacity / (NUINT) 2);
    if (capacity < array->length) capacity = array->length;
    if (capacity == array->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x1C, 0x01, 0);
    }
    RESULT resize = catmm_heap_realloc(&array->data, (NUINT) (capacity * array->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x1C, 0x02, resize.detail);
    }
    array->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x03, 0);
}

CATALYST_RESULT catrl_array_free(CATRLLIB_ARRAY* array) {
    if (array->data == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01, 0);
    }
    RESULT release = catmm_heap_free(array->data);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x1D, 0x02, release.detail);
    }
    array->data = 0;
    array->length = 0;
    array->capacity = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x1D, 0x03, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: FILE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data, CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*)) {
    file->metadata = *metadata;
    file->data = *data;
    file->resolver = resolver;
    catrl_array_init(metadata, metadata->size);
    catrl_array_init(data, data->size);
}

CATALYST_RESULT catrl_file_get(const CATRLLIB_FILE* file, CATALYST_NUINT index, CATRLLIB_ARRAY* output) {
    RESULT resolution = file->resolver(file, index, output);
    return RESULT(resolution.status, 0x1F, 0x01, resolution.detail);
}

CATALYST_RESULT catrl_file_free(CATRLLIB_FILE* file) {
    if (file->metadata.data == 0 && file->data.data == 0 && file->resolver == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x20, 0x01, 0);
    }
    RESULT metadata = catrl_array_free(&file->metadata);
    if (!status_code_is_success(metadata.status)) {
        return RESULT(metadata.status, 0x20, 0x02, metadata.detail);
    }
    RESULT data = catrl_array_free(&file->data);
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x20, 0x03, data.detail);
    }
    file->resolver = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x20, 0x04, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: STREAM OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Bookkeeping follows ssize aligned producer bytes. Copy metadata through aligned locals; trailing alignment and native pointer representations are not assumed. Chunks retain CATMMLIB fundamental alignment. */
typedef struct CATRLLIB_STREAM_CHUNK {
    POINTERW data;
    NUINT capacity;
} CATRLLIB_STREAM_CHUNK;

typedef struct CATRLLIB_STREAM_MEMORY {
    NUINT lifetime;
    NUINT cursor;
    CATRLLIB_STREAM_CHUNK spare;
} CATRLLIB_STREAM_MEMORY;

/* Copy fixed-size private descriptors through aligned locals. Compiler
 * constant-size byte copies avoid the general transfer dispatch on warm
 * calls; other compilers use the existing CATMMLIB operation. Both
 * regions are valid, non-overlapping extents of Size bytes. */
template<NUINT Size>
static inline void catrl_stream_copy(POINTERW output, POINTER input) {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_memcpy(output, input, Size);
#else
    catmm_copy(output, input, Size);
#endif
}

/* Initialized, non-freed Stream; index < its stored lifetime. The complete descriptor extent was checked at initialization. */
static BYTE* catrl_stream_slot(CATRLLIB_STREAM* stream, NUINT index) {
    return (BYTE*) stream->state + stream->ssize + (NUINT) sizeof(CATRLLIB_STREAM_MEMORY) + (NUINT) (index * (NUINT) sizeof(CATRLLIB_STREAM_CHUNK));
}

/* Result operations: init 01 private extent overflow, 02 state extent overflow, 03 slot extent overflow, 04 allocation failure, 05 success; next 01 required growth failure, 02 rejected managed chunk, 03 producer outcome, 04 optional shrink warning (detail = dependency status), 05 fatal shrink failure (status/detail retained); free 01 no-op, 02 slot release failure, 03 spare release failure, 04 state release failure, 05 success. */

/* Requires unused stream storage, nonzero lifetime, and callable next; ssize may be zero. Failure leaves a freed Stream. Producers may access only ssize state bytes and the supplied csize chunk bytes, without changing Stream fields or reentering it. */
CATALYST_RESULT catrl_stream_init(CATRLLIB_STREAM* stream, CATALYST_NUINT ssize, CATALYST_NUINT lifetime, CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW, CATALYST_NUINT)) {
    stream->state = 0;
    stream->ssize = 0;
    stream->next = 0;
    if (sizeof(CATRLLIB_STREAM_MEMORY) > CATALYST_NUINT_MAX || sizeof(CATRLLIB_STREAM_CHUNK) > CATALYST_NUINT_MAX) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x21, 0x01, 0);
    }
    NUINT headerSize = (NUINT) sizeof(CATRLLIB_STREAM_MEMORY);
    NUINT slotSize = (NUINT) sizeof(CATRLLIB_STREAM_CHUNK);
    if (ssize > (NUINT) (CATALYST_NUINT_MAX - headerSize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x21, 0x02, 0);
    }
    NUINT prefix = (NUINT) (ssize + headerSize);
    if (lifetime > (NUINT) ((NUINT) (CATALYST_NUINT_MAX - prefix) / slotSize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x21, 0x03, 0);
    }
    RESULT allocation = catmm_heap_alloc(&stream->state, (NUINT) (prefix + (NUINT) (lifetime * slotSize)));
    if (!status_code_is_success(allocation.status)) {
        return RESULT(allocation.status, 0x21, 0x04, allocation.detail);
    }
    stream->ssize = ssize;
    stream->next = next;
    CATRLLIB_STREAM_MEMORY memory;
    memory.lifetime = lifetime;
    memory.cursor = 0;
    memory.spare.data = 0;
    memory.spare.capacity = 0;
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>((BYTE*) stream->state + ssize, &memory);
    /* Zero-filled storage need not represent null pointers. */
    for (NUINT index = 0; index < lifetime; ++index) {
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(catrl_stream_slot(stream, index), &memory.spare);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x21, 0x05, 0);
}

/* Requires an active Stream and output slot outside its object and allocations. Only producer SUCCESS promises all csize bytes and publishes a chunk; all other producer outcomes preserve output/lifetimes. Producer state is not rolled back. Post-publication shrink diagnostics leave output valid and lifetimes advanced, including fatal diagnostics. */
CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATALYST_NUINT csize, CATMMLIB_POINTERW* output) {
    CATRLLIB_STREAM_MEMORY memory;
    BYTE* bookkeeping = (BYTE*) stream->state + stream->ssize;
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(&memory, bookkeeping);
    NUINT required = csize == 0 ? (NUINT) 1 : csize;
    NUINT capacity = memory.spare.capacity;
    if (capacity < required) {
        if (capacity == 0) capacity = 1;
        while (capacity < required) {
            if (capacity > (NUINT) (CATALYST_NUINT_MAX / (NUINT) 2)) {
                capacity = required;
                break;
            }
            capacity = (NUINT) (capacity * (NUINT) 2);
        }
        RESULT resize = catmm_heap_realloc(&memory.spare.data, capacity);
        if (!status_code_is_success(resize.status)) {
            return RESULT(resize.status, 0x22, 0x01, resize.detail);
        }
        memory.spare.capacity = capacity;
        /* Keep replacement ownership reachable before invoking producer code. */
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, &memory);
    }
    RESULT production = stream->next(stream, memory.spare.data, csize);
    if (production.status == STATUS_CODE_SUCCESS) {
        CATRLLIB_STREAM_CHUNK expired;
        BYTE* slot = catrl_stream_slot(stream, memory.cursor);
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(&expired, slot);
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(slot, &memory.spare);
        *output = memory.spare.data;
        memory.spare = expired;
        ++memory.cursor;
        if (memory.cursor == memory.lifetime) memory.cursor = 0;
        /* Only the expired chunk is now scratch; never resize the published chunk or a retained slot. Quarter utilization provides hysteresis before halving. */
        if (required <= (NUINT) (memory.spare.capacity / (NUINT) 4)) {
            NUINT smaller = (NUINT) (memory.spare.capacity / (NUINT) 2);
            RESULT shrink = catmm_heap_realloc(&memory.spare.data, smaller);
            if (!status_code_is_success(shrink.status)) {
                // Production is already published; commit the rotated state even on failure.
                catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, &memory);
                if (status_code_is_fatal(shrink.status)) {
                    return RESULT(shrink.status, 0x22, 0x05, shrink.detail);
                }
                return RESULT(STATUS_CODE_WARNING, 0x22, 0x04, shrink.status);
            }
            memory.spare.capacity = smaller;
        }
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, &memory);
    }
    /* Map producer csize to public argument 1; rejecting the managed chunk indicates invalid internal state. */
    if (production.status == STATUS_CODE_ERROR_INVALID_ARGUMENT) {
        if (production.detail == 1) {
            return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x22, 0x02, 0);
        }
        if (production.detail == 2) production.detail = 1;
    }
    return RESULT(production.status, 0x22, 0x03, production.detail);
}

/* Requires an initialized, failed-init, or freed Stream. Free ends production; on failure retain the object and retry free only. Release producer-owned references before freeing raw state. Record each released allocation before the next fallible step. */
CATALYST_RESULT catrl_stream_free(CATRLLIB_STREAM* stream) {
    if (stream->state == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x23, 0x01, 0);
    }
    stream->next = 0;
    CATRLLIB_STREAM_MEMORY memory;
    BYTE* bookkeeping = (BYTE*) stream->state + stream->ssize;
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(&memory, bookkeeping);
    for (NUINT index = 0; index < memory.lifetime; ++index) {
        CATRLLIB_STREAM_CHUNK chunk;
        BYTE* slot = catrl_stream_slot(stream, index);
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(&chunk, slot);
        if (chunk.data != 0) {
            RESULT release = catmm_heap_free(chunk.data);
            if (!status_code_is_success(release.status)) {
                return RESULT(release.status, 0x23, 0x02, release.detail);
            }
            chunk.data = 0;
            chunk.capacity = 0;
            catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(slot, &chunk);
        }
    }
    if (memory.spare.data != 0) {
        RESULT release = catmm_heap_free(memory.spare.data);
        if (!status_code_is_success(release.status)) {
            return RESULT(release.status, 0x23, 0x03, release.detail);
        }
        memory.spare.data = 0;
        memory.spare.capacity = 0;
        catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, &memory);
    }
    RESULT release = catmm_heap_free(stream->state);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x23, 0x04, release.detail);
    }
    stream->state = 0;
    stream->ssize = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x23, 0x05, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: QUEUE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Queue expand/shrink/free contexts: 29/2A/2B.
 * Stack expand/shrink/free: 31/32/33.
 * Pool expand/shrink/free: 3A/3B/3C.
 * Single-buffer result sites follow the earlier collection conventions.
 * Pool expand: 01 no-op, 02 overflow, 03 bitmap resize, 04 data resize, 05 success.
 * Pool shrink: 01 no-op, 02 data resize, 03 bitmap resize, 04 success.
 * Pool free: 01 no-op, 02 data release, 03 bitmap release, 04 success.
 */

/* Rotate two adjacent nonempty byte ranges using a bounded temporary.
 * Each swap reduces the unsorted extent. Once one range fits in the
 * temporary, move the other range in bulk instead of swapping tiny blocks. */
static void catrl_rotate(BYTE* data, NUINT left, NUINT right) {
    BYTE scratch[32];
    while (left > (NUINT) sizeof(scratch) && right > (NUINT) sizeof(scratch)) {
        NUINT count = left < right ? left : right;
        BYTE* other = data + (left < right ? right : left);
        for (NUINT offset = 0; offset < count;) {
            NUINT size = (NUINT) (count - offset);
            if (size > (NUINT) sizeof(scratch)) size = (NUINT) sizeof(scratch);
            catmm_copy(scratch, data + offset, size);
            catmm_copy(data + offset, other + offset, size);
            catmm_copy(other + offset, scratch, size);
            offset = (NUINT) (offset + size);
        }
        if (left == right) return;
        if (left < right) {
            right = (NUINT) (right - left);
        } else {
            data += right;
            left = (NUINT) (left - right);
        }
    }
    if (left <= right) {
        catmm_copy(scratch, data, left);
        catmm_move(data, data + left, right);
        catmm_copy(data + right, scratch, left);
    } else {
        catmm_copy(scratch, data + left, right);
        catmm_move(data + right, data, left);
        catmm_copy(data, scratch, right);
    }
}

/* Normalize an initialized ring. Extra bytes are available immediately
 * after the old extent on successful expansion, or zero before shrinking.
 * Never move unused slots; preserve logical order without another allocation. */
static void catrl_queue_normalize(CATRLLIB_QUEUE* queue, NUINT extra) {
    if (queue->head == 0) return;
    BYTE* data = (BYTE*) queue->data;
    NUINT offset = (NUINT) (queue->head * queue->size);
    NUINT extent = (NUINT) (queue->capacity * queue->size);
    NUINT live = (NUINT) (queue->length * queue->size);
    NUINT tail = (NUINT) (extent - offset);
    if (live <= tail) {
        catmm_move(data, data + offset, live);
    } else {
        NUINT prefix = (NUINT) (live - tail);
        if (prefix <= extra && (prefix <= tail || tail > extra)) {
            catmm_copy(data + extent, data, prefix);
            catmm_move(data, data + offset, tail);
            catmm_copy(data + tail, data + extent, prefix);
        } else if (tail <= extra) {
            catmm_copy(data + extent, data + offset, tail);
            catmm_move(data + tail, data, prefix);
            catmm_copy(data, data + extent, tail);
        } else {
            /* Compact the gap, then rotate only the two live ranges. */
            if (prefix != offset) catmm_move(data + prefix, data + offset, tail);
            catrl_rotate(data, prefix, tail);
        }
    }
    queue->head = 0;
}

void catrl_queue_init(CATRLLIB_QUEUE* queue, CATALYST_NUINT size) {
    queue->data = 0;
    queue->size = size;
    queue->length = 0;
    queue->capacity = 0;
    queue->head = 0;
}

void catrl_queue_enqueue(CATRLLIB_QUEUE* queue, CATMMLIB_POINTER element) {
    NUINT remaining = (NUINT) (queue->capacity - queue->head);
    NUINT tail = queue->length < remaining
        ? (NUINT) (queue->head + queue->length)
        : (NUINT) (queue->length - remaining);
    catrl_store((BYTE*) queue->data + (NUINT) (tail * queue->size), element, queue->size);
    ++queue->length;
}

CATALYST_BOOL catrl_queue_dequeue(CATRLLIB_QUEUE* queue, CATMMLIB_POINTERW output) {
    if (queue->length == 0) return FALSE;
    catrl_store(output, (const BYTE*) queue->data + (NUINT) (queue->head * queue->size), queue->size);
    --queue->length;
    ++queue->head;
    if (queue->length == 0 || queue->head == queue->capacity) queue->head = 0;
    return TRUE;
}

CATMMLIB_POINTERW catrl_queue_peek(CATRLLIB_QUEUE* queue) {
    if (queue->length == 0) return 0;
    return (BYTE*) queue->data + (NUINT) (queue->head * queue->size);
}

void catrl_queue_clear(CATRLLIB_QUEUE* queue) {
    queue->length = 0;
    queue->head = 0;
}

CATALYST_RESULT catrl_queue_expand(CATRLLIB_QUEUE* queue, CATALYST_NUINT capacity) {
    if (capacity <= queue->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x29, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / queue->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x29, 0x02, 0);
    }
    RESULT resize = catmm_heap_realloc(&queue->data, (NUINT) (capacity * queue->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x29, 0x03, resize.detail);
    }
    catrl_queue_normalize(queue, (NUINT) ((capacity - queue->capacity) * queue->size));
    queue->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x29, 0x04, 0);
}

CATALYST_RESULT catrl_queue_shrink(CATRLLIB_QUEUE* queue) {
    NUINT capacity = (NUINT) (queue->capacity / (NUINT) 2);
    if (capacity < queue->length) capacity = queue->length;
    if (capacity == queue->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x2A, 0x01, 0);
    }
    /* Normalize before truncation; failure preserves logical order, not physical placement. */
    catrl_queue_normalize(queue, 0);
    RESULT resize = catmm_heap_realloc(&queue->data, (NUINT) (capacity * queue->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x2A, 0x02, resize.detail);
    }
    queue->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x2A, 0x03, 0);
}

CATALYST_RESULT catrl_queue_free(CATRLLIB_QUEUE* queue) {
    if (queue->data == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x2B, 0x01, 0);
    }
    RESULT release = catmm_heap_free(queue->data);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x2B, 0x02, release.detail);
    }
    queue->data = 0;
    queue->capacity = 0;
    catrl_queue_clear(queue);
    return RESULT(STATUS_CODE_SUCCESS, 0x2B, 0x03, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: STACK OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_stack_init(CATRLLIB_STACK* stack, CATALYST_NUINT size) {
    stack->data = 0;
    stack->size = size;
    stack->length = 0;
    stack->capacity = 0;
}

void catrl_stack_push(CATRLLIB_STACK* stack, CATMMLIB_POINTER element) {
    catrl_store((BYTE*) stack->data + (NUINT) (stack->length * stack->size), element, stack->size);
    ++stack->length;
}

CATALYST_BOOL catrl_stack_pop(CATRLLIB_STACK* stack, CATMMLIB_POINTERW output) {
    if (stack->length == 0) return FALSE;
    --stack->length;
    catrl_store(output, (const BYTE*) stack->data + (NUINT) (stack->length * stack->size), stack->size);
    return TRUE;
}

CATMMLIB_POINTERW catrl_stack_peek(CATRLLIB_STACK* stack) {
    if (stack->length == 0) return 0;
    return (BYTE*) stack->data + (NUINT) ((stack->length - (NUINT) 1) * stack->size);
}

void catrl_stack_clear(CATRLLIB_STACK* stack) {
    stack->length = 0;
}

CATALYST_RESULT catrl_stack_expand(CATRLLIB_STACK* stack, CATALYST_NUINT capacity) {
    if (capacity <= stack->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x31, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / stack->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x31, 0x02, 0);
    }
    RESULT resize = catmm_heap_realloc(&stack->data, (NUINT) (capacity * stack->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x31, 0x03, resize.detail);
    }
    stack->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x31, 0x04, 0);
}

CATALYST_RESULT catrl_stack_shrink(CATRLLIB_STACK* stack) {
    NUINT capacity = (NUINT) (stack->capacity / (NUINT) 2);
    if (capacity < stack->length) capacity = stack->length;
    if (capacity == stack->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x32, 0x01, 0);
    }
    RESULT resize = catmm_heap_realloc(&stack->data, (NUINT) (capacity * stack->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x32, 0x02, resize.detail);
    }
    stack->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x32, 0x03, 0);
}

CATALYST_RESULT catrl_stack_free(CATRLLIB_STACK* stack) {
    if (stack->data == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x33, 0x01, 0);
    }
    RESULT release = catmm_heap_free(stack->data);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x33, 0x02, release.detail);
    }
    stack->data = 0;
    stack->length = 0;
    stack->capacity = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x33, 0x03, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: POOL OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Round up without overflowing a pointer-width count. */
static NUINT catrl_pool_bitmap_size(NUINT capacity) {
    return (NUINT) (capacity / (NUINT) 8 + (capacity % (NUINT) 8 != 0 ? (NUINT) 1 : (NUINT) 0));
}

/* Bit zero is the lowest index in each byte. Any representable index is valid here. */
static BYTE catrl_pool_mask(NUINT index) {
    return (BYTE) ((BYTE) 1 << (BYTE) (index % (NUINT) 8));
}

void catrl_pool_init(CATRLLIB_POOL* pool, CATALYST_NUINT size) {
    pool->data = 0;
    pool->occupied = 0;
    pool->size = size;
    pool->cardinality = 0;
    pool->capacity = 0;
}

CATALYST_BOOL catrl_pool_acquire(CATRLLIB_POOL* pool, CATALYST_NUINT* index) {
    if (pool->cardinality == pool->capacity) return FALSE;
    BYTE* occupied = (BYTE*) pool->occupied;
    NUINT bytes = catrl_pool_bitmap_size(pool->capacity);
    NUINT i = 0;
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT
#if CATALYST_SUPPORTS_64BIT
    typedef ULONG CATRL_POOL_WORD;
    const CATRL_POOL_WORD full = CATALYST_ULONG_MAX;
#else
    typedef UINT CATRL_POOL_WORD;
    const CATRL_POOL_WORD full = CATALYST_UINT_MAX;
#endif
    const NUINT width = (NUINT) sizeof(CATRL_POOL_WORD);
    /* Skip full words, using byte access for unaligned storage and for
     * the final partial word. The all-ones test is endian independent. */
    while ((NUINT) (bytes - i) >= width) {
        CATRL_POOL_WORD word;
#if defined(__GNUC__) || defined(__clang__)
        __builtin_memcpy(&word, occupied + i, sizeof(word));
#else
        for (BYTE j = 0; j < (BYTE) sizeof(word); ++j) ((BYTE*) &word)[j] = occupied[i + j];
#endif
        if (word != full) break;
        i = (NUINT) (i + width);
    }
#endif
    /* A free logical slot exists, so the first zero bit is not padding. */
    while (occupied[i] == CATALYST_BYTE_MAX) ++i;
    BYTE available = (BYTE) ~occupied[i];
    BYTE bit = 0;
    if ((available & (BYTE) 0x0F) == 0) {
        available = (BYTE) (available >> 4);
        bit = 4;
    }
    if ((available & (BYTE) 0x03) == 0) {
        available = (BYTE) (available >> 2);
        bit = (BYTE) (bit + (BYTE) 2);
    }
    if ((available & (BYTE) 1) == 0) ++bit;
    occupied[i] = (BYTE) (occupied[i] | (BYTE) ((BYTE) 1 << bit));
    ++pool->cardinality;
    *index = (NUINT) (i * (NUINT) 8 + (NUINT) bit);
    return TRUE;
}

CATALYST_BOOL catrl_pool_contains(const CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    if (index >= pool->capacity) return FALSE;
    return (((const BYTE*) pool->occupied)[index / (NUINT) 8] & catrl_pool_mask(index)) != 0 ? TRUE : FALSE;
}

CATALYST_BOOL catrl_pool_release(CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    if (!catrl_pool_contains(pool, index)) return FALSE;
    BYTE* occupied = (BYTE*) pool->occupied + index / (NUINT) 8;
    *occupied = (BYTE) (*occupied & (BYTE) ~catrl_pool_mask(index));
    --pool->cardinality;
    return TRUE;
}

CATMMLIB_POINTERW catrl_pool_get(CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    if (!catrl_pool_contains(pool, index)) return 0;
    return (BYTE*) pool->data + (NUINT) (index * pool->size);
}

void catrl_pool_clear(CATRLLIB_POOL* pool) {
    if (pool->cardinality != 0) catmm_fill(pool->occupied, 0, catrl_pool_bitmap_size(pool->capacity));
    pool->cardinality = 0;
}

CATALYST_RESULT catrl_pool_expand(CATRLLIB_POOL* pool, CATALYST_NUINT capacity) {
    if (capacity <= pool->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x3A, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / pool->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x3A, 0x02, 0);
    }
    NUINT bytes = catrl_pool_bitmap_size(capacity);
    if (bytes != catrl_pool_bitmap_size(pool->capacity)) {
        RESULT bitmap = catmm_heap_realloc(&pool->occupied, bytes);
        if (!status_code_is_success(bitmap.status)) {
            return RESULT(bitmap.status, 0x3A, 0x03, bitmap.detail);
        }
    }
    /* The bitmap may retain extra zero bits on data resize failure. */
    RESULT data = catmm_heap_realloc(&pool->data, (NUINT) (capacity * pool->size));
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x3A, 0x04, data.detail);
    }
    pool->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x3A, 0x05, 0);
}

CATALYST_RESULT catrl_pool_shrink(CATRLLIB_POOL* pool) {
    NUINT capacity = (NUINT) (pool->capacity / (NUINT) 2);
    if (capacity < pool->cardinality) capacity = pool->cardinality;
    /* Never truncate a live index. Skip empty bitmap bytes when seeking the last live slot. */
    NUINT end = pool->capacity;
    if (pool->cardinality != 0) {
        const BYTE* occupied = (const BYTE*) pool->occupied;
        while (end > capacity) {
            NUINT last = (NUINT) (end - (NUINT) 1);
            NUINT byte = (NUINT) (last / (NUINT) 8);
            if (occupied[byte] == 0) {
                NUINT start = (NUINT) (byte * (NUINT) 8);
                end = start < capacity ? capacity : start;
            } else if ((occupied[byte] & catrl_pool_mask(last)) != 0) {
                capacity = end;
                break;
            } else {
                --end;
            }
        }
    }
    if (capacity == pool->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x3B, 0x01, 0);
    }
    NUINT old_bytes = catrl_pool_bitmap_size(pool->capacity);
    RESULT data = catmm_heap_realloc(&pool->data, (NUINT) (capacity * pool->size));
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x3B, 0x02, data.detail);
    }
    pool->capacity = capacity;
    NUINT bytes = catrl_pool_bitmap_size(capacity);
    if (bytes != old_bytes) {
        RESULT bitmap = catmm_heap_realloc(&pool->occupied, bytes);
        if (!status_code_is_success(bitmap.status)) {
            return RESULT(bitmap.status, 0x3B, 0x03, bitmap.detail);
        }
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x3B, 0x04, 0);
}

CATALYST_RESULT catrl_pool_free(CATRLLIB_POOL* pool) {
    if (pool->data == 0 && pool->occupied == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x3C, 0x01, 0);
    }
    if (pool->data != 0) {
        RESULT data = catmm_heap_free(pool->data);
        if (!status_code_is_success(data.status)) {
            return RESULT(data.status, 0x3C, 0x02, data.detail);
        }
        pool->data = 0;
        catrl_pool_clear(pool);
        pool->capacity = 0;
    }
    if (pool->occupied != 0) {
        RESULT bitmap = catmm_heap_free(pool->occupied);
        if (!status_code_is_success(bitmap.status)) {
            return RESULT(bitmap.status, 0x3C, 0x03, bitmap.detail);
        }
        pool->occupied = 0;
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x3C, 0x04, 0);
}
