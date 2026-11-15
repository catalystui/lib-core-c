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

static void set_result(CATALYST_RESULT* result, STATUS_CODE status, OPERATION_CODE operation, DETAIL_CODE detail) {
    if (result != 0) *result = RESULT(status, 0, operation, detail);
}

static void invalid_argument(CATALYST_RESULT* result, DETAIL_CODE index) {
    set_result(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, index);
}

static void success(CATALYST_RESULT* result) {
    set_result(result, STATUS_CODE_SUCCESS, 0, 0);
}

static void noop(CATALYST_RESULT* result) {
    set_result(result, STATUS_CODE_SUCCESS_NOOP, 0, 0);
}

static void not_found(CATALYST_RESULT* result) {
    set_result(result, STATUS_CODE_ERROR_NOT_FOUND, 0, 0);
}

static void overflow(CATALYST_RESULT* result) {
    set_result(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0, 0);
}

static void dependency_failure(CATALYST_RESULT* result, OPERATION_CODE operation,
                               STATUS_CODE status) {
    if (status == STATUS_CODE_ERROR_BUFFER_OVERFLOW) {
        overflow(result);
    } else if (operation == 2 && status == STATUS_CODE_ERROR_INVALID_ARGUMENT) {
        /* CATMMLIB cannot guarantee the old allocation after a release failure. */
        set_result(result, STATUS_CODE_FATAL_INVARIANT_VIOLATION, operation, status);
    } else {
        set_result(result, STATUS_CODE_ERROR_DEPENDENCY_FAILURE, operation, status);
    }
}

static void* create_storage(NUINT object_size, NUINT stride,
                            NUINT count, CATALYST_RESULT* result) {
    void* object = 0;
    void* bytes = 0;
    catrl_storage* storage;
    RESULT status;
    RESULT cleanup;
    if (count > CATALYST_NUINT_MAX / stride) {
        overflow(result);
        return 0;
    }
    catmm_alloc(&object, object_size, &status);
    if (!status_code_is_success(status.status)) {
        dependency_failure(result, 1, status.status);
        return 0;
    }
    if (count != 0) {
        catmm_alloc(&bytes, count * stride, &status);
        if (!status_code_is_success(status.status)) {
            catmm_free(object, &cleanup);
            if (!status_code_is_success(cleanup.status)) {
                dependency_failure(result, 3, cleanup.status);
            } else {
                dependency_failure(result, 1, status.status);
            }
            return 0;
        }
    }
    storage = static_cast<catrl_storage*>(object);
    storage->bytes = static_cast<BYTE*>(bytes);
    storage->stride = stride;
    storage->count = count;
    storage->capacity = count;
    success(result);
    return object;
}

static void destroy_storage(void* object, catrl_storage* storage, CATALYST_RESULT* result) {
    RESULT bytes_status;
    RESULT object_status;
    if (storage->bytes != 0) {
        catmm_free(storage->bytes, &bytes_status);
    }
    catmm_free(object, &object_status);
    if (!status_code_is_success(bytes_status.status)) {
        dependency_failure(result, 3, bytes_status.status);
    } else if (!status_code_is_success(object_status.status)) {
        dependency_failure(result, 3, object_status.status);
    } else {
        success(result);
    }
}

/* Reserve room for one more element, with bounded geometric growth. */
static BOOL grow(catrl_storage* storage, CATALYST_RESULT* result) {
    NUINT limit;
    NUINT capacity;
    void* bytes;
    RESULT status;
    if (storage->count < storage->capacity) return TRUE;
    limit = CATALYST_NUINT_MAX / storage->stride;
    if (storage->count == limit) {
        overflow(result);
        return FALSE;
    }
    if (storage->capacity == 0) {
        capacity = 1;
    } else if (storage->capacity > limit / 2) {
        capacity = limit;
    } else {
        capacity = storage->capacity * 2;
    }
    bytes = storage->bytes;
    catmm_realloc(&bytes, capacity * storage->stride, &status);
    if (!status_code_is_success(status.status)) {
        dependency_failure(result, 2, status.status);
        return FALSE;
    }
    storage->bytes = static_cast<BYTE*>(bytes);
    storage->capacity = capacity;
    return TRUE;
}

