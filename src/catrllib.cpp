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
            const NUINT group = (NUINT) (width * (NUINT) 4);
            /* Four independent word tests share one branch in the miss path.
             * All loads are within the checked extent and need no alignment. */
            while ((NUINT) (count - index) >= group) {
                CATRL_SEARCH_WORD a, b, c, d;
#if defined(__GNUC__) || defined(__clang__)
                __builtin_memcpy(&a, current, sizeof(a));
                __builtin_memcpy(&b, current + width, sizeof(b));
                __builtin_memcpy(&c, current + (NUINT) (width * (NUINT) 2), sizeof(c));
                __builtin_memcpy(&d, current + (NUINT) (width * (NUINT) 3), sizeof(d));
#else
                for (BYTE i = 0; i < (BYTE) sizeof(a); ++i) {
                    ((BYTE*) &a)[i] = current[i];
                    ((BYTE*) &b)[i] = current[width + i];
                    ((BYTE*) &c)[i] = current[(NUINT) (width * (NUINT) 2) + i];
                    ((BYTE*) &d)[i] = current[(NUINT) (width * (NUINT) 3) + i];
                }
#endif
                a ^= pattern; b ^= pattern; c ^= pattern; d ^= pattern;
                a = (CATRL_SEARCH_WORD) ((a - ones) & ~a & high);
                b = (CATRL_SEARCH_WORD) ((b - ones) & ~b & high);
                c = (CATRL_SEARCH_WORD) ((c - ones) & ~c & high);
                d = (CATRL_SEARCH_WORD) ((d - ones) & ~d & high);
                if ((a | b | c | d) != 0) {
                    NUINT offset = 0;
                    CATRL_SEARCH_WORD matches = a;
                    if (matches == 0) {
                        offset = width; matches = b;
                        if (matches == 0) {
                            offset = (NUINT) (width * (NUINT) 2); matches = c;
                            if (matches == 0) {
                                offset = (NUINT) (width * (NUINT) 3); matches = d;
                            }
                        }
                    }
                    /* Borrow propagation cannot mark a byte before the first
                     * true match in little-endian memory order. */
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if CATALYST_SUPPORTS_64BIT && defined(__SIZEOF_LONG_LONG__) && __SIZEOF_LONG_LONG__ == 8
                    return (NUINT) (index + offset + (NUINT) ((NUINT) __builtin_ctzll(matches) / (NUINT) 8));
#elif !CATALYST_SUPPORTS_64BIT && CATALYST_SUPPORTS_32BIT && defined(__SIZEOF_INT__) && __SIZEOF_INT__ == 4
                    return (NUINT) (index + offset + (NUINT) ((NUINT) __builtin_ctz(matches) / (NUINT) 8));
#endif
#endif
                    for (NUINT i = 0; i < width; ++i) {
                        if (current[offset + i] == first) return (NUINT) (index + offset + i);
                    }
                }
                current += group;
                index = (NUINT) (index + group);
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
#if TARGET_SIZEOF_VOID_P > 1 && (defined(__GNUC__) || defined(__clang__))
    /* Pointer-width elements can be compared as one exact-width Catalyst
     * word. Fixed-size byte copies preserve unaligned access and byte equality
     * on either endian; no pointer casts or wider representation are needed. */
    if (size == (NUINT) sizeof(NUINT)) {
        NUINT key;
        __builtin_memcpy(&key, target, sizeof(key));
        const NUINT width = (NUINT) sizeof(NUINT);
        while ((NUINT) (count - index) >= (NUINT) 4) {
            NUINT a, b, c, d;
            __builtin_memcpy(&a, current, sizeof(a));
            __builtin_memcpy(&b, current + width, sizeof(b));
            __builtin_memcpy(&c, current + (NUINT) (width * (NUINT) 2), sizeof(c));
            __builtin_memcpy(&d, current + (NUINT) (width * (NUINT) 3), sizeof(d));
            if (a == key) return index;
            if (b == key) return (NUINT) (index + (NUINT) 1);
            if (c == key) return (NUINT) (index + (NUINT) 2);
            if (d == key) return (NUINT) (index + (NUINT) 3);
            current += (NUINT) (width * (NUINT) 4);
            index = (NUINT) (index + (NUINT) 4);
        }
        while (index < count) {
            NUINT candidate;
            __builtin_memcpy(&candidate, current, sizeof(candidate));
            if (candidate == key) return index;
            current += width;
            ++index;
        }
        return count;
    }
#endif
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
#if defined(__GNUC__) || defined(__clang__)
    } else if (size == (NUINT) 8) {
        __builtin_memcpy(output, input, 8);
    } else if (size == (NUINT) 16) {
        __builtin_memcpy(output, input, 16);
#else
    } else if (size == (NUINT) 8) {
        for (BYTE i = 0; i < (BYTE) 8; ++i) output[i] = input[i];
#endif
    } else {
        catmm_copy(destination, source, size);
    }
}

