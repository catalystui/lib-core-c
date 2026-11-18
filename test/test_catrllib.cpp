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
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: array create\n");
        return 1;
    }
    result = array_length(array, &count);
    print_result("array length", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: array length\n");
        return 2;
    }
    for (BYTE i = 0; i < 8; i++) {
        value = 99;
        result = array_get(array, i, &value);
        print_result("array initial value", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != 0) {
            printf("FAIL: array initial value\n");
            return 3;
        }
        value = i + 10;
        result = array_set(array, i, &value);
        print_result("array set", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
            printf("FAIL: array set\n");
            return 4;
        }
    }
    for (BYTE i = 0; i < 8; i++) {
        result = array_get(array, i, &value);
        print_result("array get", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != i + 10) {
            printf("FAIL: array get\n");
            return 5;
        }
    }
    value = 99;
    result = array_get(array, 8, &value);
    print_result("array out of bounds", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x02 || result.detail != 0x01 || value != 99) {
        printf("FAIL: array out of bounds\n");
        return 6;
    }
    result = array_destroy(array);
    print_result("array destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: array destroy\n");
        return 7;
    }
    array = 0;
    result = array_create(&array, sizeof(BYTE), 0);
    print_result("empty array create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: empty array create\n");
        return 8;
    }
    result = array_length(array, &count);
    print_result("empty array length", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: empty array length\n");
        return 9;
    }
    result = array_destroy(array);
    print_result("empty array destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
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
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || set == 0) {
        printf("FAIL: set create\n");
        return 11;
    }
    for (BYTE i = 0; i < 8; i++) {
        result = set_add(set, &i);
        print_result("set add", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00) {
            printf("FAIL: set add\n");
            return 12;
        }
    }
    value = 3;
    result = set_add(set, &value);
    print_result("set duplicate", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: set duplicate\n");
        return 13;
    }
    result = set_count(set, &count);
    print_result("set count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: set count\n");
        return 14;
    }
    result = set_contains(set, &value, &contains);
    print_result("set contains", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: set contains\n");
        return 15;
    }
    BYTE seen = 0;
    for (BYTE i = 0; i < count; i++) {
        result = set_at(set, i, &value);
        print_result("set enumerate", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value >= 8 || (seen & ((BYTE) 1 << value)) != 0) {
            printf("FAIL: set enumerate\n");
            return 16;
        }
        seen |= (BYTE) 1 << value;
    }
    value = 3;
    result = set_remove(set, &value);
    print_result("set remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x06 || result.detail != 0x00) {
        printf("FAIL: set remove\n");
        return 17;
    }
    result = set_contains(set, &value, &contains);
    print_result("set removed membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != FALSE) {
        printf("FAIL: set removed membership\n");
        return 18;
    }
    result = set_remove(set, &value);
    print_result("set remove absent", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: set remove absent\n");
        return 19;
    }
    result = set_count(set, &count);
    print_result("set count after remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 7) {
        printf("FAIL: set count after remove\n");
        return 20;
    }
    result = set_clear(set);
    print_result("set clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: set clear\n");
        return 21;
    }
    result = set_count(set, &count);
    print_result("set empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: set empty count\n");
        return 22;
    }
    result = set_clear(set);
    print_result("set clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: set clear empty\n");
        return 23;
    }
    result = set_add(set, &value);
    print_result("set reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00) {
        printf("FAIL: set reuse\n");
        return 24;
    }
    result = set_contains(set, &value, &contains);
    print_result("set reuse membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: set reuse membership\n");
        return 25;
    }
    result = set_destroy(set);
    print_result("set destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
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
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || map == 0) {
        printf("FAIL: map create\n");
        return 27;
    }
    for (BYTE i = 0; i < 8; i++) {
        value = (NUINT) i + 100;
        result = map_set(map, &i, &value);
        print_result("map set", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
            printf("FAIL: map set\n");
            return 28;
        }
    }
    key = 3;
    value = 199;
    result = map_set(map, &key, &value);
    print_result("map replace", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map replace\n");
        return 29;
    }
    value = 0;
    result = map_get(map, &key, &value);
    print_result("map get replaced", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || value != 199) {
        printf("FAIL: map get replaced\n");
        return 30;
    }
    result = map_count(map, &count);
    print_result("map count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: map count\n");
        return 31;
    }
    result = map_contains(map, &key, &contains);
    print_result("map contains", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: map contains\n");
        return 32;
    }
    seen = 0;
    for (BYTE i = 0; i < count; i++) {
        result = map_at(map, i, &key, &value);
        print_result("map enumerate", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || key >= 8 || (seen & ((BYTE) 1 << key)) != 0 || value != (key == 3 ? 199 : (BYTE) (key + 100))) {
            printf("FAIL: map enumerate\n");
            return 33;
        }
        seen |= (BYTE) 1 << key;
    }
    key = 3;
    result = map_remove(map, &key);
    print_result("map remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x06 || result.detail != 0x00) {
        printf("FAIL: map remove\n");
        return 34;
    }
    result = map_contains(map, &key, &contains);
    print_result("map removed membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != FALSE) {
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
    result = map_remove(map, &key);
    print_result("map remove absent", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: map remove absent\n");
        return 37;
    }
    result = map_count(map, &count);
    print_result("map count after remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 7) {
        printf("FAIL: map count after remove\n");
        return 38;
    }
    result = map_clear(map);
    print_result("map clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: map clear\n");
        return 39;
    }
    result = map_count(map, &count);
    print_result("map empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: map empty count\n");
        return 40;
    }
    result = map_clear(map);
    print_result("map clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: map clear empty\n");
        return 41;
    }
    result = map_set(map, &key, &value);
    print_result("map reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map reuse\n");
        return 42;
    }
    value = 0;
    result = map_get(map, &key, &value);
    print_result("map reuse value", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || value != 99) {
        printf("FAIL: map reuse value\n");
        return 43;
    }
    result = map_destroy(map);
    print_result("map destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: map destroy\n");
        return 44;
    }
    printf("Map verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Stack: capacity growth and last-in, first-out behavior.
     * --------------------------------------------------------------------------------------------- */

    STACK* stack = 0;
    result = stack_create(&stack, sizeof(BYTE));
    print_result("stack create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || stack == 0) {
        printf("FAIL: stack create\n");
        return 45;
    }
    value = 99;
    result = stack_pop(stack, &value);
    print_result("stack empty pop", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x1A || result.operation != 0x03 || result.detail != 0x00 || value != 99) {
        printf("FAIL: stack empty pop\n");
        return 46;
    }
    result = stack_peek(stack, &value);
    print_result("stack empty peek", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x1B || result.operation != 0x03 || result.detail != 0x00 || value != 99) {
        printf("FAIL: stack empty peek\n");
        return 47;
    }
    for (BYTE i = 0; i < 8; i++) {
        result = stack_push(stack, &i);
        print_result("stack push", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
            printf("FAIL: stack push\n");
            return 48;
        }
    }
    result = stack_count(stack, &count);
    print_result("stack count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: stack count\n");
        return 49;
    }
    result = stack_peek(stack, &value);
    print_result("stack peek", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != 7) {
        printf("FAIL: stack peek\n");
        return 50;
    }
    result = stack_count(stack, &count);
    print_result("stack count after peek", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: stack count after peek\n");
        return 51;
    }
    for (BYTE i = 8; i != 0; i--) {
        result = stack_pop(stack, &value);
        print_result("stack pop", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != i - 1) {
            printf("FAIL: stack pop\n");
            return 52;
        }
    }
    result = stack_count(stack, &count);
    print_result("stack empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: stack empty count\n");
        return 53;
    }
    result = stack_push(stack, &value);
    print_result("stack reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
        printf("FAIL: stack reuse\n");
        return 54;
    }
    result = stack_clear(stack);
    print_result("stack clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: stack clear\n");
        return 55;
    }
    result = stack_count(stack, &count);
    print_result("stack cleared count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: stack cleared count\n");
        return 56;
    }
    result = stack_clear(stack);
    print_result("stack clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: stack clear empty\n");
        return 57;
    }
    result = stack_destroy(stack);
    print_result("stack destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: stack destroy\n");
        return 58;
    }
    printf("Stack verified successfully.\n");

    /* ---------------------------------------------------------------------------------------------
     * Invalid arguments and representable size limits.
     * --------------------------------------------------------------------------------------------- */

    array = 0;
    set = 0;
    map = 0;
    stack = 0;
    result = array_create(&array, 0, 1);
    print_result("array zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x01 || result.operation != 0x02 || result.detail != 0x01 || array != 0) {
        printf("FAIL: array zero element size\n");
        return 59;
    }
    result = array_create(&array, 2, CATALYST_NUINT_MAX);
    print_result("array size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x01 || result.operation != 0x03 || result.detail != 0x01 || array != 0) {
        printf("FAIL: array size overflow\n");
        return 60;
    }
    result = set_create(&set, 0);
    print_result("set zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x06 || result.operation != 0x02 || result.detail != 0x01 || set != 0) {
        printf("FAIL: set zero element size\n");
        return 61;
    }
    result = map_create(&map, CATALYST_NUINT_MAX, 1);
    print_result("map size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0E || result.operation != 0x04 || result.detail != 0x00 || map != 0) {
        printf("FAIL: map size overflow\n");
        return 62;
    }
    result = stack_create(&stack, 0);
    print_result("stack zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x17 || result.operation != 0x02 || result.detail != 0x01 || stack != 0) {
        printf("FAIL: stack zero element size\n");
        return 63;
    }
    result = array_get(0, 0, &value);
    print_result("array null handle", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x01 || result.detail != 0x00) {
        printf("FAIL: array null handle\n");
        return 64;
    }

    // Growth fails before touching the element when allocator extent overflows.
    // This deliberately probes the failure path before a maximum-size payload could be read.
    set = 0;
    result = set_create(&set, CATALYST_NUINT_MAX);
    if (set == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 65;
    result = set_add(set, &value);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x08 || result.operation != 0x05 || result.detail != 0x02) return 66;
    result = set_count(set, &count);
    if (count != 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 67;
    result = set_destroy(set);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 68;

    // Ignoring the returned Result does not change the operation.
    BYTE item = 0xA5;
    array_create(&array, 1, 1);
    if (array == 0) return 69;
    array_set(array, 0, &item);
    value = 0;
    array_get(array, 0, &value);
    if (value != item) return 70;
    array_destroy(array);

    // Validate remaining argument positions and leave output values unchanged.
    result = array_create(&array, 1, 1);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 71;
    result = array_get(array, 0, 0);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x03 || result.detail != 2) return 72;
    result = array_set(array, 1, &item);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x04 || result.operation != 0x02 || result.detail != 1) return 73;
    result = array_destroy(array);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 74;

    result = map_create(&map, 1, 0);
    if (map != 0 || result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x0E || result.operation != 0x03 || result.detail != 2) return 75;

