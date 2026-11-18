#ifndef CATMMLIB_HPP
#define CATMMLIB_HPP
#include "catalyst.hpp"
#include "catmmlib.h"
namespace catalyst {

typedef ::CATALYST_MEMORY_HEADER MEMORY_HEADER;

inline RESULT alloc(void** memory, NUINT size) {
    return ::catmm_alloc(memory, size);
}

inline RESULT realloc(void** memory, NUINT size) {
    return ::catmm_realloc(memory, size);
}

inline RESULT free(void* memory) {
    return ::catmm_free(memory);
}

inline void copy(void* destination, const void* source, NUINT size) {
    ::catmm_copy(destination, source, size);
}

inline void move(void* destination, const void* source, NUINT size) {
    ::catmm_move(destination, source, size);
}

inline SBYTE compare(const void* first, const void* second, NUINT size) {
    return ::catmm_compare(first, second, size);
}

inline void set(void* memory, BYTE value, NUINT size) {
    ::catmm_set(memory, value, size);
}

} /* namespace catalyst */
#endif /* CATMMLIB_HPP */