/* Single-buffer collections share identical lifecycle contracts. Callers
 * supply valid initialized fields, nonzero element size, and their fixed
 * public method context. No structure casts or changes to public layouts. */
static inline RESULT catrl_grow_storage(POINTERW* data, NUINT extent) {
    return *data == 0 ? catmm_heap_alloc(data, extent) : catmm_heap_realloc(data, extent);
}

static inline NUINT catrl_shrink_capacity(NUINT capacity, NUINT count) {
    NUINT target = (NUINT) (capacity / (NUINT) 2);
    return target < count ? count : target;
}

static inline RESULT catrl_buffer_expand(POINTERW* data, NUINT size, NUINT* current, NUINT capacity, CONTEXT_CODE context) {
    if (capacity <= *current) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, context, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, context, 0x02, 0);
    }
    RESULT resize = catrl_grow_storage(data, (NUINT) (capacity * size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, context, 0x03, resize.detail);
    }
    *current = capacity;
    return RESULT(STATUS_CODE_SUCCESS, context, 0x04, 0);
}

static inline RESULT catrl_buffer_shrink(POINTERW* data, NUINT size, NUINT* current, NUINT count, CONTEXT_CODE context) {
    NUINT capacity = catrl_shrink_capacity(*current, count);
    if (capacity == *current) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, context, 0x01, 0);
    }
    RESULT resize = catmm_heap_realloc(data, (NUINT) (capacity * size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, context, 0x02, resize.detail);
    }
    *current = capacity;
    return RESULT(STATUS_CODE_SUCCESS, context, 0x03, 0);
}

static inline RESULT catrl_buffer_free(POINTERW* data, NUINT* count, NUINT* capacity, CONTEXT_CODE context) {
    if (*data == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, context, 0x01, 0);
    }
    RESULT release = catmm_heap_free(*data);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, context, 0x02, release.detail);
    }
    *data = 0;
    *count = 0;
    *capacity = 0;
    return RESULT(STATUS_CODE_SUCCESS, context, 0x03, 0);
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
    return catrl_buffer_expand(&set->data, set->size, &set->capacity, capacity, 0x06);
}

CATALYST_RESULT catrl_set_shrink(CATRLLIB_SET* set) {
    return catrl_buffer_shrink(&set->data, set->size, &set->capacity, set->cardinality, 0x07);
}

CATALYST_RESULT catrl_set_free(CATRLLIB_SET* set) {
    return catrl_buffer_free(&set->data, &set->cardinality, &set->capacity, 0x08);
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
    RESULT keys = catrl_grow_storage(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x10, 0x04, keys.detail);
    }
    /* Commit the logical capacity only when both buffers can hold it. If the
     * second resize fails, the larger key buffer is still owned by the map.
     */
    RESULT values = catrl_grow_storage(&map->values, (NUINT) (capacity * map->vsize));
    if (!status_code_is_success(values.status)) {
        return RESULT(values.status, 0x10, 0x05, values.detail);
    }
    map->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x10, 0x06, 0);
}

CATALYST_RESULT catrl_map_shrink(CATRLLIB_MAP* map) {
    NUINT capacity = catrl_shrink_capacity(map->capacity, map->cardinality);
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
    return catrl_buffer_expand(&array->data, array->size, &array->capacity, capacity, 0x1B);
}

CATALYST_RESULT catrl_array_shrink(CATRLLIB_ARRAY* array) {
    return catrl_buffer_shrink(&array->data, array->size, &array->capacity, array->length, 0x1C);
}

