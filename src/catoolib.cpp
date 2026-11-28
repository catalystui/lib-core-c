#include "catoolib.hpp"
#include "catrllib.internal.hpp"
#include <stddef.h>

using namespace catalyst;

/* Original contexts follow the public declaration order, including non-reporting operations.
 * Member resize uses the next available context (0x1E).
 * Each reporting site has a distinct operation code within its context.
 * Dependency failures preserve their status and detail; callback argument
 * indices are translated at the interface boundary.
 */

CATALYST_RESULT catoo_member_init(MEMBER* member, NUINT size) {
    member->data = 0;
    member->size = 0;
    if (size == 0) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x01, 1);
    RESULT allocated = catmm_heap_alloc(&member->data, size);
    if (!status_code_is_success(allocated.status)) return RESULT(allocated.status, 0x01, 0x02, allocated.detail);
    member->size = size;
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x03, 0);
}

void catoo_member_get(const MEMBER* member, POINTERW output) {
    catmm_move(output, member->data, member->size);
}

void catoo_member_set(MEMBER* member, POINTER data) {
    catmm_move(member->data, data, member->size);
}

CATALYST_RESULT catoo_member_resize(MEMBER* member, NUINT size) {
    if (size == 0) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1E, 0x01, 1);
    if (size == member->size) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x1E, 0x02, 0);
    RESULT resized = catmm_heap_realloc(&member->data, size);
    if (!status_code_is_success(resized.status)) return RESULT(resized.status, 0x1E, 0x03, resized.detail);
    member->size = size;
    return RESULT(STATUS_CODE_SUCCESS, 0x1E, 0x04, 0);
}

CATALYST_RESULT catoo_member_free(MEMBER* member) {
    if (member->data == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x04, 0x01, 0);
    RESULT released = catmm_heap_free(member->data);
    if (!status_code_is_success(released.status)) return RESULT(released.status, 0x04, 0x02, released.detail);
    member->data = 0;
    member->size = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x03, 0);
}

/* Object map values are actual MEMBER records of fixed size. The map owns
 * its key/record arrays; each record owns its separate payload.
 */
void catoo_object_init(OBJECT* object, NUINT ksize) {
    catrl_map_init(&object->members, ksize, (NUINT) sizeof(MEMBER));
}

CATALYST_RESULT catoo_object_add(OBJECT* object, POINTER key, POINTER data, NUINT size) {
    if (catrl_map_contains(&object->members, key)) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x06, 0x01, 0);
    if (size == 0) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x02, 3);
    if (object->members.cardinality == object->members.capacity) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x06, 0x03, 0);
    MEMBER member;
    RESULT allocated = catoo_member_init(&member, size);
    if (!status_code_is_success(allocated.status)) return RESULT(allocated.status, 0x06, 0x04, allocated.detail);
    catmm_copy(member.data, data, size);
    /* Absence and spare capacity were established above. Transfer payload
     * ownership into the map's copied record; no further allocation occurs. */
    catrl_map_append_known(&object->members, key, &member);
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x05, 0);
}

CATALYST_RESULT catoo_object_remove(OBJECT* object, POINTER key) {
    MAP* map = &object->members;
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01, 0);
    MEMBER* member = (MEMBER*) map->values + index;
    RESULT released = catoo_member_free(member);
    if (!status_code_is_success(released.status)) return RESULT(released.status, 0x07, 0x02, released.detail);
    catrl_map_remove_at(map, index);
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x03, 0);
}

CATALYST_BOOL catoo_object_contains(const OBJECT* object, POINTER key) {
    return catrl_map_contains(&object->members, key);
}

CATOOLIB_MEMBER* catoo_object_get(OBJECT* object, POINTER key) {
    return (MEMBER*) catrl_map_get(&object->members, key);
}

CATALYST_RESULT catoo_object_expand(OBJECT* object, NUINT capacity) {
    RESULT result = catrl_map_expand(&object->members, capacity);
    return RESULT(result.status, 0x0A, 0x01, result.detail);
}

CATALYST_RESULT catoo_object_shrink(OBJECT* object) {
    RESULT result = catrl_map_shrink(&object->members);
    return RESULT(result.status, 0x0B, 0x01, result.detail);
}

