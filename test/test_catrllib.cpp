#include <cstdio>

#include "catalyst.hpp"
#include "catrllib.hpp"

using namespace catalyst;

static BOOL matches(CATALYST_RESULT result, STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation, DETAIL_CODE detail = 0) {
    return result.status == status && result.context == context && result.operation == operation && result.detail == detail ? TRUE : FALSE;
}

static BOOL check(int* failed, BOOL passed, const char* name) {
    std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    if (!passed) ++*failed;
    return passed;
}

// Each metadata byte identifies the beginning of a two-element view of the data.
static CATALYST_RESULT resolve_pair(const CATRLLIB_FILE* file, NUINT index, CATRLLIB_ARRAY* output) {
    if (index >= file->metadata.length) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xA1, 0x01, 1);
    NUINT offset = ((const BYTE*) file->metadata.data)[index];
    if (offset >= file->data.length || file->data.length - offset < (NUINT) 2) return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0xA1, 0x02, 0xA5);
    output->data = (BYTE*) file->data.data + offset;
    output->size = 1;
    output->length = 2;
    output->capacity = 2;
    return RESULT(STATUS_CODE_SUCCESS, 0xA1, 0x03, 0);
}

// State: number of calls, selected outcome, selected detail.
static CATALYST_RESULT produce_chunk(CATRLLIB_STREAM* stream, POINTERW chunk, NUINT csize) {
    BYTE* state = (BYTE*) stream->state;
    ++state[0];
    for (NUINT i = 0; i < csize; ++i) ((BYTE*) chunk)[i] = (BYTE) (state[0] + (BYTE) i);
    return RESULT(state[1], 0xA2, 0x01, state[2]);
}

static CATALYST_RESULT produce_stateless(CATRLLIB_STREAM*, POINTERW chunk, NUINT csize) {
    for (NUINT i = 0; i < csize; ++i) ((BYTE*) chunk)[i] = 0x5A;
    return RESULT(STATUS_CODE_SUCCESS, 0xA3, 0x01, 0);
}

static BOOL value_matches(POINTER actual, POINTER expected, NUINT size) {
    return actual != 0 && catmm_compare(actual, expected, size) == 0 ? TRUE : FALSE;
}

static BOOL chunk_matches(POINTER chunk, NUINT size, BYTE first) {
    if (chunk == 0) return FALSE;
    for (NUINT i = 0; i < size; ++i) {
        if (((const BYTE*) chunk)[i] != (BYTE) (first + (BYTE) i)) return FALSE;
    }
    return TRUE;
}

