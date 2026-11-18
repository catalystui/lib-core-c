#include "catrllib.hpp"

#include <stdio.h>

using namespace catalyst;

static void print_result(const char* label, const RESULT& result) {
    printf(
        "%s -> status=0x%02X context=0x%02X operation=0x%02X detail=0x%02X\n",
        label,
        (unsigned int) result.status,
        (unsigned int) result.context,
        (unsigned int) result.operation,
        (unsigned int) result.detail
    );
}

// Use a persistent-storage provider, with no cursor exposed by CATRLLIB.
extern "C" CATALYST_RESULT test_file_reader(void* state, CATALYST_NUINT index, CATALYST_BYTE* value) {
    ::FILE* source = (::FILE*) state;
    // The fixture contains only three bytes.
    if (fseek(source, (long) index, SEEK_SET) != 0 || fread(value, 1, 1, source) != 1) {
        return RESULT(STATUS_CODE_ERROR_IO_ERROR, 1, 1, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 1, 2, 0);
}

extern "C" CATALYST_RESULT test_file_failure(void*, CATALYST_NUINT, CATALYST_BYTE* value) {
    *value = 0;
    return RESULT(STATUS_CODE_ERROR_ACCESS_DENIED, 1, 1, 0);
}

struct test_stream_state {
    BYTE step;
    BYTE bytes[2];
};

extern "C" CATALYST_RESULT test_stream_reader(void* state, const CATALYST_BYTE** chunk, CATALYST_NUINT* length) {
    test_stream_state* producer = (test_stream_state*) state;
    switch (producer->step) {
        case 0:
            producer->step = 1;
            producer->bytes[0] = 0;
            producer->bytes[1] = 0xFF;
            *chunk = producer->bytes;
            *length = 2;
            return RESULT(STATUS_CODE_SUCCESS, 1, 1, 0);
        case 1:
            producer->step = 2;
            *chunk = 0;
            *length = 0;
            return RESULT(STATUS_CODE_SUCCESS, 1, 2, 0);
        case 2:
            producer->step = 3;
            producer->bytes[0] = 0x42;
            *chunk = producer->bytes;
            *length = 1;
            return RESULT(STATUS_CODE_SUCCESS, 1, 3, 0);
        case 3:
            producer->step = 4;
            return RESULT(STATUS_CODE_ERROR_IO_ERROR, 1, 4, 0);
        default:
            producer->step = 5;
            return RESULT(STATUS_CODE_SUCCESS_NOOP, 1, 5, 0);
    }
}

extern "C" CATALYST_RESULT test_bad_chunk(void*, const CATALYST_BYTE** chunk, CATALYST_NUINT* length) {
    *chunk = 0;
    *length = 1;
    return RESULT(STATUS_CODE_SUCCESS, 1, 1, 0);
}

static BOOL matches(RESULT result, STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation, DETAIL_CODE detail) {
    return result.status == status && result.context == context && result.operation == operation && result.detail == detail ? TRUE : FALSE;
}

static BOOL test_file_and_stream() {
    ::FILE* source = tmpfile();
    if (source == 0) return FALSE;
    BYTE data[3] = {0, 0x42, 0xFF};
    if (fwrite(data, 1, 3, source) != 3 || fflush(source) != 0) {
        fclose(source);
        return FALSE;
    }
    catalyst::FILE* file = 0;
    BYTE value = 0xAA;
    BOOL ok = matches(file_create(&file, source, test_file_reader, 3), STATUS_CODE_SUCCESS, 0x17, 2, 0);
    if (ok) {
        ok = file_length(file) == 3 &&
             matches(file_get(file, 2, &value), STATUS_CODE_SUCCESS, 0x19, 3, 0) && value == 0xFF &&
             matches(file_get(file, 0, &value), STATUS_CODE_SUCCESS, 0x19, 3, 0) && value == 0 &&
             matches(file_get(file, 2, &value), STATUS_CODE_SUCCESS, 0x19, 3, 0) && value == 0xFF;
        if (!matches(file_destroy(file), STATUS_CODE_SUCCESS, 0x18, 2, 0)) ok = FALSE;
    }
    // Destroying the relation does not close the persistent source.
    if (fseek(source, 1, SEEK_SET) != 0 || fread(&value, 1, 1, source) != 1 || value != 0x42) ok = FALSE;
    if (ok) {
        ok = matches(file_create(&file, source, test_file_reader, 0), STATUS_CODE_SUCCESS, 0x17, 2, 0);
        if (ok) {
            ok = file_length(file) == 0;
            if (!matches(file_destroy(file), STATUS_CODE_SUCCESS, 0x18, 2, 0)) ok = FALSE;
        }
    }
    fclose(source);
    if (!ok) return FALSE;
    if (!matches(file_create(&file, 0, test_file_failure, 1), STATUS_CODE_SUCCESS, 0x17, 2, 0)) return FALSE;
    value = 0xAA;
    if (!matches(file_get(file, 0, &value), STATUS_CODE_ERROR_ACCESS_DENIED, 0x19, 2, 0) || value != 0xAA) return FALSE;
    if (!matches(file_destroy(file), STATUS_CODE_SUCCESS, 0x18, 2, 0)) return FALSE;

    test_stream_state producer = {0, {0, 0}};
    STREAM* stream = 0;
    const BYTE* chunk = 0;
    NUINT length = 0;
    if (!matches(stream_create(&stream, &producer, test_stream_reader), STATUS_CODE_SUCCESS, 0x1B, 2, 0) || producer.step != 0) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_SUCCESS, 0x1D, 6, 0) || length != 2 || chunk != producer.bytes || chunk[0] != 0 || chunk[1] != 0xFF) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_SUCCESS, 0x1D, 6, 0) || chunk != 0 || length != 0) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_SUCCESS, 0x1D, 6, 0) || length != 1 || chunk[0] != 0x42) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_ERROR_IO_ERROR, 0x1D, 4, 0) || chunk != producer.bytes || length != 1) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_SUCCESS_NOOP, 0x1D, 2, 0) || chunk != 0 || length != 0 || producer.step != 5) return FALSE;
    producer.step = 0;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_SUCCESS_NOOP, 0x1D, 1, 0) || chunk != 0 || length != 0 || producer.step != 0) return FALSE;
    if (!matches(stream_destroy(stream), STATUS_CODE_SUCCESS, 0x1C, 2, 0)) return FALSE;
    if (!matches(stream_create(&stream, 0, test_bad_chunk), STATUS_CODE_SUCCESS, 0x1B, 2, 0)) return FALSE;
    if (!matches(stream_next(stream, &chunk, &length), STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x1D, 5, 0)) return FALSE;
    if (!matches(stream_destroy(stream), STATUS_CODE_SUCCESS, 0x1C, 2, 0)) return FALSE;
    printf("File and Stream verified successfully.\n");
    return TRUE;
}

