
#ifndef CATMMLIB_HPP
#define CATMMLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: MEMORY UNITS
 * ------------------------------------------------------------------------------------------------- */

typedef ::CATMMLIB_WORD WORD;
typedef ::CATMMLIB_ADDRESS ADDRESS;
typedef ::CATMMLIB_POINTER POINTER;
typedef ::CATMMLIB_POINTERW POINTERW;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: MEMORY ALLOCATION & MANAGEMENT
 * ------------------------------------------------------------------------------------------------- */

inline RESULT alloc(POINTERW* memory, NUINT size) {
    return ::catmm_alloc(memory, size);
}

inline RESULT realloc(POINTERW* memory, NUINT new_size) {
    return ::catmm_realloc(memory, new_size);
}

inline RESULT free(POINTERW memory) {
    return ::catmm_free(memory);
}

inline RESULT heap_alloc(POINTERW* memory, NUINT size) {
    return ::catmm_heap_alloc(memory, size);
}

inline RESULT heap_realloc(POINTERW* memory, NUINT new_size) {
    return ::catmm_heap_realloc(memory, new_size);
}

inline RESULT heap_free(POINTERW memory) {
    return ::catmm_heap_free(memory);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: MEMORY OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

inline void copy(POINTERW destination, POINTER source, NUINT size) {
    ::catmm_copy(destination, source, size);
}

inline void move(POINTERW destination, POINTER source, NUINT size) {
    ::catmm_move(destination, source, size);
}

inline SBYTE compare(POINTER memory1, POINTER memory2, NUINT size) {
    return ::catmm_compare(memory1, memory2, size);
}

inline void fill(POINTERW memory, WORD value, NUINT size) {
    ::catmm_fill(memory, value, size);
}

} /* namespace catalyst */
#endif /* CATMMLIB_HPP */
