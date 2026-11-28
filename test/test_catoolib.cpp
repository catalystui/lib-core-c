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

/* Exercise binary keys, member byte copies, map relocation, and reuse. */
static int test_object_lifecycle() {
    OBJECT object;
    BYTE key[] = { 0, 1 };
    const BYTE original_key[] = { 0, 1 };
    const BYTE second_key[] = { 0, 2 };
    const BYTE third_key[] = { 1, 1 };
    BYTE data[] = { 0x12, 0x34, 0x56 };
    const BYTE expected[] = { 0x12, 0x34, 0x56 };
    BYTE output[3] = { 0, 0, 0 };

    catoo_object_init(&object, 2);
    CHECK(object.members.cardinality == 0 && object.members.capacity == 0);
    CHECK(matches(object_expand(&object, 0), STATUS_CODE_SUCCESS_NOOP, 0x0A, 1));
    CHECK(matches(object_expand(&object, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0A, 1));
    CHECK(object.members.cardinality == 0 && object.members.capacity == 0);
    CHECK(matches(object_expand(&object, 3), STATUS_CODE_SUCCESS, 0x0A, 1));
    CHECK(matches(object_add(&object, key, data, 0), STATUS_CODE_ERROR_INVALID_ARGUMENT, 6, 2, 3));
    CHECK(object_get(&object, key) == 0);
    CHECK(matches(object_add(&object, key, data, 3), STATUS_CODE_SUCCESS, 6, 5));

    key[1] = 2;
    data[0] = 0xFF;
    CHECK(object_contains(&object, original_key) && !object_contains(&object, key));
    MEMBER* member = catoo_object_get(&object, original_key);
    CHECK(member != 0 && member->size == 3);
    catoo_member_get(member, output);
    CHECK(catmm_compare(output, expected, 3) == 0);
    catoo_member_set(member, member->data);
    catoo_member_get(member, member->data);
    CHECK(catmm_compare(member->data, expected, 3) == 0);

    CHECK(matches(object_add(&object, second_key, data, 3), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_add(&object, third_key, expected, 3), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_expand(&object, 6), STATUS_CODE_SUCCESS, 0x0A, 1));
    CHECK(matches(object_remove(&object, second_key), STATUS_CODE_SUCCESS, 7, 3));
    CHECK(object_get(&object, second_key) == 0);
    CHECK(matches(object_shrink(&object), STATUS_CODE_SUCCESS, 0x0B, 1));
    CHECK(object.members.capacity == 3 && object.members.cardinality == 2);

    FIELD field;
    CHECK(catoo_field_init(&field, &object, third_key));
    CHECK(field.object == &object && field.key != third_key);
    CHECK(catmm_compare(field.key, third_key, 2) == 0);
    BYTE query[] = { 1, 1 };
    CHECK(field_init(&field, &object, query));
    query[1] = 2;
    CHECK(catmm_compare(field.key, third_key, 2) == 0);
    CHECK(field.member == object_get(&object, third_key));
    catoo_field_get(&field, output);
    CHECK(catmm_compare(output, expected, 3) == 0);
    catoo_field_set(&field, data);
    CHECK(catmm_compare(field.member->data, data, 3) == 0);
    CHECK(!field_init(&field, &object, second_key));
    CHECK(field.object == 0 && field.key == 0 && field.member == 0);

    CHECK(matches(object_free(&object), STATUS_CODE_SUCCESS, 0x0D, 2));
    CHECK(object.members.keys == 0 && object.members.values == 0);
    CHECK(object.members.ksize == 2 && object.members.capacity == 0);
    CHECK(matches(object_expand(&object, 1), STATUS_CODE_SUCCESS, 0x0A, 1));
    CHECK(matches(object_add(&object, original_key, expected, 3), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_free(&object), STATUS_CODE_SUCCESS, 0x0D, 2));
    return 0;
}

/* The class copies payload bytes but does not own objects referenced by them. */
static int test_independent_instances() {
    OBJECT definition;
    BYTE shared = 7, initial = 11, replacement = 22, output = 0;
    BYTE* reference = &shared;
    BYTE* recovered = 0;
    const BYTE reference_key = 5;

    object_init(&definition, 1);
    CHECK(status_code_is_success(object_expand(&definition, 2).status));
    CHECK(status_code_is_success(object_add(&definition, &value_key, &initial, 1).status));
    CHECK(status_code_is_success(object_add(&definition, &reference_key, &reference, (NUINT) sizeof(reference)).status));

    CLASS class_;
    CHECK(matches(catoo_class_init(&class_, &definition), STATUS_CODE_SUCCESS, 0x17, 5));
    CHECK(status_code_is_success(object_free(&definition).status));

    OBJECT first, second;
    CHECK(matches(catoo_class_create(&class_, &first), STATUS_CODE_SUCCESS, 0x18, 5));
    CHECK(matches(class_create(&class_, &second), STATUS_CODE_SUCCESS, 0x18, 5));
    CHECK(matches(catoo_class_free(&class_), STATUS_CODE_SUCCESS, 0x1A, 1));

    MEMBER* first_member = object_get(&first, &value_key);
    MEMBER* second_member = object_get(&second, &value_key);
    CHECK(first_member != 0 && second_member != 0);
    CHECK(first_member->data != second_member->data);
    member_set(first_member, &replacement);
    member_get(second_member, &output);
    CHECK(output == initial);
    member_get(first_member, &output);
    CHECK(output == replacement);

    MEMBER* first_reference = object_get(&first, &reference_key);
    MEMBER* second_reference = object_get(&second, &reference_key);
    CHECK(first_reference != 0 && second_reference != 0);
    CHECK(first_reference->data != second_reference->data);
    member_get(first_reference, &recovered);
    CHECK(recovered == &shared);
    *recovered = 9;
    member_get(second_reference, &recovered);
    CHECK(recovered == &shared && *recovered == 9);
    CHECK(status_code_is_success(object_free(&first).status));
    CHECK(shared == 9);
    member_get(second_member, &output);
    CHECK(output == initial);
    CHECK(status_code_is_success(object_free(&second).status));
    return 0;
}

static CATALYST_RESULT warning_method(OBJECT*, POINTER, POINTERW) {
    return RESULT(STATUS_CODE_WARNING_PARTIAL, 0xB0, 0x05, 0x5A);
}

static CATALYST_RESULT failing_getter(OBJECT*, POINTERW) {
    return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xB1, 0x06, 0x33);
}