CATALYST_RESULT catrl_array_free(CATRLLIB_ARRAY* array) {
    return catrl_buffer_free(&array->data, &array->length, &array->capacity, 0x1D);
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

/* Initialized bookkeeping and its aligned local representation. Lifetime
 * is immutable. On compilers with layout offsets, write only the changed
 * suffix or spare descriptor without assuming struct packing. */
static inline void catrl_stream_write_cursor(BYTE* bookkeeping, const CATRLLIB_STREAM_MEMORY* memory) {
#if defined(__GNUC__) || defined(__clang__)
    const NUINT offset = (NUINT) __builtin_offsetof(CATRLLIB_STREAM_MEMORY, cursor);
    catrl_stream_copy<(NUINT) (sizeof(CATRLLIB_STREAM_MEMORY) - __builtin_offsetof(CATRLLIB_STREAM_MEMORY, cursor))>(
        bookkeeping + offset, (const BYTE*) memory + offset);
#else
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, memory);
#endif
}

static inline void catrl_stream_write_spare(BYTE* bookkeeping, const CATRLLIB_STREAM_MEMORY* memory) {
#if defined(__GNUC__) || defined(__clang__)
    const NUINT offset = (NUINT) __builtin_offsetof(CATRLLIB_STREAM_MEMORY, spare);
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_CHUNK)>(bookkeeping + offset, &memory->spare);
#else
    catrl_stream_copy<(NUINT) sizeof(CATRLLIB_STREAM_MEMORY)>(bookkeeping, memory);
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
        catrl_stream_write_spare(bookkeeping, &memory);
    }
    RESULT production = stream->next(stream, memory.spare.data, csize);
    if (production.status == STATUS_CODE_SUCCESS) {
        CATRLLIB_STREAM_CHUNK expired;
        BYTE* slot = bookkeeping + (NUINT) sizeof(CATRLLIB_STREAM_MEMORY) +
            (NUINT) (memory.cursor * (NUINT) sizeof(CATRLLIB_STREAM_CHUNK));
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
                catrl_stream_write_cursor(bookkeeping, &memory);
                if (status_code_is_fatal(shrink.status)) {
                    return RESULT(shrink.status, 0x22, 0x05, shrink.detail);
                }
                return RESULT(STATUS_CODE_WARNING, 0x22, 0x04, shrink.status);
            }
            memory.spare.capacity = smaller;
        }
        catrl_stream_write_cursor(bookkeeping, &memory);
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
    RESULT resize = catrl_grow_storage(&queue->data, (NUINT) (capacity * queue->size));
    if (!status_code_is_success(resize.status)) {
        return RESULT(resize.status, 0x29, 0x03, resize.detail);
    }
    catrl_queue_normalize(queue, (NUINT) ((capacity - queue->capacity) * queue->size));
    queue->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x29, 0x04, 0);
}

CATALYST_RESULT catrl_queue_shrink(CATRLLIB_QUEUE* queue) {
    NUINT capacity = catrl_shrink_capacity(queue->capacity, queue->length);
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
    return catrl_buffer_expand(&stack->data, stack->size, &stack->capacity, capacity, 0x31);
}

CATALYST_RESULT catrl_stack_shrink(CATRLLIB_STACK* stack) {
    return catrl_buffer_shrink(&stack->data, stack->size, &stack->capacity, stack->length, 0x32);
}

CATALYST_RESULT catrl_stack_free(CATRLLIB_STACK* stack) {
    return catrl_buffer_free(&stack->data, &stack->length, &stack->capacity, 0x33);
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
    NUINT i = 0;
    /* Most acquisitions reuse an early slot. Only prepare the wider
     * search when the first byte is completely occupied. */
    if (occupied[0] == CATALYST_BYTE_MAX) {
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT
#if CATALYST_SUPPORTS_64BIT
        typedef ULONG CATRL_POOL_WORD;
        const CATRL_POOL_WORD full = CATALYST_ULONG_MAX;
#else
        typedef UINT CATRL_POOL_WORD;
        const CATRL_POOL_WORD full = CATALYST_UINT_MAX;
#endif
        const NUINT width = (NUINT) sizeof(CATRL_POOL_WORD);
        const NUINT bytes = catrl_pool_bitmap_size(pool->capacity);
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
    }
    BYTE available = (BYTE) ~occupied[i];
    BYTE bit = 0;
    if ((available & (BYTE) 1) == 0) {
        if ((available & (BYTE) 0x0F) == 0) {
            available = (BYTE) (available >> 4);
            bit = 4;
        }
        if ((available & (BYTE) 0x03) == 0) {
            available = (BYTE) (available >> 2);
            bit = (BYTE) (bit + (BYTE) 2);
        }
        if ((available & (BYTE) 1) == 0) ++bit;
    }
    occupied[i] = (BYTE) (occupied[i] | (BYTE) ((BYTE) 1 << bit));
    ++pool->cardinality;
    *index = (NUINT) (i * (NUINT) 8 + (NUINT) bit);
    return TRUE;
}

