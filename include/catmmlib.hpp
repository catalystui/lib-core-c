#ifndef CATMMLIB_HPP
#define CATMMLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
/**
 * C++ namespace wrappers for catmmlib.h. All ownership, lifetime, byte-count,
 * null-argument, and result conventions are inherited from the C API.
 */
namespace catalyst {

typedef ::CATALYST_MEMORY_HEADER MEMORY_HEADER;

inline void alloc(void** memory, NUINT size, RESULT* result) {
    ::catmm_alloc(memory, size, result);
}

inline void realloc(void** memory, NUINT size, RESULT* result) {
    ::catmm_realloc(memory, size, result);
}

inline void free(void* memory, RESULT* result) {
    ::catmm_free(memory, result);
}

} /* namespace catalyst */
#endif /* CATMMLIB_HPP */