static CATALYST_RESULT failing_setter(OBJECT*, POINTER) {
    return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0xB2, 0x07, 0x44);
}

static CATALYST_RESULT warning_validator(const MEMBER*, const MEMBER*, BOOL* valid) {
    *valid = FALSE;
    return RESULT(STATUS_CODE_WARNING_PARTIAL, 0xB3, 0x08, 0x66);
}

/* Check dispatch results and distinguish nonconformance from validation errors. */
static int test_callback_results() {
    OBJECT object, requirements;
    BYTE output = 0x77, input = 1;
    const BYTE required[] = { 1, 2 };
    METHOD method = warning_method;
    PROPERTY property;
    property.get = failing_getter;
    property.set = failing_setter;

    object_init(&object, 1);
    CHECK(status_code_is_success(object_expand(&object, 3).status));
    CHECK(status_code_is_success(object_add(&object, &method_key, &method, (NUINT) sizeof(method)).status));
    CHECK(status_code_is_success(object_add(&object, &property_key, &property, (NUINT) sizeof(property)).status));
    CHECK(status_code_is_success(object_add(&object, &value_key, &input, 1).status));

    FIELD field;
    CHECK(field_init(&field, &object, &method_key));
    CHECK(matches(catoo_method_invoke(&field, &input, &output), STATUS_CODE_WARNING_PARTIAL, 0x11, 1, 0x5A));
    CHECK(output == 0x77);
    CHECK(field_init(&field, &object, &property_key));
    CHECK(matches(catoo_property_get(&field, &output), STATUS_CODE_ERROR_NOT_FOUND, 0x12, 1, 0x33));
    CHECK(output == 0x77);
    CHECK(matches(catoo_property_set(&field, &input), STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x13, 1, 0x44));

    object_init(&requirements, 1);
    CHECK(status_code_is_success(object_expand(&requirements, 1).status));
    CHECK(status_code_is_success(object_add(&requirements, &value_key, required, 2).status));

    INTERFACE interface_;
    BOOL valid = TRUE;
    CHECK(matches(catoo_interface_init(&interface_, &requirements, match_size), STATUS_CODE_SUCCESS, 0x1B, 5));
    CHECK(matches(catoo_interface_validate(&interface_, &object, &valid), STATUS_CODE_SUCCESS, 0x1C, 7));
    CHECK(valid == FALSE);
    CHECK(matches(catoo_interface_free(&interface_), STATUS_CODE_SUCCESS, 0x1D, 1));

    CHECK(matches(interface_init(&interface_, &requirements, warning_validator), STATUS_CODE_SUCCESS, 0x1B, 5));
    valid = TRUE;
    CHECK(matches(interface_validate(&interface_, &object, &valid), STATUS_CODE_WARNING_PARTIAL, 0x1C, 6, 0x66));
    CHECK(valid == TRUE);
    CHECK(status_code_is_success(interface_free(&interface_).status));
    CHECK(status_code_is_success(object_free(&requirements).status));
    CHECK(status_code_is_success(object_free(&object).status));
    return 0;
}

