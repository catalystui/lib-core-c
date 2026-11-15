#ifndef CATRLLIB_HPP
#define CATRLLIB_HPP
#include "catalyst.hpp"
#include "catrllib.h"
namespace catalyst {

typedef ::CATALYST_ARRAY ARRAY;
typedef ::CATALYST_SET SET;
typedef ::CATALYST_MAP MAP;
typedef ::CATALYST_STACK STACK;

inline void array_create(ARRAY** array, NUINT element_size, NUINT length, RESULT* result) {
    ::catrl_array_create(array, element_size, length, result);
}

inline void array_destroy(ARRAY* array, RESULT* result) {
    ::catrl_array_destroy(array, result);
}

inline void array_get(const ARRAY* array, NUINT index, void* value, RESULT* result) {
    ::catrl_array_get(array, index, value, result);
}

inline void array_set(ARRAY* array, NUINT index, const void* value, RESULT* result) {
    ::catrl_array_set(array, index, value, result);
}

inline void array_length(const ARRAY* array, NUINT* length, RESULT* result) {
    ::catrl_array_length(array, length, result);
}

inline void set_create(SET** set, NUINT element_size, RESULT* result) {
    ::catrl_set_create(set, element_size, result);
}

inline void set_destroy(SET* set, RESULT* result) {
    ::catrl_set_destroy(set, result);
}

inline void set_add(SET* set, const void* value, RESULT* result) {
    ::catrl_set_add(set, value, result);
}

inline void set_remove(SET* set, const void* value, RESULT* result) {
    ::catrl_set_remove(set, value, result);
}

inline void set_contains(const SET* set, const void* value, BOOL* contains, RESULT* result) {
    ::catrl_set_contains(set, value, contains, result);
}

inline void set_count(const SET* set, NUINT* count, RESULT* result) {
    ::catrl_set_count(set, count, result);
}

inline void set_at(const SET* set, NUINT index, void* value, RESULT* result) {
    ::catrl_set_at(set, index, value, result);
}

inline void set_clear(SET* set, RESULT* result) {
    ::catrl_set_clear(set, result);
}

inline void map_create(MAP** map, NUINT key_size, NUINT value_size, RESULT* result) {
    ::catrl_map_create(map, key_size, value_size, result);
}

inline void map_destroy(MAP* map, RESULT* result) {
    ::catrl_map_destroy(map, result);
}

inline void map_get(const MAP* map, const void* key, void* value, RESULT* result) {
    ::catrl_map_get(map, key, value, result);
}

inline void map_set(MAP* map, const void* key, const void* value, RESULT* result) {
    ::catrl_map_set(map, key, value, result);
}

inline void map_remove(MAP* map, const void* key, RESULT* result) {
    ::catrl_map_remove(map, key, result);
}

inline void map_contains(const MAP* map, const void* key, BOOL* contains, RESULT* result) {
    ::catrl_map_contains(map, key, contains, result);
}

inline void map_count(const MAP* map, NUINT* count, RESULT* result) {
    ::catrl_map_count(map, count, result);
}

inline void map_at(const MAP* map, NUINT index, void* key, void* value, RESULT* result) {
    ::catrl_map_at(map, index, key, value, result);
}

inline void map_clear(MAP* map, RESULT* result) {
    ::catrl_map_clear(map, result);
}

inline void stack_create(STACK** stack, NUINT element_size, RESULT* result) {
    ::catrl_stack_create(stack, element_size, result);
}

inline void stack_destroy(STACK* stack, RESULT* result) {
    ::catrl_stack_destroy(stack, result);
}

inline void stack_push(STACK* stack, const void* value, RESULT* result) {
    ::catrl_stack_push(stack, value, result);
}

inline void stack_pop(STACK* stack, void* value, RESULT* result) {
    ::catrl_stack_pop(stack, value, result);
}

inline void stack_peek(const STACK* stack, void* value, RESULT* result) {
    ::catrl_stack_peek(stack, value, result);
}

inline void stack_count(const STACK* stack, NUINT* count, RESULT* result) {
    ::catrl_stack_count(stack, count, result);
}

inline void stack_clear(STACK* stack, RESULT* result) {
    ::catrl_stack_clear(stack, result);
}

} /* namespace catalyst */
#endif /* CATRLLIB_HPP */
