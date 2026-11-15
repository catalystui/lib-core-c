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

/* File-local primitives; arguments are evaluated once, except the address expression. */
#define CATRL_RESULT(output, code, stage, info) do { \
    CATALYST_RESULT* const catrl_output = (output); \
    if (catrl_output != 0) { \
        catrl_output->status = (code); \
        catrl_output->context = 0; \
        catrl_output->operation = (stage); \
        catrl_output->detail = (info); \
    } \
} while (0)

#define CATRL_ELEMENT_AT(storage, index) ((storage)->bytes + (index) * (storage)->stride)

/* Translate allocator failures, including a potentially lost allocation during growth. */
#define CATRL_DEPENDENCY_FAILURE(output, stage, code) do { \
    const OPERATION_CODE catrl_stage = (stage); \
    const STATUS_CODE catrl_status = (code); \
    if (catrl_status == STATUS_CODE_ERROR_BUFFER_OVERFLOW) { \
        CATRL_RESULT(output, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0, 0); \
    } else if (catrl_stage == 2 && catrl_status == STATUS_CODE_ERROR_INVALID_ARGUMENT) { \
        CATRL_RESULT(output, STATUS_CODE_FATAL_INVARIANT_VIOLATION, catrl_stage, catrl_status); \
    } else { \
        CATRL_RESULT(output, STATUS_CODE_ERROR_DEPENDENCY_FAILURE, catrl_stage, catrl_status); \
    } \
} while (0)

static void* create_storage(NUINT object_size, NUINT stride,
                            NUINT count, CATALYST_RESULT* result) {
    void* object = 0;
    void* bytes = 0;
    catrl_storage* storage;
    RESULT status;
    RESULT cleanup;
    if (count > CATALYST_NUINT_MAX / stride) {
        CATRL_RESULT(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0, 0);
        return 0;
    }
    catmm_alloc(&object, object_size, &status);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(status.status)) {
        CATRL_DEPENDENCY_FAILURE(result, 1, status.status);
        return 0;
    }
    if (count != 0) {
        catmm_alloc(&bytes, count * stride, &status);
        if (!CATALYST_STATUS_CODE_IS_SUCCESS(status.status)) {
            catmm_free(object, &cleanup);
            if (!CATALYST_STATUS_CODE_IS_SUCCESS(cleanup.status)) {
                CATRL_DEPENDENCY_FAILURE(result, 3, cleanup.status);
            } else {
                CATRL_DEPENDENCY_FAILURE(result, 1, status.status);
            }
            return 0;
        }
    }
    storage = static_cast<catrl_storage*>(object);
    storage->bytes = static_cast<BYTE*>(bytes);
    storage->stride = stride;
    storage->count = count;
    storage->capacity = count;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
    return object;
}

static void destroy_storage(void* object, catrl_storage* storage, CATALYST_RESULT* result) {
    RESULT bytes_status;
    RESULT object_status;
    if (storage->bytes != 0) {
        catmm_free(storage->bytes, &bytes_status);
    }
    catmm_free(object, &object_status);
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(bytes_status.status)) {
        CATRL_DEPENDENCY_FAILURE(result, 3, bytes_status.status);
    } else if (!CATALYST_STATUS_CODE_IS_SUCCESS(object_status.status)) {
        CATRL_DEPENDENCY_FAILURE(result, 3, object_status.status);
    } else {
        CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
    }
}