/* Reject collisions before changing the derived layout, including a base
 * with both an absent key and an overlapping key. */
static int test_class_extension_boundaries() {
    OBJECT derived_definition, base_definition, wide_definition, empty_definition;
    BYTE initial = 1, other = 2;
    const BYTE wide_key[] = { 0, 1 };
    object_init(&derived_definition, 1);
    object_init(&base_definition, 1);
    object_init(&wide_definition, 2);
    object_init(&empty_definition, 1);
    CHECK(status_code_is_success(object_expand(&derived_definition, 1).status));
    CHECK(status_code_is_success(object_expand(&base_definition, 2).status));
    CHECK(status_code_is_success(object_expand(&wide_definition, 1).status));
    CHECK(status_code_is_success(object_add(&derived_definition, &value_key, &initial, 1).status));
    CHECK(status_code_is_success(object_add(&base_definition, &extra_key, &other, 1).status));
    CHECK(status_code_is_success(object_add(&base_definition, &value_key, &other, 1).status));
    CHECK(status_code_is_success(object_add(&wide_definition, wide_key, &other, 1).status));

    CLASS derived, base, wide, empty;
    CHECK(status_code_is_success(class_init(&derived, &derived_definition).status));
    CHECK(status_code_is_success(class_init(&base, &base_definition).status));
    CHECK(status_code_is_success(class_init(&wide, &wide_definition).status));
    CHECK(status_code_is_success(class_init(&empty, &empty_definition).status));
    CHECK(matches(class_extend(&derived, &empty), STATUS_CODE_SUCCESS_NOOP, 0x19, 1));
    CHECK(matches(class_extend(&derived, &wide), STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 2, 1));
    CHECK(matches(catoo_class_extend(&derived, &base), STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x19, 3));
    CHECK(derived.object.members.cardinality == 1 && derived.object.members.capacity == 1);
    CHECK(!object_contains(&derived.object, &extra_key));
    MEMBER* original = object_get(&derived.object, &value_key);
    BYTE output = 0;
    CHECK(original != 0);
    member_get(original, &output);
    CHECK(output == initial);

    STRUCTURE structure;
    CHECK(matches(catoo_structure_init(&structure, &empty_definition), STATUS_CODE_SUCCESS, 0x14, 5));
    CHECK(catoo_structure_get(&structure, &value_key) == 0);
    CHECK(matches(catoo_structure_free(&structure), STATUS_CODE_SUCCESS_NOOP, 0x16, 1));
    OBJECT instance;
    CHECK(matches(class_create(&empty, &instance), STATUS_CODE_SUCCESS, 0x18, 5));
    CHECK(instance.members.cardinality == 0 && instance.members.capacity == 0);
    CHECK(matches(object_free(&instance), STATUS_CODE_SUCCESS_NOOP, 0x0D, 2));

    CHECK(status_code_is_success(class_free(&derived).status));
    CHECK(status_code_is_success(class_free(&base).status));
    CHECK(status_code_is_success(class_free(&wide).status));
    CHECK(status_code_is_success(class_free(&empty).status));
    CHECK(status_code_is_success(object_free(&derived_definition).status));
    CHECK(status_code_is_success(object_free(&base_definition).status));
    CHECK(status_code_is_success(object_free(&wide_definition).status));
    CHECK(status_code_is_success(object_free(&empty_definition).status));
    return 0;
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
    CHECK(test_object_lifecycle() == 0);
    std::printf("PASS: binary keys, capacity, borrowed fields, and reuse\n");
    CHECK(test_independent_instances() == 0);
    std::printf("PASS: independent instances and borrowed references\n");
    CHECK(test_callback_results() == 0);
    std::printf("PASS: callback status and structural validation\n");
    CHECK(test_class_extension_boundaries() == 0);
    std::printf("PASS: class extension boundaries and empty composites\n");
    std::printf("PASS: CATOOLIB composites\n");
    return 0;
}