int main() {
    int failed = 0;
    RESULT result;

    // ------------------------------------------------------------
    // Set: byte equality, distinctness, capacity, removal, and reuse
    // ------------------------------------------------------------
    {
        SET set;
        BYTE a[] = { 1, 2 };
        BYTE duplicate[] = { 1, 2 };
        BYTE b[] = { 1, 3 };
        BYTE c[] = { 2, 1 };
        catrl_set_init(&set, 2);
        check(&failed, set.data == 0 && set.size == 2 && set.cardinality == 0 && set.capacity == 0 && !set_contains(&set, a), "catrl_set_init empty Set");
        check(&failed, matches(set_expand(&set, 0), STATUS_CODE_SUCCESS_NOOP, 0x06, 0x01), "catrl_set_expand zero capacity no-op");
        check(&failed, matches(set_shrink(&set), STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01), "catrl_set_shrink empty unallocated Set");
        check(&failed, matches(set_expand(&set, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x02) && set.data == 0 && set.capacity == 0, "catrl_set_expand extent overflow");

        result = catrl_set_expand(&set, 2);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x06, 0x04) && set.data != 0 && set.capacity == 2, "catrl_set_expand two elements")) {
            check(&failed, catrl_set_add(&set, a) && set_add(&set, b) && set.cardinality == 2, "catrl_set_add distinct elements");
            check(&failed, !set_add(&set, duplicate) && set.cardinality == 2 && set.capacity == 2, "catrl_set_add duplicate at full capacity");
            a[0] = 9;
            check(&failed, catrl_set_contains(&set, duplicate) && !set_contains(&set, a) && !set_contains(&set, c), "catrl_set_contains byte equality and copied input");
            check(&failed, !catrl_set_remove(&set, c) && set.cardinality == 2, "catrl_set_remove absent element");
            POINTERW original = set.data;
            check(&failed, matches(set_expand(&set, 1), STATUS_CODE_SUCCESS_NOOP, 0x06, 0x01) && set.data == original && set.capacity == 2, "catrl_set_expand smaller request preserves allocation");
            check(&failed, matches(set_expand(&set, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x02) && set.data == original && set.cardinality == 2 && set.capacity == 2 && set_contains(&set, duplicate) && set_contains(&set, b), "catrl_set_expand overflow preserves members");
            check(&failed, matches(set_shrink(&set), STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01), "catrl_set_shrink full Set");
            result = set_expand(&set, 8);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x06, 0x04) && set.capacity == 8 && set.cardinality == 2 && set_contains(&set, duplicate) && set_contains(&set, b), "catrl_set_expand preserves members")) {
                check(&failed, set_add(&set, c) && set.cardinality == 3, "catrl_set_add after expansion");
                check(&failed, set_remove(&set, set.data) && set.cardinality == 2 && !set_contains(&set, duplicate) && set_contains(&set, b) && set_contains(&set, c), "catrl_set_remove stored member and preserve remaining members");
                check(&failed, matches(catrl_set_shrink(&set), STATUS_CODE_SUCCESS, 0x07, 0x03) && set.capacity == 4 && set.cardinality == 2 && set_contains(&set, b) && set_contains(&set, c), "catrl_set_shrink preserves members");
                catrl_set_clear(&set);
                check(&failed, set.cardinality == 0 && set.capacity == 4 && !set_contains(&set, b), "catrl_set_clear retains capacity");
                check(&failed, set_add(&set, duplicate) && set.cardinality == 1, "catrl_set_add reuses cleared storage");
                set_clear(&set);
                for (NUINT capacity = 2; capacity != 0; capacity = (NUINT) (capacity / (NUINT) 2)) {
                    result = set_shrink(&set);
                    if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x07, 0x03) && set.capacity == capacity, "catrl_set_shrink empty Set")) break;
                }
                if (set.capacity == 1) {
                    check(&failed, matches(set_shrink(&set), STATUS_CODE_SUCCESS, 0x07, 0x03) && set.data == 0 && set.capacity == 0 && set.size == 2, "catrl_set_shrink releases final slot");
                }
            }
        }
        result = catrl_set_free(&set);
        check(&failed, (matches(result, STATUS_CODE_SUCCESS, 0x08, 0x03) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x08, 0x01)) && set.data == 0 && set.cardinality == 0 && set.capacity == 0 && set.size == 2, "catrl_set_free");
        check(&failed, matches(set_free(&set), STATUS_CODE_SUCCESS_NOOP, 0x08, 0x01), "catrl_set_free repeated release");
        if (check(&failed, matches(set_expand(&set, 1), STATUS_CODE_SUCCESS, 0x06, 0x04), "catrl_set_expand after free")) {
            check(&failed, set_add(&set, duplicate) && set_contains(&set, duplicate), "catrl_set_add after free");
            check(&failed, matches(set_free(&set), STATUS_CODE_SUCCESS, 0x08, 0x03), "catrl_set_free reused Set");
        }
    }

    // ------------------------------------------------------------
    // Map: independent key/value sizes, replacement, and capacity
    // ------------------------------------------------------------
    {
        MAP map;
        BYTE key1[] = { 1, 2 };
        BYTE key2[] = { 1, 3 };
        BYTE key3[] = { 2, 1 };
        BYTE value1[] = { 4, 5, 6 };
        BYTE value2[] = { 7, 8, 9 };
        BYTE value3[] = { 10, 11, 12 };
        map_init(&map, 2, 3);
        check(&failed, map.keys == 0 && map.values == 0 && map.ksize == 2 && map.vsize == 3 && map.cardinality == 0 && map.capacity == 0, "catrl_map_init independent sizes");
        check(&failed, matches(map_shrink(&map), STATUS_CODE_SUCCESS_NOOP, 0x11, 0x01), "catrl_map_shrink unallocated Map");
        check(&failed, matches(map_expand(&map, 0), STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01), "catrl_map_expand zero capacity no-op");
        check(&failed, catrl_map_get(&map, key1) == 0, "catrl_map_get absent key returns null");

        NUINT valueOverflow = (NUINT) (CATALYST_NUINT_MAX / (NUINT) 3 + (NUINT) 1);
        check(&failed, matches(catrl_map_expand(&map, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x02), "catrl_map_expand key extent overflow");
        check(&failed, matches(map_expand(&map, valueOverflow), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x03) && map.keys == 0 && map.values == 0 && map.capacity == 0, "catrl_map_expand value extent overflow");

        result = map_expand(&map, 2);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x10, 0x06) && map.keys != 0 && map.values != 0 && map.capacity == 2, "catrl_map_expand two pairs")) {
            check(&failed, catrl_map_add(&map, key1, value1) && map_add(&map, key2, value1) && map.cardinality == 2, "catrl_map_add distinct keys with equal values");
            check(&failed, !map_add(&map, key1, value2) && map.cardinality == 2 && map.capacity == 2, "catrl_map_add duplicate at full capacity");
            check(&failed, catrl_map_contains(&map, key1) && map_contains(&map, key2) && !map_contains(&map, key3), "catrl_map_contains");
            BYTE* value = (BYTE*) map_get(&map, key1);
            if (check(&failed, value != 0 && value == map.values && catmm_compare(value, value1, 3) == 0, "catrl_map_get borrows exact value address")) {
                value[1] = 0xA5;
                check(&failed, catrl_map_get(&map, key1) == value && ((const BYTE*) map.values)[1] == 0xA5 && catmm_compare((const BYTE*) map.values + 3, value1, 3) == 0, "catrl_map_get writes in place without affecting another value");
            }
            check(&failed, map_get(&map, key3) == 0, "catrl_map_get absent from nonempty Map");
            check(&failed, !map_set(&map, key3, value3) && !map_remove(&map, key3) && map.cardinality == 2, "catrl_map_set and remove absent key");
            check(&failed, catrl_map_set(&map, map.keys, value2) && value_matches(map_get(&map, key1), value2, 3), "catrl_map_set replaces existing value");
            check(&failed, value_matches(map_get(&map, key2), value1, 3), "catrl_map_set leaves other values unchanged");
            POINTERW keys = map.keys;
            POINTERW values = map.values;
            check(&failed, matches(map_expand(&map, 2), STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01) && map.keys == keys && map.values == values, "catrl_map_expand equal capacity no-op");
            check(&failed, matches(map_expand(&map, valueOverflow), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x03) && map.keys == keys && map.values == values && map.cardinality == 2 && map.capacity == 2 && value_matches(map_get(&map, key1), value2, 3), "catrl_map_expand overflow preserves pairs");
            check(&failed, matches(map_shrink(&map), STATUS_CODE_SUCCESS_NOOP, 0x11, 0x01), "catrl_map_shrink full Map");
            result = catrl_map_expand(&map, 8);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x10, 0x06) && map.capacity == 8 && map.cardinality == 2 && value_matches(map_get(&map, key1), value2, 3), "catrl_map_expand preserves pairs")) {
                check(&failed, map_add(&map, key3, value3) && map.cardinality == 3, "catrl_map_add after expansion");
                check(&failed, catrl_map_remove(&map, map.keys) && map.cardinality == 2 && !map_contains(&map, key1) && value_matches(map_get(&map, key3), value3, 3), "catrl_map_remove keeps moved key and value associated");
                check(&failed, matches(catrl_map_shrink(&map), STATUS_CODE_SUCCESS, 0x11, 0x04) && map.capacity == 4 && map.cardinality == 2 && value_matches(map_get(&map, key2), value1, 3), "catrl_map_shrink preserves pairs");
                map_clear(&map);
                check(&failed, map.cardinality == 0 && map.capacity == 4 && !map_contains(&map, key2), "catrl_map_clear retains capacity");
                check(&failed, map_add(&map, key1, value1) && map.cardinality == 1, "catrl_map_add reuses cleared storage");
                catrl_map_clear(&map);
                for (NUINT capacity = 2; capacity != 0; capacity = (NUINT) (capacity / (NUINT) 2)) {
                    result = map_shrink(&map);
                    if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x11, 0x04) && map.capacity == capacity, "catrl_map_shrink empty Map")) break;
                }
                if (map.capacity == 1) {
                    check(&failed, matches(map_shrink(&map), STATUS_CODE_SUCCESS, 0x11, 0x04) && map.keys == 0 && map.values == 0 && map.capacity == 0, "catrl_map_shrink releases both final slots");
                }
            }
        }
        result = catrl_map_free(&map);
        check(&failed, (matches(result, STATUS_CODE_SUCCESS, 0x12, 0x04) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x12, 0x03)) && map.keys == 0 && map.values == 0 && map.cardinality == 0 && map.capacity == 0 && map.ksize == 2 && map.vsize == 3, "catrl_map_free");
        check(&failed, matches(map_free(&map), STATUS_CODE_SUCCESS_NOOP, 0x12, 0x03), "catrl_map_free repeated release");
        if (check(&failed, matches(map_expand(&map, 1), STATUS_CODE_SUCCESS, 0x10, 0x06), "catrl_map_expand after free")) {
            check(&failed, map_add(&map, key1, value1) && map_contains(&map, key1), "catrl_map_add after free");
            check(&failed, matches(map_free(&map), STATUS_CODE_SUCCESS, 0x12, 0x04), "catrl_map_free reused Map");
        }
    }

    // ------------------------------------------------------------
    // Array: indexed access, duplicates, insertion, removal, and order
    // ------------------------------------------------------------
    {
        ARRAY array;
        BYTE a[] = { 1, 2 };
        BYTE b[] = { 3, 4 };
        BYTE c[] = { 5, 6 };
        catrl_array_init(&array, 2);
        check(&failed, array.data == 0 && array.size == 2 && array.length == 0 && array.capacity == 0 && !array_contains(&array, a), "catrl_array_init empty Array");
        check(&failed, matches(array_expand(&array, 0), STATUS_CODE_SUCCESS_NOOP, 0x1B, 0x01), "catrl_array_expand zero capacity no-op");
        check(&failed, matches(array_shrink(&array), STATUS_CODE_SUCCESS_NOOP, 0x1C, 0x01), "catrl_array_shrink unallocated Array");
        check(&failed, matches(array_expand(&array, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1B, 0x02), "catrl_array_expand extent overflow");

        result = catrl_array_expand(&array, 8);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1B, 0x04) && array.data != 0 && array.capacity == 8, "catrl_array_expand eight elements")) {
            catrl_array_add(&array, a);
            array_add(&array, a);
            catrl_array_insert(&array, 0, b);
            array_insert(&array, 2, c);
            array_insert(&array, array.length, b);
            BYTE expected[] = { 3, 4, 1, 2, 5, 6, 1, 2, 3, 4 };
            check(&failed, array.length == 5 && array.capacity == 8 && catmm_compare(array.data, expected, 10) == 0, "catrl_array_insert front, middle, and end with duplicates");
            if (array.length == 5) {
                BYTE* element = (BYTE*) catrl_array_get(&array, 2);
                if (check(&failed, element != 0 && element == (BYTE*) array.data + 4 && value_matches(element, c, 2), "catrl_array_get borrows exact indexed address")) {
                    element[0] = 0xA5;
                    check(&failed, array_get(&array, 2) == element && ((const BYTE*) array.data)[4] == 0xA5 && value_matches(array_get(&array, 3), a, 2), "catrl_array_get writes in place without affecting next element");
                }
                check(&failed, array_get(&array, 0) == array.data && value_matches(array_get(&array, 0), b, 2) && value_matches(array_get(&array, 4), b, 2), "catrl_array_get C++ wrapper first and last indices");
                catrl_array_set(&array, 2, b);
                array_set(&array, 1, c);
                BYTE replaced[] = { 3, 4, 5, 6, 3, 4, 1, 2, 3, 4 };
                check(&failed, array.length == 5 && catmm_compare(array.data, replaced, 10) == 0 && catrl_array_contains(&array, c), "catrl_array_set replaces indexed elements");
                catrl_array_remove(&array, 0);
                if (array.length == 4) array_remove(&array, 1);
                if (array.length == 3) array_remove(&array, 2);
                BYTE remaining[] = { 5, 6, 1, 2 };
                check(&failed, array.length == 2 && array.capacity == 8 && catmm_compare(array.data, remaining, 4) == 0 && !array_contains(&array, b), "catrl_array_remove front, middle, and end preserves order");
                POINTERW original = array.data;
                check(&failed, matches(array_expand(&array, 8), STATUS_CODE_SUCCESS_NOOP, 0x1B, 0x01) && array.data == original, "catrl_array_expand equal capacity no-op");
                check(&failed, matches(array_expand(&array, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1B, 0x02) && array.data == original && array.length == 2 && array.capacity == 8 && catmm_compare(array.data, remaining, 4) == 0, "catrl_array_expand overflow preserves elements");
                result = array_expand(&array, 10);
                if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1B, 0x04) && array.capacity == 10 && array.length == 2 && catmm_compare(array.data, remaining, 4) == 0, "catrl_array_expand preserves order")) {
                    check(&failed, value_matches(array_get(&array, 0), c, 2) && value_matches(array_get(&array, 1), a, 2), "catrl_array_get reacquires elements after growth");
                    check(&failed, matches(catrl_array_shrink(&array), STATUS_CODE_SUCCESS, 0x1C, 0x03) && array.capacity == 5 && array.length == 2 && catmm_compare(array.data, remaining, 4) == 0, "catrl_array_shrink preserves order");
                    check(&failed, matches(array_shrink(&array), STATUS_CODE_SUCCESS, 0x1C, 0x03) && array.capacity == 2 && array.length == 2, "catrl_array_shrink retains live elements");
                    check(&failed, matches(array_shrink(&array), STATUS_CODE_SUCCESS_NOOP, 0x1C, 0x01), "catrl_array_shrink full Array");
                    catrl_array_clear(&array);
                    check(&failed, array.length == 0 && array.capacity == 2 && !array_contains(&array, a), "catrl_array_clear retains capacity");
                    if (array.capacity != 0) {
                        array_add(&array, a);
                        check(&failed, array.length == 1 && array_contains(&array, a), "catrl_array_add reuses cleared storage");
                    }
                    array_clear(&array);
                    check(&failed, matches(array_shrink(&array), STATUS_CODE_SUCCESS, 0x1C, 0x03) && array.capacity == 1, "catrl_array_shrink empty Array");
                    check(&failed, matches(array_shrink(&array), STATUS_CODE_SUCCESS, 0x1C, 0x03) && array.data == 0 && array.capacity == 0, "catrl_array_shrink releases final slot");
                }
            }
        }
        result = catrl_array_free(&array);
        check(&failed, (matches(result, STATUS_CODE_SUCCESS, 0x1D, 0x03) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01)) && array.data == 0 && array.length == 0 && array.capacity == 0 && array.size == 2, "catrl_array_free");
        check(&failed, matches(array_free(&array), STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01), "catrl_array_free repeated release");
        if (check(&failed, matches(array_expand(&array, 1), STATUS_CODE_SUCCESS, 0x1B, 0x04), "catrl_array_expand after free")) {
            array_add(&array, c);
            check(&failed, array.length == 1 && array_contains(&array, c), "catrl_array_add after free");
            check(&failed, matches(array_free(&array), STATUS_CODE_SUCCESS, 0x1D, 0x03), "catrl_array_free reused Array");
        }
    }

    // ------------------------------------------------------------
    // Allocation dependency overflow with representable collection extents
    // ------------------------------------------------------------
    {
        SET set;
        MAP map;
        ARRAY array;
        set_init(&set, 1);
        map_init(&map, 1, 1);
        array_init(&array, 1);
        BYTE element = 0xA5;
        RESULT setResult = set_expand(&set, 1);
        RESULT mapResult = map_expand(&map, 1);
        RESULT arrayResult = array_expand(&array, 1);
        if (check(&failed, matches(setResult, STATUS_CODE_SUCCESS, 0x06, 0x04) && matches(mapResult, STATUS_CODE_SUCCESS, 0x10, 0x06) && matches(arrayResult, STATUS_CODE_SUCCESS, 0x1B, 0x04), "CATRLLIB allocation dependency overflow setup")) {
            set_add(&set, &element);
            map_add(&map, &element, &element);
            array_add(&array, &element);
            POINTERW setStorage = set.data;
            POINTERW keyStorage = map.keys;
            POINTERW valueStorage = map.values;
            POINTERW arrayStorage = array.data;
            check(&failed, matches(set_expand(&set, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x03) && set.data == setStorage && set.capacity == 1 && set.cardinality == 1 && set_contains(&set, &element), "catrl_set_expand dependency overflow preserves allocation");
            check(&failed, matches(map_expand(&map, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x10, 0x04) && map.keys == keyStorage && map.values == valueStorage && map.capacity == 1 && map.cardinality == 1 && map_contains(&map, &element), "catrl_map_expand dependency overflow preserves allocations");
            check(&failed, matches(array_expand(&array, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1B, 0x03) && array.data == arrayStorage && array.capacity == 1 && array.length == 1 && array_contains(&array, &element), "catrl_array_expand dependency overflow preserves allocation");
        }
        result = set_free(&set);
        check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x08, 0x03) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x08, 0x01), "catrl_set_free dependency overflow test");
        result = map_free(&map);
        check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x12, 0x04) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x12, 0x03), "catrl_map_free dependency overflow test");
        result = array_free(&array);
        check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1D, 0x03) || matches(result, STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01), "catrl_array_free dependency overflow test");
    }

    // ------------------------------------------------------------
    // File: transfer ownership, metadata resolution, and callback Results
    // ------------------------------------------------------------
    {
        ARRAY metadata;
        ARRAY data;
        array_init(&metadata, 1);
        array_init(&data, 1);
        RESULT metadataResult = array_expand(&metadata, 2);
        RESULT dataResult = array_expand(&data, 4);
        if (check(&failed, matches(metadataResult, STATUS_CODE_SUCCESS, 0x1B, 0x04) && matches(dataResult, STATUS_CODE_SUCCESS, 0x1B, 0x04), "catrl_file_init source Arrays")) {
            BYTE offsets[] = { 2, 0 };
            BYTE bytes[] = { 10, 20, 30, 40 };
            for (NUINT i = 0; i < (NUINT) 2; ++i) array_add(&metadata, offsets + i);
            for (NUINT i = 0; i < (NUINT) 4; ++i) array_add(&data, bytes + i);
            POINTERW metadataStorage = metadata.data;
            POINTERW dataStorage = data.data;
            catalyst::FILE file;
            catrl_file_init(&file, &metadata, &data, resolve_pair);
            check(&failed, file.metadata.data == metadataStorage && file.data.data == dataStorage && file.metadata.length == 2 && file.data.length == 4 && metadata.data == 0 && metadata.length == 0 && metadata.capacity == 0 && metadata.size == 1 && data.data == 0 && data.length == 0 && data.capacity == 0 && data.size == 1, "catrl_file_init transfers ownership without copying");

            ARRAY output;
            array_init(&output, 1);
            result = catrl_file_get(&file, 0, &output);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1F, 0x01) && output.data == (BYTE*) file.data.data + 2 && output.size == 1 && output.length == 2 && output.capacity == 2, "catrl_file_get metadata selects borrowed view")) {
                check(&failed, ((const BYTE*) output.data)[0] == 30 && ((const BYTE*) output.data)[1] == 40, "catrl_file_get resolved bytes");
            }
            result = file_get(&file, 1, &output);
            check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1F, 0x01) && output.data == file.data.data && output.length == 2 && ((const BYTE*) output.data)[0] == 10 && ((const BYTE*) output.data)[1] == 20, "catrl_file_get index differs from data offset");

            BYTE newOffset = 1;
            array_set(&file.metadata, 1, &newOffset);
            result = file_get(&file, 1, &output);
            check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x1F, 0x01) && output.data == (BYTE*) file.data.data + 1 && ((const BYTE*) output.data)[0] == 20 && ((const BYTE*) output.data)[1] == 30, "catrl_file_get uses current metadata");
            POINTERW previous = output.data;
            NUINT previousLength = output.length;
            result = file_get(&file, 2, &output);
            check(&failed, matches(result, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1F, 0x01, 1) && output.data == previous && output.length == previousLength, "catrl_file_get propagates invalid index");
            newOffset = 3;
            array_set(&file.metadata, 1, &newOffset);
            result = file_get(&file, 1, &output);
            check(&failed, matches(result, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x1F, 0x01, 0xA5) && output.data == previous && output.length == previousLength, "catrl_file_get propagates resolver failure and detail");

            // The output borrows File storage; it must not be freed independently.
            result = catrl_file_free(&file);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x20, 0x04) && file.metadata.data == 0 && file.data.data == 0 && file.metadata.size == 1 && file.data.size == 1 && file.resolver == 0, "catrl_file_free owned Arrays")) {
                check(&failed, matches(file_free(&file), STATUS_CODE_SUCCESS_NOOP, 0x20, 0x01), "catrl_file_free repeated release");
                file_init(&file, &metadata, &data, resolve_pair);
                check(&failed, matches(file_free(&file), STATUS_CODE_SUCCESS, 0x20, 0x04), "catrl_file_init and free empty Arrays");
            } else {
                check(&failed, matches(file_free(&file), STATUS_CODE_SUCCESS, 0x20, 0x04), "catrl_file_free retry cleanup");
            }
        }
        check(&failed, (metadata.data == 0 ? matches(array_free(&metadata), STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01) : matches(array_free(&metadata), STATUS_CODE_SUCCESS, 0x1D, 0x03)), "catrl_array_free source metadata");
        check(&failed, (data.data == 0 ? matches(array_free(&data), STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x01) : matches(array_free(&data), STATUS_CODE_SUCCESS, 0x1D, 0x03)), "catrl_array_free source data");
    }

    // ------------------------------------------------------------
    // Stream: deterministic initialization overflow and stateless producer
    // ------------------------------------------------------------
    {
        STREAM stream;
        result = catrl_stream_init(&stream, CATALYST_NUINT_MAX, 1, produce_stateless);
        check(&failed, matches(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x21, 0x02) && stream.state == 0 && stream.ssize == 0 && stream.next == 0, "catrl_stream_init state extent overflow");
        check(&failed, matches(stream_free(&stream), STATUS_CODE_SUCCESS_NOOP, 0x23, 0x01), "catrl_stream_free after failed init");
        result = stream_init(&stream, 0, CATALYST_NUINT_MAX, produce_stateless);
        check(&failed, matches(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x21, 0x03) && stream.state == 0 && stream.ssize == 0 && stream.next == 0, "catrl_stream_init retention extent overflow");

        result = stream_init(&stream, 0, 1, produce_stateless);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x21, 0x05) && stream.state != 0 && stream.ssize == 0 && stream.next == produce_stateless, "catrl_stream_init zero state")) {
            POINTERW output = 0;
            result = catrl_stream_next(&stream, 0, &output);
            check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x22, 0x03) && output != 0, "catrl_stream_next zero-byte chunk");
            result = stream_next(&stream, 4, &output);
            check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x22, 0x03) && output != 0 && ((const BYTE*) output)[0] == 0x5A && ((const BYTE*) output)[3] == 0x5A, "catrl_stream_next stateless producer");
            check(&failed, matches(catrl_stream_free(&stream), STATUS_CODE_SUCCESS, 0x23, 0x05) && stream.state == 0 && stream.ssize == 0 && stream.next == 0, "catrl_stream_free stateless Stream");
            check(&failed, matches(stream_free(&stream), STATUS_CODE_SUCCESS_NOOP, 0x23, 0x01), "catrl_stream_free repeated release");
        }
    }

    // ------------------------------------------------------------
    // Stream: exact production, retained chunks, failures, and buffer reuse
    // ------------------------------------------------------------
    {
        NUINT lifetimes[] = { 1, 3 };
        NUINT sizes[] = { 16, 32, 8, 1, 2, 0, 3, 1, 24, 2, 1, 1 };
        struct ProducerTest { STATUS_CODE status; DETAIL_CODE detail; STATUS_CODE expectedStatus; OPERATION_CODE expectedOperation; DETAIL_CODE expectedDetail; };
        ProducerTest outcomes[] = {
            { STATUS_CODE_SUCCESS_NOOP, 0, STATUS_CODE_SUCCESS_NOOP, 0x03, 0 },
            { STATUS_CODE_WARNING, 0x35, STATUS_CODE_WARNING, 0x03, 0x35 },
            { STATUS_CODE_WARNING_PARTIAL, 0x36, STATUS_CODE_WARNING_PARTIAL, 0x03, 0x36 },
            { STATUS_CODE_WARNING_DEPRECATED, 0, STATUS_CODE_WARNING_DEPRECATED, 0x03, 0 },
            { STATUS_CODE_ERROR_MALFORMED_INPUT, 0x37, STATUS_CODE_ERROR_MALFORMED_INPUT, 0x03, 0x37 },
            { STATUS_CODE_ERROR_INVALID_ARGUMENT, 0, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0 },
            { STATUS_CODE_ERROR_INVALID_ARGUMENT, 1, STATUS_CODE_ERROR_INVALID_STATE, 0x02, 0 },
            { STATUS_CODE_ERROR_INVALID_ARGUMENT, 2, STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 1 }
        };
        for (NUINT test = 0; test < (NUINT) 2; ++test) {
            STREAM stream;
            NUINT lifetime = lifetimes[test];
            result = stream_init(&stream, 3, lifetime, produce_chunk);
            if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x21, 0x05) && stream.state != 0 && stream.ssize == 3 && stream.next == produce_chunk, "catrl_stream_init managed state and lifetime")) continue;
            BYTE* state = (BYTE*) stream.state;
            check(&failed, state[0] == 0 && state[1] == 0 && state[2] == 0, "catrl_stream_init zero-filled state");

            POINTERW chunks[3] = { 0, 0, 0 };
            NUINT chunkSizes[3] = { 0, 0, 0 };
            BYTE chunkStarts[3] = { 0, 0, 0 };
            NUINT retained = 0;
            NUINT cursor = 0;
            POINTERW output = 0;
            BOOL sequencePassed = TRUE;
            for (NUINT i = 0; i < (NUINT) (sizeof(sizes) / sizeof(sizes[0])); ++i) {
                BYTE expected = (BYTE) (state[0] + (BYTE) 1);
                state[1] = STATUS_CODE_SUCCESS;
                state[2] = 0;
                result = stream_next(&stream, sizes[i], &output);
                BOOL published = matches(result, STATUS_CODE_SUCCESS, 0x22, 0x03);
                if (!check(&failed, published && output != 0 && stream.state == state && state[0] == expected && chunk_matches(output, sizes[i], expected), "catrl_stream_next exact chunk and stable state")) { sequencePassed = FALSE; break; }
                chunks[cursor] = output;
                chunkSizes[cursor] = sizes[i];
                chunkStarts[cursor] = expected;
                if (retained < lifetime) ++retained;
                ++cursor;
                if (cursor == lifetime) cursor = 0;

                BOOL passed = TRUE;
                for (NUINT slot = 0; slot < retained; ++slot) {
                    if (!chunk_matches(chunks[slot], chunkSizes[slot], chunkStarts[slot])) passed = FALSE;
                    for (NUINT other = 0; other < slot; ++other) {
                        if (chunks[slot] == chunks[other]) passed = FALSE;
                    }
                }
                if (!check(&failed, passed, "catrl_stream_next retained chunks remain distinct and unchanged")) { sequencePassed = FALSE; break; }

                // Exercise each non-publication result with a full retention window.
                if (i == lifetime - (NUINT) 1) {
                    POINTERW previous = output;
                    BYTE calls = state[0];
                    result = catrl_stream_next(&stream, CATALYST_NUINT_MAX, &output);
                    if (!check(&failed, matches(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x22, 0x01) && output == previous && state[0] == calls, "catrl_stream_next growth overflow preserves output and skips producer")) { sequencePassed = FALSE; break; }
                    for (NUINT outcome = 0; outcome < (NUINT) (sizeof(outcomes) / sizeof(outcomes[0])); ++outcome) {
                        state[1] = outcomes[outcome].status;
                        state[2] = outcomes[outcome].detail;
                        calls = state[0];
                        result = catrl_stream_next(&stream, 4, &output);
                        passed = matches(result, outcomes[outcome].expectedStatus, 0x22, outcomes[outcome].expectedOperation, outcomes[outcome].expectedDetail) && output == previous && state[0] == (BYTE) (calls + (BYTE) 1);
                        if (passed) {
                            for (NUINT slot = 0; slot < retained; ++slot) {
                                if (!chunk_matches(chunks[slot], chunkSizes[slot], chunkStarts[slot])) passed = FALSE;
                            }
                        }
                        if (!check(&failed, passed, "catrl_stream_next non-publication preserves output and retention, remaps Result")) { sequencePassed = FALSE; break; }
                    }
                    if (!sequencePassed) break;
                }
            }
            if (sequencePassed) {
                // A fatal producer outcome does not publish; end this Stream afterward.
                POINTERW previous = output;
                state[1] = STATUS_CODE_FATAL_INVARIANT_VIOLATION;
                state[2] = 0x38;
                result = stream_next(&stream, 1, &output);
                BOOL passed = matches(result, STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x22, 0x03, 0x38) && output == previous;
                for (NUINT slot = 0; slot < retained; ++slot) {
                    if (!chunk_matches(chunks[slot], chunkSizes[slot], chunkStarts[slot])) passed = FALSE;
                }
                check(&failed, passed, "catrl_stream_next fatal producer preserves retained chunks");
            }
            check(&failed, matches(stream_free(&stream), STATUS_CODE_SUCCESS, 0x23, 0x05) && stream.state == 0 && stream.ssize == 0 && stream.next == 0, "catrl_stream_free all managed chunks and state");
            check(&failed, matches(stream_free(&stream), STATUS_CODE_SUCCESS_NOOP, 0x23, 0x01), "catrl_stream_free repeated release");
        }
    }

    // Search block boundaries, unaligned input, and late-byte mismatches.
    {
        const BYTE widths[] = {1, 2,
#if TARGET_SIZEOF_ADDRESS >= 2
            8, 16,
#endif
        };
        BOOL passed = TRUE;
        for (BYTE w = 0; w < (BYTE) (sizeof(widths) / sizeof(widths[0])); ++w) {
            NUINT width = widths[w];
            SET set;
            MAP map;
            ARRAY array;
            set_init(&set, width);
            map_init(&map, width, 1);
            array_init(&array, width);
            if (status_code_is_success(set_expand(&set, 9).status) &&
                status_code_is_success(map_expand(&map, 9).status) &&
                status_code_is_success(array_expand(&array, 9).status)) {
                BYTE input[17];
                BYTE* key = input + 1;
                for (NUINT i = 0; i < width; ++i) key[i] = 0xA5;
                for (BYTE n = 0; n < 10; ++n) {
                    for (BYTE k = 0; k <= n; ++k) {
                        key[width - (NUINT) 1] = k;
                        BOOL expected = k < n ? TRUE : FALSE;
                        POINTER value = map_get(&map, key);
                        if (set_contains(&set, key) != expected || map_contains(&map, key) != expected ||
                            array_contains(&array, key) != expected || (value != 0 ? TRUE : FALSE) != expected ||
                            (value != 0 && *(const BYTE*) value != k)) passed = FALSE;
                    }
                    if (n < 9) {
                        key[width - (NUINT) 1] = n;
                        if (!set_add(&set, key) || !map_add(&map, key, &n)) passed = FALSE;
                        array_add(&array, key);
                    }
                }
            } else passed = FALSE;
            if (!status_code_is_success(set_free(&set).status)) passed = FALSE;
            if (!status_code_is_success(map_free(&map).status)) passed = FALSE;
            if (!status_code_is_success(array_free(&array).status)) passed = FALSE;
        }
        check(&failed, passed, "linear searches preserve equality across block boundaries and late mismatches");
    }

    // Small element stores must copy exact extents, including unaligned sources.
    {
        const BYTE widths[] = {1, 8};
        BOOL passed = TRUE;
        for (BYTE w = 0; w < (BYTE) (sizeof(widths) / sizeof(widths[0])); ++w) {
            NUINT width = widths[w];
            ARRAY array;
            array_init(&array, width);
            if (status_code_is_success(array_expand(&array, 4).status)) {
                BYTE input[10];
                BYTE original[8];
                for (NUINT i = 0; i < width; ++i) {
                    input[i + (NUINT) 1] = (BYTE) (0x10 + i);
                    original[i] = (BYTE) (0x80 + i);
                }
                array_add(&array, original);
                array_add(&array, original);
                array_insert(&array, 1, input + 1);
                if (!value_matches(array_get(&array, 0), original, width) ||
                    !value_matches(array_get(&array, 1), input + 1, width) ||
                    !value_matches(array_get(&array, 2), original, width)) passed = FALSE;
                array_set(&array, 1, original);
                if (!value_matches(array_get(&array, 1), original, width)) passed = FALSE;
                array_remove(&array, 1);
                if (array.length != 2 || !value_matches(array_get(&array, 1), original, width)) passed = FALSE;
            } else passed = FALSE;
            if (!status_code_is_success(array_free(&array).status)) passed = FALSE;
        }
        check(&failed, passed, "small element stores preserve neighbors and unaligned inputs");
    }

    // Byte searches across wide blocks and tails, including every key value.
    {
        MAP map;
        map_init(&map, 1, 1);
#if TARGET_SIZEOF_ADDRESS >= 2
        const BYTE capacity = 65;
#else
        const BYTE capacity = 17;
#endif
        BOOL passed = status_code_is_success(map_expand(&map, capacity).status);
        if (passed) {
            const BYTE starts[] = {0, 0x7F, 0x80, 0xF0, 0xFF};
            for (BYTE s = 0; s < (BYTE) (sizeof(starts) / sizeof(starts[0])); ++s) {
                map_clear(&map);
                for (BYTE n = 0; n <= capacity; ++n) {
                    BYTE key = 0;
                    do {
                        BYTE index = (BYTE) (key - starts[s]);
                        const BYTE* value = (const BYTE*) map_get(&map, &key);
                        if (index < n) {
                            if (value == 0 || *value != index) passed = FALSE;
                        } else if (value != 0) passed = FALSE;
                        ++key;
                    } while (key != 0);
                    if (n < capacity) {
                        key = (BYTE) (starts[s] + n);
                        if (!map_add(&map, &key, &n)) passed = FALSE;
                    }
                }
            }
        }
        if (!status_code_is_success(map_free(&map).status)) passed = FALSE;
        check(&failed, passed, "single-byte searches preserve exact indices across block boundaries and byte values");
    }


    // ------------------------------------------------------------
    // Queue: FIFO order, circular wrap, resize, copied output, reuse
    // ------------------------------------------------------------
    {
        QUEUE queue;
        BYTE input[2] = {0, 0xA5};
        BYTE output[2] = {0xEE, 0xEF};
        catrl_queue_init(&queue, 2);
        check(&failed, queue.data == 0 && queue.size == 2 && queue.length == 0 &&
            queue.capacity == 0 && queue.head == 0, "queue_init empty");
        check(&failed, !queue_dequeue(&queue, output) && output[0] == 0xEE &&
            output[1] == 0xEF && queue_peek(&queue) == 0, "queue empty retrieval preserves output");
        check(&failed, matches(queue_expand(&queue, 0), STATUS_CODE_SUCCESS_NOOP, 0x29, 1) &&
            matches(queue_shrink(&queue), STATUS_CODE_SUCCESS_NOOP, 0x2A, 1) &&
            matches(queue_free(&queue), STATUS_CODE_SUCCESS_NOOP, 0x2B, 1), "queue unallocated lifecycle no-ops");
        check(&failed, matches(queue_expand(&queue, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x29, 2) &&
            queue.data == 0 && queue.capacity == 0, "queue extent overflow");
        if (check(&failed, matches(queue_expand(&queue, 4), STATUS_CODE_SUCCESS, 0x29, 4), "queue expand")) {
            for (BYTE i = 0; i < 4; ++i) {
                input[0] = i;
                catrl_queue_enqueue(&queue, input);
            }
            BOOL passed = TRUE;
            for (BYTE i = 0; i < 3; ++i) {
                if (!queue_dequeue(&queue, output) || output[0] != i || output[1] != 0xA5) passed = FALSE;
            }
            for (BYTE i = 4; i < 7; ++i) {
                input[0] = i;
                queue_enqueue(&queue, input);
            }
            check(&failed, passed && queue.head == 3 && queue.length == 4 &&
                queue_peek(&queue) != 0 && ((const BYTE*) queue_peek(&queue))[0] == 3,
                "queue wraps and preserves FIFO");
            check(&failed, matches(queue_shrink(&queue), STATUS_CODE_SUCCESS_NOOP, 0x2A, 1) &&
                matches(queue_expand(&queue, 3), STATUS_CODE_SUCCESS_NOOP, 0x29, 1) &&
                matches(queue_expand(&queue, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x29, 2) &&
                queue.head == 3 && queue.capacity == 4 && queue.length == 4, "queue wrapped no-ops and overflow");
            if (check(&failed, matches(catrl_queue_expand(&queue, 8), STATUS_CODE_SUCCESS, 0x29, 4) &&
                    queue.capacity == 8 && queue.head == 0, "queue expand wrapped ring")) {
                passed = TRUE;
                for (BYTE i = 3; i < 7; ++i) {
                    if (!catrl_queue_dequeue(&queue, output) || output[0] != i || output[1] != 0xA5) passed = FALSE;
                }
                check(&failed, passed && queue.length == 0 && queue.head == 0, "queue expanded order");
                for (BYTE i = 0; i < 8; ++i) {
                    input[0] = i;
                    queue_enqueue(&queue, input);
                }
                for (BYTE i = 0; i < 7; ++i) queue_dequeue(&queue, output);
                input[0] = 8; queue_enqueue(&queue, input);
                input[0] = 9; queue_enqueue(&queue, input);
                if (check(&failed, matches(queue_shrink(&queue), STATUS_CODE_SUCCESS, 0x2A, 3) &&
                        queue.capacity == 4 && queue.head == 0 && queue.length == 3, "queue shrink wrapped ring")) {
                    passed = TRUE;
                    for (BYTE i = 7; i < 10; ++i)
                        if (!queue_dequeue(&queue, output) || output[0] != i || output[1] != 0xA5) passed = FALSE;
                    check(&failed, passed, "queue shrunken order");
                }
            }
            queue_clear(&queue);
            check(&failed, queue.head == 0 && queue.length == 0 && queue.capacity != 0, "queue clear retains storage");
            input[0] = 0x5A;
            queue_enqueue(&queue, input);
            queue_enqueue(&queue, input);
            check(&failed, queue.length == 2 && queue_dequeue(&queue, output) && output[0] == 0x5A &&
                queue_dequeue(&queue, output) && output[0] == 0x5A, "queue permits duplicate values");
        }
        result = catrl_queue_free(&queue);
        check(&failed, status_code_is_success(result.status) && queue.data == 0 && queue.capacity == 0 &&
            queue.length == 0 && queue.head == 0 && queue.size == 2, "queue free retains size");
        if (check(&failed, matches(queue_expand(&queue, 1), STATUS_CODE_SUCCESS, 0x29, 4), "queue reuse after free"))
            check(&failed, matches(queue_shrink(&queue), STATUS_CODE_SUCCESS, 0x2A, 3) &&
                queue.data == 0 && queue.capacity == 0, "queue shrink final slot");
        check(&failed, status_code_is_success(queue_free(&queue).status), "queue cleanup");
    }

    // ------------------------------------------------------------
    // Stack: LIFO, duplicated values, resize, borrowed top and reuse
    // ------------------------------------------------------------
    {
        STACK stack;
        BYTE a[2] = {0x12, 0x34}, b[2] = {0x56, 0x78};
        BYTE output[2] = {0xEE, 0xEF};
        stack_init(&stack, 2);
        check(&failed, stack.data == 0 && stack.length == 0 && stack.capacity == 0 && stack.size == 2 &&
            stack_peek(&stack) == 0 && !stack_pop(&stack, output) && output[0] == 0xEE && output[1] == 0xEF,
            "stack init and empty retrieval");
        check(&failed, matches(stack_expand(&stack, 0), STATUS_CODE_SUCCESS_NOOP, 0x31, 1) &&
            matches(stack_shrink(&stack), STATUS_CODE_SUCCESS_NOOP, 0x32, 1) &&
            matches(stack_free(&stack), STATUS_CODE_SUCCESS_NOOP, 0x33, 1), "stack unallocated lifecycle no-ops");
        check(&failed, matches(stack_expand(&stack, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x31, 2),
            "stack overflow");
        if (check(&failed, matches(catrl_stack_expand(&stack, 3), STATUS_CODE_SUCCESS, 0x31, 4), "stack expand")) {
            catrl_stack_push(&stack, a); stack_push(&stack, b); stack_push(&stack, a);
            check(&failed, stack.length == 3 && value_matches(stack_peek(&stack), a, 2) &&
                matches(stack_shrink(&stack), STATUS_CODE_SUCCESS_NOOP, 0x32, 1), "stack full with duplicates");
            if (check(&failed, matches(stack_expand(&stack, 8), STATUS_CODE_SUCCESS, 0x31, 4) &&
                    matches(stack_shrink(&stack), STATUS_CODE_SUCCESS, 0x32, 3) && stack.capacity == 4, "stack resize")) {
                check(&failed, value_matches(catrl_stack_peek(&stack), a, 2) &&
                    catrl_stack_pop(&stack, output) && value_matches(output, a, 2) &&
                    stack_pop(&stack, output) && value_matches(output, b, 2) &&
                    stack_pop(&stack, output) && value_matches(output, a, 2) && stack.length == 0, "stack LIFO");
            }
            stack_clear(&stack);
            check(&failed, stack.capacity != 0 && stack.length == 0, "stack clear retains capacity");
            stack_push(&stack, a);
            check(&failed, value_matches(stack_peek(&stack), a, 2), "stack clear reuse");
        }
        check(&failed, status_code_is_success(catrl_stack_free(&stack).status) && stack.data == 0 &&
            stack.length == 0 && stack.capacity == 0 && stack.size == 2, "stack free retains size");
        if (check(&failed, matches(stack_expand(&stack, 1), STATUS_CODE_SUCCESS, 0x31, 4), "stack reuse after free"))
            check(&failed, matches(stack_shrink(&stack), STATUS_CODE_SUCCESS, 0x32, 3) &&
                stack.data == 0 && stack.capacity == 0, "stack shrink final slot");
        check(&failed, status_code_is_success(stack_free(&stack).status), "stack cleanup");
    }

    // ------------------------------------------------------------
    // Pool: occupancy bits, padding, sparse indices, resize and reuse
    // ------------------------------------------------------------
    {
        POOL pool;
        NUINT index = CATALYST_NUINT_MAX;
        pool_init(&pool, 2);
        check(&failed, pool.data == 0 && pool.occupied == 0 && pool.size == 2 && pool.cardinality == 0 &&
            pool.capacity == 0 && !pool_acquire(&pool, &index) && index == CATALYST_NUINT_MAX &&
            !pool_contains(&pool, 0) && !pool_release(&pool, 0) && pool_get(&pool, 0) == 0, "pool empty");
        check(&failed, matches(pool_expand(&pool, 0), STATUS_CODE_SUCCESS_NOOP, 0x3A, 1) &&
            matches(pool_shrink(&pool), STATUS_CODE_SUCCESS_NOOP, 0x3B, 1) &&
            matches(pool_free(&pool), STATUS_CODE_SUCCESS_NOOP, 0x3C, 1), "pool unallocated lifecycle no-ops");
        check(&failed, matches(pool_expand(&pool, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x3A, 2) &&
            pool.data == 0 && pool.occupied == 0, "pool extent overflow");
        if (check(&failed, matches(catrl_pool_expand(&pool, 9), STATUS_CODE_SUCCESS, 0x3A, 5), "pool expand across bitmap boundary")) {
            BOOL passed = TRUE;
            for (NUINT i = 0; i < 9; ++i) {
                if (!catrl_pool_acquire(&pool, &index) || index != i) { passed = FALSE; break; }
                BYTE* slot = (BYTE*) pool_get(&pool, index);
                if (slot == 0) { passed = FALSE; break; }
                slot[0] = (BYTE) i; slot[1] = 0xA5;
            }
            check(&failed, passed && pool.cardinality == 9 &&
                ((const BYTE*) pool.occupied)[0] == 0xFF && ((const BYTE*) pool.occupied)[1] == 1,
                "pool lowest free indices and zero padding");
            if (passed) {
                index = CATALYST_NUINT_MAX;
                check(&failed, !pool_acquire(&pool, &index) && index == CATALYST_NUINT_MAX &&
                    !pool_contains(&pool, 9) && pool_get(&pool, 9) == 0 && !pool_release(&pool, 9), "pool full and out of range");
                for (NUINT i = 0; i < 8; ++i) if (!catrl_pool_release(&pool, i)) passed = FALSE;
                check(&failed, passed && !pool_release(&pool, 0) && !pool_contains(&pool, 0) &&
                    pool_get(&pool, 0) == 0 && pool.cardinality == 1 &&
                    matches(pool_shrink(&pool), STATUS_CODE_SUCCESS_NOOP, 0x3B, 1), "pool sparse highest index prevents truncation");
                if (check(&failed, matches(pool_expand(&pool, 17), STATUS_CODE_SUCCESS, 0x3A, 5) &&
                        pool.capacity == 17 && pool_contains(&pool, 8) && !pool_contains(&pool, 9),
                        "pool expand preserves sparse index")) {
                    check(&failed, matches(pool_shrink(&pool), STATUS_CODE_SUCCESS, 0x3B, 4) &&
                        pool.capacity == 9 && pool.cardinality == 1 && pool_contains(&pool, 8),
                        "pool shrink retains highest occupied index");
                    const BYTE expected[2] = {8, 0xA5};
                    check(&failed, value_matches(catrl_pool_get(&pool, 8), expected, 2), "pool retained slot contents");
                    check(&failed, pool_acquire(&pool, &index) && index == 0 && catrl_pool_contains(&pool, 0),
                        "pool reacquires released index");
                }
            }
            pool_clear(&pool);
            check(&failed, pool.cardinality == 0 && pool.capacity != 0 && !pool_contains(&pool, 8) &&
                pool_acquire(&pool, &index) && index == 0, "pool clear retains capacity and clears bits");
        }
        check(&failed, status_code_is_success(catrl_pool_free(&pool).status) && pool.data == 0 &&
            pool.occupied == 0 && pool.capacity == 0 && pool.cardinality == 0 && pool.size == 2, "pool free retains size");
        if (check(&failed, matches(pool_expand(&pool, 1), STATUS_CODE_SUCCESS, 0x3A, 5), "pool reuse after free"))
            check(&failed, matches(pool_shrink(&pool), STATUS_CODE_SUCCESS, 0x3B, 4) &&
                pool.data == 0 && pool.occupied == 0 && pool.capacity == 0, "pool shrink final slot");
        check(&failed, status_code_is_success(pool_free(&pool).status), "pool cleanup");
    }


    // Verify dependent allocation failures, not merely collection arithmetic.
    {
        QUEUE queue;
        STACK stack;
        BYTE value = 0xA5;
        queue_init(&queue, 1);
        stack_init(&stack, 1);
        if (check(&failed, matches(queue_expand(&queue, 2), STATUS_CODE_SUCCESS, 0x29, 4), "queue dependency failure setup")) {
            queue_enqueue(&queue, &value);
            POINTERW original = queue.data;
            check(&failed, matches(queue_expand(&queue, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x29, 3) &&
                queue.data == original && queue.head == 0 && queue.length == 1 && queue.capacity == 2 &&
                value_matches(queue_peek(&queue), &value, 1), "queue dependent overflow retains ring");
        }
        if (check(&failed, matches(stack_expand(&stack, 2), STATUS_CODE_SUCCESS, 0x31, 4), "stack dependency failure setup")) {
            stack_push(&stack, &value);
            POINTERW original = stack.data;
            check(&failed, matches(stack_expand(&stack, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x31, 3) &&
                stack.data == original && stack.length == 1 && stack.capacity == 2 &&
                value_matches(stack_peek(&stack), &value, 1), "stack dependent overflow retains elements");
        }
        check(&failed, status_code_is_success(queue_free(&queue).status), "queue dependency failure cleanup");
        check(&failed, status_code_is_success(stack_free(&stack).status), "stack dependency failure cleanup");

        // A one-slot bitmap fits; its maximum-size payload cannot include the
        // allocator's private prefix. Retry expansion and release retained ownership.
        POOL pool;
        pool_init(&pool, CATALYST_NUINT_MAX);
        result = pool_expand(&pool, 1);
        if (check(&failed, matches(result, STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x3A, 4) &&
                pool.occupied != 0 && pool.data == 0 && pool.capacity == 0 && pool.cardinality == 0,
                "pool data allocation failure retains bitmap ownership")) {
            NUINT index = CATALYST_NUINT_MAX;
            check(&failed, !pool_acquire(&pool, &index) && index == CATALYST_NUINT_MAX &&
                !pool_contains(&pool, 0) && pool_get(&pool, 0) == 0 &&
                matches(pool_expand(&pool, 1), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x3A, 4),
                "pool partial expansion remains logically empty on retry");
            check(&failed, matches(pool_shrink(&pool), STATUS_CODE_SUCCESS_NOOP, 0x3B, 1),
                "pool zero logical capacity shrink after partial expansion");
        }
        check(&failed, status_code_is_success(pool_free(&pool).status) && pool.occupied == 0 &&
            pool.data == 0 && pool.capacity == 0 && pool.cardinality == 0, "pool partial expansion cleanup");
        check(&failed, matches(pool_free(&pool), STATUS_CODE_SUCCESS_NOOP, 0x3C, 1), "pool retry cleanup no-op");
    }

    // Exercise both scratch-assisted expansion and bounded rotations of a
    // wrapped ring, including gaps, equal ranges, and unequal large ranges.
    {
#if TARGET_SIZEOF_ADDRESS >= 2
        const NUINT capacity = 128;
        const NUINT heads[] = {1, 17, 40, 64, 87, 127};
#else
        const NUINT capacity = 8;
        const NUINT heads[] = {1, 3, 4, 7};
#endif
        BOOL passed = TRUE;
        for (NUINT h = 0; h < (NUINT) (sizeof(heads) / sizeof(heads[0])) && passed; ++h) {
            for (BYTE mode = 0; mode < (BYTE) 3 && passed; ++mode) {
                for (BYTE gap = 0; gap < (BYTE) 2 && passed; ++gap) {
                    QUEUE queue;
                    queue_init(&queue, 1);
                    passed = matches(queue_expand(&queue, capacity), STATUS_CODE_SUCCESS, 0x29, 4);
                    if (passed) {
                        for (NUINT i = 0; i < capacity; ++i) {
                            BYTE v = (BYTE) i;
                            queue_enqueue(&queue, &v);
                        }
                        BYTE output;
                        for (NUINT i = 0; i < heads[h]; ++i)
                            if (!queue_dequeue(&queue, &output) || output != (BYTE) i) passed = FALSE;
                        NUINT added = gap == 0 ? heads[h] : (NUINT) (heads[h] / (NUINT) 2);
                        for (NUINT i = 0; i < added; ++i) {
                            BYTE v = (BYTE) (capacity + i);
                            queue_enqueue(&queue, &v);
                        }
                        NUINT live = (NUINT) (capacity - heads[h] + added);
                        if (mode == 0) {
                            passed = matches(queue_expand(&queue, (NUINT) (capacity + (NUINT) 1)), STATUS_CODE_SUCCESS, 0x29, 4) && passed;
                        } else if (mode == 1) {
                            NUINT expanded = (NUINT) (capacity + (NUINT) (capacity / (NUINT) 2));
                            passed = matches(queue_expand(&queue, expanded), STATUS_CODE_SUCCESS, 0x29, 4) && passed;
                        } else {
                            result = queue_shrink(&queue);
                            NUINT expected = live > capacity / (NUINT) 2 ? live : (NUINT) (capacity / (NUINT) 2);
                            passed = matches(result, expected == capacity ? STATUS_CODE_SUCCESS_NOOP : STATUS_CODE_SUCCESS,
                                0x2A, expected == capacity ? 1 : 3) && queue.capacity == expected && passed;
                        }
                        for (NUINT i = 0; i < live && passed; ++i)
                            if (!queue_dequeue(&queue, &output) || output != (BYTE) (heads[h] + i)) passed = FALSE;
                        if (queue.length != 0 || queue.head != 0) passed = FALSE;
                    }
                    if (!status_code_is_success(queue_free(&queue).status)) passed = FALSE;
                }
            }
        }
        check(&failed, passed, "queue normalization preserves wrapped and gapped sequences through resize");
    }

    // Force the bitmap word scan, every first-zero-bit position, and padding.
    {
#if TARGET_SIZEOF_ADDRESS >= 2
        const NUINT capacity = 129;
#else
        const NUINT capacity = 17;
#endif
        POOL pool;
        pool_init(&pool, 1);
        BOOL passed = matches(pool_expand(&pool, capacity), STATUS_CODE_SUCCESS, 0x3A, 5);
        for (NUINT i = 0; i < capacity && passed; ++i) {
            NUINT index;
            passed = pool_acquire(&pool, &index) && index == i;
        }
        for (NUINT i = 0; i < capacity && passed; ++i) {
            NUINT index = CATALYST_NUINT_MAX;
            passed = pool_release(&pool, i) && pool_acquire(&pool, &index) && index == i;
        }
        if (passed) {
            NUINT index = CATALYST_NUINT_MAX;
            passed = !pool_acquire(&pool, &index) && index == CATALYST_NUINT_MAX;
            pool_clear(&pool);
            pool_clear(&pool);
            passed = pool.cardinality == 0 && pool_acquire(&pool, &index) && index == 0 && passed;
        }
        if (!status_code_is_success(pool_free(&pool).status)) passed = FALSE;
        check(&failed, passed, "pool bitmap scans all bit positions, word boundaries and padding");
    }

    // ------------------------------------------------------------
    // Done
    // ------------------------------------------------------------
    if (failed != 0) {
        std::printf("\n%d CATRLLIB test(s) failed.\n", failed);
        return 1;
    }
    std::printf("\nAll CATRLLIB tests passed.\n");
    return 0;
}
