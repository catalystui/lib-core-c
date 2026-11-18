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

inline RESULT copy(void* destination, const void* source, NUINT size) {
    return ::catmm_copy(destination, source, size);
}

inline RESULT move(void* destination, const void* source, NUINT size) {
    return ::catmm_move(destination, source, size);
}

inline RESULT compare(const void* first, const void* second, NUINT size, SBYTE* comparison) {
    return ::catmm_compare(first, second, size, comparison);
}

inline RESULT set(void* memory, BYTE value, NUINT size) {
    return ::catmm_set(memory, value, size);
}

} /* namespace catalyst */
#endif /* CATMMLIB_HPP */