CATALYST_RESULT catoo_object_clear(OBJECT* object) {
    if (object->members.cardinality == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0C, 0x01, 0);
    /* Release the final live member first. No search or relocation is
     * needed; the remaining prefix stays valid if release fails. */
    MAP* map = &object->members;
    while (map->cardinality != 0) {
        NUINT last = (NUINT) (map->cardinality - (NUINT) 1);
        RESULT released = catoo_member_free((MEMBER*) map->values + last);
        if (!status_code_is_success(released.status)) return RESULT(released.status, 0x0C, 0x02, released.detail);
        catrl_map_remove_at(map, last);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x03, 0);
}

CATALYST_RESULT catoo_object_free(OBJECT* object) {
    RESULT cleared = catoo_object_clear(object);
    if (!status_code_is_success(cleared.status)) return RESULT(cleared.status, 0x0D, 0x01, cleared.detail);
    RESULT released = catrl_map_free(&object->members);
    return RESULT(released.status, 0x0D, 0x02, released.detail);
}

CATALYST_BOOL catoo_field_init(FIELD* field, OBJECT* object, POINTER key) {
    MAP* map = &object->members;
    NUINT index = catrl_find(map->keys, map->cardinality, map->ksize, key);
    if (index == map->cardinality) {
        field->object = 0;
        field->key = 0;
        field->member = 0;
        return FALSE;
    }
    field->object = object;
    field->key = (const BYTE*) map->keys + (NUINT) (index * map->ksize);
    field->member = (MEMBER*) map->values + index;
    return TRUE;
}

void catoo_field_get(const FIELD* field, POINTERW output) {
    catoo_member_get(field->member, output);
}

void catoo_field_set(FIELD* field, POINTER data) {
    catoo_member_set(field->member, data);
}

CATALYST_RESULT catoo_method_invoke(const FIELD* field, POINTER arguments, POINTERW output) {
    METHOD method;
    catrl_store(&method, field->member->data, (NUINT) sizeof(method));
    RESULT result = method(field->object, arguments, output);
    /* Callback receiver index zero corresponds to the public Field.
     * All remaining callback argument positions match the public signature. */
    if (result.status == STATUS_CODE_ERROR_INVALID_ARGUMENT && result.detail > (DETAIL_CODE) 2)
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x11, 0x02, result.detail);
    return RESULT(result.status, 0x11, 0x01, result.detail);
}

CATALYST_RESULT catoo_property_get(const FIELD* field, POINTERW output) {
    CATALYST_RESULT (*get)(OBJECT*, POINTERW);
    catrl_store(&get, (const BYTE*) field->member->data + (NUINT) offsetof(PROPERTY, get), (NUINT) sizeof(get));
    RESULT result = get(field->object, output);
    /* Callback receiver index zero corresponds to the public Field.
     * All remaining callback argument positions match the public signature. */
    if (result.status == STATUS_CODE_ERROR_INVALID_ARGUMENT && result.detail > (DETAIL_CODE) 1)
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x12, 0x02, result.detail);
    return RESULT(result.status, 0x12, 0x01, result.detail);
}

CATALYST_RESULT catoo_property_set(FIELD* field, POINTER data) {
    CATALYST_RESULT (*set)(OBJECT*, POINTER);
    catrl_store(&set, (const BYTE*) field->member->data + (NUINT) offsetof(PROPERTY, set), (NUINT) sizeof(set));
    RESULT result = set(field->object, data);
    /* Callback receiver index zero corresponds to the public Field.
     * All remaining callback argument positions match the public signature. */
    if (result.status == STATUS_CODE_ERROR_INVALID_ARGUMENT && result.detail > (DETAIL_CODE) 1)
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x13, 0x02, result.detail);
    return RESULT(result.status, 0x13, 0x01, result.detail);
}

/* Source is live; destination is unused and disjoint. Map extents already
 * bound the key offsets and record indices, including narrow targets.
 * On failed cloning, release the partial copy; a cleanup failure leaves
 * a freeable destination with its unreleased resources still reachable.
 */
static RESULT catoo_clone_cleanup(OBJECT* destination, RESULT failure, CONTEXT_CODE context, OPERATION_CODE operation, OPERATION_CODE cleanup_operation) {
    RESULT released = catoo_object_free(destination);
    if (!status_code_is_success(released.status)) {
        if (status_code_is_fatal(failure.status)) return RESULT(failure.status, context, cleanup_operation, failure.detail);
        return RESULT(released.status, context, cleanup_operation, released.detail);
    }
    return RESULT(failure.status, context, operation, failure.detail);
}

/* A distinct source key and spare destination capacity are established.
 * Commit the owned record only after allocation and copying succeed. */
/* The caller reserves the complete destination range and copies its keys.
 * Publish each member only after allocation and payload copy succeed. */
