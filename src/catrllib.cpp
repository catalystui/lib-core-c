#include "catalyst.hpp"
#include "catrllib.h"
#include "catmmlib.h"

using namespace catalyst;

/* Private storage shared by the four collection implementations. */
struct catrl_storage {
    BYTE* bytes;
    NUINT stride;
    NUINT count;
    NUINT capacity;
};

struct CATALYST_ARRAY { catrl_storage storage; };
struct CATALYST_SET { catrl_storage storage; };
struct CATALYST_MAP {
    catrl_storage storage;
    NUINT key_size;
};
struct CATALYST_STACK { catrl_storage storage; };

#define CATRL_ELEMENT_AT(storage, index) ((storage)->bytes + (index) * (storage)->stride)

// Context IDs follow CATRLLIB public declaration order (0x01 through 0x1D).
// Private helpers use 0x1E through 0x21.
// A caller reports its own operation and uses the helper opcode as failure detail.
static void* create_storage(NUINT objectSize, NUINT stride, NUINT count, CATALYST_RESULT* result) {
    void* object = 0;
    void* bytes = 0;
    RESULT status;
    if (count > CATALYST_NUINT_MAX / stride) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1E, 0x01, 0);
        return 0;
    }
    catmm_alloc(&object, objectSize, &status);
    if (!status_code_is_success(status.status)) {
        if (result != 0) *result = RESULT(status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status, 0x1E, 0x02, status.operation);
        return 0;
    }
    if (count != 0) {
        catmm_alloc(&bytes, count * stride, &status);
        if (!status_code_is_success(status.status)) {
            RESULT cleanup;
            catmm_free(object, &cleanup);
            if (!status_code_is_success(cleanup.status) && !status_code_is_fatal(status.status)) {
                if (result != 0) *result = RESULT(cleanup.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : cleanup.status, 0x1E, 0x03, cleanup.operation);
                return 0;
            }
            if (result != 0) *result = RESULT(status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status, 0x1E, 0x04, status.operation);
            return 0;
        }
    }
    catrl_storage* storage = (catrl_storage*) object;
    storage->bytes = (BYTE*) bytes;
    storage->stride = stride;
    storage->count = count;
    storage->capacity = count;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
    return object;
}

static void destroy_storage(void* object, catrl_storage* storage, CATALYST_RESULT* result) {
    RESULT bytesStatus;
    RESULT objectStatus;
    if (storage->bytes != 0) {
        catmm_free(storage->bytes, &bytesStatus);
    }
    catmm_free(object, &objectStatus);
    if (!status_code_is_success(bytesStatus.status) && (status_code_is_fatal(bytesStatus.status) || !status_code_is_fatal(objectStatus.status))) {
        if (result != 0) *result = RESULT(bytesStatus.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : bytesStatus.status, 0x1F, 0x01, bytesStatus.operation);
    } else if (!status_code_is_success(objectStatus.status)) {
        if (result != 0) *result = RESULT(objectStatus.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : objectStatus.status, 0x1F, 0x02, objectStatus.operation);
    } else {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
    }
}

/* Reserve room for one more element, with bounded geometric growth. */
static BOOL grow(catrl_storage* storage, CATALYST_RESULT* result) {
    NUINT limit = CATALYST_NUINT_MAX / storage->stride;
    if (storage->count == limit) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x20, 0x01, 0);
        return FALSE;
    }
    NUINT capacity;
    if (storage->capacity == 0) {
        capacity = 1;
    } else if (storage->capacity > limit / 2) {
        capacity = limit;
    } else {
        capacity = storage->capacity * 2;
    }
    void* bytes = storage->bytes;
    RESULT status;
    catmm_realloc(&bytes, capacity * storage->stride, &status);
    if (!status_code_is_success(status.status)) {
        // A failed old-block release can invalidate the collection's storage.
        if (result != 0) *result = RESULT(status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT || (status.status == STATUS_CODE_ERROR_INVALID_STATE && status.operation == 0x06) ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status, 0x20, 0x02, status.operation);
        return FALSE;
    }
    storage->bytes = (BYTE*) bytes;
    storage->capacity = capacity;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
    return TRUE;
}

/* Return count when the key or element is absent. */
static inline NUINT find(const catrl_storage* storage, const void* value, NUINT size, CATALYST_RESULT* result) {
    for (NUINT i = 0; i < storage->count; i++) {
        SBYTE comparison;
        RESULT compareResult;
        catmm_compare(CATRL_ELEMENT_AT(storage, i), value, size, &comparison, &compareResult);
        if (!status_code_is_success(compareResult.status)) {
            if (result != 0) *result = RESULT(compareResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : compareResult.status, 0x21, 0x01, compareResult.operation);
            return storage->count;
        }
        if (comparison == 0) {
            if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x02, 0);
            return i;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
    return storage->count;
}

/* ARRAY */
void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result) {
    if (array == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x01, 0);
        return;
    }
    *array = 0;
    if (element_size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x02, 1);
        return;
    }
    RESULT storageResult;
    *array = (CATALYST_ARRAY*) create_storage(sizeof(CATALYST_ARRAY), element_size, length, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x01, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result) {
    if (array == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x01, 0);
        return;
    }
    RESULT storageResult;
    destroy_storage(array, &array->storage, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x02, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (array == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 0);
        return;
    }
    if (index >= array->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x02, 1);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x03, 2);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&array->storage, index), array->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x03, 0x04, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result) {
    if (array == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x01, 0);
        return;
    }
    if (index >= array->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x02, 1);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x03, 2);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(CATRL_ELEMENT_AT(&array->storage, index), value, array->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x04, 0x04, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result) {
    if (array == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x01, 0);
        return;
    }
    if (length == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x02, 1);
        return;
    }
    *length = array->storage.count;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

