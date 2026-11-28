#ifndef CATOOLIB_H
#define CATOOLIB_H
#include "catalyst.h"
#include "catmmlib.h"
#include "catrllib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATOOLIB_EXPORTS)
        #define CATOOLIB_API __declspec(dllexport)
    #elif defined(CATOOLIB_USE_DLL)
        #define CATOOLIB_API __declspec(dllimport)
    #else
        #define CATOOLIB_API
    #endif
#else
    #define CATOOLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: COMPOSITE STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** An element belonging to an object's member map. */
typedef struct CATOOLIB_MEMBER {
    CATMMLIB_POINTERW data;
    CATALYST_NUINT size;
} CATOOLIB_MEMBER;

/** An addressable composite containing distinct keyed members. */
typedef struct CATOOLIB_OBJECT {
    CATRLLIB_MAP members;
} CATOOLIB_OBJECT;

/** A keyed member resolved from an object's member map. */
typedef struct CATOOLIB_FIELD {
    CATOOLIB_OBJECT* object;
    CATMMLIB_POINTER key;
    CATOOLIB_MEMBER* member;
} CATOOLIB_FIELD;

/** An executable member of an object. */
typedef CATALYST_RESULT (*CATOOLIB_METHOD)(
    CATOOLIB_OBJECT* object,
    CATMMLIB_POINTER arguments,
    CATMMLIB_POINTERW output
);

/** A member defining retrieval and assignment behavior. */
typedef struct CATOOLIB_PROPERTY {
    CATALYST_RESULT (*get)(CATOOLIB_OBJECT*, CATMMLIB_POINTERW);
    CATALYST_RESULT (*set)(CATOOLIB_OBJECT*, CATMMLIB_POINTER);
} CATOOLIB_PROPERTY;

/** An object whose keyed member layout is fixed. */
typedef struct CATOOLIB_STRUCTURE {
    CATOOLIB_OBJECT object;
} CATOOLIB_STRUCTURE;

/** An object whose members define the layout used to produce other objects. */
typedef struct CATOOLIB_CLASS {
    CATOOLIB_OBJECT object;
} CATOOLIB_CLASS;

/** An object whose members define the requirements of another object. */
typedef struct CATOOLIB_INTERFACE {
    CATOOLIB_OBJECT object;
    CATALYST_RESULT (*validate)(
        const CATOOLIB_MEMBER* member,
        const CATOOLIB_MEMBER* requirement,
        CATALYST_BOOL* valid
    );
} CATOOLIB_INTERFACE;

/* Ownership: initialized composites own their map records and payload bytes,
 * but not objects referenced by those bytes. Do not shallow-copy ownership,
 * directly mutate underlying map metadata, or change a Structure's key set.
 * Keep borrowed descriptors/keys live, synchronize externally, and supply
 * distinct unused destination storage for initialization and cloning.
 */

/* -------------------------------------------------------------------------------------------------
 * C API :: MEMBER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused member storage. Nonzero size allocates zero-filled owned bytes; zero size or allocation failure leaves a freeable empty member. */
CATOOLIB_API CATALYST_RESULT catoo_member_init(CATOOLIB_MEMBER* member, CATALYST_NUINT size);

/** Copy a live member's size bytes into sufficient writable output storage outside its descriptor; overlap with payload is supported. */
CATOOLIB_API void catoo_member_get(const CATOOLIB_MEMBER* member, CATMMLIB_POINTERW output);

/** Copy size readable bytes into a live member's payload; overlap is supported. Does not change its size or ownership. */
CATOOLIB_API void catoo_member_set(CATOOLIB_MEMBER* member, CATMMLIB_POINTER data);

/** Resize a live member to a nonzero byte size, preserving the common prefix and zero-filling growth. Same size is a no-op; failure preserves the payload and size. Successful resize may invalidate payload pointers but not the member descriptor, its key, or its resolved Field. */
CATOOLIB_API CATALYST_RESULT catoo_member_resize(CATOOLIB_MEMBER* member, CATALYST_NUINT size);