static BYTE* element_at(const catrl_storage* storage, NUINT index) {
    return storage->bytes + index * storage->stride;
}

/* Copy and compare byte representations without C library functions. */
static void copy_bytes(void* destination, const void* source, NUINT size) {
    BYTE* output = static_cast<BYTE*>(destination);
    const BYTE* input = static_cast<const BYTE*>(source);
    for (NUINT i = 0; i < size; i++) {
        output[i] = input[i];
    }
}

static BOOL bytes_equal(const void* first, const void* second, NUINT size) {
    const BYTE* a = static_cast<const BYTE*>(first);
    const BYTE* b = static_cast<const BYTE*>(second);
    for (NUINT i = 0; i < size; i++) {
        if (a[i] != b[i]) return FALSE;
    }
    return TRUE;
}

/* Return count when the key or element is absent. */
static NUINT find(const catrl_storage* storage, const void* value, NUINT size) {
    NUINT i;
    for (i = 0; i < storage->count; i++) {
        if (bytes_equal(element_at(storage, i), value, size)) return i;
    }
    return storage->count;
}

/* Set and map enumeration is unordered: move the last entry into the gap. */
static void remove_at(catrl_storage* storage, NUINT index) {
    storage->count--;
    if (index != storage->count) {
        copy_bytes(element_at(storage, index), element_at(storage, storage->count), storage->stride);
    }
}

static void clear(catrl_storage* storage, CATALYST_RESULT* result) {
    if (storage->count == 0) {
        noop(result);
    } else {
        storage->count = 0;
        success(result);
    }
}

/* ARRAY */

void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result) {
    if (array == 0) { invalid_argument(result, 0); return; }
    *array = 0;
    if (element_size == 0) { invalid_argument(result, 1); return; }
    *array = static_cast<CATALYST_ARRAY*>(create_storage(sizeof(CATALYST_ARRAY), element_size, length, result));
}

void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result) {
    if (array == 0) { invalid_argument(result, 0); return; }
    destroy_storage(array, &array->storage, result);
}

void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (array == 0) { invalid_argument(result, 0); return; }
    if (index >= array->storage.count) { invalid_argument(result, 1); return; }
    if (value == 0) { invalid_argument(result, 2); return; }
    copy_bytes(value, element_at(&array->storage, index), array->storage.stride);
    success(result);
}

void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result) {
    if (array == 0) { invalid_argument(result, 0); return; }
    if (index >= array->storage.count) { invalid_argument(result, 1); return; }
    if (value == 0) { invalid_argument(result, 2); return; }
    copy_bytes(element_at(&array->storage, index), value, array->storage.stride);
    success(result);
}

void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result) {
    if (array == 0) { invalid_argument(result, 0); return; }
    if (length == 0) { invalid_argument(result, 1); return; }
    *length = array->storage.count;
    success(result);
}

/* SET */

void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    *set = 0;
    if (element_size == 0) { invalid_argument(result, 1); return; }
    *set = static_cast<CATALYST_SET*>(create_storage(sizeof(CATALYST_SET), element_size, 0, result));
}

void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    destroy_storage(set, &set->storage, result);
}

void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    if (find(&set->storage, value, set->storage.stride) != set->storage.count) { noop(result); return; }
    if (!grow(&set->storage, result)) return;
    copy_bytes(element_at(&set->storage, set->storage.count), value, set->storage.stride);
    set->storage.count++;
    success(result);
}

void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    NUINT index;
    if (set == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    index = find(&set->storage, value, set->storage.stride);
    if (index == set->storage.count) { noop(result); return; }
    remove_at(&set->storage, index);
    success(result);
}

void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    if (contains == 0) { invalid_argument(result, 2); return; }
    *contains = find(&set->storage, value, set->storage.stride) != set->storage.count ? TRUE : FALSE;
    success(result);
}

void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    if (count == 0) { invalid_argument(result, 1); return; }
    *count = set->storage.count;
    success(result);
}

