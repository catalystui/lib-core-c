#ifndef CATOOLIB_H
#define CATOOLIB_H
#include "catalyst.h"
#include "catmmlib.h"
#include "catrllib.h"

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}
#endif
#endif /* CATOOLIB_H */