/* Reserve room for one more element, with bounded geometric growth. */
static BOOL grow(catrl_storage* storage, CATALYST_RESULT* result) {
    NUINT limit;
    NUINT capacity;
    void* bytes;
    RESULT status;
    limit = CATALYST_NUINT_MAX / storage->stride;
    if (storage->count == limit) {
        CATRL_RESULT(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0, 0);
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
    if (!CATALYST_STATUS_CODE_IS_SUCCESS(status.status)) {
        CATRL_DEPENDENCY_FAILURE(result, 2, status.status);
        return FALSE;
    }
    storage->bytes = static_cast<BYTE*>(bytes);
    storage->capacity = capacity;
    return TRUE;
}

/* Return count when the key or element is absent. */
static inline NUINT find(const catrl_storage* storage, const void* value, NUINT size) {
    for (NUINT i = 0; i < storage->count; i++) {
        SBYTE comparison;
        catmm_compare(CATRL_ELEMENT_AT(storage, i), value, size, &comparison, 0);
        if (comparison == 0) return i;
    }
    return storage->count;
}

/* ARRAY */

void catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length, CATALYST_RESULT* result) {
    if (array == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    *array = 0;
    if (element_size == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *array = static_cast<CATALYST_ARRAY*>(create_storage(sizeof(CATALYST_ARRAY), element_size, length, result));
}

void catrl_array_destroy(CATALYST_ARRAY* array, CATALYST_RESULT* result) {
    if (array == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    destroy_storage(array, &array->storage, result);
}

void catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (array == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (index >= array->storage.count) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    catmm_copy(value, CATRL_ELEMENT_AT(&array->storage, index), array->storage.stride, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value, CATALYST_RESULT* result) {
    if (array == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (index >= array->storage.count) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    catmm_copy(CATRL_ELEMENT_AT(&array->storage, index), value, array->storage.stride, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length, CATALYST_RESULT* result) {
    if (array == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (length == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *length = array->storage.count;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

/* SET */

void catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    *set = 0;
    if (element_size == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *set = static_cast<CATALYST_SET*>(create_storage(sizeof(CATALYST_SET), element_size, 0, result));
}

void catrl_set_destroy(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    destroy_storage(set, &set->storage, result);
}

void catrl_set_add(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (find(&set->storage, value, set->storage.stride) != set->storage.count) { CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0); return; }
    if (set->storage.count == set->storage.capacity && !grow(&set->storage, result)) return;
    catmm_copy(CATRL_ELEMENT_AT(&set->storage, set->storage.count), value, set->storage.stride, 0);
    set->storage.count++;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_set_remove(CATALYST_SET* set, const void* value, CATALYST_RESULT* result) {
    NUINT index;
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    index = find(&set->storage, value, set->storage.stride);
    if (index == set->storage.count) { CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0); return; }
    set->storage.count--;
    if (index != set->storage.count) {
        catmm_copy(CATRL_ELEMENT_AT(&set->storage, index),
                         CATRL_ELEMENT_AT(&set->storage, set->storage.count), set->storage.stride, 0);
    }
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (contains == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    *contains = find(&set->storage, value, set->storage.stride) != set->storage.count ? TRUE : FALSE;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (count == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *count = set->storage.count;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (index >= set->storage.count) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    catmm_copy(value, CATRL_ELEMENT_AT(&set->storage, index), set->storage.stride, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_set_clear(CATALYST_SET* set, CATALYST_RESULT* result) {
    if (set == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (set->storage.count == 0) {
        CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0);
        return;
    }
    set->storage.count = 0;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

/* MAP */

void catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    *map = 0;
    if (key_size == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value_size == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    if (key_size > CATALYST_NUINT_MAX - value_size) { CATRL_RESULT(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0, 0); return; }
    *map = static_cast<CATALYST_MAP*>(create_storage(sizeof(CATALYST_MAP), key_size + value_size, 0, result));
    if (*map != 0) (*map)->key_size = key_size;
}

void catrl_map_destroy(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    destroy_storage(map, &map->storage, result);
}

void catrl_map_get(const CATALYST_MAP* map, const void* key, void* value, CATALYST_RESULT* result) {
    NUINT index;
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (key == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) { CATRL_RESULT(result, STATUS_CODE_ERROR_NOT_FOUND, 0, 0); return; }
    catmm_copy(value, CATRL_ELEMENT_AT(&map->storage, index) + map->key_size, map->storage.stride - map->key_size, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_set(CATALYST_MAP* map, const void* key, const void* value, CATALYST_RESULT* result) {
    NUINT index;
    BYTE* entry;
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (key == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) {
        if (map->storage.count == map->storage.capacity && !grow(&map->storage, result)) return;
        entry = CATRL_ELEMENT_AT(&map->storage, index);
        catmm_copy(entry, key, map->key_size, 0);
        map->storage.count++;
    } else {
        entry = CATRL_ELEMENT_AT(&map->storage, index);
    }
    catmm_copy(entry + map->key_size, value, map->storage.stride - map->key_size, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_remove(CATALYST_MAP* map, const void* key, CATALYST_RESULT* result) {
    NUINT index;
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (key == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    index = find(&map->storage, key, map->key_size);
    if (index == map->storage.count) { CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0); return; }
    map->storage.count--;
    if (index != map->storage.count) {
        catmm_copy(CATRL_ELEMENT_AT(&map->storage, index),
                         CATRL_ELEMENT_AT(&map->storage, map->storage.count), map->storage.stride, 0);
    }
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (key == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (contains == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    *contains = find(&map->storage, key, map->key_size) != map->storage.count ? TRUE : FALSE;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (count == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *count = map->storage.count;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (index >= map->storage.count) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (key == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 2); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 3); return; }
    catmm_copy(key, CATRL_ELEMENT_AT(&map->storage, index), map->key_size, 0);
    catmm_copy(value, CATRL_ELEMENT_AT(&map->storage, index) + map->key_size, map->storage.stride - map->key_size, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_map_clear(CATALYST_MAP* map, CATALYST_RESULT* result) {
    if (map == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (map->storage.count == 0) {
        CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0);
        return;
    }
    map->storage.count = 0;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

/* STACK */

void catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    *stack = 0;
    if (element_size == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *stack = static_cast<CATALYST_STACK*>(create_storage(sizeof(CATALYST_STACK), element_size, 0, result));
}

void catrl_stack_destroy(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    destroy_storage(stack, &stack->storage, result);
}

void catrl_stack_push(CATALYST_STACK* stack, const void* value, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (stack->storage.count == stack->storage.capacity && !grow(&stack->storage, result)) return;
    catmm_copy(CATRL_ELEMENT_AT(&stack->storage, stack->storage.count), value, stack->storage.stride, 0);
    stack->storage.count++;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_stack_pop(CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (stack->storage.count == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_NOT_FOUND, 0, 0); return; }
    catmm_copy(value, CATRL_ELEMENT_AT(&stack->storage, stack->storage.count - 1), stack->storage.stride, 0);
    stack->storage.count--;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_stack_peek(const CATALYST_STACK* stack, void* value, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (value == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    if (stack->storage.count == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_NOT_FOUND, 0, 0); return; }
    catmm_copy(value, CATRL_ELEMENT_AT(&stack->storage, stack->storage.count - 1), stack->storage.stride, 0);
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (count == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 1); return; }
    *count = stack->storage.count;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

void catrl_stack_clear(CATALYST_STACK* stack, CATALYST_RESULT* result) {
    if (stack == 0) { CATRL_RESULT(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, 0); return; }
    if (stack->storage.count == 0) {
        CATRL_RESULT(result, STATUS_CODE_SUCCESS_NOOP, 0, 0);
        return;
    }
    stack->storage.count = 0;
    CATRL_RESULT(result, STATUS_CODE_SUCCESS, 0, 0);
}

#undef CATRL_DEPENDENCY_FAILURE
#undef CATRL_ELEMENT_AT
#undef CATRL_RESULT