#if TARGET_SIZEOF_VOID_P > 1
    // The 17th 17-byte element crosses an eight-bit offset boundary.
    BYTE wide[17] = {0};
    BYTE readback[17] = {0};
    wide[0] = 0x12;
    wide[16] = 0x34;
    result = array_create(&array, 17, 17);
    if (array == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 76;
    result = array_set(array, 16, wide);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x05 || result.detail != 0) return 77;
    result = array_get(array, 16, readback);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x05 || result.detail != 0 || readback[0] != 0x12 || readback[16] != 0x34) return 78;
    result = array_destroy(array);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 79;

    // A map's combined key/value extent crosses an eight-bit carry boundary.
    BYTE largeKey[255] = {0};
    BYTE largeValue[2] = {0x56, 0x78};
    BYTE largeReadback[2] = {0};
    largeKey[254] = 0x9A;
    result = map_create(&map, 255, 2);
    if (map == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x05 || result.detail != 0) return 80;
    result = map_set(map, largeKey, largeValue);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x08 || result.detail != 0) return 81;
    result = map_get(map, largeKey, largeReadback);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x07 || result.detail != 0 || largeReadback[0] != 0x56 || largeReadback[1] != 0x78) return 82;
    result = map_destroy(map);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 83;

    // Exercise count carry/borrow and growth over 255 elements.
    result = stack_create(&stack, 1);
    if (stack == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 84;
    for (BYTE pass = 0; pass < 2; ++pass) {
        for (BYTE i = 0; i < 128; ++i) {
            result = stack_push(stack, &item);
            if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x05 || result.detail != 0) return 85;
        }
    }
    result = stack_count(stack, &count);
    if (count != 256 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 86;
    result = stack_pop(stack, &value);
    if (value != item || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x05 || result.detail != 0) return 87;
    result = stack_count(stack, &count);
    if (count != 255 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 88;
    result = stack_destroy(stack);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 89;
#endif

    printf("CATRLTEST :: All relational collection tests passed. 🔥\n");

    return 0;
}