static RESULT catoo_append_copy(OBJECT* destination, const MEMBER* source) {
    MEMBER copy;
    RESULT allocated = catoo_member_init(&copy, source->size);
    if (!status_code_is_success(allocated.status)) return allocated;
    catmm_copy(copy.data, source->data, source->size);
    MAP* map = &destination->members;
    catrl_store((BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), &copy, map->vsize);
    ++map->cardinality;
    return allocated;
}

/* Source and destination are disjoint; successful reservation establishes
 * representable extents. Keys beyond the live prefix are not published. */
static void catoo_copy_keys(MAP* destination, const MAP* source) {
    if (source->cardinality != 0)
        catmm_copy((BYTE*) destination->keys + (NUINT) (destination->cardinality * destination->ksize),
            source->keys, (NUINT) (source->cardinality * source->ksize));
}

static RESULT catoo_clone(OBJECT* destination, const OBJECT* source, CONTEXT_CODE context) {
    catoo_object_init(destination, source->members.ksize);
    RESULT expanded = catoo_object_expand(destination, source->members.cardinality);
    if (!status_code_is_success(expanded.status)) return catoo_clone_cleanup(destination, expanded, context, 0x01, 0x03);
    catoo_copy_keys(&destination->members, &source->members);
    const MEMBER* member = (const MEMBER*) source->members.values;
    for (NUINT i = 0; i < source->members.cardinality; ++i) {
        RESULT added = catoo_append_copy(destination, member);
        if (!status_code_is_success(added.status)) return catoo_clone_cleanup(destination, added, context, 0x02, 0x04);
        ++member;
    }
    return RESULT(STATUS_CODE_SUCCESS, context, 0x05, 0);
}

CATALYST_RESULT catoo_structure_init(STRUCTURE* structure, const OBJECT* object) {
    return catoo_clone(&structure->object, object, 0x14);
}

CATOOLIB_MEMBER* catoo_structure_get(STRUCTURE* structure, POINTER key) {
    return catoo_object_get(&structure->object, key);
}

CATALYST_RESULT catoo_structure_free(STRUCTURE* structure) {
    RESULT result = catoo_object_free(&structure->object);
    return RESULT(result.status, 0x16, 0x01, result.detail);
}

CATALYST_RESULT catoo_class_init(CLASS* class_, const OBJECT* definition) {
    return catoo_clone(&class_->object, definition, 0x17);
}

CATALYST_RESULT catoo_class_create(const CLASS* class_, OBJECT* object) {
    return catoo_clone(object, &class_->object, 0x18);
}

/* Equal, nonzero key widths. Single-byte key sets have only 256 possible
 * values; for larger pairs a bounded bitmap avoids repeated searches.
 * The crossover is conservative and subject to benchmark verification. */
static BOOL catoo_class_overlaps(const MAP* destination, const MAP* source) {
    const BYTE* key = (const BYTE*) source->keys;
    if (destination->ksize == (NUINT) 1 && destination->cardinality >= (NUINT) 32 &&
        source->cardinality >= (NUINT) 8) {
        BYTE occupied[32] = { 0 };
        const BYTE* existing = (const BYTE*) destination->keys;
        for (NUINT i = 0; i < destination->cardinality; ++i) {
            const BYTE value = existing[i];
            occupied[value >> 3] |= (BYTE) ((BYTE) 1 << (value & (BYTE) 7));
        }
        for (NUINT i = 0; i < source->cardinality; ++i) {
            const BYTE value = key[i];
            if ((occupied[value >> 3] & (BYTE) ((BYTE) 1 << (value & (BYTE) 7))) != 0) return TRUE;
        }
        return FALSE;
    }
    for (NUINT i = 0; i < source->cardinality; ++i) {
        if (catrl_map_contains(destination, key)) return TRUE;
        key += source->ksize;
    }
    return FALSE;
}

