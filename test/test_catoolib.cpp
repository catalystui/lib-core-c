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

/* Exercise deterministic allocation-extent failure and independent clones.
 * The allocator rejects an unrepresentable extent before allocating memory. */
static int test_allocation_extent_and_clone() {
    MEMBER member;
    CHECK(matches(member_init(&member, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 1, 2));
    CHECK(member.data == 0 && member.size == 0);
    CHECK(matches(member_free(&member), STATUS_CODE_SUCCESS_NOOP, 4, 1));

    OBJECT source;
    object_init(&source, 2);
    const BYTE keys[][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 } };
    BYTE data[] = { 0x12, 0x34, 0x56 };
    CHECK(status_code_is_success(object_expand(&source, 3).status));
    for (BYTE i = 0; i < 3; ++i)
        CHECK(matches(object_add(&source, keys[i], &data[i], 1), STATUS_CODE_SUCCESS, 6, 5));
    CHECK(matches(object_expand(&source, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0A, 1));
    CHECK(source.members.cardinality == 3 && source.members.capacity == 3);

    STRUCTURE structure;
    CHECK(matches(structure_init(&structure, &source), STATUS_CODE_SUCCESS, 0x14, 5));
    CLASS class_;
    CHECK(matches(class_init(&class_, &source), STATUS_CODE_SUCCESS, 0x17, 5));
    OBJECT clone;
    CHECK(matches(class_create(&class_, &clone), STATUS_CODE_SUCCESS, 0x18, 5));
    CHECK(matches(object_clear(&source), STATUS_CODE_SUCCESS, 0x0C, 3));
    CHECK(source.members.cardinality == 0 && source.members.capacity == 3);
    CHECK(matches(object_free(&source), STATUS_CODE_SUCCESS, 0x0D, 2));
    CHECK(matches(class_free(&class_), STATUS_CODE_SUCCESS, 0x1A, 1));

    for (BYTE i = 0; i < 3; ++i) {
        MEMBER* a = structure_get(&structure, keys[i]);
        MEMBER* b = object_get(&clone, keys[i]);
        BYTE out = 0;
        CHECK(a != 0 && b != 0 && a->data != b->data);
        member_get(a, &out);
        CHECK(out == data[i]);
        member_get(b, &out);
        CHECK(out == data[i]);
    }
    CHECK(matches(structure_free(&structure), STATUS_CODE_SUCCESS, 0x16, 1));
    CHECK(matches(object_free(&clone), STATUS_CODE_SUCCESS, 0x0D, 2));
    return 0;
}

static BYTE validation_count = 0;
static CATALYST_RESULT validate_order(const MEMBER* member, const MEMBER* requirement, BOOL* valid) {
    BYTE actual = 0, expected = 0;
    member_get(member, &actual);
    member_get(requirement, &expected);
    ++validation_count;
    *valid = actual == expected && expected == validation_count ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0xA5, 1, 0);
}

static int test_interface_layouts() {
    OBJECT required, candidate;
    object_init(&required, 2);
    object_init(&candidate, 2);
    const BYTE keys[][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 } };
    const BYTE values[] = { 1, 2, 3, 4 };
    CHECK(status_code_is_success(object_expand(&required, 3).status));
    CHECK(status_code_is_success(object_expand(&candidate, 4).status));
    for (BYTE i = 0; i < 3; ++i) {
        CHECK(status_code_is_success(object_add(&required, keys[i], &values[i], 1).status));
        CHECK(status_code_is_success(object_add(&candidate, keys[i], &values[i], 1).status));
    }
    INTERFACE interface_;
    CHECK(status_code_is_success(interface_init(&interface_, &required, validate_order).status));
    BOOL valid = FALSE;
    validation_count = 0;
    CHECK(matches(interface_validate(&interface_, &candidate, &valid), STATUS_CODE_SUCCESS, 0x1C, 8) && valid && validation_count == 3);
    CHECK(status_code_is_success(object_remove(&candidate, keys[0]).status));
    CHECK(status_code_is_success(object_add(&candidate, keys[3], &values[3], 1).status));
    CHECK(status_code_is_success(object_add(&candidate, keys[0], &values[0], 1).status));
    validation_count = 0;
    CHECK(matches(interface_validate(&interface_, &candidate, &valid), STATUS_CODE_SUCCESS, 0x1C, 8) && valid && validation_count == 3);
    CHECK(status_code_is_success(object_remove(&candidate, keys[2]).status));
    validation_count = 0;
    CHECK(matches(interface_validate(&interface_, &candidate, &valid), STATUS_CODE_SUCCESS, 0x1C, 2) && !valid && validation_count == 2);
    CHECK(status_code_is_success(interface_free(&interface_).status));
    CHECK(status_code_is_success(object_free(&candidate).status));
    CHECK(status_code_is_success(object_free(&required).status));
    return 0;
}

