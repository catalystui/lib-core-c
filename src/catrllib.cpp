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
    RESULT resize = catmm_heap_realloc(&set->data, (NUINT) (capacity * set->size));
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
    RESULT keys = catmm_heap_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x10, 0x04, keys.detail);
    }
    /* Commit the logical capacity only when both buffers can hold it. If the
     * second resize fails, the larger key buffer is still owned by the map.
     */
    RESULT values = catmm_heap_realloc(&map->values, (NUINT) (capacity * map->vsize));
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
    RESULT resize = catmm_heap_realloc(&array->data, (NUINT) (capacity * array->size));
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
    catmm_copy((BYTE*) stream->state + ssize, &memory, headerSize);
    /* Zero-filled storage need not represent null pointers. */
    for (NUINT index = 0; index < lifetime; ++index) {
        catmm_copy(catrl_stream_slot(stream, index), &memory.spare, slotSize);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x21, 0x05, 0);
}

/* Requires an active Stream and output slot outside its object and allocations. Only producer SUCCESS promises all csize bytes and publishes a chunk; all other producer outcomes preserve output/lifetimes. Producer state is not rolled back. Post-publication shrink diagnostics leave output valid and lifetimes advanced, including fatal diagnostics. */
CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATALYST_NUINT csize, CATMMLIB_POINTERW* output) {
    CATRLLIB_STREAM_MEMORY memory;
    BYTE* bookkeeping = (BYTE*) stream->state + stream->ssize;
    catmm_copy(&memory, bookkeeping, (NUINT) sizeof(memory));
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
    }
    if (capacity != memory.spare.capacity) {
        RESULT resize = catmm_heap_realloc(&memory.spare.data, capacity);
        if (!status_code_is_success(resize.status)) {
            return RESULT(resize.status, 0x22, 0x01, resize.detail);
        }
        memory.spare.capacity = capacity;
        /* Keep replacement ownership reachable before invoking producer code. */
        catmm_copy(bookkeeping, &memory, (NUINT) sizeof(memory));
    }
    RESULT production = stream->next(stream, memory.spare.data, csize);
    if (production.status == STATUS_CODE_SUCCESS) {
        CATRLLIB_STREAM_CHUNK expired;
        BYTE* slot = catrl_stream_slot(stream, memory.cursor);
        catmm_copy(&expired, slot, (NUINT) sizeof(expired));
        catmm_copy(slot, &memory.spare, (NUINT) sizeof(memory.spare));
        *output = memory.spare.data;
        memory.spare = expired;
        ++memory.cursor;
        if (memory.cursor == memory.lifetime) memory.cursor = 0;
        catmm_copy(bookkeeping, &memory, (NUINT) sizeof(memory));
        /* Only the expired chunk is now scratch; never resize the published chunk or a retained slot. Quarter utilization provides hysteresis before halving. */
        if (required <= (NUINT) (memory.spare.capacity / (NUINT) 4)) {
            NUINT smaller = (NUINT) (memory.spare.capacity / (NUINT) 2);
            RESULT shrink = catmm_heap_realloc(&memory.spare.data, smaller);
            if (!status_code_is_success(shrink.status)) {
                if (status_code_is_fatal(shrink.status)) {
                    return RESULT(shrink.status, 0x22, 0x05, shrink.detail);
                }
                return RESULT(STATUS_CODE_WARNING, 0x22, 0x04, shrink.status);
            }
            memory.spare.capacity = smaller;
            catmm_copy(bookkeeping, &memory, (NUINT) sizeof(memory));
        }
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
    catmm_copy(&memory, bookkeeping, (NUINT) sizeof(memory));
    for (NUINT index = 0; index < memory.lifetime; ++index) {
        CATRLLIB_STREAM_CHUNK chunk;
        BYTE* slot = catrl_stream_slot(stream, index);
        catmm_copy(&chunk, slot, (NUINT) sizeof(chunk));
        if (chunk.data != 0) {
            RESULT release = catmm_heap_free(chunk.data);
            if (!status_code_is_success(release.status)) {
                return RESULT(release.status, 0x23, 0x02, release.detail);
            }
            chunk.data = 0;
            chunk.capacity = 0;
            catmm_copy(slot, &chunk, (NUINT) sizeof(chunk));
        }
    }
    if (memory.spare.data != 0) {
        RESULT release = catmm_heap_free(memory.spare.data);
        if (!status_code_is_success(release.status)) {
            return RESULT(release.status, 0x23, 0x03, release.detail);
        }
        memory.spare.data = 0;
        memory.spare.capacity = 0;
        catmm_copy(bookkeeping, &memory, (NUINT) sizeof(memory));
    }
    RESULT release = catmm_heap_free(stream->state);
    if (!status_code_is_success(release.status)) {
        return RESULT(release.status, 0x23, 0x04, release.detail);
    }
    stream->state = 0;
    stream->ssize = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x23, 0x05, 0);
}
