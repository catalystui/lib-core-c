#include "catmmlib.hpp"
#include "catrllib.h"

using namespace catalyst;

/* Contexts follow public declaration order (including non-reporting methods).
 * Set expand/shrink/free: 06/07/08; Map expand/shrink/free: 10/11/12;
 * Array expand/shrink/free: 1B/1C/1D; File get/free: 1F/20; Stream next: 22.
 *
 * Operations, assigned before implementation:
 * Single-buffer expand: 01 no-op, 02 extent overflow, 03 resize failure, 04 success.
 * Single-buffer shrink: 01 no-op, 02 resize failure, 03 success.
 * Single-buffer free: 01 no-op, 02 release failure, 03 success.
 * Map expand: 01 no-op, 02 key overflow, 03 value overflow, 04 key resize failure,
 *             05 value resize failure, 06 success.
 * Map shrink: 01 no-op, 02 key resize failure, 03 value resize failure, 04 success.
 * Map free: 01 key release failure, 02 value release failure, 03 no-op, 04 success.
 * File free: 01 no-op, 02 metadata release failure, 03 data release failure, 04 success.
 * File get and Stream next: 01 callback result, including success and no-op.
 * Dependency status and detail are retained; callback signatures use the same
 * argument indices as their reporting boundary.
 */

/* Nonzero size; count * size bytes are readable when count is nonzero, and
 * element provides size readable bytes. Return count for ordinary absence.
 * All stored extents were checked when capacity was established.
 */
static NUINT catrl_find(POINTER data, NUINT count, NUINT size, POINTER element) {
    const BYTE* current = (const BYTE*) data;
    NUINT index = 0;
    while (index < count) {
        if (catmm_compare(current, element, size) == 0) return index;
        current += size;
        ++index;
    }
    return count;
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
    catmm_copy((BYTE*) set->data + (NUINT) (set->cardinality * set->size), element, set->size);
    ++set->cardinality;
    return TRUE;
}

CATALYST_BOOL catrl_set_remove(CATRLLIB_SET* set, CATMMLIB_POINTER element) {
    NUINT index = catrl_find(set->data, set->cardinality, set->size, element);
    if (index == set->cardinality) return FALSE;
    --set->cardinality;
    if (index != set->cardinality) {
        catmm_copy((BYTE*) set->data + (NUINT) (index * set->size),
            (const BYTE*) set->data + (NUINT) (set->cardinality * set->size), set->size);
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
    RESULT resize = catmm_realloc(&set->data, (NUINT) (capacity * set->size));
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
    RESULT resize = catmm_realloc(&set->data, (NUINT) (capacity * set->size));
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
    RESULT release = catmm_free(set->data);
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
    catmm_copy((BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), key, map->ksize);
    catmm_copy((BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), value, map->vsize);
    ++map->cardinality;
    return TRUE;
}

CATALYST_BOOL catrl_map_remove(CATRLLIB_MAP* map, CATMMLIB_POINTER key) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return FALSE;
    --map->cardinality;
    if (index != map->cardinality) {
        catmm_copy((BYTE*) map->keys + (NUINT) (index * map->ksize),
            (const BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), map->ksize);
        catmm_copy((BYTE*) map->values + (NUINT) (index * map->vsize),
            (const BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), map->vsize);
    }
    return TRUE;
}

CATALYST_BOOL catrl_map_contains(const CATRLLIB_MAP* map, CATMMLIB_POINTER key) {
    return catrl_find(map->keys, map->cardinality, map->ksize, key) != map->cardinality ? TRUE : FALSE;
}

CATALYST_BOOL catrl_map_get(const CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTERW value) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return FALSE;
    catmm_copy(value, (const BYTE*) map->values + (NUINT) (index * map->vsize), map->vsize);
    return TRUE;
}

CATALYST_BOOL catrl_map_set(CATRLLIB_MAP* map, CATMMLIB_POINTER key, CATMMLIB_POINTER value) {
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return FALSE;
    catmm_copy((BYTE*) map->values + (NUINT) (index * map->vsize), value, map->vsize);
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
    RESULT keys = catmm_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x10, 0x04, keys.detail);
    }
    /* Commit the logical capacity only when both buffers can hold it. If the
     * second resize fails, the larger key buffer is still owned by the map.
     */
    RESULT values = catmm_realloc(&map->values, (NUINT) (capacity * map->vsize));
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
    RESULT keys = catmm_realloc(&map->keys, (NUINT) (capacity * map->ksize));
    if (!status_code_is_success(keys.status)) {
        return RESULT(keys.status, 0x11, 0x02, keys.detail);
    }
    /* Publish the smaller safe capacity before the second fallible resize.
     * Both buffers still contain every live pair on either outcome.
     */
    map->capacity = capacity;
    RESULT values = catmm_realloc(&map->values, (NUINT) (capacity * map->vsize));
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
        RESULT keys = catmm_free(map->keys);
        if (!status_code_is_success(keys.status)) {
            return RESULT(keys.status, 0x12, 0x01, keys.detail);
        }
        map->keys = 0;
        map->cardinality = 0;
        map->capacity = 0;
    }
    if (map->values != 0) {
        RESULT values = catmm_free(map->values);
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
    catmm_copy((BYTE*) array->data + (NUINT) (array->length * array->size), element, array->size);
    ++array->length;
}

void catrl_array_insert(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element) {
    BYTE* destination = (BYTE*) array->data + (NUINT) (index * array->size);
    if (index < array->length) {
        catmm_move(destination + array->size, destination, (NUINT) ((array->length - index) * array->size));
    }
    catmm_copy(destination, element, array->size);
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

void catrl_array_get(const CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTERW element) {
    catmm_copy(element, (const BYTE*) array->data + (NUINT) (index * array->size), array->size);
}

void catrl_array_set(CATRLLIB_ARRAY* array, CATALYST_NUINT index, CATMMLIB_POINTER element) {
    catmm_copy((BYTE*) array->data + (NUINT) (index * array->size), element, array->size);
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
    RESULT resize = catmm_realloc(&array->data, (NUINT) (capacity * array->size));
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
    RESULT resize = catmm_realloc(&array->data, (NUINT) (capacity * array->size));
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
    RESULT release = catmm_free(array->data);
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

void catrl_file_init(CATRLLIB_FILE* file, CATRLLIB_ARRAY* metadata, CATRLLIB_ARRAY* data,
    CATALYST_RESULT (*resolver)(const CATRLLIB_FILE*, CATALYST_NUINT, CATRLLIB_ARRAY*)) {
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

void catrl_stream_init(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW state,
    CATALYST_RESULT (*next)(CATRLLIB_STREAM*, CATMMLIB_POINTERW*)) {
    stream->state = state;
    stream->next = next;
}

CATALYST_RESULT catrl_stream_next(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW* output) {
    RESULT production = stream->next(stream, output);
    return RESULT(production.status, 0x22, 0x01, production.detail);
}
