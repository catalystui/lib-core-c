#ifndef CATRLLIB_HPP
#define CATRLLIB_HPP
#include "catalyst.hpp"
#include "catrllib.h"
namespace catalyst {

typedef ::CATALYST_ARRAY ARRAY;
typedef ::CATALYST_SET SET;
typedef ::CATALYST_MAP MAP;
typedef ::CATALYST_STACK STACK;

inline RESULT array_create(ARRAY** array, NUINT element_size, NUINT length) {
    return ::catrl_array_create(array, element_size, length);
}

inline RESULT array_destroy(ARRAY* array) {
    return ::catrl_array_destroy(array);
}

inline RESULT array_get(const ARRAY* array, NUINT index, void* value) {
    return ::catrl_array_get(array, index, value);
}

inline RESULT array_set(ARRAY* array, NUINT index, const void* value) {
    return ::catrl_array_set(array, index, value);
}

inline RESULT array_length(const ARRAY* array, NUINT* length) {
    return ::catrl_array_length(array, length);
}

inline RESULT set_create(SET** set, NUINT element_size) {
    return ::catrl_set_create(set, element_size);
}

inline RESULT set_destroy(SET* set) {
    return ::catrl_set_destroy(set);
}

inline RESULT set_add(SET* set, const void* value) {
    return ::catrl_set_add(set, value);
}

inline RESULT set_remove(SET* set, const void* value) {
    return ::catrl_set_remove(set, value);
}

inline RESULT set_contains(const SET* set, const void* value, BOOL* contains) {
    return ::catrl_set_contains(set, value, contains);
}

inline RESULT set_count(const SET* set, NUINT* count) {
    return ::catrl_set_count(set, count);
}

inline RESULT set_at(const SET* set, NUINT index, void* value) {
    return ::catrl_set_at(set, index, value);
}

inline RESULT set_clear(SET* set) {
    return ::catrl_set_clear(set);
}

inline RESULT map_create(MAP** map, NUINT key_size, NUINT value_size) {
    return ::catrl_map_create(map, key_size, value_size);
}

inline RESULT map_destroy(MAP* map) {
    return ::catrl_map_destroy(map);
}

inline RESULT map_get(const MAP* map, const void* key, void* value) {
    return ::catrl_map_get(map, key, value);
}

inline RESULT map_set(MAP* map, const void* key, const void* value) {
    return ::catrl_map_set(map, key, value);
}

inline RESULT map_remove(MAP* map, const void* key) {
    return ::catrl_map_remove(map, key);
}

inline RESULT map_contains(const MAP* map, const void* key, BOOL* contains) {
    return ::catrl_map_contains(map, key, contains);
}

inline RESULT map_count(const MAP* map, NUINT* count) {
    return ::catrl_map_count(map, count);
}

inline RESULT map_at(const MAP* map, NUINT index, void* key, void* value) {
    return ::catrl_map_at(map, index, key, value);
}

inline RESULT map_clear(MAP* map) {
    return ::catrl_map_clear(map);
}

inline RESULT stack_create(STACK** stack, NUINT element_size) {
    return ::catrl_stack_create(stack, element_size);
}

inline RESULT stack_destroy(STACK* stack) {
    return ::catrl_stack_destroy(stack);
}

inline RESULT stack_push(STACK* stack, const void* value) {
    return ::catrl_stack_push(stack, value);
}

inline RESULT stack_pop(STACK* stack, void* value) {
    return ::catrl_stack_pop(stack, value);
}

inline RESULT stack_peek(const STACK* stack, void* value) {
    return ::catrl_stack_peek(stack, value);
}

inline RESULT stack_count(const STACK* stack, NUINT* count) {
    return ::catrl_stack_count(stack, count);
}

inline RESULT stack_clear(STACK* stack) {
    return ::catrl_stack_clear(stack);
}

} /* namespace catalyst */
#endif /* CATRLLIB_HPP */
