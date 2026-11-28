#include <cstdio>
#include "catoolib.hpp"

using namespace catalyst;

#define CHECK(condition) do { if (!(condition)) { std::printf("FAIL: %s (line %d)\n", #condition, __LINE__); return 1; } } while (0)

static BOOL matches(CATALYST_RESULT result, STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation, DETAIL_CODE detail = 0) {
    return result.status == status && result.context == context && result.operation == operation && result.detail == detail ? TRUE : FALSE;
}

static const BYTE value_key = 1;
static const BYTE method_key = 2;
static const BYTE property_key = 3;
static const BYTE extra_key = 4;

static CATALYST_RESULT get_value(OBJECT* object, POINTERW output) {
    MEMBER* member = object_get(object, &value_key);
    if (!member) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xA0, 1, 0);
    member_get(member, output);
    return RESULT(STATUS_CODE_SUCCESS, 0xA0, 2, 0);
}

static CATALYST_RESULT set_value(OBJECT* object, POINTER input) {
    MEMBER* member = object_get(object, &value_key);
    if (!member) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xA1, 1, 0);
    member_set(member, input);
    return RESULT(STATUS_CODE_SUCCESS, 0xA1, 2, 0);
}

static CATALYST_RESULT call_value(OBJECT* object, POINTER arguments, POINTERW output) {
    RESULT result = set_value(object, arguments);
    if (!status_code_is_success(result.status)) return result;
    return get_value(object, output);
}

static CATALYST_RESULT match_size(const MEMBER* member, const MEMBER* requirement, BOOL* valid) {
    *valid = member->size == requirement->size ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0xA2, 1, 0);
}

static CATALYST_RESULT fail_validation(const MEMBER*, const MEMBER*, BOOL*) {
    return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xA3, 1, 0);
}