/* An initialized Pool; arbitrary index. Check the logical extent before
 * reading the bitmap. A free or out-of-range slot is ordinarily absent. */
static inline BOOL catrl_pool_is_occupied(const CATRLLIB_POOL* pool, NUINT index) {
    if (index >= pool->capacity) return FALSE;
    return (((const BYTE*) pool->occupied)[index / (NUINT) 8] & catrl_pool_mask(index)) != 0 ? TRUE : FALSE;
}

CATALYST_BOOL catrl_pool_contains(const CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    return catrl_pool_is_occupied(pool, index);
}

CATALYST_BOOL catrl_pool_release(CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    if (index >= pool->capacity) return FALSE;
    BYTE* occupied = (BYTE*) pool->occupied + index / (NUINT) 8;
    BYTE mask = catrl_pool_mask(index);
    BYTE bits = *occupied;
    if ((bits & mask) == 0) return FALSE;
    *occupied = (BYTE) (bits & (BYTE) ~mask);
    --pool->cardinality;
    return TRUE;
}

CATMMLIB_POINTERW catrl_pool_get(CATRLLIB_POOL* pool, CATALYST_NUINT index) {
    if (!catrl_pool_is_occupied(pool, index)) return 0;
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
        RESULT bitmap = catrl_grow_storage(&pool->occupied, bytes);
        if (!status_code_is_success(bitmap.status)) {
            return RESULT(bitmap.status, 0x3A, 0x03, bitmap.detail);
        }
    }
    /* The bitmap may retain extra zero bits on data resize failure. */
    RESULT data = catrl_grow_storage(&pool->data, (NUINT) (capacity * pool->size));
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x3A, 0x04, data.detail);
    }
    pool->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x3A, 0x05, 0);
}