/** Release owned bytes, not referenced objects. Success empties the member; failure retains ownership. Empty members are a no-op. */
CATOOLIB_API CATALYST_RESULT catoo_member_free(CATOOLIB_MEMBER* member);

/* -------------------------------------------------------------------------------------------------
 * C API :: OBJECT OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize unused object storage without allocation; ksize is the nonzero byte width of every key. */
CATOOLIB_API void catoo_object_init(CATOOLIB_OBJECT* object, CATALYST_NUINT ksize);

/** Copy a distinct key and nonzero size readable payload bytes into an initialized object. Duplicate keys are no-ops; exhausted capacity is an error. Key must be outside the map and its allocations. Failure retains existing members. */
CATOOLIB_API CATALYST_RESULT catoo_object_add(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key, CATMMLIB_POINTER data, CATALYST_NUINT size);

/** Remove a keyed member and release its payload. Absent keys are no-ops; release failure retains the member. The key must be readable during lookup; it need not outlive the removed payload. */
CATOOLIB_API CATALYST_RESULT catoo_object_remove(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Test a readable ksize-byte key against an initialized object. */
CATOOLIB_API CATALYST_BOOL catoo_object_contains(const CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Borrow the member associated with a readable key, or return null if absent. Do not free it separately; reacquire after removal, clear, map resize or free attempts that may invalidate the descriptor. */
CATOOLIB_API CATOOLIB_MEMBER* catoo_object_get(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Expand the initialized member map to the requested capacity; smaller/equal requests are no-ops. Failure retains members and logical capacity; key aliases may be invalidated. */
CATOOLIB_API CATALYST_RESULT catoo_object_expand(CATOOLIB_OBJECT* object, CATALYST_NUINT capacity);

/** Halve map capacity without dropping members; failure may retain a smaller logical capacity. Reacquire borrowed members and fields after map resize attempts. */
CATOOLIB_API CATALYST_RESULT catoo_object_shrink(CATOOLIB_OBJECT* object);

/** Release and remove all members, retaining map capacity. On failure, already removed members remain removed; remaining members are live. */
CATOOLIB_API CATALYST_RESULT catoo_object_clear(CATOOLIB_OBJECT* object);

/** Release all members then map storage. Failure retains unreleased resources for retry with object_free; success leaves a reusable empty object. */
CATOOLIB_API CATALYST_RESULT catoo_object_free(CATOOLIB_OBJECT* object);

/* -------------------------------------------------------------------------------------------------
 * C API :: FIELD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Resolve a borrowed field from an initialized object using a readable key. The field borrows the object's stored key and member; input key storage is not retained. Reacquire after invalidating map operations. Absence clears the field and returns FALSE. */
CATOOLIB_API CATALYST_BOOL catoo_field_init(CATOOLIB_FIELD* field, CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Copy a live resolved member into sufficient writable output storage outside its descriptor. */
CATOOLIB_API void catoo_field_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);

/** Copy size readable bytes into a live resolved member without changing its size or key. */
CATOOLIB_API void catoo_field_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);

/* -------------------------------------------------------------------------------------------------
 * C API :: METHOD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke a live field containing a CATOOLIB_METHOD value with the exact callable type. Caller supplies valid arguments/output per callback; callback defines output validity and may invalidate the field. Invalid-argument indices 0–2 identify field, arguments, and output; any larger index is an invalid callback state. */
CATOOLIB_API CATALYST_RESULT catoo_method_invoke(const CATOOLIB_FIELD* field, CATMMLIB_POINTER arguments, CATMMLIB_POINTERW output);

/* -------------------------------------------------------------------------------------------------
 * C API :: PROPERTY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke a live field containing a CATOOLIB_PROPERTY with a callable getter. Caller supplies valid output; callback defines its validity and may invalidate the field. Invalid-argument indices 0–1 identify field and output; any larger index is an invalid callback state. */
CATOOLIB_API CATALYST_RESULT catoo_property_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);

/** Invoke a live field containing a CATOOLIB_PROPERTY with a callable setter. Caller supplies valid input; callback defines behavior and may invalidate the field. Invalid-argument indices 0–1 identify field and data; any larger index is an invalid callback state. */
CATOOLIB_API CATALYST_RESULT catoo_property_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);

