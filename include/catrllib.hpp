#ifndef CATRLLIB_HPP
#define CATRLLIB_HPP
#include "catalyst.hpp"
#include "catrllib.h"
namespace catalyst {

typedef ::CATALYST_ARRAY ARRAY;
typedef ::CATALYST_SET SET;
typedef ::CATALYST_MAP MAP;
typedef ::CATALYST_FILE FILE;
typedef ::CATALYST_STREAM STREAM;
typedef ::CATALYST_FILE_READER FILE_READER;
typedef ::CATALYST_STREAM_READER STREAM_READER;

inline RESULT array_create(ARRAY** array, NUINT element_size, NUINT length) {
    return ::catrl_array_create(array, element_size, length);
}

inline RESULT array_destroy(ARRAY* array) {
    return ::catrl_array_destroy(array);
}

inline void array_get(const ARRAY* array, NUINT index, void* value) {
    ::catrl_array_get(array, index, value);
}

inline void array_set(ARRAY* array, NUINT index, const void* value) {
    ::catrl_array_set(array, index, value);
}

inline NUINT array_length(const ARRAY* array) {
    return ::catrl_array_length(array);
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

inline void set_remove(SET* set, const void* value) {
    ::catrl_set_remove(set, value);
}

inline BOOL set_contains(const SET* set, const void* value) {
    return ::catrl_set_contains(set, value);
}

inline NUINT set_count(const SET* set) {
    return ::catrl_set_count(set);
}

inline void set_at(const SET* set, NUINT index, void* value) {
    ::catrl_set_at(set, index, value);
}

inline void set_clear(SET* set) {
    ::catrl_set_clear(set);
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

inline void map_remove(MAP* map, const void* key) {
    ::catrl_map_remove(map, key);
}

inline BOOL map_contains(const MAP* map, const void* key) {
    return ::catrl_map_contains(map, key);
}

inline NUINT map_count(const MAP* map) {
    return ::catrl_map_count(map);
}

inline void map_at(const MAP* map, NUINT index, void* key, void* value) {
    ::catrl_map_at(map, index, key, value);
}

inline void map_clear(MAP* map) {
    ::catrl_map_clear(map);
}

inline RESULT file_create(FILE** file, void* state, FILE_READER reader, NUINT length) {
    return ::catrl_file_create(file, state, reader, length);
}

inline RESULT file_destroy(FILE* file) {
    return ::catrl_file_destroy(file);
}

inline RESULT file_get(const FILE* file, NUINT index, BYTE* value) {
    return ::catrl_file_get(file, index, value);
}

inline NUINT file_length(const FILE* file) {
    return ::catrl_file_length(file);
}

inline RESULT stream_create(STREAM** stream, void* state, STREAM_READER reader) {
    return ::catrl_stream_create(stream, state, reader);
}

inline RESULT stream_destroy(STREAM* stream) {
    return ::catrl_stream_destroy(stream);
}

inline RESULT stream_next(STREAM* stream, const BYTE** chunk, NUINT* length) {
    return ::catrl_stream_next(stream, chunk, length);
}

} /* namespace catalyst */
#endif /* CATRLLIB_HPP */
