#ifndef CATOOLIB_HPP
#define CATOOLIB_HPP
#include "catrllib.hpp"
#include "catoolib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: COMPOSITE STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

typedef ::CATOOLIB_MEMBER MEMBER;
typedef ::CATOOLIB_OBJECT OBJECT;
typedef ::CATOOLIB_FIELD FIELD;
typedef ::CATOOLIB_METHOD METHOD;
typedef ::CATOOLIB_PROPERTY PROPERTY;
typedef ::CATOOLIB_STRUCTURE STRUCTURE;
typedef ::CATOOLIB_CLASS CLASS;
typedef ::CATOOLIB_INTERFACE INTERFACE;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: MEMBER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member. */
inline RESULT member_init(MEMBER* member, NUINT size) {
    return ::catoo_member_init(member, size);
}

/** Copy a live member's size bytes into sufficient writable output storage outside its descriptor; overlap with payload is supported. */
inline void member_get(const MEMBER* member, POINTERW output) {
    ::catoo_member_get(member, output);
}

/** Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership. */
inline void member_set(MEMBER* member, POINTER data) {
    ::catoo_member_set(member, data);
}

/** Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op. */
inline RESULT member_free(MEMBER* member) {
    return ::catoo_member_free(member);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: OBJECT OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused object storage without allocation; ksize is the nonzero byte width of every key. */
inline void object_init(OBJECT* object, NUINT ksize) {
    ::catoo_object_init(object, ksize);
}

/** Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members. */
inline RESULT object_add(OBJECT* object, POINTER key, POINTER data, NUINT size) {
    return ::catoo_object_add(object, key, data, size);
}

/** Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The readable key must remain live through removal, including after payload release. */
inline RESULT object_remove(OBJECT* object, POINTER key) {
    return ::catoo_object_remove(object, key);
}

/** Test a readable ksize-byte key against an initialized object. */
inline BOOL object_contains(const OBJECT* object, POINTER key) {
    return ::catoo_object_contains(object, key);
}

/** Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, resize or free attempts. */
inline MEMBER* object_get(OBJECT* object, POINTER key) {
    return ::catoo_object_get(object, key);
}

/** Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated. */
inline RESULT object_expand(OBJECT* object, NUINT capacity) {
    return ::catoo_object_expand(object, capacity);
}

/** Halve capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed fields after resize attempts. */
inline RESULT object_shrink(OBJECT* object) {
    return ::catoo_object_shrink(object);
}

/** Release and remove all members, retaining map capacity. On failure, already removed members remain removed; remaining members are live. */
inline RESULT object_clear(OBJECT* object) {
    return ::catoo_object_clear(object);
}

/** Release all members then map storage. Failure retains unreleased resources for retry with object_free; success leaves a reusable empty object. */
inline RESULT object_free(OBJECT* object) {
    return ::catoo_object_free(object);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: FIELD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Resolve a borrowed field from an initialized object using a readable key. The field borrows the object's stored key and member; input key storage is not retained. Reacquire after invalidating map operations. Absence clears the field and returns FALSE. */
inline BOOL field_init(FIELD* field, OBJECT* object, POINTER key) {
    return ::catoo_field_init(field, object, key);
}

/** Copy a live resolved member into sufficient writable output storage outside its descriptor. */
inline void field_get(const FIELD* field, POINTERW output) {
    ::catoo_field_get(field, output);
}

/** Copy size readable bytes into a live resolved member without changing its size or key. */
inline void field_set(FIELD* field, POINTER data) {
    ::catoo_field_set(field, data);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: METHOD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field. */
inline RESULT method_invoke(const FIELD* field, POINTER arguments, POINTERW output) {
    return ::catoo_method_invoke(field, arguments, output);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: PROPERTY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field. */
inline RESULT property_get(const FIELD* field, POINTERW output) {
    return ::catoo_property_get(field, output);
}

/** Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field. */
inline RESULT property_set(FIELD* field, POINTER data) {
    return ::catoo_property_set(field, data);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: STRUCTURE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone a live object's members into unused structure storage. Keys are fixed by contract. Failure attempts cleanup; remaining ownership is freeable via structure_free. */
inline RESULT structure_init(STRUCTURE* structure, const OBJECT* object) {
    return ::catoo_structure_init(structure, object);
}

/** Borrow the member associated with a readable key, or return null if absent. Do not change the structure's key set. */
inline MEMBER* structure_get(STRUCTURE* structure, POINTER key) {
    return ::catoo_structure_get(structure, key);
}

/** Release the structure's owned storage; failure retains unreleased resources for retry. */
inline RESULT structure_free(STRUCTURE* structure) {
    return ::catoo_structure_free(structure);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: CLASS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone a live object into unused class storage as member definitions/defaults. Failure attempts cleanup; remaining ownership is freeable via class_free. */
inline RESULT class_init(CLASS* class_, const OBJECT* definition) {
    return ::catoo_class_init(class_, definition);
}

/** Clone a live class's definitions into unused object storage. Payload bytes are independent; embedded references remain shared. Failure attempts cleanup; remaining ownership is freeable via object_free. */
inline RESULT class_create(const CLASS* class_, OBJECT* object) {
    return ::catoo_class_create(class_, object);
}

/** Copy non-overlapping member definitions from a live base class with equal key width. Empty base is a no-op; overlapping keys are not implemented. On insertion failure attempt rollback; failed rollback may retain an appended prefix. Capacity/aliases may change. */
inline RESULT class_extend(CLASS* class_, const CLASS* base) {
    return ::catoo_class_extend(class_, base);
}

/** Release the class's owned storage; failure retains unreleased resources for retry. Produced objects have independent payload storage. */
inline RESULT class_free(CLASS* class_) {
    return ::catoo_class_free(class_);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: INTERFACE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone live requirements into unused interface storage. Nonempty requirements need a callable validator. Failure attempts cleanup; remaining ownership is freeable via interface_free. */
inline RESULT interface_init(INTERFACE* interface_, const OBJECT* requirements, ::CATALYST_RESULT (*validate)(const MEMBER*, const MEMBER*, BOOL*)) {
    return ::catoo_interface_init(interface_, requirements, validate);
}

/** Check each required key and its validator. Missing/incompatible keys produce FALSE; extra keys are permitted. Validator must not mutate either object and writes validity on SUCCESS. Non-SUCCESS callback results stop validation without writing output. */
inline RESULT interface_validate(const INTERFACE* interface_, const OBJECT* object, BOOL* valid) {
    return ::catoo_interface_validate(interface_, object, valid);
}

/** Release the interface's owned storage; failure retains unreleased resources for retry. Success clears the validator. */
inline RESULT interface_free(INTERFACE* interface_) {
    return ::catoo_interface_free(interface_);
}

} /* namespace catalyst */
#endif /* CATOOLIB_HPP */