/* -------------------------------------------------------------------------------------------------
 * C API :: STRUCTURE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone a live object's members into unused structure storage. Keys are fixed by contract. Failure attempts cleanup; remaining ownership is freeable via structure_free. */
CATOOLIB_API CATALYST_RESULT catoo_structure_init(CATOOLIB_STRUCTURE* structure, const CATOOLIB_OBJECT* object);

/** Borrow the member associated with a readable key, or return null if absent. Do not change the structure's key set. */
CATOOLIB_API CATOOLIB_MEMBER* catoo_structure_get(CATOOLIB_STRUCTURE* structure, CATMMLIB_POINTER key);

/** Release the structure's owned storage; failure retains unreleased resources for retry. */
CATOOLIB_API CATALYST_RESULT catoo_structure_free(CATOOLIB_STRUCTURE* structure);

/* -------------------------------------------------------------------------------------------------
 * C API :: CLASS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone a live object into unused class storage as member definitions/defaults. Failure attempts cleanup; remaining ownership is freeable via class_free. */
CATOOLIB_API CATALYST_RESULT catoo_class_init(CATOOLIB_CLASS* class_, const CATOOLIB_OBJECT* definition);

/** Clone a live class's definitions into unused object storage. Payload bytes are independent; embedded references remain shared. Failure attempts cleanup; remaining ownership is freeable via object_free. */
CATOOLIB_API CATALYST_RESULT catoo_class_create(const CATOOLIB_CLASS* class_, CATOOLIB_OBJECT* object);

/** Copy non-overlapping member definitions from a live base class with equal key width. Empty base is a no-op; overlapping keys are not implemented. On insertion failure attempt rollback; failed rollback may retain an appended prefix. Capacity/aliases may change. */
CATOOLIB_API CATALYST_RESULT catoo_class_extend(CATOOLIB_CLASS* class_, const CATOOLIB_CLASS* base);

/** Release the class's owned storage; failure retains unreleased resources for retry. Produced objects have independent payload storage. */
CATOOLIB_API CATALYST_RESULT catoo_class_free(CATOOLIB_CLASS* class_);

/* -------------------------------------------------------------------------------------------------
 * C API :: INTERFACE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Clone live requirements into unused interface storage. Nonempty requirements need a callable validator. Failure attempts cleanup; remaining ownership is freeable via interface_free. */
CATOOLIB_API CATALYST_RESULT catoo_interface_init(
    CATOOLIB_INTERFACE* interface_,
    const CATOOLIB_OBJECT* requirements,
    CATALYST_RESULT (*validate)(const CATOOLIB_MEMBER*, const CATOOLIB_MEMBER*, CATALYST_BOOL*)
);

/** Check each required key and its validator. Missing/incompatible keys produce FALSE; extra keys are permitted. Validator must not mutate either object and writes validity on SUCCESS. Non-SUCCESS callback results stop validation without writing output. */
CATOOLIB_API CATALYST_RESULT catoo_interface_validate(const CATOOLIB_INTERFACE* interface_, const CATOOLIB_OBJECT* object, CATALYST_BOOL* valid);

/** Release the interface's owned storage; failure retains unreleased resources for retry. Success clears the validator. */
CATOOLIB_API CATALYST_RESULT catoo_interface_free(CATOOLIB_INTERFACE* interface_);

#ifdef __cplusplus
}
#endif
#endif /* CATOOLIB_H */