static int test_remove_payload_key() {
    OBJECT object;
    object_init(&object, 1);
    BYTE key = 7;
    CHECK(status_code_is_success(object_expand(&object, 1).status));
    CHECK(status_code_is_success(object_add(&object, &key, &key, 1).status));
    MEMBER* member = object_get(&object, &key);
    CHECK(member != 0);
    CHECK(matches(object_remove(&object, member->data), STATUS_CODE_SUCCESS, 7, 3));
    CHECK(!object_contains(&object, &key));
    CHECK(status_code_is_success(object_free(&object).status));
    return 0;
}

static int test_byte_class_extension() {
#if TARGET_SIZEOF_VOID_P >= 2
    OBJECT definition, incoming;
    object_init(&definition, 1);
    object_init(&incoming, 1);
    CHECK(status_code_is_success(object_expand(&definition, 32).status));
    CHECK(status_code_is_success(object_expand(&incoming, 8).status));
    for (BYTE i = 0; i < 32; ++i)
        CHECK(status_code_is_success(object_add(&definition, &i, &i, 1).status));
    for (BYTE i = 0; i < 7; ++i) {
        BYTE key = (BYTE) (248 + i);
        CHECK(status_code_is_success(object_add(&incoming, &key, &key, 1).status));
    }
    BYTE overlap = 31, high = 255;
    CHECK(status_code_is_success(object_add(&incoming, &overlap, &overlap, 1).status));
    CLASS destination, base;
    CHECK(status_code_is_success(class_init(&destination, &definition).status));
    CHECK(status_code_is_success(class_init(&base, &incoming).status));
    CHECK(matches(class_extend(&destination, &base), STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x19, 3));
    CHECK(destination.object.members.cardinality == 32 && destination.object.members.capacity == 32);
    CHECK(matches(class_free(&base), STATUS_CODE_SUCCESS, 0x1A, 1));
    CHECK(status_code_is_success(object_remove(&incoming, &overlap).status));
    CHECK(status_code_is_success(object_add(&incoming, &high, &high, 1).status));
    CHECK(status_code_is_success(class_init(&base, &incoming).status));
    CHECK(matches(class_extend(&destination, &base), STATUS_CODE_SUCCESS, 0x19, 8));
    CHECK(destination.object.members.cardinality == 40);
    for (BYTE i = 0; i < 8; ++i) {
        BYTE key = (BYTE) (248 + i), out = 0;
        MEMBER* member = object_get(&destination.object, &key);
        CHECK(member != 0);
        member_get(member, &out);
        CHECK(out == key);
    }
    CHECK(matches(class_extend(&destination, &base), STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x19, 3));
    CHECK(status_code_is_success(class_free(&base).status));
    CHECK(status_code_is_success(class_free(&destination).status));
    CHECK(status_code_is_success(object_free(&definition).status));
    CHECK(status_code_is_success(object_free(&incoming).status));
#endif
    return 0;
}

static BYTE rejected_argument = 0;
static CATALYST_RESULT reject_method(OBJECT*, POINTER, POINTERW) {
    return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xB4, 1, rejected_argument);
}
static CATALYST_RESULT reject_getter(OBJECT*, POINTERW) {
    return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xB5, 1, rejected_argument);
}
static CATALYST_RESULT reject_setter(OBJECT*, POINTER) {
    return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xB6, 1, rejected_argument);
}

