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

/* -------------------------------------------------------------------------------------------------
 * C API :: MEMBER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Allocate storage for a member of the specified size in bytes. */
CATOOLIB_API CATALYST_RESULT catoo_member_init(CATOOLIB_MEMBER* member, CATALYST_NUINT size);

/** Copy the member's bytes into writable output storage of sufficient size. */
CATOOLIB_API void catoo_member_get(const CATOOLIB_MEMBER* member, CATMMLIB_POINTERW output);

/** Copy size bytes into the member's existing storage. */
CATOOLIB_API void catoo_member_set(CATOOLIB_MEMBER* member, CATMMLIB_POINTER data);

/** Release the member's owned storage, not objects referenced by its bytes. */
CATOOLIB_API CATALYST_RESULT catoo_member_free(CATOOLIB_MEMBER* member);

/* -------------------------------------------------------------------------------------------------
 * C API :: OBJECT OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an empty object with keys of the specified nonzero size in bytes. */
CATOOLIB_API void catoo_object_init(CATOOLIB_OBJECT* object, CATALYST_NUINT ksize);

/** Add a distinct keyed member by copying size bytes into owned storage. */
CATOOLIB_API CATALYST_RESULT catoo_object_add(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key, CATMMLIB_POINTER data, CATALYST_NUINT size);

/** Remove a keyed member and release its owned storage. */
CATOOLIB_API CATALYST_RESULT catoo_object_remove(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Determine whether the specified key belongs to the object. */
CATOOLIB_API CATALYST_BOOL catoo_object_contains(const CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Borrow the member associated with the specified key, or return null if absent. */
CATOOLIB_API CATOOLIB_MEMBER* catoo_object_get(CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Increase the capacity of the object's member map. */
CATOOLIB_API CATALYST_RESULT catoo_object_expand(CATOOLIB_OBJECT* object, CATALYST_NUINT capacity);

/** Reduce the capacity of the object's member map. */
CATOOLIB_API CATALYST_RESULT catoo_object_shrink(CATOOLIB_OBJECT* object);

/** Release all members without releasing the member map's storage. */
CATOOLIB_API CATALYST_RESULT catoo_object_clear(CATOOLIB_OBJECT* object);

/** Release all owned member and map storage. */
CATOOLIB_API CATALYST_RESULT catoo_object_free(CATOOLIB_OBJECT* object);

/* -------------------------------------------------------------------------------------------------
 * C API :: FIELD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Resolve a borrowed field from an object; return FALSE if the key is absent. */
CATOOLIB_API CATALYST_BOOL catoo_field_init(CATOOLIB_FIELD* field, CATOOLIB_OBJECT* object, CATMMLIB_POINTER key);

/** Copy the resolved member's bytes into writable output storage of sufficient size. */
CATOOLIB_API void catoo_field_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);

/** Copy data into the resolved member without changing its size or key. */
CATOOLIB_API void catoo_field_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);

/* -------------------------------------------------------------------------------------------------
 * C API :: METHOD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke the method stored in a field on its associated object; the caller establishes the method type and argument/output contracts. */
CATOOLIB_API CATALYST_RESULT catoo_method_invoke(const CATOOLIB_FIELD* field, CATMMLIB_POINTER arguments, CATMMLIB_POINTERW output);

/* -------------------------------------------------------------------------------------------------
 * C API :: PROPERTY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Invoke the getter stored in a property field; the caller establishes the property type and output contract. */
CATOOLIB_API CATALYST_RESULT catoo_property_get(const CATOOLIB_FIELD* field, CATMMLIB_POINTERW output);

/** Invoke the setter stored in a property field; the caller establishes the property type and input contract. */
CATOOLIB_API CATALYST_RESULT catoo_property_set(CATOOLIB_FIELD* field, CATMMLIB_POINTER data);

/* -------------------------------------------------------------------------------------------------
 * C API :: STRUCTURE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize a structure with independent copies of an object's members and a fixed set of keys. */
CATOOLIB_API CATALYST_RESULT catoo_structure_init(CATOOLIB_STRUCTURE* structure, const CATOOLIB_OBJECT* object);

/** Borrow the member associated with the specified key, or return null if absent. */
CATOOLIB_API CATOOLIB_MEMBER* catoo_structure_get(CATOOLIB_STRUCTURE* structure, CATMMLIB_POINTER key);

/** Release the structure's owned storage. */
CATOOLIB_API CATALYST_RESULT catoo_structure_free(CATOOLIB_STRUCTURE* structure);

/* -------------------------------------------------------------------------------------------------
 * C API :: CLASS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize a class from an object defining its members. */
CATOOLIB_API CATALYST_RESULT catoo_class_init(CATOOLIB_CLASS* class_, const CATOOLIB_OBJECT* definition);

/** Produce an object with independent member storage following the class's definitions. */
CATOOLIB_API CATALYST_RESULT catoo_class_create(const CATOOLIB_CLASS* class_, CATOOLIB_OBJECT* object);

/** Extend a class with another class's member definitions; conflict policy remains to be specified. */
CATOOLIB_API CATALYST_RESULT catoo_class_extend(CATOOLIB_CLASS* class_, const CATOOLIB_CLASS* base);

/** Release the class's owned storage. */
CATOOLIB_API CATALYST_RESULT catoo_class_free(CATOOLIB_CLASS* class_);

/* -------------------------------------------------------------------------------------------------
 * C API :: INTERFACE OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Initialize an interface from an object defining member requirements and a validation rule. */
CATOOLIB_API CATALYST_RESULT catoo_interface_init(
    CATOOLIB_INTERFACE* interface_,
    const CATOOLIB_OBJECT* requirements,
    CATALYST_RESULT (*validate)(const CATOOLIB_MEMBER*, const CATOOLIB_MEMBER*, CATALYST_BOOL*)
);

/** Determine whether an object contains and satisfies every required member. */
CATOOLIB_API CATALYST_RESULT catoo_interface_validate(const CATOOLIB_INTERFACE* interface_, const CATOOLIB_OBJECT* object, CATALYST_BOOL* valid);

/** Release the interface's owned storage. */
CATOOLIB_API CATALYST_RESULT catoo_interface_free(CATOOLIB_INTERFACE* interface_);

#ifdef __cplusplus
}
#endif
#endif /* CATOOLIB_H */