CATALYST_RESULT catrl_pool_shrink(CATRLLIB_POOL* pool) {
    NUINT capacity = catrl_shrink_capacity(pool->capacity, pool->cardinality);
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

/* -------------------------------------------------------------------------------------------------
 * C API :: HASHED COLLECTIONS
 * ------------------------------------------------------------------------------------------------- */

/* Buckets store dense indices plus one; zero denotes empty. A maximum
 * load of one half guarantees an empty bucket on every probe. Keys and
 * set members remain immutable while present. The private bucket words
 * are transferred through actual NUINT objects, not raw typed aliases.
 *
 * HashSet expand/shrink/free: 42/43/44; HashMap: 4C/4D/4E.
 * Expand sites: 01 no-op, 02 element/key overflow, Map 03 value overflow;
 * remaining sites: bucket overflow, element/key resize, optional value
 * resize, bucket resize, success.
 * Shrink: 01 no-op, 02 element/key resize, optional value resize,
 * bucket resize, success.
 * Free: 01 no-op, 02 element/key release, optional value release,
 * bucket release, success.
 */

/* Nonzero size and readable key bytes. FNV-1a at native Catalyst
 * pointer width, with smaller constants on narrow representations.
 * The narrow products fit signed promotions where applicable. */
static NUINT catrl_hash(POINTER key, NUINT size) {
#if TARGET_SIZEOF_VOID_P == 8 && CATALYST_SUPPORTS_64BIT
    NUINT hash = (NUINT) (((NUINT) 0xCBF29CE4 << 32) | (NUINT) 0x84222325);
    const NUINT prime = (NUINT) (((NUINT) 0x100 << 32) | (NUINT) 0x1B3);
#elif TARGET_SIZEOF_VOID_P == 4 && CATALYST_SUPPORTS_32BIT
    NUINT hash = (NUINT) 0x811C9DC5;
    const NUINT prime = (NUINT) 0x01000193;
#elif TARGET_SIZEOF_VOID_P == 2 && CATALYST_SUPPORTS_16BIT
    NUINT hash = (NUINT) 0x9DC5;
    const NUINT prime = (NUINT) 0x0193;
#else
    NUINT hash = (NUINT) 0xC5;
    const NUINT prime = (NUINT) 0x13;
#endif
    const BYTE* bytes = (const BYTE*) key;
    for (NUINT i = 0; i < size; ++i) {
        hash = (NUINT) (hash ^ (NUINT) bytes[i]);
        hash = (NUINT) (hash * prime);
    }
    return hash;
}

/* Valid bucket storage and index. */
static inline NUINT catrl_hash_bucket(POINTER buckets, NUINT index) {
    NUINT entry;
    catrl_store(&entry, (const BYTE*) buckets + (NUINT) (index * (NUINT) sizeof(NUINT)), (NUINT) sizeof(NUINT));
    return entry;
}

/* Valid writable bucket storage and index; entry is zero or a live
 * dense index plus one. */
static inline void catrl_hash_write(POINTERW buckets, NUINT index, NUINT entry) {
    catrl_store((BYTE*) buckets + (NUINT) (index * (NUINT) sizeof(NUINT)), &entry, (NUINT) sizeof(NUINT));
}

/* Valid bucket index in a nonempty ring. */
static inline NUINT catrl_hash_next(NUINT index, NUINT capacity) {
    ++index;
    return index == capacity ? (NUINT) 0 : index;
}

/* Valid indices in the same nonempty ring. */
static inline NUINT catrl_hash_distance(NUINT from, NUINT to, NUINT capacity) {
    return to >= from ? (NUINT) (to - from) : (NUINT) (capacity - from + to);
}

/* Valid nonempty bucket ring with an empty bucket, dense key storage for
 * each occupied entry, and readable key bytes. Return the matching
 * bucket or first empty bucket. Equality is always byte equality. */
static NUINT catrl_hash_probe(POINTER buckets, NUINT bcapacity, POINTER data, NUINT size, POINTER key) {
    NUINT bucket = (NUINT) (catrl_hash(key, size) % bcapacity);
    for (;;) {
        NUINT entry = catrl_hash_bucket(buckets, bucket);
        if (entry == 0) return bucket;
        const BYTE* candidate = (const BYTE*) data + (NUINT) ((entry - (NUINT) 1) * size);
        if (candidate[0] == ((const BYTE*) key)[0] &&
            (size == (NUINT) 1 || catmm_compare(candidate + 1, (const BYTE*) key + 1, (NUINT) (size - (NUINT) 1)) == 0)) {
            return bucket;
        }
        bucket = catrl_hash_next(bucket, bcapacity);
    }
}

/* Initialized index and readable key. Return count for absence; an
 * empty collection need not own dense storage or buckets. */
static NUINT catrl_hash_find(POINTER buckets, NUINT bcapacity, POINTER data, NUINT size, NUINT count, POINTER key) {
    if (count == 0) return count;
    NUINT bucket = catrl_hash_probe(buckets, bcapacity, data, size, key);
    NUINT entry = catrl_hash_bucket(buckets, bucket);
    return entry == 0 ? count : (NUINT) (entry - (NUINT) 1);
}

/* Valid bucket ring, occupied hole and readable live keys. Shift only
 * entries whose probe path crosses the hole. The final empty bucket
 * terminates future unsuccessful searches without tombstones. */
static void catrl_hash_erase(POINTERW buckets, NUINT bcapacity, POINTER data, NUINT size, NUINT hole) {
    NUINT current = catrl_hash_next(hole, bcapacity);
    NUINT entry = catrl_hash_bucket(buckets, current);
    while (entry != 0) {
        const BYTE* key = (const BYTE*) data + (NUINT) ((entry - (NUINT) 1) * size);
        NUINT home = (NUINT) (catrl_hash(key, size) % bcapacity);
        if (catrl_hash_distance(home, hole, bcapacity) < catrl_hash_distance(home, current, bcapacity)) {
            catrl_hash_write(buckets, hole, entry);
            hole = current;
        }
        current = catrl_hash_next(current, bcapacity);
        entry = catrl_hash_bucket(buckets, current);
    }
    catrl_hash_write(buckets, hole, 0);
}

/* Initialized index and writable dense storage. Return the removed
 * dense index or count if absent. Lookup precedes mutation, permitting
 * a search key that identifies a live member. Move the last key into
 * the removed slot and correct its bucket; caller updates the count
 * and any associated values. */
static NUINT catrl_hash_remove(POINTERW buckets, NUINT bcapacity, POINTERW data, NUINT size, NUINT count, POINTER key) {
    if (count == 0) return count;
    NUINT bucket = catrl_hash_probe(buckets, bcapacity, data, size, key);
    NUINT entry = catrl_hash_bucket(buckets, bucket);
    if (entry == 0) return count;
    NUINT index = (NUINT) (entry - (NUINT) 1);
    NUINT last = (NUINT) (count - (NUINT) 1);
    catrl_hash_erase(buckets, bcapacity, data, size, bucket);
    if (index != last) {
        BYTE* source = (BYTE*) data + (NUINT) (last * size);
        NUINT moved = catrl_hash_probe(buckets, bcapacity, data, size, source);
        catrl_hash_write(buckets, moved, entry);
        catrl_store((BYTE*) data + (NUINT) (index * size), source, size);
    }
    return index;
}

/* Initialized index; nonzero count requires live bucket storage. */
static void catrl_hash_clear(POINTERW buckets, NUINT bcapacity, NUINT* count) {
    if (*count != 0) {
        catmm_fill(buckets, 0, (NUINT) (bcapacity * (NUINT) sizeof(NUINT)));
        *count = 0;
    }
}

/* Check the byte extent for exactly two buckets per element without
 * multiplying until the result is representable. */
static inline BOOL catrl_hash_capacity_fits(NUINT capacity) {
    return capacity <= (NUINT) (CATALYST_NUINT_MAX / (NUINT) sizeof(NUINT) / (NUINT) 2) ? TRUE : FALSE;
}

/* Valid new bucket extent and count distinct dense keys. A zero bucket
 * count requires zero live keys. No allocation or reporting here. */
static void catrl_hash_rebuild(POINTERW buckets, NUINT bcapacity, POINTER data, NUINT size, NUINT count) {
    if (bcapacity == 0) return;
    catmm_fill(buckets, 0, (NUINT) (bcapacity * (NUINT) sizeof(NUINT)));
    for (NUINT index = 0; index < count; ++index) {
        const BYTE* key = (const BYTE*) data + (NUINT) (index * size);
        NUINT bucket = (NUINT) (catrl_hash(key, size) % bcapacity);
        while (catrl_hash_bucket(buckets, bucket) != 0) {
            bucket = catrl_hash_next(bucket, bcapacity);
        }
        catrl_hash_write(buckets, bucket, (NUINT) (index + (NUINT) 1));
    }
}

/* -------------------------------------------------------------------------------------------------
 * C API :: HASHSET OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_hashset_init(CATRLLIB_HASHSET* set, CATALYST_NUINT size) {
    set->data = 0;
    set->buckets = 0;
    set->size = size;
    set->cardinality = 0;
    set->capacity = 0;
    set->bcapacity = 0;
}

CATALYST_BOOL catrl_hashset_add(CATRLLIB_HASHSET* set, CATMMLIB_POINTER element) {
    NUINT bucket = catrl_hash_probe(set->buckets, set->bcapacity, set->data, set->size, element);
    if (catrl_hash_bucket(set->buckets, bucket) != 0) return FALSE;
    catrl_store((BYTE*) set->data + (NUINT) (set->cardinality * set->size), element, set->size);
    ++set->cardinality;
    catrl_hash_write(set->buckets, bucket, set->cardinality);
    return TRUE;
}

CATALYST_BOOL catrl_hashset_remove(CATRLLIB_HASHSET* set, CATMMLIB_POINTER element) {
    NUINT index = catrl_hash_remove(set->buckets, set->bcapacity, set->data, set->size, set->cardinality, element);
    if (index == set->cardinality) return FALSE;
    --set->cardinality;
    return TRUE;
}

CATALYST_BOOL catrl_hashset_contains(const CATRLLIB_HASHSET* set, CATMMLIB_POINTER element) {
    return catrl_hash_find(set->buckets, set->bcapacity, set->data, set->size, set->cardinality, element) != set->cardinality ? TRUE : FALSE;
}

void catrl_hashset_clear(CATRLLIB_HASHSET* set) {
    catrl_hash_clear(set->buckets, set->bcapacity, &set->cardinality);
}

CATALYST_RESULT catrl_hashset_expand(CATRLLIB_HASHSET* set, CATALYST_NUINT capacity) {
    if (capacity <= set->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x42, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / set->size)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x42, 0x02, 0);
    }
    if (!catrl_hash_capacity_fits(capacity)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x42, 0x03, 0);
    }
    RESULT data = catrl_grow_storage(&set->data, (NUINT) (capacity * set->size));
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x42, 0x04, data.detail);
    }
    /* A failed later resize retains the old logical capacity and index.
     * Earlier successful resizes may have grown or moved dense storage. */
    NUINT target = (NUINT) (capacity * (NUINT) 2);
    if (target != set->bcapacity) {
        RESULT buckets = catmm_heap_realloc(&set->buckets, (NUINT) (target * (NUINT) sizeof(NUINT)));
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x42, 0x05, buckets.detail);
        }
        set->bcapacity = target;
        catrl_hash_rebuild(set->buckets, target, set->data, set->size, set->cardinality);
    }
    set->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x42, 0x06, 0);
}