int main() {
    RESULT result;
    NUINT count = 0;
    BYTE value = 0;
    BOOL contains = FALSE;

    printf("CATRLTEST :: Starting relational collection tests...\n");

    /* ---------------------------------------------------------------------------------------------
     * Array: fixed length, zero initialization, and indexed access.
     * --------------------------------------------------------------------------------------------- */

    ARRAY* array = 0;
    result = array_create(&array, sizeof(BYTE), 8);
    print_result("array create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: array create\n");
        return 1;
    }
    count = array_length(array);
    if (count != 8) {
        printf("FAIL: array length\n");
        return 2;
    }
    for (BYTE i = 0; i < 8; i++) {
        value = 99;
        array_get(array, i, &value);
        if (value != 0) {
            printf("FAIL: array initial value\n");
            return 3;
        }
        value = i + 10;
        array_set(array, i, &value);
    }
    for (BYTE i = 0; i < 8; i++) {
        array_get(array, i, &value);
        if (value != i + 10) {
            printf("FAIL: array get\n");
            return 5;
        }
    }
    result = array_destroy(array);
    print_result("array destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: array destroy\n");
        return 7;
    }
    array = 0;
    result = array_create(&array, sizeof(BYTE), 0);
    print_result("empty array create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: empty array create\n");
        return 8;
    }
    count = array_length(array);
    if (count != 0) {
        printf("FAIL: empty array length\n");
        return 9;
    }
    result = array_destroy(array);
    print_result("empty array destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: empty array destroy\n");
        return 10;
    }
    printf("Array verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Set: distinct elements, growth, unordered enumeration, and removal.
     * --------------------------------------------------------------------------------------------- */

    SET* set = 0;
    result = set_create(&set, sizeof(BYTE));
    print_result("set create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x06 || result.operation != 0x03 || result.detail != 0x00 || set == 0) {
        printf("FAIL: set create\n");
        return 11;
    }
    for (BYTE i = 0; i < 8; i++) {
        result = set_add(set, &i);
        print_result("set add", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x08 || result.operation != 0x07 || result.detail != 0x00) {
            printf("FAIL: set add\n");
            return 12;
        }
    }
    value = 3;
    result = set_add(set, &value);
    print_result("set duplicate", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x08 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: set duplicate\n");
        return 13;
    }
    count = set_count(set);
    if (count != 8) {
        printf("FAIL: set count\n");
        return 14;
    }
    contains = set_contains(set, &value);
    if (contains != TRUE) {
        printf("FAIL: set contains\n");
        return 15;
    }
    BYTE seen = 0;
    for (BYTE i = 0; i < count; i++) {
        set_at(set, i, &value);
        if (value >= 8 || (seen & ((BYTE) 1 << value)) != 0) {
            printf("FAIL: set enumerate\n");
            return 16;
        }
        seen |= (BYTE) 1 << value;
    }
    value = 3;
    set_remove(set, &value);
    contains = set_contains(set, &value);
    if (contains != FALSE) {
        printf("FAIL: set removed membership\n");
        return 18;
    }
    set_remove(set, &value);
    count = set_count(set);
    if (count != 7) {
        printf("FAIL: set count after remove\n");
        return 20;
    }
    set_clear(set);
    count = set_count(set);
    if (count != 0) {
        printf("FAIL: set empty count\n");
        return 22;
    }
    set_clear(set);
    result = set_add(set, &value);
    print_result("set reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x08 || result.operation != 0x07 || result.detail != 0x00) {
        printf("FAIL: set reuse\n");
        return 24;
    }
    contains = set_contains(set, &value);
    if (contains != TRUE) {
        printf("FAIL: set reuse membership\n");
        return 25;
    }
    result = set_destroy(set);
    print_result("set destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x07 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: set destroy\n");
        return 26;
    }
    printf("Set verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Map: key/value associations, replacement, growth, and enumeration.
     * --------------------------------------------------------------------------------------------- */

    MAP* map = 0;
    BYTE key = 0;
    result = map_create(&map, sizeof(BYTE), sizeof(BYTE));
    print_result("map create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x0E || result.operation != 0x05 || result.detail != 0x00 || map == 0) {
        printf("FAIL: map create\n");
        return 27;
    }
    for (BYTE i = 0; i < 8; i++) {
        value = (NUINT) i + 100;
        result = map_set(map, &i, &value);
        print_result("map set", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x11 || result.operation != 0x08 || result.detail != 0x00) {
            printf("FAIL: map set\n");
            return 28;
        }
    }
    key = 3;
    value = 199;
    result = map_set(map, &key, &value);
    print_result("map replace", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x11 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map replace\n");
        return 29;
    }
    value = 0;
    result = map_get(map, &key, &value);
    print_result("map get replaced", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x10 || result.operation != 0x07 || result.detail != 0x00 || value != 199) {
        printf("FAIL: map get replaced\n");
        return 30;
    }
    count = map_count(map);
    if (count != 8) {
        printf("FAIL: map count\n");
        return 31;
    }
    contains = map_contains(map, &key);
    if (contains != TRUE) {
        printf("FAIL: map contains\n");
        return 32;
    }
    seen = 0;
    for (BYTE i = 0; i < count; i++) {
        map_at(map, i, &key, &value);
        if (key >= 8 || (seen & ((BYTE) 1 << key)) != 0 || value != (key == 3 ? 199 : (BYTE) (key + 100))) {
            printf("FAIL: map enumerate\n");
            return 33;
        }
        seen |= (BYTE) 1 << key;
    }
    key = 3;
    map_remove(map, &key);
    contains = map_contains(map, &key);
    if (contains != FALSE) {
        printf("FAIL: map removed membership\n");
        return 35;
    }
    value = 99;
    result = map_get(map, &key, &value);
    print_result("map missing key", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x10 || result.operation != 0x05 || result.detail != 0x00 || value != 99) {
        printf("FAIL: map missing key\n");
        return 36;
    }
    map_remove(map, &key);
    count = map_count(map);
    if (count != 7) {
        printf("FAIL: map count after remove\n");
        return 38;
    }
    map_clear(map);
    count = map_count(map);
    if (count != 0) {
        printf("FAIL: map empty count\n");
        return 40;
    }
    map_clear(map);
    result = map_set(map, &key, &value);
    print_result("map reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x11 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map reuse\n");
        return 42;
    }
    value = 0;
    result = map_get(map, &key, &value);
    print_result("map reuse value", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x10 || result.operation != 0x07 || result.detail != 0x00 || value != 99) {
        printf("FAIL: map reuse value\n");
        return 43;
    }
    result = map_destroy(map);
    print_result("map destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x0F || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: map destroy\n");
        return 44;
    }
    printf("Map verified successfully.\n");

    if (!test_file_and_stream()) return 45;

    /* ---------------------------------------------------------------------------------------------
     * Representable size limits.
     * --------------------------------------------------------------------------------------------- */

    array = 0;
    set = 0;
    map = 0;
    result = array_create(&array, 2, CATALYST_NUINT_MAX);
    print_result("array size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x01 || result.operation != 0x03 || result.detail != 0x01 || array != 0) {
        printf("FAIL: array size overflow\n");
        return 60;
    }
    result = map_create(&map, CATALYST_NUINT_MAX, 1);
    print_result("map size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0E || result.operation != 0x04 || result.detail != 0x00 || map != 0) {
        printf("FAIL: map size overflow\n");
        return 62;
    }

    // Ignoring the returned Result does not change the operation.
    BYTE item = 0xA5;
    array_create(&array, 1, 1);
    if (array == 0) return 69;
    array_set(array, 0, &item);
    value = 0;
    array_get(array, 0, &value);
    if (value != item) return 70;
    array_destroy(array);