CATALYST_RESULT catoo_class_extend(CLASS* class_, const CLASS* base) {
    OBJECT* destination = &class_->object;
    const OBJECT* source = &base->object;
    if (source->members.cardinality == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x19, 0x01, 0);
    if (destination->members.ksize != source->members.ksize) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 0x02, 1);
    /* Overlap policy is not established. Reject before changing the class
     * rather than silently overriding or accepting incompatible definitions. */
    if (catoo_class_overlaps(&destination->members, &source->members))
        return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x19, 0x03, 0);
    const NUINT original = destination->members.cardinality;
    if (source->members.cardinality > (NUINT) (CATALYST_NUINT_MAX - original)) return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x19, 0x04, 0);
    RESULT expanded = catoo_object_expand(destination, (NUINT) (original + source->members.cardinality));
    if (!status_code_is_success(expanded.status)) return RESULT(expanded.status, 0x19, 0x05, expanded.detail);
    catoo_copy_keys(&destination->members, &source->members);
    const MEMBER* member = (const MEMBER*) source->members.values;
    for (NUINT i = 0; i < source->members.cardinality; ++i) {
        RESULT added = catoo_append_copy(destination, member);
        if (!status_code_is_success(added.status)) {
            while (destination->members.cardinality > original) {
                NUINT last = (NUINT) (destination->members.cardinality - (NUINT) 1);
                RESULT removed = catoo_member_free((MEMBER*) destination->members.values + last);
                if (!status_code_is_success(removed.status)) {
                    if (status_code_is_fatal(added.status)) return RESULT(added.status, 0x19, 0x07, added.detail);
                    return RESULT(removed.status, 0x19, 0x07, removed.detail);
                }
                catrl_map_remove_at(&destination->members, last);
            }
            return RESULT(added.status, 0x19, 0x06, added.detail);
        }
        ++member;
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x19, 0x08, 0);
}

CATALYST_RESULT catoo_class_free(CLASS* class_) {
    RESULT result = catoo_object_free(&class_->object);
    return RESULT(result.status, 0x1A, 0x01, result.detail);
}

CATALYST_RESULT catoo_interface_init(INTERFACE* interface_, const OBJECT* requirements, CATALYST_RESULT (*validate)(const MEMBER*, const MEMBER*, BOOL*)) {
    interface_->validate = 0;
    RESULT result = catoo_clone(&interface_->object, requirements, 0x1B);
    if (status_code_is_success(result.status)) interface_->validate = validate;
    return result;
}

CATALYST_RESULT catoo_interface_validate(const INTERFACE* interface_, const OBJECT* object, BOOL* valid) {
    const MAP* requirements = &interface_->object.members;
    if (requirements->cardinality != 0 && requirements->ksize != object->members.ksize) {
        *valid = FALSE;
        return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x01, 0);
    }
    const BYTE* key = (const BYTE*) requirements->keys;
    const MEMBER* requirement = (const MEMBER*) requirements->values;
    const MAP* candidate = &object->members;
    const NUINT width = requirements->ksize;
    const NUINT count = candidate->cardinality;
    const BYTE* candidate_keys = (const BYTE*) candidate->keys;
    for (NUINT i = 0; i < requirements->cardinality; ++i) {
        /* Matching layouts resolve in order. A different layout falls back
         * to the ordinary search without changing callback order. */
        NUINT index = count;
        if (width == (NUINT) 1) {
            /* Common byte layouts avoid multiplication and generic search
             * dispatch, including reordered and missing requirements. */
            if (i < count && candidate_keys[i] == *key) index = i;
            else index = catrl_find_byte(candidate_keys, count, *key);
        } else {
            if (i < count) {
                const BYTE* at = candidate_keys + (NUINT) (i * width);
                if (at[0] == key[0] && catmm_compare(at + 1, key + 1, (NUINT) (width - (NUINT) 1)) == 0) index = i;
            }
            if (index == count) index = catrl_find(candidate_keys, count, width, key);
        }
        if (index == count) {
            *valid = FALSE;
            return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x02, 0);
        }
        const MEMBER* member = (const MEMBER*) candidate->values + index;
        BOOL satisfied = FALSE;
        RESULT result = interface_->validate(member, requirement, &satisfied);
        if (result.status != STATUS_CODE_SUCCESS) {
            if (result.status == STATUS_CODE_ERROR_INVALID_ARGUMENT) {
                if (result.detail == 0) return RESULT(result.status, 0x1C, 0x03, 1);
                if (result.detail == 1) return RESULT(result.status, 0x1C, 0x04, 0);
                return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x1C, 0x05, 0);
            }
            return RESULT(result.status, 0x1C, 0x06, result.detail);
        }
        if (!satisfied) {
            *valid = FALSE;
            return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x07, 0);
        }
        key += width;
        ++requirement;
    }
    *valid = TRUE;
    return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x08, 0);
}

CATALYST_RESULT catoo_interface_free(INTERFACE* interface_) {
    RESULT result = catoo_object_free(&interface_->object);
    if (status_code_is_success(result.status)) interface_->validate = 0;
    return RESULT(result.status, 0x1D, 0x01, result.detail);
}