CATALYST_RESULT catrl_hashset_shrink(CATRLLIB_HASHSET* set) {
    NUINT capacity = catrl_shrink_capacity(set->capacity, set->cardinality);
    if (capacity == set->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x43, 0x01, 0);
    }
    RESULT data = catmm_heap_realloc(&set->data, (NUINT) (capacity * set->size));
    if (!status_code_is_success(data.status)) {
        return RESULT(data.status, 0x43, 0x02, data.detail);
    }
    /* Commit the smaller safe capacity before later fallible operations.
     * The old index remains valid if its resize fails. */
    set->capacity = capacity;
    NUINT target = (NUINT) (capacity * (NUINT) 2);
    if (target != set->bcapacity) {
        RESULT buckets = catmm_heap_realloc(&set->buckets, (NUINT) (target * (NUINT) sizeof(NUINT)));
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x43, 0x03, buckets.detail);
        }
        set->bcapacity = target;
        catrl_hash_rebuild(set->buckets, target, set->data, set->size, set->cardinality);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x43, 0x04, 0);
}

CATALYST_RESULT catrl_hashset_free(CATRLLIB_HASHSET* set) {
    if (set->data == 0 && set->buckets == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x44, 0x01, 0);
    }
    if (set->data != 0) {
        RESULT data = catmm_heap_free(set->data);
        if (!status_code_is_success(data.status)) {
            return RESULT(data.status, 0x44, 0x02, data.detail);
        }
        set->data = 0;
        catrl_hash_clear(set->buckets, set->bcapacity, &set->cardinality);
        set->capacity = 0;
    }
    if (set->buckets != 0) {
        RESULT buckets = catmm_heap_free(set->buckets);
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x44, 0x03, buckets.detail);
        }
        set->buckets = 0;
        set->bcapacity = 0;
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x44, 0x04, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: HASHMAP OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