/* SET */
void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x01, 0);
        return;
    }
    *set = 0;
    if (element_size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x02, 1);
        return;
    }
    RESULT storageResult;
    *set = (CATALYST_SET*) create_storage(sizeof(CATALYST_SET), element_size, 0, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x06, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 0);
        return;
    }
    RESULT storageResult;
    destroy_storage(set, &set->storage, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x07, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x02, 1);
        return;
    }
    RESULT findResult;
    NUINT index = find(&set->storage, value, set->storage.stride, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x08, 0x03, findResult.operation);
        return;
    }
    if (index != set->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x04, 0);
        return;
    }
    if (set->storage.count == set->storage.capacity) {
        RESULT growthResult;
        if (!grow(&set->storage, &growthResult)) {
            if (result != 0) *result = RESULT(growthResult.status, 0x08, 0x05, growthResult.operation);
            return;
        }
    }
    {
        RESULT copyResult;
        catmm_copy(CATRL_ELEMENT_AT(&set->storage, set->storage.count), value, set->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x08, 0x06, copyResult.operation);
            return;
        }
    }
    set->storage.count++;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x07, 0);
}

void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x02, 1);
        return;
    }
    RESULT findResult;
    NUINT index = find(&set->storage, value, set->storage.stride, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x09, 0x03, findResult.operation);
        return;
    }
    if (index == set->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x04, 0);
        return;
    }
    set->storage.count--;
    if (index != set->storage.count) {
        {
            RESULT copyResult;
            catmm_copy(CATRL_ELEMENT_AT(&set->storage, index), CATRL_ELEMENT_AT(&set->storage, set->storage.count), set->storage.stride, &copyResult);
            if (!status_code_is_success(copyResult.status)) {
                if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x09, 0x05, copyResult.operation);
                return;
            }
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x06, 0);
}

void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x02, 1);
        return;
    }
    if (contains == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x03, 2);
        return;
    }
    RESULT findResult;
    NUINT index = find(&set->storage, value, set->storage.stride, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x0A, 0x04, findResult.operation);
        return;
    }
    *contains = index != set->storage.count ? TRUE : FALSE;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x01, 0);
        return;
    }
    if (count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x02, 1);
        return;
    }
    *count = set->storage.count;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x01, 0);
        return;
    }
    if (index >= set->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x02, 1);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x03, 2);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&set->storage, index), set->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x0C, 0x04, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0D, 0x01, 0);
        return;
    }
    if (set->storage.count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x02, 0);
        return;
    }
    set->storage.count = 0;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

/* MAP */
void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x01, 0);
        return;
    }
    *map = 0;
    if (key_size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x02, 1);
        return;
    }
    if (value_size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x03, 2);
        return;
    }
    if (key_size > CATALYST_NUINT_MAX - value_size) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0E, 0x04, 0);
        return;
    }
    RESULT storageResult;
    *map = (CATALYST_MAP*) create_storage(sizeof(CATALYST_MAP), key_size + value_size, 0, &storageResult);
    if (*map != 0) (*map)->key_size = key_size;
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x0E, 0x05, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0F, 0x01, 0);
        return;
    }
    RESULT storageResult;
    destroy_storage(map, &map->storage, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x0F, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x01, 0);
        return;
    }
    if (key == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x02, 1);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x03, 2);
        return;
    }
    RESULT findResult;
    NUINT index = find(&map->storage, key, map->key_size, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x10, 0x04, findResult.operation);
        return;
    }
    if (index == map->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x10, 0x05, 0);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&map->storage, index) + map->key_size, map->storage.stride - map->key_size, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x10, 0x06, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x07, 0);
}

