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
    NUINT value = 0;
    BOOL contains = FALSE;

    printf("CATRLTEST :: Starting relational collection tests...\n");

    /* ---------------------------------------------------------------------------------------------
     * Array: fixed length, zero initialization, and indexed access.
     * --------------------------------------------------------------------------------------------- */

    ARRAY* array = 0;
    array_create(&array, sizeof(NUINT), 8, &result);
    print_result("array create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: array create\n");
        return 1;
    }
    array_length(array, &count, &result);
    print_result("array length", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: array length\n");
        return 2;
    }
    for (NUINT i = 0; i < 8; i++) {
        value = 99;
        array_get(array, i, &value, &result);
        print_result("array initial value", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != 0) {
            printf("FAIL: array initial value\n");
            return 3;
        }
        value = i + 10;
        array_set(array, i, &value, &result);
        print_result("array set", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
            printf("FAIL: array set\n");
            return 4;
        }
    }
    for (NUINT i = 0; i < 8; i++) {
        array_get(array, i, &value, &result);
        print_result("array get", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != i + 10) {
            printf("FAIL: array get\n");
            return 5;
        }
    }
    value = 99;
    array_get(array, 8, &value, &result);
    print_result("array out of bounds", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x02 || result.detail != 0x01 || value != 99) {
        printf("FAIL: array out of bounds\n");
        return 6;
    }
    array_destroy(array, &result);
    print_result("array destroy", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: array destroy\n");
        return 7;
    }
    array = 0;
    array_create(&array, sizeof(NUINT), 0, &result);
    print_result("empty array create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || array == 0) {
        printf("FAIL: empty array create\n");
        return 8;
    }
    array_length(array, &count, &result);
    print_result("empty array length", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: empty array length\n");
        return 9;
    }
    array_destroy(array, &result);
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
    set_create(&set, sizeof(NUINT), &result);
    print_result("set create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || set == 0) {
        printf("FAIL: set create\n");
        return 11;
    }
    for (NUINT i = 0; i < 8; i++) {
        set_add(set, &i, &result);
        print_result("set add", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00) {
            printf("FAIL: set add\n");
            return 12;
        }
    }
    value = 3;
    set_add(set, &value, &result);
    print_result("set duplicate", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: set duplicate\n");
        return 13;
    }
    set_count(set, &count, &result);
    print_result("set count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: set count\n");
        return 14;
    }
    set_contains(set, &value, &contains, &result);
    print_result("set contains", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: set contains\n");
        return 15;
    }
    NUINT seen = 0;
    for (NUINT i = 0; i < count; i++) {
        set_at(set, i, &value, &result);
        print_result("set enumerate", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value >= 8 || (seen & ((NUINT) 1 << value)) != 0) {
            printf("FAIL: set enumerate\n");
            return 16;
        }
        seen |= (NUINT) 1 << value;
    }
    value = 3;
    set_remove(set, &value, &result);
    print_result("set remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x06 || result.detail != 0x00) {
        printf("FAIL: set remove\n");
        return 17;
    }
    set_contains(set, &value, &contains, &result);
    print_result("set removed membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != FALSE) {
        printf("FAIL: set removed membership\n");
        return 18;
    }
    set_remove(set, &value, &result);
    print_result("set remove absent", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: set remove absent\n");
        return 19;
    }
    set_count(set, &count, &result);
    print_result("set count after remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 7) {
        printf("FAIL: set count after remove\n");
        return 20;
    }
    set_clear(set, &result);
    print_result("set clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: set clear\n");
        return 21;
    }
    set_count(set, &count, &result);
    print_result("set empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: set empty count\n");
        return 22;
    }
    set_clear(set, &result);
    print_result("set clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: set clear empty\n");
        return 23;
    }
    set_add(set, &value, &result);
    print_result("set reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00) {
        printf("FAIL: set reuse\n");
        return 24;
    }
    set_contains(set, &value, &contains, &result);
    print_result("set reuse membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: set reuse membership\n");
        return 25;
    }
    set_destroy(set, &result);
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
    map_create(&map, sizeof(BYTE), sizeof(NUINT), &result);
    print_result("map create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || map == 0) {
        printf("FAIL: map create\n");
        return 27;
    }
    for (BYTE i = 0; i < 8; i++) {
        value = (NUINT) i + 100;
        map_set(map, &i, &value, &result);
        print_result("map set", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
            printf("FAIL: map set\n");
            return 28;
        }
    }
    key = 3;
    value = 999;
    map_set(map, &key, &value, &result);
    print_result("map replace", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map replace\n");
        return 29;
    }
    value = 0;
    map_get(map, &key, &value, &result);
    print_result("map get replaced", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || value != 999) {
        printf("FAIL: map get replaced\n");
        return 30;
    }
    map_count(map, &count, &result);
    print_result("map count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: map count\n");
        return 31;
    }
    map_contains(map, &key, &contains, &result);
    print_result("map contains", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != TRUE) {
        printf("FAIL: map contains\n");
        return 32;
    }
    seen = 0;
    for (NUINT i = 0; i < count; i++) {
        map_at(map, i, &key, &value, &result);
        print_result("map enumerate", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || key >= 8 || (seen & ((NUINT) 1 << key)) != 0 || value != (key == 3 ? 999 : (NUINT) key + 100)) {
            printf("FAIL: map enumerate\n");
            return 33;
        }
        seen |= (NUINT) 1 << key;
    }
    key = 3;
    map_remove(map, &key, &result);
    print_result("map remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x06 || result.detail != 0x00) {
        printf("FAIL: map remove\n");
        return 34;
    }
    map_contains(map, &key, &contains, &result);
    print_result("map removed membership", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || contains != FALSE) {
        printf("FAIL: map removed membership\n");
        return 35;
    }
    value = 99;
    map_get(map, &key, &value, &result);
    print_result("map missing key", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x10 || result.operation != 0x05 || result.detail != 0x00 || value != 99) {
        printf("FAIL: map missing key\n");
        return 36;
    }
    map_remove(map, &key, &result);
    print_result("map remove absent", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x04 || result.detail != 0x00) {
        printf("FAIL: map remove absent\n");
        return 37;
    }
    map_count(map, &count, &result);
    print_result("map count after remove", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 7) {
        printf("FAIL: map count after remove\n");
        return 38;
    }
    map_clear(map, &result);
    print_result("map clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: map clear\n");
        return 39;
    }
    map_count(map, &count, &result);
    print_result("map empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: map empty count\n");
        return 40;
    }
    map_clear(map, &result);
    print_result("map clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: map clear empty\n");
        return 41;
    }
    map_set(map, &key, &value, &result);
    print_result("map reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x08 || result.detail != 0x00) {
        printf("FAIL: map reuse\n");
        return 42;
    }
    value = 0;
    map_get(map, &key, &value, &result);
    print_result("map reuse value", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x07 || result.detail != 0x00 || value != 99) {
        printf("FAIL: map reuse value\n");
        return 43;
    }
    map_destroy(map, &result);
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
    stack_create(&stack, sizeof(NUINT), &result);
    print_result("stack create", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || stack == 0) {
        printf("FAIL: stack create\n");
        return 45;
    }
    value = 99;
    stack_pop(stack, &value, &result);
    print_result("stack empty pop", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x1A || result.operation != 0x03 || result.detail != 0x00 || value != 99) {
        printf("FAIL: stack empty pop\n");
        return 46;
    }
    stack_peek(stack, &value, &result);
    print_result("stack empty peek", result);
    if (result.status != STATUS_CODE_ERROR_NOT_FOUND || result.context != 0x1B || result.operation != 0x03 || result.detail != 0x00 || value != 99) {
        printf("FAIL: stack empty peek\n");
        return 47;
    }
    for (NUINT i = 0; i < 8; i++) {
        stack_push(stack, &i, &result);
        print_result("stack push", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
            printf("FAIL: stack push\n");
            return 48;
        }
    }
    stack_count(stack, &count, &result);
    print_result("stack count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: stack count\n");
        return 49;
    }
    stack_peek(stack, &value, &result);
    print_result("stack peek", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != 7) {
        printf("FAIL: stack peek\n");
        return 50;
    }
    stack_count(stack, &count, &result);
    print_result("stack count after peek", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 8) {
        printf("FAIL: stack count after peek\n");
        return 51;
    }
    for (NUINT i = 8; i != 0; i--) {
        stack_pop(stack, &value, &result);
        print_result("stack pop", result);
        if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00 || value != i - 1) {
            printf("FAIL: stack pop\n");
            return 52;
        }
    }
    stack_count(stack, &count, &result);
    print_result("stack empty count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: stack empty count\n");
        return 53;
    }
    stack_push(stack, &value, &result);
    print_result("stack reuse", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x05 || result.detail != 0x00) {
        printf("FAIL: stack reuse\n");
        return 54;
    }
    stack_clear(stack, &result);
    print_result("stack clear", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00) {
        printf("FAIL: stack clear\n");
        return 55;
    }
    stack_count(stack, &count, &result);
    print_result("stack cleared count", result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0x00 || result.operation != 0x03 || result.detail != 0x00 || count != 0) {
        printf("FAIL: stack cleared count\n");
        return 56;
    }
    stack_clear(stack, &result);
    print_result("stack clear empty", result);
    if (result.status != STATUS_CODE_SUCCESS_NOOP || result.context != 0x00 || result.operation != 0x02 || result.detail != 0x00) {
        printf("FAIL: stack clear empty\n");
        return 57;
    }
    stack_destroy(stack, &result);
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
    array_create(&array, 0, 1, &result);
    print_result("array zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x01 || result.operation != 0x02 || result.detail != 0x01 || array != 0) {
        printf("FAIL: array zero element size\n");
        return 59;
    }
    array_create(&array, 2, CATALYST_NUINT_MAX, &result);
    print_result("array size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x01 || result.operation != 0x03 || result.detail != 0x01 || array != 0) {
        printf("FAIL: array size overflow\n");
        return 60;
    }
    set_create(&set, 0, &result);
    print_result("set zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x06 || result.operation != 0x02 || result.detail != 0x01 || set != 0) {
        printf("FAIL: set zero element size\n");
        return 61;
    }
    map_create(&map, CATALYST_NUINT_MAX, 1, &result);
    print_result("map size overflow", result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x0E || result.operation != 0x04 || result.detail != 0x00 || map != 0) {
        printf("FAIL: map size overflow\n");
        return 62;
    }
    stack_create(&stack, 0, &result);
    print_result("stack zero element size", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x17 || result.operation != 0x02 || result.detail != 0x01 || stack != 0) {
        printf("FAIL: stack zero element size\n");
        return 63;
    }
    array_get(0, 0, &value, &result);
    print_result("array null handle", result);
    if (result.status != STATUS_CODE_ERROR_INVALID_ARGUMENT || result.context != 0x03 || result.operation != 0x01 || result.detail != 0x00) {
        printf("FAIL: array null handle\n");
        return 64;
    }

    // Growth fails before touching the element when allocator extent overflows.
    set = 0;
    set_create(&set, CATALYST_NUINT_MAX, &result);
    if (set == 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 65;
    set_add(set, &value, &result);
    if (result.status != STATUS_CODE_ERROR_BUFFER_OVERFLOW || result.context != 0x08 || result.operation != 0x05 || result.detail != 0x02) return 66;
    set_count(set, &count, &result);
    if (count != 0 || result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x03 || result.detail != 0) return 67;
    set_destroy(set, &result);
    if (result.status != STATUS_CODE_SUCCESS || result.context != 0 || result.operation != 0x02 || result.detail != 0) return 68;

    printf("CATRLTEST :: All relational collection tests passed. 🔥\n");

    return 0;
}