static int test_dispatch_argument_results() {
    OBJECT object;
    object_init(&object, 1);
    CHECK(status_code_is_success(object_expand(&object, 2).status));
    METHOD method = reject_method;
    PROPERTY property = { reject_getter, reject_setter };
    CHECK(status_code_is_success(object_add(&object, &method_key, &method, (NUINT) sizeof(method)).status));
    CHECK(status_code_is_success(object_add(&object, &property_key, &property, (NUINT) sizeof(property)).status));
    FIELD callable, accessor;
    CHECK(field_init(&callable, &object, &method_key));
    CHECK(field_init(&accessor, &object, &property_key));
    BYTE input = 1, output = 0x77;
    for (BYTE i = 0; i < 3; ++i) {
        rejected_argument = i;
        CHECK(matches(method_invoke(&callable, &input, &output), STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 1, i));
        STATUS_CODE status = i < 2 ? STATUS_CODE_ERROR_INVALID_ARGUMENT : STATUS_CODE_ERROR_INVALID_STATE;
        OPERATION_CODE operation = i < 2 ? (OPERATION_CODE) 1 : (OPERATION_CODE) 2;
        CHECK(matches(property_get(&accessor, &output), status, 0x12, operation, i));
        CHECK(matches(property_set(&accessor, &input), status, 0x13, operation, i));
        CHECK(output == 0x77);
    }
    rejected_argument = 3;
    CHECK(matches(method_invoke(&callable, &input, &output), STATUS_CODE_ERROR_INVALID_STATE, 0x11, 2, 3));
    rejected_argument = CATALYST_BYTE_MAX;
    CHECK(matches(method_invoke(&callable, &input, &output), STATUS_CODE_ERROR_INVALID_STATE, 0x11, 2, CATALYST_BYTE_MAX));
    CHECK(matches(property_get(&accessor, &output), STATUS_CODE_ERROR_INVALID_STATE, 0x12, 2, CATALYST_BYTE_MAX));
    CHECK(matches(property_set(&accessor, &input), STATUS_CODE_ERROR_INVALID_STATE, 0x13, 2, CATALYST_BYTE_MAX));
    CHECK(output == 0x77);
    CHECK(status_code_is_success(object_free(&object).status));
    return 0;
}

/* Explicit reservation is retained across successful disjoint extensions. */
static int test_reserved_class_extension() {
    OBJECT definition, incoming;
    object_init(&definition, 1);
    object_init(&incoming, 1);
    CHECK(status_code_is_success(object_expand(&definition, 1).status));
    CHECK(status_code_is_success(object_expand(&incoming, 1).status));
    BYTE key = 0, value = 42;
    CHECK(status_code_is_success(object_add(&definition, &key, &value, 1).status));
    CLASS destination;
    CHECK(status_code_is_success(class_init(&destination, &definition).status));
    CHECK(status_code_is_success(object_expand(&destination.object, 5).status));
    POINTER keys = destination.object.members.keys;
    POINTER members = destination.object.members.values;
    for (BYTE i = 1; i < 5; ++i) {
        CHECK(status_code_is_success(object_add(&incoming, &i, &i, 1).status));
        CLASS base;
        CHECK(status_code_is_success(class_init(&base, &incoming).status));
        CHECK(matches(class_extend(&destination, &base), STATUS_CODE_SUCCESS, 0x19, 8));
        CHECK(destination.object.members.capacity == 5);
        CHECK(destination.object.members.cardinality == (NUINT) (i + 1));
        CHECK(destination.object.members.keys == keys && destination.object.members.values == members);
        CHECK(status_code_is_success(class_free(&base).status));
        CHECK(status_code_is_success(object_clear(&incoming).status));
    }
    OBJECT instance;
    CHECK(status_code_is_success(class_create(&destination, &instance).status));
    CHECK(instance.members.cardinality == 5);
    for (BYTE i = 0; i < 5; ++i) {
        BYTE actual = 0;
        MEMBER* m = object_get(&instance, &i);
        CHECK(m != 0);
        member_get(m, &actual);
        CHECK(actual == (i == 0 ? value : i));
    }
    CHECK(status_code_is_success(object_free(&instance).status));
    CHECK(status_code_is_success(class_free(&destination).status));
    CHECK(status_code_is_success(object_free(&incoming).status));
    CHECK(status_code_is_success(object_free(&definition).status));
    return 0;
}

