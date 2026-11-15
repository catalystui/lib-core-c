#ifndef CATMMLIB_HPP
#define CATMMLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
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

inline void copy(void* destination, const void* source, NUINT size, RESULT* result) {
    ::catmm_copy(destination, source, size, result);
}

inline void move(void* destination, const void* source, NUINT size, RESULT* result) {
    ::catmm_move(destination, source, size, result);
}

inline void compare(const void* first, const void* second, NUINT size, SBYTE* comparison, RESULT* result) {
    ::catmm_compare(first, second, size, comparison, result);
}

inline void set(void* memory, BYTE value, NUINT size, RESULT* result) {
    ::catmm_set(memory, value, size, result);
}

} /* namespace catalyst */
#endif /* CATMMLIB_HPP */
