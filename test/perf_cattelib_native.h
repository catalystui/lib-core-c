#ifndef PERF_CATTELIB_NATIVE_H
#define PERF_CATTELIB_NATIVE_H
#include <limits.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C90 reference: use the smallest conventional native scalar that holds Unicode. */
#if UINT_MAX >= 0xFFFFFFFFUL
typedef unsigned int NativeScalar;
#else
typedef unsigned long NativeScalar;
#endif

enum { PERF_BYTES = 52, PERF_UTF16_UNITS = PERF_BYTES / 2 };
enum { PERF_ASCII, PERF_CP1252, PERF_UTF8, PERF_UTF16LE };

typedef struct NativeData {
    NativeScalar scalar;
    const unsigned char* input;
    unsigned char output[PERF_BYTES];
    size_t count;
    unsigned char status;
} NativeData;

/* These are benchmark-private reference functions, not Core APIs. */
#define PERF_DECLARE_ENCODING(name) \
    void native_##name##_encode(NativeData* data); \
    void native_##name##_decode(NativeData* data); \
    void native_##name##_length(NativeData* data);
#define PERF_DECLARE_CONVERSION(from, to) \
    void native_##from##_to_##to(NativeData* data);

PERF_DECLARE_ENCODING(ascii)
PERF_DECLARE_ENCODING(cp1252)
PERF_DECLARE_ENCODING(utf8)
PERF_DECLARE_ENCODING(utf16le)
PERF_DECLARE_CONVERSION(ascii, cp1252)
PERF_DECLARE_CONVERSION(ascii, utf8)
PERF_DECLARE_CONVERSION(ascii, utf16le)
PERF_DECLARE_CONVERSION(cp1252, ascii)
PERF_DECLARE_CONVERSION(cp1252, utf8)
PERF_DECLARE_CONVERSION(cp1252, utf16le)
PERF_DECLARE_CONVERSION(utf8, ascii)
PERF_DECLARE_CONVERSION(utf8, cp1252)
PERF_DECLARE_CONVERSION(utf8, utf16le)
PERF_DECLARE_CONVERSION(utf16le, ascii)
PERF_DECLARE_CONVERSION(utf16le, cp1252)
PERF_DECLARE_CONVERSION(utf16le, utf8)

#undef PERF_DECLARE_ENCODING
#undef PERF_DECLARE_CONVERSION

#ifdef __cplusplus
}
#endif
#endif