int main() {
    BYTE value = 17, changed = 42, output = 0;
    MEMBER member;
    CHECK(matches(member_init(&member, 0), STATUS_CODE_ERROR_INVALID_ARGUMENT, 1, 1, 1));
    CHECK(matches(member_free(&member), STATUS_CODE_SUCCESS_NOOP, 4, 1));
    CHECK(matches(member_init(&member, 1), STATUS_CODE_SUCCESS, 1, 3));
    member_get(&member, &output);
    CHECK(output == 0);
    member_set(&member, &value);
    member_get(&member, &output);
    CHECK(output == value);
    CHECK(matches(member_free(&member), STATUS_CODE_SUCCESS, 4, 3));
    CHECK(member.data == 0 && member.size == 0);

    OBJECT object;
    object_init(&object, 1);
    CHECK(matches(object_add(&object, &value_key, &value, 1), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 6, 3));
    CHECK(matches(object_expand(&object, 3), STATUS_CODE_SUCCESS, 10, 1));
    CHECK(matches(catoo_object_add(&object, &value_key, &value, 1), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_add(&object, &value_key, &changed, 1), STATUS_CODE_SUCCESS_NOOP, 6, 1));
    CHECK(object_contains(&object, &value_key));
    FIELD field;
    CHECK(!field_init(&field, &object, &extra_key) && !field.object && !field.key && !field.member);
    CHECK(field_init(&field, &object, &value_key));
    field_get(&field, &output);
    CHECK(output == value);
    field_set(&field, &changed);
    field_get(&field, &output);
    CHECK(output == changed && value == 17);

    METHOD method = call_value;
    PROPERTY property;
    property.get = get_value;
    property.set = set_value;
    CHECK(matches(object_add(&object, &method_key, &method, (NUINT) sizeof(method)), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_add(&object, &property_key, &property, (NUINT) sizeof(property)), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(field_init(&field, &object, &method_key));
    CHECK(matches(method_invoke(&field, &value, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == value);
    CHECK(field_init(&field, &object, &property_key));
    CHECK(matches(property_set(&field, &changed), STATUS_CODE_SUCCESS, 0x13, 1));
    CHECK(matches(property_get(&field, &output), STATUS_CODE_SUCCESS, 0x12, 1) && output == changed);

    STRUCTURE structure;
    CHECK(matches(structure_init(&structure, &object), STATUS_CODE_SUCCESS, 0x14, 5));
    MEMBER* stored = structure_get(&structure, &value_key);
    CHECK(stored != 0 && stored->data != object_get(&object, &value_key)->data);
    member_set(stored, &value);
    CHECK(structure_get(&structure, &extra_key) == 0);
    member_get(object_get(&object, &value_key), &output);
    CHECK(output == changed);
    CHECK(matches(structure_free(&structure), STATUS_CODE_SUCCESS, 0x16, 1));

    CLASS class_, base;
    CHECK(matches(class_init(&class_, &object), STATUS_CODE_SUCCESS, 0x17, 5));
    OBJECT extra;
    object_init(&extra, 1);
    CHECK(status_code_is_success(object_expand(&extra, 1).status));
    CHECK(status_code_is_success(object_add(&extra, &extra_key, &value, 1).status));
    CHECK(matches(class_init(&base, &extra), STATUS_CODE_SUCCESS, 0x17, 5));
    CHECK(matches(class_extend(&class_, &base), STATUS_CODE_SUCCESS, 0x19, 8));
    CHECK(matches(class_extend(&class_, &base), STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x19, 3));
    CHECK(class_.object.members.cardinality == 4);
    OBJECT instance;
    CHECK(matches(class_create(&class_, &instance), STATUS_CODE_SUCCESS, 0x18, 5));
    CHECK(object_contains(&instance, &extra_key));
    CHECK(field_init(&field, &instance, &method_key));
    CHECK(matches(method_invoke(&field, &value, &output), STATUS_CODE_SUCCESS, 0x11, 1));
    member_get(object_get(&class_.object, &value_key), &output);
    CHECK(output == changed);
    CHECK(matches(class_free(&base), STATUS_CODE_SUCCESS, 0x1A, 1));
    CHECK(matches(class_free(&class_), STATUS_CODE_SUCCESS, 0x1A, 1));
    CHECK(field_init(&field, &instance, &property_key));
    CHECK(matches(property_get(&field, &output), STATUS_CODE_SUCCESS, 0x12, 1) && output == value);

    INTERFACE interface_;
    BOOL valid = FALSE;
    CHECK(matches(interface_init(&interface_, &extra, match_size), STATUS_CODE_SUCCESS, 0x1B, 5));
    CHECK(matches(interface_validate(&interface_, &instance, &valid), STATUS_CODE_SUCCESS, 0x1C, 8) && valid);
    CHECK(matches(interface_validate(&interface_, &object, &valid), STATUS_CODE_SUCCESS, 0x1C, 2) && !valid);
    CHECK(matches(interface_free(&interface_), STATUS_CODE_SUCCESS, 0x1D, 1) && interface_.validate == 0);
    CHECK(matches(interface_init(&interface_, &extra, fail_validation), STATUS_CODE_SUCCESS, 0x1B, 5));
    valid = TRUE;
    CHECK(matches(interface_validate(&interface_, &instance, &valid), STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1C, 3, 1) && valid);
    CHECK(status_code_is_success(interface_free(&interface_).status));

    OBJECT wide;
    object_init(&wide, 2);
    CHECK(matches(interface_init(&interface_, &extra, match_size), STATUS_CODE_SUCCESS, 0x1B, 5));
    CHECK(matches(interface_validate(&interface_, &wide, &valid), STATUS_CODE_SUCCESS, 0x1C, 1) && !valid);
    CHECK(status_code_is_success(interface_free(&interface_).status));
    CHECK(matches(interface_init(&interface_, &wide, 0), STATUS_CODE_SUCCESS, 0x1B, 5));
    CHECK(matches(interface_validate(&interface_, &instance, &valid), STATUS_CODE_SUCCESS, 0x1C, 8) && valid);
    CHECK(status_code_is_success(interface_free(&interface_).status));
    CHECK(status_code_is_success(object_free(&wide).status));

    CHECK(matches(object_remove(&object, &extra_key), STATUS_CODE_SUCCESS_NOOP, 7, 1));
    CHECK(matches(object_remove(&object, &method_key), STATUS_CODE_SUCCESS, 7, 3));
    CHECK(!object_contains(&object, &method_key));
    CHECK(matches(object_clear(&object), STATUS_CODE_SUCCESS, 0x0C, 3) && object.members.cardinality == 0 && object.members.capacity == 3);
    CHECK(matches(object_clear(&object), STATUS_CODE_SUCCESS_NOOP, 0x0C, 1));
    CHECK(matches(object_shrink(&object), STATUS_CODE_SUCCESS, 0x0B, 1) && object.members.capacity == 1);
    CHECK(matches(object_free(&object), STATUS_CODE_SUCCESS, 0x0D, 2));
    CHECK(matches(object_free(&object), STATUS_CODE_SUCCESS_NOOP, 0x0D, 2));
    CHECK(status_code_is_success(object_free(&extra).status));
    CHECK(status_code_is_success(object_free(&instance).status));
    std::printf("PASS: CATOOLIB composites\n");
    return 0;
}