/* A fixed-key structure can still contain variable-length values. */
static int test_member_resize() {
    MEMBER member;
    CHECK(status_code_is_success(member_init(&member, 3).status));
    const BYTE initial[] = { 0x12, 0x34, 0x56 };
    BYTE actual[6];
    member_set(&member, initial);
    POINTER original = member.data;
    CHECK(matches(member_resize(&member, 0), STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1E, 1, 1));
    CHECK(matches(member_resize(&member, 3), STATUS_CODE_SUCCESS_NOOP, 0x1E, 2));
    CHECK(matches(member_resize(&member, CATALYST_NUINT_MAX), STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1E, 3));
    CHECK(member.data == original && member.size == 3);
    CHECK(catmm_compare(member.data, initial, 3) == 0);
    CHECK(matches(catoo_member_resize(&member, 6), STATUS_CODE_SUCCESS, 0x1E, 4));
    member_get(&member, actual);
    CHECK(catmm_compare(actual, initial, 3) == 0);
    CHECK(actual[3] == 0 && actual[4] == 0 && actual[5] == 0);
    const BYTE replacement[] = { 1, 2, 3, 4, 5, 6 };
    member_set(&member, replacement);
    CHECK(matches(member_resize(&member, 2), STATUS_CODE_SUCCESS, 0x1E, 4));
    CHECK(matches(member_resize(&member, 6), STATUS_CODE_SUCCESS, 0x1E, 4));
    member_get(&member, actual);
    CHECK(actual[0] == 1 && actual[1] == 2);
    for (BYTE i = 2; i < 6; ++i) CHECK(actual[i] == 0);
    CHECK(status_code_is_success(member_free(&member).status));

    OBJECT definition;
    object_init(&definition, 1);
    CHECK(status_code_is_success(object_expand(&definition, 1).status));
    CHECK(status_code_is_success(object_add(&definition, &value_key, initial, 3).status));
    STRUCTURE structure;
    CHECK(status_code_is_success(structure_init(&structure, &definition).status));
    FIELD field;
    CHECK(field_init(&field, &structure.object, &value_key));
    MEMBER* resolved = field.member;
    POINTER key = field.key;
    CHECK(matches(member_resize(resolved, 6), STATUS_CODE_SUCCESS, 0x1E, 4));
    field_set(&field, replacement);
    field_get(&field, actual);
    CHECK(catmm_compare(actual, replacement, 6) == 0);
    CHECK(structure.object.members.cardinality == 1 && field.key == key);
    CHECK(field.member == resolved && structure_get(&structure, &value_key) == resolved);
    CHECK(object_get(&definition, &value_key)->size == 3);
    CHECK(status_code_is_success(structure_free(&structure).status));
    CHECK(status_code_is_success(object_free(&definition).status));
    return 0;
}

/* Distinct language-level implementations share a method key and ABI.
 * Nested invocation and receiver mutation must not retain stale descriptors. */
static const BYTE nested_key = 10;
static const BYTE outer_key = 11;
static const BYTE delete_key = 12;
static const BYTE added_key = 13;
static const BYTE grow_key = 14;

extern "C" {
static CATALYST_RESULT polymorphic_first(OBJECT* object, POINTER, POINTERW output) {
    MEMBER* member = object_get(object, &value_key);
    if (!member) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xC0, 1, 0);
    member_get(member, output);
    return RESULT(STATUS_CODE_SUCCESS, 0xC0, 2, 0);
}

static CATALYST_RESULT polymorphic_second(OBJECT* object, POINTER, POINTERW output) {
    MEMBER* member = object_get(object, &value_key);
    if (!member) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xC1, 1, 0);
    BYTE value = 0;
    member_get(member, &value);
    *(BYTE*) output = (BYTE) (value ^ (BYTE) 0xFF);
    return RESULT(STATUS_CODE_SUCCESS, 0xC1, 2, 0);
}

static CATALYST_RESULT invoke_nested(OBJECT* object, POINTER arguments, POINTERW output) {
    MEMBER* member = object_get(object, &nested_key);
    if (!member) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xC2, 1, 0);
    OBJECT* child = 0;
    member_get(member, &child);
    FIELD method;
    if (!field_init(&method, child, &method_key)) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xC2, 2, 0);
    return method_invoke(&method, arguments, output);
}

static CATALYST_RESULT invoke_receiver(OBJECT* object, POINTER arguments, POINTERW output) {
    FIELD field;
    if (!field_init(&field, object, &method_key)) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xC3, 1, 0);
    return method_invoke(&field, arguments, output);
}