void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    if (index >= set->storage.count) { invalid_argument(result, 1); return; }
    if (value == 0) { invalid_argument(result, 2); return; }
    copy_bytes(value, element_at(&set->storage, index), set->storage.stride);
    success(result);
}

void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) { invalid_argument(result, 0); return; }
    clear(&set->storage, result);
}

/* MAP */

void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    *map = 0;
    if (key_size == 0) { invalid_argument(result, 1); return; }
    if (value_size == 0) { invalid_argument(result, 2); return; }
    if (key_size > CATALYST_NUINT_MAX - value_size) { overflow(result); return; }
    *map = static_cast<CATALYST_MAP*>(create_storage(sizeof(CATALYST_MAP), key_size + value_size, 0, result));
    if (*map != 0) (*map)->key_size = key_size;
}

void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    destroy_storage(map, &map->storage, result);
}

void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result) {
    NUINT index;
    if (map == 0) { invalid_argument(result, 0); return; }
    if (key == 0) { invalid_argument(result, 1); return; }
    if (value == 0) { invalid_argument(result, 2); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) { not_found(result); return; }
    copy_bytes(value, element_at(&map->storage, index) + map->key_size, map->storage.stride - map->key_size);
    success(result);
}

void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result) {
    NUINT index;
    BYTE* entry;
    if (map == 0) { invalid_argument(result, 0); return; }
    if (key == 0) { invalid_argument(result, 1); return; }
    if (value == 0) { invalid_argument(result, 2); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) {
        if (!grow(&map->storage, result)) return;
        entry = element_at(&map->storage, index);
        copy_bytes(entry, key, map->key_size);
        map->storage.count++;
    } else {
        entry = element_at(&map->storage, index);
    }
    copy_bytes(entry + map->key_size, value, map->storage.stride - map->key_size);
    success(result);
}

void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result) {
    NUINT index;
    if (map == 0) { invalid_argument(result, 0); return; }
    if (key == 0) { invalid_argument(result, 1); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) { noop(result); return; }
    remove_at(&map->storage, index);
    success(result);
}

void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    if (key == 0) { invalid_argument(result, 1); return; }
    if (contains == 0) { invalid_argument(result, 2); return; }
    *contains = find(&map->storage, key, map->key_size) != map->storage.count ? TRUE : FALSE;
    success(result);
}

void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    if (count == 0) { invalid_argument(result, 1); return; }
    *count = map->storage.count;
    success(result);
}

void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    if (index >= map->storage.count) { invalid_argument(result, 1); return; }
    if (key == 0) { invalid_argument(result, 2); return; }
    if (value == 0) { invalid_argument(result, 3); return; }
    copy_bytes(key, element_at(&map->storage, index), map->key_size);
    copy_bytes(value, element_at(&map->storage, index) + map->key_size, map->storage.stride - map->key_size);
    success(result);
}

void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) { invalid_argument(result, 0); return; }
    clear(&map->storage, result);
}

/* STACK */

void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    *stack = 0;
    if (element_size == 0) { invalid_argument(result, 1); return; }
    *stack = static_cast<CATALYST_STACK*>(create_storage(sizeof(CATALYST_STACK), element_size, 0, result));
}

void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    destroy_storage(stack, &stack->storage, result);
}

void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    if (!grow(&stack->storage, result)) return;
    copy_bytes(element_at(&stack->storage, stack->storage.count), value, stack->storage.stride);
    stack->storage.count++;
    success(result);
}

void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    if (stack->storage.count == 0) { not_found(result); return; }
    copy_bytes(value, element_at(&stack->storage, stack->storage.count - 1), stack->storage.stride);
    stack->storage.count--;
    success(result);
}

void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    if (value == 0) { invalid_argument(result, 1); return; }
    if (stack->storage.count == 0) { not_found(result); return; }
    copy_bytes(value, element_at(&stack->storage, stack->storage.count - 1), stack->storage.stride);
    success(result);
}

void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    if (count == 0) { invalid_argument(result, 1); return; }
    *count = stack->storage.count;
    success(result);
}

void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) { invalid_argument(result, 0); return; }
    clear(&stack->storage, result);
}