void catrl_hashmap_init(CATRLLIB_HASHMAP* map, CATALYST_NUINT ksize, CATALYST_NUINT vsize) {
    map->keys = 0;
    map->values = 0;
    map->buckets = 0;
    map->ksize = ksize;
    map->vsize = vsize;
    map->cardinality = 0;
    map->capacity = 0;
    map->bcapacity = 0;
}

CATALYST_BOOL catrl_hashmap_add(CATRLLIB_HASHMAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value) {
    NUINT bucket = catrl_hash_probe(map->buckets, map->bcapacity, map->keys, map->ksize, key);
    if (catrl_hash_bucket(map->buckets, bucket) != 0) return FALSE;
    catrl_store((BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), key, map->ksize);
    catrl_store((BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), value, map->vsize);
    ++map->cardinality;
    catrl_hash_write(map->buckets, bucket, map->cardinality);
    return TRUE;
}

CATALYST_BOOL catrl_hashmap_remove(CATRLLIB_HASHMAP* map, CATMMLIB_POINTER key) {
    NUINT index = catrl_hash_remove(map->buckets, map->bcapacity, map->keys, map->ksize, map->cardinality, key);
    if (index == map->cardinality) return FALSE;
    --map->cardinality;
    if (index != map->cardinality) {
        catrl_store((BYTE*) map->values + (NUINT) (index * map->vsize),
            (const BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), map->vsize);
    }
    return TRUE;
}

CATALYST_BOOL catrl_hashmap_contains(const CATRLLIB_HASHMAP* map, CATMMLIB_POINTER key) {
    return catrl_hash_find(map->buckets, map->bcapacity, map->keys, map->ksize, map->cardinality, key) != map->cardinality ? TRUE : FALSE;
}

CATMMLIB_POINTERW catrl_hashmap_get(CATRLLIB_HASHMAP* map, CATMMLIB_POINTER key) {
    NUINT index = catrl_hash_find(map->buckets, map->bcapacity, map->keys, map->ksize, map->cardinality, key);
    if (index == map->cardinality) return 0;
    return (BYTE*) map->values + (NUINT) (index * map->vsize);
}