#if TARGET_SIZEOF_VOID_P > 1
    // The 17th 17-byte element crosses an eight-bit offset boundary.
    BYTE wide[17] = {0};
    BYTE readback[17] = {0};
    wide[0] = 0x12;
    wide[16] = 0x34;
    result = array_create(&array, 17, 17);
    if (array == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0x01 || result.operation != 0x03 || result.detail != 0) return 76;
    array_set(array, 16, wide);
    array_get(array, 16, readback);
    if (readback[0] != 0x12 || readback[16] != 0x34) return 78;
    result = array_destroy(array);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x02 || result.operation != 0x02 || result.detail != 0) return 79;

    // A map's combined key/value extent crosses an eight-bit carry boundary.
    BYTE largeKey[255] = {0};
    BYTE largeValue[2] = {0x56, 0x78};
    BYTE largeReadback[2] = {0};
    largeKey[254] = 0x9A;
    result = map_create(&map, 255, 2);
    if (map == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0x0E || result.operation != 0x05 || result.detail != 0) return 80;
    result = map_set(map, largeKey, largeValue);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x11 || result.operation != 0x08 || result.detail != 0) return 81;
    result = map_get(map, largeKey, largeReadback);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x10 || result.operation != 0x07 || result.detail != 0 || largeReadback[0] != 0x56 || largeReadback[1] != 0x78) return 82;
    result = map_destroy(map);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x0F || result.operation != 0x02 || result.detail != 0) return 83;

    // Exercise count carry/borrow and growth over 255 distinct elements.
    result = set_create(&set, 2);
    if (set == 0 || result.status != STATUS_CODE_SUCCESS) return 84;
    BYTE keyBytes[2] = {0, 0};
    for (BYTE pass = 0; pass < 2; ++pass) {
        keyBytes[0] = pass;
        for (BYTE i = 0; i < 128; ++i) {
            keyBytes[1] = i;
            if (set_add(set, keyBytes).status != STATUS_CODE_SUCCESS) return 85;
        }
    }
    if (set_count(set) != 256) return 86;
    keyBytes[1] = 127;
    set_remove(set, keyBytes);
    if (set_count(set) != 255) return 88;
    if (set_destroy(set).status != STATUS_CODE_SUCCESS) return 89;
#endif

    // Empty collection searches are ordinary outcomes.
    result = set_create(&set, 1);
    if (set == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0x06 || result.operation != 0x03 || result.detail != 0) return 90;
    contains = set_contains(set, &item);
    if (contains != FALSE) return 91;
    result = set_destroy(set);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x07 || result.operation != 0x02 || result.detail != 0) return 96;

    result = map_create(&map, 1, 1);
    if (map == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0x0E || result.operation != 0x05 || result.detail != 0) return 97;
    contains = TRUE;
    contains = map_contains(map, &item);
    if (contains != FALSE) return 98;
    value = 99;
    result = map_get(map, &item, &value);
    if (value != 99 || result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x10 || result.operation != 0x05 || result.detail != 0) return 99;
    result = map_destroy(map);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x0F || result.operation != 0x02 || result.detail != 0) return 105;

    printf("CATRLTEST :: All relational collection tests passed. 🔥\n");

    return 0;
}