static CATALYST_RESULT remove_self(OBJECT* object, POINTER arguments, POINTERW output) {
    RESULT result = object_remove(object, &delete_key);
    if (!status_code_is_success(result.status)) return result;
    *(BYTE*) output = *(const BYTE*) arguments;
    return RESULT(STATUS_CODE_SUCCESS, 0xC4, 1, 0);
}

static CATALYST_RESULT grow_receiver(OBJECT* object, POINTER arguments, POINTERW output) {
    RESULT result = object_expand(object, (NUINT) (object->members.capacity + (NUINT) 1));
    if (!status_code_is_success(result.status)) return result;
    result = object_add(object, &added_key, arguments, 1);
    if (!status_code_is_success(result.status)) return result;
    *(BYTE*) output = *(const BYTE*) arguments;
    return RESULT(STATUS_CODE_SUCCESS, 0xC5, 1, 0);
}

static CATALYST_RESULT validate_byte_value(const MEMBER* member, const MEMBER* requirement, BOOL* valid) {
    *valid = member->size == requirement->size &&
        catmm_compare(member->data, requirement->data, member->size) == 0 ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0xC6, 1, 0);
}
}

static int test_polymorphism_and_nested_objects() {
    OBJECT definition;
    object_init(&definition, 1);
    CHECK(status_code_is_success(object_expand(&definition, 2).status));
    BYTE value = 0x27;
    METHOD method = polymorphic_first;
    CHECK(status_code_is_success(object_add(&definition, &value_key, &value, 1).status));
    CHECK(status_code_is_success(object_add(&definition, &method_key, &method, (NUINT) sizeof(method)).status));
    CLASS first, second;
    CHECK(status_code_is_success(class_init(&first, &definition).status));
    CHECK(status_code_is_success(class_init(&second, &definition).status));
    method = polymorphic_second;
    MEMBER* override_member = object_get(&second.object, &method_key);
    CHECK(override_member != 0);
    member_set(override_member, &method);
    OBJECT instances[2];
    CHECK(status_code_is_success(class_create(&first, &instances[0]).status));
    CHECK(status_code_is_success(class_create(&second, &instances[1]).status));
    CHECK(status_code_is_success(class_free(&first).status));
    CHECK(status_code_is_success(class_free(&second).status));
    CHECK(status_code_is_success(object_free(&definition).status));

    /* Two independent interfaces: a callable-shaped member and a value rule. */
    OBJECT requirement;
    object_init(&requirement, 1);
    CHECK(status_code_is_success(object_expand(&requirement, 1).status));
    CHECK(status_code_is_success(object_add(&requirement, &method_key, &method, (NUINT) sizeof(method)).status));
    INTERFACE callable, state;
    CHECK(status_code_is_success(interface_init(&callable, &requirement, match_size).status));
    CHECK(status_code_is_success(object_clear(&requirement).status));
    CHECK(status_code_is_success(object_add(&requirement, &value_key, &value, 1).status));
    CHECK(status_code_is_success(interface_init(&state, &requirement, validate_byte_value).status));
    CHECK(status_code_is_success(object_free(&requirement).status));

    for (BYTE i = 0; i < 2; ++i) {
        BOOL valid = FALSE;
        CHECK(status_code_is_success(interface_validate(&callable, &instances[i], &valid).status) && valid);
        CHECK(status_code_is_success(interface_validate(&state, &instances[i], &valid).status) && valid);
        FIELD field;
        BYTE output = 0;
        CHECK(field_init(&field, &instances[i], &method_key));
        CHECK(matches(method_invoke(&field, 0, &output), STATUS_CODE_SUCCESS, 0x11, 1));
        CHECK(output == (i == 0 ? value : (BYTE) (value ^ (BYTE) 0xFF)));
    }

    OBJECT parent;
    object_init(&parent, 1);
    CHECK(status_code_is_success(object_expand(&parent, 2).status));
    OBJECT* child = &instances[0];
    CHECK(status_code_is_success(object_add(&parent, &nested_key, &child, (NUINT) sizeof(child)).status));
    method = invoke_nested;
    CHECK(status_code_is_success(object_add(&parent, &method_key, &method, (NUINT) sizeof(method)).status));
    CLASS parent_class;
    CHECK(status_code_is_success(class_init(&parent_class, &parent).status));
    OBJECT other_parent;
    CHECK(status_code_is_success(class_create(&parent_class, &other_parent).status));
    CHECK(status_code_is_success(class_free(&parent_class).status));
    CHECK(status_code_is_success(object_free(&parent).status));
    FIELD field;
    BYTE output = 0;
    CHECK(field_init(&field, &other_parent, &method_key));
    CHECK(matches(method_invoke(&field, 0, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == value);
    child = &instances[1];
    MEMBER* reference = object_get(&other_parent, &nested_key);
    CHECK(reference != 0);
    member_set(reference, &child);
    CHECK(matches(method_invoke(&field, 0, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == (BYTE) (value ^ (BYTE) 0xFF));
    CHECK(status_code_is_success(object_free(&other_parent).status));
    /* Freeing parents did not release either referenced instance. */
    BYTE changed = 0x30;
    MEMBER* member = object_get(&instances[0], &value_key);
    CHECK(member != 0);
    member_set(member, &changed);
    BOOL valid = TRUE;
    CHECK(status_code_is_success(interface_validate(&state, &instances[0], &valid).status) && !valid);
    CHECK(status_code_is_success(interface_validate(&callable, &instances[0], &valid).status) && valid);
    CHECK(status_code_is_success(interface_free(&callable).status));
    CHECK(status_code_is_success(interface_free(&state).status));
    CHECK(status_code_is_success(object_free(&instances[0]).status));
    CHECK(status_code_is_success(object_free(&instances[1]).status));
    return 0;
}

static int test_reentrant_and_mutating_methods() {
    OBJECT object;
    object_init(&object, 1);
    CHECK(status_code_is_success(object_expand(&object, 5).status));
    BYTE value = 0x42, input = 0x93, output = 0;
    METHOD inner = polymorphic_first, outer = invoke_receiver, erase = remove_self, grow = grow_receiver;
    CHECK(status_code_is_success(object_add(&object, &value_key, &value, 1).status));
    CHECK(status_code_is_success(object_add(&object, &method_key, &inner, (NUINT) sizeof(inner)).status));
    CHECK(status_code_is_success(object_add(&object, &outer_key, &outer, (NUINT) sizeof(outer)).status));
    CHECK(status_code_is_success(object_add(&object, &delete_key, &erase, (NUINT) sizeof(erase)).status));
    CHECK(status_code_is_success(object_add(&object, &grow_key, &grow, (NUINT) sizeof(grow)).status));
    FIELD field;
    CHECK(field_init(&field, &object, &outer_key));
    CHECK(matches(method_invoke(&field, 0, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == value);
    CHECK(field_init(&field, &object, &delete_key));
    CHECK(matches(method_invoke(&field, &input, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == input);
    CHECK(!object_contains(&object, &delete_key) && object.members.cardinality == 4);
    /* The previous field may now be invalid; resolve again before using it. */
    CHECK(field_init(&field, &object, &grow_key));
    CHECK(matches(method_invoke(&field, &input, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == input);
    CHECK(object.members.capacity == 6 && object.members.cardinality == 5);
    CHECK(field_init(&field, &object, &added_key));
    field_get(&field, &output);
    CHECK(output == input);
    CHECK(field_init(&field, &object, &outer_key));
    CHECK(matches(method_invoke(&field, 0, &output), STATUS_CODE_SUCCESS, 0x11, 1) && output == value);
    CHECK(status_code_is_success(object_free(&object).status));
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
    CHECK(test_dispatch_argument_results() == 0);
    std::printf("PASS: callback status and structural validation\n");
    CHECK(test_class_extension_boundaries() == 0);
    std::printf("PASS: class extension boundaries and empty composites\n");
    CHECK(test_allocation_extent_and_clone() == 0);
    std::printf("PASS: allocation extent failure and independent clones\n");
    CHECK(test_interface_layouts() == 0);
    std::printf("PASS: ordered, reordered and incomplete interfaces\n");
    CHECK(test_remove_payload_key() == 0);
    std::printf("PASS: removal with payload-backed lookup key\n");
    CHECK(test_byte_class_extension() == 0);
    CHECK(test_reserved_class_extension() == 0);
    CHECK(test_member_resize() == 0);
    CHECK(test_polymorphism_and_nested_objects() == 0);
    CHECK(test_reentrant_and_mutating_methods() == 0);
    std::printf("PASS: polymorphism, nested objects, composed interfaces and mutating methods\n");
    std::printf("PASS: CATOOLIB composites\n");
    return 0;
}