void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x01, 0);
        return;
    }
    if (key == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x02, 1);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x03, 2);
        return;
    }
    BYTE* entry;
    RESULT findResult;
    NUINT index = find(&map->storage, key, map->key_size, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x11, 0x04, findResult.operation);
        return;
    }
    if (index == map->storage.count) {
        if (map->storage.count == map->storage.capacity) {
            RESULT growthResult;
            if (!grow(&map->storage, &growthResult)) {
                if (result != 0) *result = RESULT(growthResult.status, 0x11, 0x05, growthResult.operation);
                return;
            }
        }
        entry = CATRL_ELEMENT_AT(&map->storage, index);
        {
            RESULT copyResult;
            catmm_copy(entry, key, map->key_size, &copyResult);
            if (!status_code_is_success(copyResult.status)) {
                if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x11, 0x06, copyResult.operation);
                return;
            }
        }
        map->storage.count++;
    } else {
        entry = CATRL_ELEMENT_AT(&map->storage, index);
    }
    {
        RESULT copyResult;
        catmm_copy(entry + map->key_size, value, map->storage.stride - map->key_size, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x11, 0x07, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x08, 0);
}

void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x01, 0);
        return;
    }
    if (key == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x02, 1);
        return;
    }
    RESULT findResult;
    NUINT index = find(&map->storage, key, map->key_size, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x12, 0x03, findResult.operation);
        return;
    }
    if (index == map->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x04, 0);
        return;
    }
    map->storage.count--;
    if (index != map->storage.count) {
        {
            RESULT copyResult;
            catmm_copy(CATRL_ELEMENT_AT(&map->storage, index), CATRL_ELEMENT_AT(&map->storage, map->storage.count), map->storage.stride, &copyResult);
            if (!status_code_is_success(copyResult.status)) {
                if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x12, 0x05, copyResult.operation);
                return;
            }
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x06, 0);
}

void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x01, 0);
        return;
    }
    if (key == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x02, 1);
        return;
    }
    if (contains == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x03, 2);
        return;
    }
    RESULT findResult;
    NUINT index = find(&map->storage, key, map->key_size, &findResult);
    if (!status_code_is_success(findResult.status)) {
        if (result != 0) *result = RESULT(findResult.status, 0x13, 0x04, findResult.operation);
        return;
    }
    *contains = index != map->storage.count ? TRUE : FALSE;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x01, 0);
        return;
    }
    if (count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x02, 1);
        return;
    }
    *count = map->storage.count;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x01, 0);
        return;
    }
    if (index >= map->storage.count) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x02, 1);
        return;
    }
    if (key == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x03, 2);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x04, 3);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(key, CATRL_ELEMENT_AT(&map->storage, index), map->key_size, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x15, 0x05, copyResult.operation);
            return;
        }
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&map->storage, index) + map->key_size, map->storage.stride - map->key_size, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x15, 0x06, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x07, 0);
}

void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x16, 0x01, 0);
        return;
    }
    if (map->storage.count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x02, 0);
        return;
    }
    map->storage.count = 0;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

/* STACK */
void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x01, 0);
        return;
    }
    *stack = 0;
    if (element_size == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x02, 1);
        return;
    }
    RESULT storageResult;
    *stack = (CATALYST_STACK*) create_storage(sizeof(CATALYST_STACK), element_size, 0, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x17, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x18, 0x01, 0);
        return;
    }
    RESULT storageResult;
    destroy_storage(stack, &stack->storage, &storageResult);
    if (result != 0) *result = RESULT(storageResult.status, status_code_is_success(storageResult.status) ? 0 : 0x18, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 0x02, 1);
        return;
    }
    if (stack->storage.count == stack->storage.capacity) {
        RESULT growthResult;
        if (!grow(&stack->storage, &growthResult)) {
            if (result != 0) *result = RESULT(growthResult.status, 0x19, 0x03, growthResult.operation);
            return;
        }
    }
    {
        RESULT copyResult;
        catmm_copy(CATRL_ELEMENT_AT(&stack->storage, stack->storage.count), value, stack->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x19, 0x04, copyResult.operation);
            return;
        }
    }
    stack->storage.count++;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1A, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1A, 0x02, 1);
        return;
    }
    if (stack->storage.count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x1A, 0x03, 0);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&stack->storage, stack->storage.count - 1), stack->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x1A, 0x04, copyResult.operation);
            return;
        }
    }
    stack->storage.count--;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1B, 0x01, 0);
        return;
    }
    if (value == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1B, 0x02, 1);
        return;
    }
    if (stack->storage.count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x1B, 0x03, 0);
        return;
    }
    {
        RESULT copyResult;
        catmm_copy(value, CATRL_ELEMENT_AT(&stack->storage, stack->storage.count - 1), stack->storage.stride, &copyResult);
        if (!status_code_is_success(copyResult.status)) {
            if (result != 0) *result = RESULT(copyResult.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : copyResult.status, 0x1B, 0x04, copyResult.operation);
            return;
        }
    }
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x05, 0);
}

void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1C, 0x01, 0);
        return;
    }
    if (count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1C, 0x02, 1);
        return;
    }
    *count = stack->storage.count;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1D, 0x01, 0);
        return;
    }
    if (stack->storage.count == 0) {
        if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS_NOOP, 0, 0x02, 0);
        return;
    }
    stack->storage.count = 0;
    if (result != 0) *result = RESULT(STATUS_CODE_SUCCESS, 0, 0x03, 0);
}

#undef CATRL_ELEMENT_AT