CATALYST_BOOL catrl_hashmap_set(CATRLLIB_HASHMAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value) {
    NUINT index = catrl_hash_find(map->buckets, map->bcapacity, map->keys, map->ksize, map->cardinality, key);
    if (index == map->cardinality) return FALSE;
    catrl_store((BYTE*) map->values + (NUINT) (index * map->vsize), value, map->vsize);
    return TRUE;
}

void catrl_hashmap_clear(CATRLLIB_HASHMAP* map) {
    catrl_hash_clear(map->buckets, map->bcapacity, &map->cardinality);
}

CATALYST_RESULT catrl_hashmap_expand(CATRLLIB_HASHMAP* map, CATALYST_NUINT capacity) {
    if (capacity <= map->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x4C, 0x01, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / map->ksize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x4C, 0x02, 0);
    }
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / map->vsize)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x4C, 0x03, 0);
    }
    if (!catrl_hash_capacity_fits(capacity)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x4C, 0x04, 0);
    }
    RESULT keys = catrl_grow_storage(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x4C, 0x05, keys.detail);
    }
    RESULT values = catrl_grow_storage(&map->values, (NUINT) (capacity * map->vsize));
    if (!status_code_is_success(values.status)) {
        return RESULT(values.status, 0x4C, 0x06, values.detail);
    }
    /* A failed later resize retains the old logical capacity and index.
     * Earlier successful resizes may have grown or moved dense storage. */
    NUINT target = (NUINT) (capacity * (NUINT) 2);
    if (target != map->bcapacity) {
        RESULT buckets = catmm_heap_realloc(&map->buckets, (NUINT) (target * (NUINT) sizeof(NUINT)));
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x4C, 0x07, buckets.detail);
        }
        map->bcapacity = target;
        catrl_hash_rebuild(map->buckets, target, map->keys, map->ksize, map->cardinality);
    }
    map->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x4C, 0x08, 0);
}

CATALYST_RESULT catrl_hashmap_shrink(CATRLLIB_HASHMAP* map) {
    NUINT capacity = catrl_shrink_capacity(map->capacity, map->cardinality);
    if (capacity == map->capacity) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x4D, 0x01, 0);
    }
    RESULT keys = catmm_heap_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x4D, 0x02, keys.detail);
    }
    /* Commit the smaller safe capacity before later fallible operations.
     * The old index remains valid if its resize fails. */
    map->capacity = capacity;
    RESULT values = catmm_heap_realloc(&map->values, (NUINT) (capacity * map->vsize));
    if (!status_code_is_success(values.status)) {
        return RESULT(values.status, 0x4D, 0x03, values.detail);
    }
    NUINT target = (NUINT) (capacity * (NUINT) 2);
    if (target != map->bcapacity) {
        RESULT buckets = catmm_heap_realloc(&map->buckets, (NUINT) (target * (NUINT) sizeof(NUINT)));
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x4D, 0x04, buckets.detail);
        }
        map->bcapacity = target;
        catrl_hash_rebuild(map->buckets, target, map->keys, map->ksize, map->cardinality);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x4D, 0x05, 0);
}

CATALYST_RESULT catrl_hashmap_free(CATRLLIB_HASHMAP* map) {
    if (map->keys == 0 && map->values == 0 && map->buckets == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x4E, 0x01, 0);
    }
    if (map->keys != 0) {
        RESULT keys = catmm_heap_free(map->keys);
        if (!status_code_is_success(keys.status)) {
            return RESULT(keys.status, 0x4E, 0x02, keys.detail);
        }
        map->keys = 0;
        catrl_hash_clear(map->buckets, map->bcapacity, &map->cardinality);
        map->capacity = 0;
    }
    if (map->values != 0) {
        RESULT values = catmm_heap_free(map->values);
        if (!status_code_is_success(values.status)) {
            return RESULT(values.status, 0x4E, 0x03, values.detail);
        }
        map->values = 0;
    }
    if (map->buckets != 0) {
        RESULT buckets = catmm_heap_free(map->buckets);
        if (!status_code_is_success(buckets.status)) {
            return RESULT(buckets.status, 0x4E, 0x04, buckets.detail);
        }
        map->buckets = 0;
        map->bcapacity = 0;
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x4E, 0x05, 0);
}
