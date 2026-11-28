#include <cstdio>
#include <ctime>
#include <climits>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "catoolib.hpp"

using namespace catalyst;

#if defined(CLOCKS_PER_SEC)

// Independent C-style reference: fixed-width byte keys, contiguous records,
// separately owned payloads, and native C allocation/copy/search operations.
struct NativeMember { unsigned char* data; size_t size; };
struct NativeObject {
    unsigned char* keys;
    NativeMember* members;
    size_t count, capacity;
};
struct NativeField { NativeObject* object; const unsigned char* key; NativeMember* member; };
typedef BOOL (*NativeMethod)(NativeObject*, const void*, void*);
struct NativeProperty {
    BOOL (*get)(NativeObject*, void*);
    BOOL (*set)(NativeObject*, const void*);
};
struct NativeInterface {
    NativeObject object;
    BOOL (*validate)(const NativeMember*, const NativeMember*);
};

static BOOL native_member_init(NativeMember* m, size_t size) {
    m->data = (unsigned char*) ::calloc(size, 1);
    m->size = m->data ? size : 0;
    return m->data ? TRUE : FALSE;
}
static void native_member_free(NativeMember* m) {
    ::free(m->data); m->data = 0; m->size = 0;
}
static void native_init(NativeObject* o) {
    o->keys = 0; o->members = 0; o->count = o->capacity = 0;
}
static size_t native_find(const NativeObject* o, unsigned char key) {
    if (o->count == 0) return 0;
    const unsigned char* p = (const unsigned char*) ::memchr(o->keys, key, o->count);
    return p ? (size_t) (p - o->keys) : o->count;
}
static NativeMember* native_get(NativeObject* o, unsigned char key) {
    size_t index = native_find(o, key);
    return index == o->count ? 0 : &o->members[index];
}
static BOOL native_resize(NativeObject* o, size_t capacity) {
    if (capacity == o->capacity) return TRUE;
    if (capacity > (size_t) -1 / sizeof(NativeMember)) return FALSE;
    if (capacity == 0) {
        ::free(o->keys); ::free(o->members);
        o->keys = 0; o->members = 0; o->capacity = 0;
        return TRUE;
    }
    size_t old = o->capacity;
    unsigned char* keys = (unsigned char*) ::realloc(o->keys, capacity);
    if (!keys) return FALSE;
    o->keys = keys;
    if (capacity > old) ::memset(keys + old, 0, capacity - old);
    if (capacity < old) o->capacity = capacity;
    NativeMember* members = (NativeMember*) ::realloc(o->members, capacity * sizeof(NativeMember));
    if (!members) return FALSE;
    o->members = members;
    if (capacity > old) ::memset(members + old, 0, (capacity - old) * sizeof(NativeMember));
    o->capacity = capacity;
    return TRUE;
}
static BOOL native_expand(NativeObject* o, size_t capacity) {
    return capacity <= o->capacity ? TRUE : native_resize(o, capacity);
}
static BOOL native_append_known(NativeObject* o, unsigned char key, const void* data, size_t size) {
    if (o->count == o->capacity || size == 0) return FALSE;
    NativeMember m;
    if (!native_member_init(&m, size)) return FALSE;
    ::memcpy(m.data, data, size);
    o->keys[o->count] = key;
    o->members[o->count++] = m;
    return TRUE;
}
static BOOL native_add(NativeObject* o, unsigned char key, const void* data, size_t size) {
    if (native_find(o, key) != o->count) return TRUE;
    return native_append_known(o, key, data, size);
}
static BOOL native_remove(NativeObject* o, unsigned char key) {
    size_t i = native_find(o, key);
    if (i == o->count) return TRUE;
    native_member_free(&o->members[i]);
    --o->count;
    if (i != o->count) {
        o->keys[i] = o->keys[o->count];
        o->members[i] = o->members[o->count];
    }
    return TRUE;
}
static BOOL native_clear(NativeObject* o) {
    while (o->count != 0) native_member_free(&o->members[--o->count]);
    return TRUE;
}
static BOOL native_free(NativeObject* o) {
    native_clear(o);
    ::free(o->keys); ::free(o->members); native_init(o);
    return TRUE;
}
static BOOL native_clone(NativeObject* out, const NativeObject* source) {
    native_init(out);
    if (!native_expand(out, source->count)) { native_free(out); return FALSE; }
    for (size_t i = 0; i < source->count; ++i) {
        if (!native_append_known(out, source->keys[i], source->members[i].data, source->members[i].size)) {
            native_free(out); return FALSE;
        }
    }
    return TRUE;
}
static BOOL native_extend(NativeObject* out, const NativeObject* base) {
    for (size_t i = 0; i < base->count; ++i)
        if (native_find(out, base->keys[i]) != out->count) return FALSE;
    size_t original = out->count;
    if (base->count > (size_t) -1 - original || !native_expand(out, original + base->count)) return FALSE;
    for (size_t i = 0; i < base->count; ++i) {
        if (!native_append_known(out, base->keys[i], base->members[i].data, base->members[i].size)) {
            while (out->count > original) native_member_free(&out->members[--out->count]);
            return FALSE;
        }
    }
    return TRUE;
}
static BOOL native_validate_member(const NativeMember* m, const NativeMember* r) {
    return m->size == r->size ? TRUE : FALSE;
}
static BOOL native_validate(NativeInterface* r, NativeObject* o, BOOL* valid) {
    for (size_t i = 0; i < r->object.count; ++i) {
        NativeMember* m = native_get(o, r->object.keys[i]);
        if (!m || !r->validate(m, &r->object.members[i])) { *valid = FALSE; return TRUE; }
    }
    *valid = TRUE; return TRUE;
}

static BOOL native_method(NativeObject* o, const void* arguments, void* output) {
    NativeMember* m = native_get(o, 0);
    if (!m) return FALSE;
    *(unsigned char*) output = (unsigned char) (m->data[0] ^ *(const unsigned char*) arguments);
    return TRUE;
}
static BOOL native_getter(NativeObject* o, void* output) {
    NativeMember* m = native_get(o, 0);
    if (!m) return FALSE;
    *(unsigned char*) output = m->data[0];
    return TRUE;
}
static BOOL native_setter(NativeObject* o, const void* input) {
    NativeMember* m = native_get(o, 0);
    if (!m) return FALSE;
    m->data[0] = *(const unsigned char*) input;
    return TRUE;
}
static CATALYST_RESULT catalyst_method(OBJECT* o, POINTER arguments, POINTERW output) {
    const BYTE key = 0;
    MEMBER* m = object_get(o, &key);
    if (!m) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xA1, 1, 0);
    *(BYTE*) output = (BYTE) (((const BYTE*) m->data)[0] ^ *(const BYTE*) arguments);
    return RESULT(STATUS_CODE_SUCCESS, 0xA1, 2, 0);
}
static CATALYST_RESULT catalyst_getter(OBJECT* o, POINTERW output) {
    const BYTE key = 0;
    MEMBER* m = object_get(o, &key);
    if (!m) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xA2, 1, 0);
    *(BYTE*) output = ((const BYTE*) m->data)[0];
    return RESULT(STATUS_CODE_SUCCESS, 0xA2, 2, 0);
}
static CATALYST_RESULT catalyst_setter(OBJECT* o, POINTER input) {
    const BYTE key = 0;
    MEMBER* m = object_get(o, &key);
    if (!m) return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0xA3, 1, 0);
    ((BYTE*) m->data)[0] = *(const BYTE*) input;
    return RESULT(STATUS_CODE_SUCCESS, 0xA3, 2, 0);
}
static CATALYST_RESULT catalyst_validate_member(const MEMBER* m, const MEMBER* r, BOOL* valid) {
    *valid = m->size == r->size ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0xA4, 1, 0);
}

static BOOL succeeded(CATALYST_RESULT result) { return status_code_is_success(result.status); }

// Thin adapters preserve identical workloads and independent implementations.
struct NativeAPI {
    typedef NativeMember Member;
    typedef NativeObject Object;
    typedef NativeField Field;
    typedef NativeObject Structure;
    typedef NativeObject Class;
    typedef NativeInterface Interface;
    static BOOL member_init(Member* m, NUINT size) { return native_member_init(m, size); }
    static BOOL member_free(Member* m) { native_member_free(m); return TRUE; }
    static void member_get(Member* m, BYTE* out) { ::memmove(out, m->data, m->size); }
    static void member_set(Member* m, const BYTE* in) { ::memmove(m->data, in, m->size); }
    static void init(Object* o) { native_init(o); }
    static BOOL expand(Object* o, NUINT n) { return native_expand(o, n); }
    static BOOL shrink(Object* o) { size_t n = o->capacity / 2; return native_resize(o, n < o->count ? o->count : n); }
    static BOOL add(Object* o, BYTE key, const void* data, NUINT size) { return native_add(o, key, data, size); }
    static BOOL remove(Object* o, BYTE key) { return native_remove(o, key); }
    static BOOL contains(Object* o, BYTE key) { return native_find(o, key) != o->count ? TRUE : FALSE; }
    static Member* get(Object* o, BYTE key) { return native_get(o, key); }
    static BOOL clear(Object* o) { return native_clear(o); }
    static BOOL release(Object* o) { return native_free(o); }
    static NUINT count(const Object* o) { return (NUINT) o->count; }
    static NUINT capacity(const Object* o) { return (NUINT) o->capacity; }
    static BOOL resolve(Field* f, Object* o, const BYTE* key) {
        size_t index = native_find(o, *key);
        if (index == o->count) {
            f->object = 0; f->key = 0; f->member = 0; return FALSE;
        }
        f->object = o; f->key = o->keys + index; f->member = &o->members[index];
        return TRUE;
    }
    static void field_get(Field* f, BYTE* out) { member_get(f->member, out); }
    static void field_set(Field* f, const BYTE* in) { member_set(f->member, in); }
    static BOOL add_method(Object* o, BYTE key) {
        NativeMethod m = native_method; return add(o, key, &m, (NUINT) sizeof(m));
    }
    static BOOL add_property(Object* o, BYTE key) {
        NativeProperty p = { native_getter, native_setter }; return add(o, key, &p, (NUINT) sizeof(p));
    }
    static BOOL invoke(Field* f, const BYTE* in, BYTE* out) {
        NativeMethod m; ::memcpy(&m, f->member->data, sizeof(m)); return m(f->object, in, out);
    }
    static BOOL property_get(Field* f, BYTE* out) {
        BOOL (*get)(NativeObject*, void*); ::memcpy(&get, f->member->data + offsetof(NativeProperty, get), sizeof(get)); return get(f->object, out);
    }
    static BOOL property_set(Field* f, const BYTE* in) {
        BOOL (*set)(NativeObject*, const void*); ::memcpy(&set, f->member->data + offsetof(NativeProperty, set), sizeof(set)); return set(f->object, in);
    }
    static BOOL structure_init(Structure* s, Object* o) { return native_clone(s, o); }
    static Member* structure_get(Structure* s, BYTE k) { return native_get(s, k); }
    static BOOL structure_free(Structure* s) { return native_free(s); }
    static BOOL class_init(Class* c, Object* o) { return native_clone(c, o); }
    static BOOL class_create(Class* c, Object* o) { return native_clone(o, c); }
    static BOOL class_extend(Class* c, Class* b) { return native_extend(c, b); }
    static Object* class_object(Class* c) { return c; }
    static BOOL class_free(Class* c) { return native_free(c); }
    static BOOL interface_init(Interface* r, Object* o) {
        r->validate = native_validate_member; return native_clone(&r->object, o);
    }
    static BOOL interface_validate(Interface* r, Object* o, BOOL* valid) { return native_validate(r, o, valid); }
    static BOOL interface_free(Interface* r) { return native_free(&r->object); }
};

struct CatalystAPI {
    typedef MEMBER Member;
    typedef OBJECT Object;
    typedef FIELD Field;
    typedef STRUCTURE Structure;
    typedef CLASS Class;
    typedef INTERFACE Interface;
    static BOOL member_init(Member* m, NUINT size) { return succeeded(catalyst::member_init(m, size)); }
    static BOOL member_free(Member* m) { return succeeded(catalyst::member_free(m)); }
    static void member_get(Member* m, BYTE* out) { catalyst::member_get(m, out); }
    static void member_set(Member* m, const BYTE* in) { catalyst::member_set(m, in); }
    static void init(Object* o) { catalyst::object_init(o, 1); }
    static BOOL expand(Object* o, NUINT n) { return succeeded(catalyst::object_expand(o, n)); }
    static BOOL shrink(Object* o) { return succeeded(catalyst::object_shrink(o)); }
    static BOOL add(Object* o, BYTE key, const void* data, NUINT size) { return succeeded(catalyst::object_add(o, &key, data, size)); }
    static BOOL remove(Object* o, BYTE key) { return succeeded(catalyst::object_remove(o, &key)); }
    static BOOL contains(Object* o, BYTE key) { return catalyst::object_contains(o, &key); }
    static Member* get(Object* o, BYTE key) { return catalyst::object_get(o, &key); }
    static BOOL clear(Object* o) { return succeeded(catalyst::object_clear(o)); }
    static BOOL release(Object* o) { return succeeded(catalyst::object_free(o)); }
    static NUINT count(const Object* o) { return o->members.cardinality; }
    static NUINT capacity(const Object* o) { return o->members.capacity; }
    static BOOL resolve(Field* f, Object* o, const BYTE* key) { return catalyst::field_init(f, o, key); }
    static void field_get(Field* f, BYTE* out) { catalyst::field_get(f, out); }
    static void field_set(Field* f, const BYTE* in) { catalyst::field_set(f, in); }
    static BOOL add_method(Object* o, BYTE key) {
        METHOD m = catalyst_method; return add(o, key, &m, (NUINT) sizeof(m));
    }
    static BOOL add_property(Object* o, BYTE key) {
        PROPERTY p = { catalyst_getter, catalyst_setter }; return add(o, key, &p, (NUINT) sizeof(p));
    }
    static BOOL invoke(Field* f, const BYTE* in, BYTE* out) { return succeeded(catalyst::method_invoke(f, in, out)); }
    static BOOL property_get(Field* f, BYTE* out) { return succeeded(catalyst::property_get(f, out)); }
    static BOOL property_set(Field* f, const BYTE* in) { return succeeded(catalyst::property_set(f, in)); }
    static BOOL structure_init(Structure* s, Object* o) { return succeeded(catalyst::structure_init(s, o)); }
    static Member* structure_get(Structure* s, BYTE k) { return catalyst::structure_get(s, &k); }
    static BOOL structure_free(Structure* s) { return succeeded(catalyst::structure_free(s)); }
    static BOOL class_init(Class* c, Object* o) { return succeeded(catalyst::class_init(c, o)); }
    static BOOL class_create(Class* c, Object* o) { return succeeded(catalyst::class_create(c, o)); }
    static BOOL class_extend(Class* c, Class* b) { return succeeded(catalyst::class_extend(c, b)); }
    static Object* class_object(Class* c) { return &c->object; }
    static BOOL class_free(Class* c) { return succeeded(catalyst::class_free(c)); }
    static BOOL interface_init(Interface* r, Object* o) { return succeeded(catalyst::interface_init(r, o, catalyst_validate_member)); }
    static BOOL interface_validate(Interface* r, Object* o, BOOL* valid) { return succeeded(catalyst::interface_validate(r, o, valid)); }
    static BOOL interface_free(Interface* r) { return succeeded(catalyst::interface_free(r)); }
};

static volatile BYTE observed = 0;
static const void* volatile observedStorage = 0;
enum Measurement { MEASURE_OK, MEASURE_CLOCK, MEASURE_FAIL };

static BOOL elapsed(std::clock_t start, unsigned long* ticks) {
    const std::clock_t end = std::clock();
    if (start == (std::clock_t) -1 || start < 0 ||
        end == (std::clock_t) -1 || end < start) return FALSE;
    if (end - start > ULONG_MAX) return FALSE;
    *ticks = (unsigned long) (end - start);
    return TRUE;
}

static unsigned long median(unsigned long* values) {
    unsigned long a = values[0], b = values[1], c = values[2];
    if (a > b) { unsigned long t = a; a = b; b = t; }
    if (b > c) { unsigned long t = b; b = c; c = t; }
    return a > b ? a : b;
}

/* Convert a fractional remainder into one decimal digit without overflow. */
static unsigned long decimal_digit(unsigned long* remainder, unsigned long divisor) {
    unsigned long original = *remainder, carry = 0, digit = 0;
    for (BYTE i = 0; i < 10; ++i) {
        if (carry >= divisor - original) {
            carry -= divisor - original;
            ++digit;
        } else {
            carry += original;
        }
    }
    *remainder = carry;
    return digit;
}

/* Print ns/call to three decimal places without floating point or large products.
 * Divide the decimal digits of elapsed seconds by the call count as they are
 * generated. maxCalls below ensures carry * 10 fits even a 32-bit unsigned long.
 */
static void print_nanoseconds(unsigned long ticks, unsigned long calls) {
    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long seconds = ticks / frequency;
    unsigned long remainder = ticks % frequency;
    unsigned long value = seconds / calls, carry = seconds % calls;
    unsigned long fraction = 0;
    for (BYTE i = 0; i < 12; ++i) {
        carry = carry * 10 + decimal_digit(&remainder, frequency);
        unsigned long digit = carry / calls;
        carry %= calls;
        if (i < 9) {
            if (value > (ULONG_MAX - digit) / 10) {
                std::printf(" %14s", "out of range");
                return;
            }
            value = value * 10 + digit;
        } else {
            fraction = fraction * 10 + digit;
        }
    }
    if (value == 0 && fraction == 0) std::printf(" %14s", "<0.001 ns");
    else std::printf(" %7lu.%03lu ns", value, fraction);
}

/* Report the change in execution time relative to Native-C, not speedup. */
static void print_difference(unsigned long native, unsigned long comparison) {
    unsigned long delta = comparison >= native ? comparison - native : native - comparison;
    unsigned long whole = delta / native, remainder = delta % native;
    unsigned long percent = decimal_digit(&remainder, native) * 10;
    percent += decimal_digit(&remainder, native);
    unsigned long tenth = decimal_digit(&remainder, native);
    if (whole > (ULONG_MAX - percent) / 100) {
        std::printf(" %12s", "out of range");
        return;
    }
    percent += whole * 100;
    // Pad the complete signed percentage, not the digits after the sign.
    BYTE digits = 1;
    for (unsigned long remaining = percent; remaining >= 10; remaining /= 10) ++digits;
    for (BYTE i = digits + 4; i < 12; ++i) std::putchar(' ');
    std::printf(" %c%lu.%lu%%", comparison < native ? '-' : '+', percent, tenth);
}

struct Summary {
    unsigned long faster;
    unsigned long similar;
    unsigned long slower;
    unsigned long review;
    unsigned long shortSamples;
    unsigned long unavailable;
    unsigned long failed;
};

static void report_unavailable(const char* name, Summary& summary) {
    std::printf("  %-28s %14s %14s %12s  No timing\n", name, "-", "-", "-");
    ++summary.unavailable;
}



enum Kind {
    MEMBER_LIFECYCLE, MEMBER_COPY, OBJECT_EMPTY, OBJECT_RESIZE, OBJECT_ADD_REMOVE,
    OBJECT_DUPLICATE_MISS, OBJECT_LOOKUP_FIRST, OBJECT_LOOKUP_LAST, OBJECT_LOOKUP_MISS,
    OBJECT_CLEAR_REFILL, FIELD_RESOLVE, FIELD_COPY, METHOD_CALL, PROPERTY_ACCESS,
    STRUCTURE_LIFECYCLE, CLASS_LIFECYCLE, CLASS_INSTANCE, CLASS_EXTENSION,
    INTERFACE_LIFECYCLE, INTERFACE_HIT, INTERFACE_MISS
};

template<typename API>
struct Trial {
    Kind kind;
    NUINT n, width;
    typename API::Object object, extra;
    typename API::Member member;
    typename API::Field field;
    typename API::Class class_, base;
    typename API::Interface interface_;
    BYTE key, miss, methodKey, propertyKey;
    BYTE data[32], alternate[32], output[32];
    BOOL memberLive, classLive, baseLive, interfaceLive;
};

static void payload(BYTE* data, NUINT width, BYTE key) {
    for (NUINT i = 0; i < width; ++i) data[i] = (BYTE) (key ^ (BYTE) i ^ (BYTE) 0x5A);
}

template<typename API>
static BOOL observe_member(typename API::Member* m, NUINT size, BYTE first, BYTE last) {
    if (!m || m->size != size || m->data == 0) return FALSE;
    observedStorage = m->data;
    const volatile BYTE* bytes = (const volatile BYTE*) m->data;
    const BYTE a = bytes[0], b = bytes[size - (NUINT) 1];
    observed = (BYTE) (a ^ b);
    observedStorage = 0;
    return a == first && b == last ? TRUE : FALSE;
}

template<typename API>
static BOOL fill(typename API::Object* o, NUINT n, NUINT width) {
    BYTE data[32];
    for (NUINT i = 0; i < n; ++i) {
        payload(data, width, (BYTE) i);
        if (!API::add(o, (BYTE) i, data, width)) return FALSE;
    }
    return TRUE;
}

template<typename API>
static BOOL observe_object(typename API::Object* o, NUINT n, NUINT width) {
    if (API::count(o) != n) return FALSE;
    BYTE last = (BYTE) (n - (NUINT) 1);
    BYTE end = (BYTE) (width - (NUINT) 1);
    return observe_member<API>(API::get(o, 0), width, 0x5A, (BYTE) (0x5A ^ end)) &&
           observe_member<API>(API::get(o, last), width, (BYTE) (last ^ 0x5A), (BYTE) (last ^ 0x5A ^ end));
}

template<typename API>
static BOOL setup(Trial<API>* t) {
    API::init(&t->object); API::init(&t->extra);
    t->memberLive = t->classLive = t->baseLive = t->interfaceLive = FALSE;
    t->key = (BYTE) (t->n - (NUINT) 1);
    t->miss = (BYTE) t->n;
    t->methodKey = (BYTE) (t->n + (NUINT) 1);
    t->propertyKey = (BYTE) (t->n + (NUINT) 2);
    payload(t->data, t->width, t->key);
    payload(t->alternate, t->width, (BYTE) (t->key ^ 0xFF));
    if (t->kind == MEMBER_LIFECYCLE || t->kind == OBJECT_EMPTY) return TRUE;
    if (t->kind == MEMBER_COPY) {
        t->memberLive = TRUE;
        if (!API::member_init(&t->member, t->width)) return FALSE;
        API::member_set(&t->member, t->data);
        return TRUE;
    }
    NUINT cap = t->n;
    if (t->kind == OBJECT_ADD_REMOVE || t->kind == METHOD_CALL || t->kind == PROPERTY_ACCESS)
        cap = (NUINT) (cap + (NUINT) 1);
    if (!API::expand(&t->object, cap) || !fill<API>(&t->object, t->n, t->width)) return FALSE;
    if (t->kind == FIELD_COPY && !API::resolve(&t->field, &t->object, &t->key)) return FALSE;
    if (t->kind == METHOD_CALL) {
        if (!API::add_method(&t->object, t->methodKey) || !API::resolve(&t->field, &t->object, &t->methodKey)) return FALSE;
    }
    if (t->kind == PROPERTY_ACCESS) {
        if (!API::add_property(&t->object, t->propertyKey) || !API::resolve(&t->field, &t->object, &t->propertyKey)) return FALSE;
    }
    if (t->kind == CLASS_INSTANCE || t->kind == CLASS_EXTENSION) {
        t->classLive = TRUE;
        if (!API::class_init(&t->class_, &t->object)) return FALSE;
    }
    if (t->kind == CLASS_EXTENSION || t->kind == INTERFACE_MISS) {
        BYTE data[32]; payload(data, t->width, t->miss);
        if (!API::expand(&t->extra, 1) || !API::add(&t->extra, t->miss, data, t->width)) return FALSE;
    }
    if (t->kind == CLASS_EXTENSION) {
        t->baseLive = TRUE;
        if (!API::class_init(&t->base, &t->extra)) return FALSE;
    }
    if (t->kind == INTERFACE_HIT || t->kind == INTERFACE_MISS) {
        t->interfaceLive = TRUE;
        if (!API::interface_init(&t->interface_, t->kind == INTERFACE_HIT ? &t->object : &t->extra)) return FALSE;
    }
    return TRUE;
}

template<typename API>
static BOOL cleanup(Trial<API>* t) {
    BOOL ok = TRUE;
    if (t->interfaceLive && !API::interface_free(&t->interface_)) ok = FALSE;
    if (t->baseLive && !API::class_free(&t->base)) ok = FALSE;
    if (t->classLive && !API::class_free(&t->class_)) ok = FALSE;
    if (t->memberLive && !API::member_free(&t->member)) ok = FALSE;
    if (!API::release(&t->extra)) ok = FALSE;
    if (!API::release(&t->object)) ok = FALSE;
    return ok;
}

// Each cycle either leaves the fixture unchanged or explicitly restores it.
template<typename API>
static BOOL cycle(Trial<API>* t) {
    BYTE end = (BYTE) (t->width - (NUINT) 1);
    BYTE a = t->data[0], b = t->data[end];
    BOOL ok = TRUE, valid = FALSE;
    switch (t->kind) {
        case MEMBER_LIFECYCLE: {
            typename API::Member m;
            ok = API::member_init(&m, t->width);
            if (ok) ok = observe_member<API>(&m, t->width, 0, 0);
            if (!API::member_free(&m)) ok = FALSE;
            return ok;
        }
        case MEMBER_COPY:
            API::member_set(&t->member, t->alternate);
            API::member_get(&t->member, t->output);
            observed = (BYTE) (t->output[0] ^ t->output[end]);
            return t->output[0] == t->alternate[0] && t->output[end] == t->alternate[end] ? TRUE : FALSE;
        case OBJECT_EMPTY: {
            typename API::Object o;
            API::init(&o);
            ok = API::count(&o) == 0 && API::capacity(&o) == 0 ? TRUE : FALSE;
            if (!API::release(&o)) ok = FALSE;
            return ok;
        }
        case OBJECT_RESIZE:
            return API::expand(&t->object, (NUINT) (t->n * (NUINT) 2)) &&
                   API::shrink(&t->object) && API::capacity(&t->object) == t->n ? TRUE : FALSE;
        case OBJECT_ADD_REMOVE:
            if (!API::add(&t->object, t->miss, t->data, t->width)) return FALSE;
            ok = observe_member<API>(API::get(&t->object, t->miss), t->width, a, b);
            if (!API::remove(&t->object, t->miss)) ok = FALSE;
            return ok;
        case OBJECT_DUPLICATE_MISS:
            return API::add(&t->object, t->key, t->alternate, t->width) &&
                   API::remove(&t->object, t->miss) && API::count(&t->object) == t->n ? TRUE : FALSE;
        case OBJECT_LOOKUP_FIRST:
            if (!API::contains(&t->object, 0)) return FALSE;
            return observe_member<API>(API::get(&t->object, 0), t->width, 0x5A, (BYTE) (0x5A ^ end));
        case OBJECT_LOOKUP_LAST:
            if (!API::contains(&t->object, t->key)) return FALSE;
            return observe_member<API>(API::get(&t->object, t->key), t->width, a, b);
        case OBJECT_LOOKUP_MISS:
            return !API::contains(&t->object, t->miss) && API::get(&t->object, t->miss) == 0 ? TRUE : FALSE;
        case OBJECT_CLEAR_REFILL:
            return API::clear(&t->object) && API::count(&t->object) == 0 &&
                   fill<API>(&t->object, t->n, t->width) ? TRUE : FALSE;
        case FIELD_RESOLVE: {
            typename API::Field field;
            if (!API::resolve(&field, &t->object, &t->key)) return FALSE;
            if (field.object != &t->object || field.key == 0 ||
                *(const BYTE*) field.key != t->key) return FALSE;
            API::field_get(&field, t->output);
            observed = (BYTE) (t->output[0] ^ t->output[end]);
            return t->output[0] == a && t->output[end] == b ? TRUE : FALSE;
        }
        case FIELD_COPY:
            API::field_set(&t->field, t->alternate);
            API::field_get(&t->field, t->output);
            observed = (BYTE) (t->output[0] ^ t->output[end]);
            ok = t->output[0] == t->alternate[0] && t->output[end] == t->alternate[end] ? TRUE : FALSE;
            API::field_set(&t->field, t->data);
            return ok;
        case METHOD_CALL: {
            const BYTE args[] = { 0xA5 };
            BYTE out = 0;
            ok = API::invoke(&t->field, args, &out);
            observed = out;
            return ok && out == (BYTE) (0x5A ^ 0xA5) ? TRUE : FALSE;
        }
        case PROPERTY_ACCESS: {
            const BYTE input = 0xA5, restore = 0x5A;
            BYTE out = 0;
            ok = API::property_set(&t->field, &input) && API::property_get(&t->field, &out);
            observed = out;
            if (!API::property_set(&t->field, &restore)) ok = FALSE;
            return ok && out == input ? TRUE : FALSE;
        }
        case STRUCTURE_LIFECYCLE: {
            typename API::Structure s;
            ok = API::structure_init(&s, &t->object);
            if (ok) ok = observe_member<API>(API::structure_get(&s, t->key), t->width, a, b);
            if (!API::structure_free(&s)) ok = FALSE;
            return ok;
        }
        case CLASS_LIFECYCLE: {
            typename API::Class c;
            ok = API::class_init(&c, &t->object);
            if (ok) ok = observe_object<API>(API::class_object(&c), t->n, t->width);
            if (!API::class_free(&c)) ok = FALSE;
            return ok;
        }
        case CLASS_INSTANCE: {
            typename API::Object o;
            ok = API::class_create(&t->class_, &o);
            if (ok) ok = observe_object<API>(&o, t->n, t->width);
            if (!API::release(&o)) ok = FALSE;
            return ok;
        }
        case CLASS_EXTENSION: {
            typename API::Class c;
            ok = API::class_init(&c, &t->object);
            if (ok) ok = API::class_extend(&c, &t->base);
            if (ok) ok = observe_object<API>(API::class_object(&c), (NUINT) (t->n + (NUINT) 1), t->width);
            if (!API::class_free(&c)) ok = FALSE;
            return ok;
        }
        case INTERFACE_LIFECYCLE: {
            typename API::Interface r;
            ok = API::interface_init(&r, &t->object);
            if (ok) ok = API::interface_validate(&r, &t->object, &valid) && valid ? TRUE : FALSE;
            if (!API::interface_free(&r)) ok = FALSE;
            return ok;
        }
        case INTERFACE_HIT:
        case INTERFACE_MISS:
            ok = API::interface_validate(&t->interface_, &t->object, &valid);
            observed = valid;
            return ok && valid == (t->kind == INTERFACE_HIT ? TRUE : FALSE) ? TRUE : FALSE;
    }
    return FALSE;
}

template<typename API>
static BOOL verify_fixture(Trial<API>* t) {
    if (t->kind == MEMBER_LIFECYCLE || t->kind == OBJECT_EMPTY) return TRUE;
    if (t->kind == MEMBER_COPY) {
        API::member_get(&t->member, t->output);
        for (NUINT i = 0; i < t->width; ++i)
            if (t->output[i] != t->alternate[i]) return FALSE;
        return TRUE;
    }
    const NUINT count = (NUINT) (t->n + ((t->kind == METHOD_CALL || t->kind == PROPERTY_ACCESS) ? (NUINT) 1 : (NUINT) 0));
    if (API::count(&t->object) != count || API::get(&t->object, t->miss) != 0) return FALSE;
    BYTE expected[32], actual[32];
    for (NUINT i = 0; i < t->n; ++i) {
        typename API::Member* m = API::get(&t->object, (BYTE) i);
        if (!m || m->size != t->width) return FALSE;
        payload(expected, t->width, (BYTE) i);
        API::member_get(m, actual);
        for (NUINT j = 0; j < t->width; ++j)
            if (actual[j] != expected[j]) return FALSE;
    }
    return TRUE;
}

template<typename API>
static Measurement measure(Trial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    BOOL ok = setup<API>(&t);
    BOOL (*volatile operation)(Trial<API>*) = cycle<API>;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        ok = operation(&t);
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify) ok = verify_fixture<API>(&t);
    if (!cleanup<API>(&t)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

static void report_failure(const char* label, Measurement result, Summary& summary) {
    if (result == MEASURE_CLOCK) report_unavailable(label, summary);
    else {
        std::printf("  %-28s FAIL: operation, fixture or verification failed\n", label);
        ++summary.failed;
    }
}

template<typename Native, typename Catalyst>
static void benchmark_case(const char* label, Native& native, Catalyst& comparison,
                           Measurement (*measureNative)(Native&, unsigned long, unsigned long*, BOOL),
                           Measurement (*measureCatalyst)(Catalyst&, unsigned long, unsigned long*, BOOL),
                           unsigned long maxCalls, Summary& summary) {
    unsigned long n = 0, c = 0;
    Measurement result = measureNative(native, 1, &n, TRUE);
    if (result == MEASURE_OK) result = measureCatalyst(comparison, 1, &c, TRUE);
    if (result != MEASURE_OK) { report_failure(label, result, summary); return; }

    const unsigned long frequency = (unsigned long) CLOCKS_PER_SEC;
    unsigned long target = frequency / 100; // 10 ms per path.
    if (target < 20) target = 20;
    unsigned long minimum = frequency / 1000;
    if (minimum < 20) minimum = 20;
    unsigned long budget = frequency / 10; // 100 ms cumulative calibration CPU budget.
    if (budget < target) budget = target;
    unsigned long sampleBudget = frequency / 10; // Bound full setup + timing + cleanup per pair.
    if (sampleBudget < target) sampleBudget = target;
    unsigned long calls = 16;
    const std::clock_t calibrationStart = std::clock();

    for (;;) {
        const std::clock_t attemptStart = std::clock();
        result = measureNative(native, calls, &n, FALSE);
        if (result == MEASURE_OK) result = measureCatalyst(comparison, calls, &c, FALSE);
        if (result != MEASURE_OK) { report_failure(label, result, summary); return; }
        unsigned long totalTicks = 0, attemptTicks = 0;
        if (!elapsed(calibrationStart, &totalTicks) || !elapsed(attemptStart, &attemptTicks)) {
            report_failure(label, MEASURE_CLOCK, summary);
            return;
        }
        unsigned long faster = n < c ? n : c;
        if (faster >= target || calls >= maxCalls || totalTicks >= budget ||
            attemptTicks >= sampleBudget) break;

        // Estimate a bounded multiplier from the last observation. No
        // unchecked multiplication, and zero-tick readings grow by eight.
        unsigned long factor = faster == 0 ? 8 : target / faster;
        if (factor < 2) factor = 2;
        if (factor > 16) factor = 16;
        if (attemptTicks != 0) {
            unsigned long remaining = budget - totalTicks;
            unsigned long allowed = remaining < sampleBudget ? remaining : sampleBudget;
            unsigned long affordable = allowed / attemptTicks;
            if (affordable < 2) break;
            if (factor > affordable) factor = affordable;
        }
        if (factor > maxCalls / calls) calls = maxCalls;
        else calls *= factor;
    }
    unsigned long nativeSamples[3], catalystSamples[3];
    for (BYTE sample = 0; sample < 3; ++sample) {
        if (sample == 1) {
            result = measureCatalyst(comparison, calls, &catalystSamples[sample], TRUE);
            if (result == MEASURE_OK) result = measureNative(native, calls, &nativeSamples[sample], TRUE);
        } else {
            result = measureNative(native, calls, &nativeSamples[sample], TRUE);
            if (result == MEASURE_OK) result = measureCatalyst(comparison, calls, &catalystSamples[sample], TRUE);
        }
        if (result != MEASURE_OK) { report_failure(label, result, summary); return; }
    }
    n = median(nativeSamples);
    c = median(catalystSamples);
    if (n == 0 || c == 0) { report_failure(label, MEASURE_CLOCK, summary); return; }
    std::printf("  %-28s", label);
    print_nanoseconds(n, calls);
    print_nanoseconds(c, calls);
    print_difference(n, c);
    if (n < minimum || c < minimum) {
        std::printf("  Short sample *");
        ++summary.shortSamples;
    } else {
        unsigned long delta = c >= n ? c - n : n - c;
        if (delta <= n / 20) {
            ++summary.similar;
            std::printf("  Comparable");
        } else if (c < n) {
            ++summary.faster;
            std::printf("  Faster");
        } else {
            ++summary.slower;
            if (delta >= n / 5 + (n % 5 != 0 ? 1 : 0)) {
                ++summary.review;
                std::printf("  Review");
            } else std::printf("  Slower");
        }
    }
    std::printf("\n");
    std::fflush(stdout);
}



static void benchmark_all(Summary& summary) {
    const char* names[] = {
        "Member: init + free", "Member: set + get", "Object: empty init + free",
        "Object: expand + shrink", "Object: add + get + remove", "Object: duplicate + absent remove",
        "Object: contains + get (first)", "Object: contains + get (last)", "Object: contains + get (miss)",
        "Object: clear + refill", "Field: resolve + get", "Field: set + get + restore",
        "Method: invoke (resolved)", "Property: set + get + restore",
        "Structure: init + get + free", "Class: init + inspect + free",
        "Class: create + inspect + free", "Class: init + extend + inspect + free",
        "Interface: init + validate + free", "Interface: validate (hit)", "Interface: validate (miss)"
    };
    const NUINT counts[] = {
        2,
#if TARGET_SIZEOF_VOID_P >= 2
        16, 64
#endif
    };
    const NUINT widths[] = {
        1,
#if TARGET_SIZEOF_VOID_P >= 2
        8, 32
#endif
    };
    for (BYTE k = 0; k < (BYTE) (sizeof(names) / sizeof(names[0])); ++k) {
        std::printf("\n%s\n", names[k]);
        for (BYTE i = 0; i < (BYTE) (sizeof(counts) / sizeof(counts[0])); ++i) {
            for (BYTE w = 0; w < (BYTE) (sizeof(widths) / sizeof(widths[0])); ++w) {
                if ((k == MEMBER_LIFECYCLE || k == MEMBER_COPY) && i != 0) continue;
                if (k == OBJECT_EMPTY && (i != 0 || w != 0)) continue;
                if ((k == METHOD_CALL || k == PROPERTY_ACCESS || k == INTERFACE_HIT || k == INTERFACE_MISS) && w != 0) continue;
                NUINT n = counts[i], width = widths[w];
                // Capacity, member descriptors, and payload sizes fit the
                // configured pointer-width representation even on narrow targets.
                if (n > (NUINT) (CATALYST_NUINT_MAX / (NUINT) sizeof(MEMBER) / (NUINT) 2) ||
                    n > (NUINT) (CATALYST_NUINT_MAX / width / (NUINT) 2)) continue;
                Trial<NativeAPI> native;
                Trial<CatalystAPI> comparison;
                native.kind = comparison.kind = (Kind) k;
                native.n = comparison.n = n;
                native.width = comparison.width = width;
                char label[48];
                if (k == MEMBER_LIFECYCLE || k == MEMBER_COPY)
                    std::sprintf(label, "%lu B payload", (unsigned long) width);
                else if (k == OBJECT_EMPTY)
                    std::sprintf(label, "empty object");
                else
                    std::sprintf(label, "%lu members x %lu B", (unsigned long) n, (unsigned long) width);
                benchmark_case(label, native, comparison, measure<NativeAPI>, measure<CatalystAPI>, 67108864UL, summary);
            }
        }
    }
}

#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATOOLIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATOOLIB Performance (Release recommended)\n");
    std::printf("Independent C reference: byte keys, contiguous records, owned payloads, callback dispatch.\n");
    std::printf("Lower time is better; positive difference means Catalyst is slower.\n");
    std::printf("Median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("Times are per named operation or cycle, including listed observation and restoration.\n");
    std::printf("Fixtures and full final verification are untimed; both paths consume outcomes.\n");
    std::printf("Both paths use established distinctness for cloning and release from the end.\n");
    std::printf("Baseline uses native C allocation and Boolean outcomes; CATOOLIB uses its managed heap and Results.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-28s %14s %14s %12s  %s\n", "Workload", "Baseline", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
    benchmark_all(summary);
    std::printf("\nPerformance summary\n");
    std::printf("  Faster (>5%%):                  %3lu\n", summary.faster);
    std::printf("  Comparable (within 5%%):        %3lu\n", summary.similar);
    std::printf("  Slower (>5%%):                  %3lu\n", summary.slower);
    std::printf("    At least 20%% slower:          %3lu (review)\n", summary.review);
    std::printf("  Short samples (*):             %3lu\n", summary.shortSamples);
    std::printf("  Timing unavailable:            %3lu\n", summary.unavailable);
    std::printf("  Verification failures:         %3lu\n", summary.failed);
    if (summary.shortSamples != 0)
        std::printf("  * Less than 1 ms or 20 ticks; excluded from performance classification.\n");
    if (summary.failed != 0) return 1;
    std::printf("\nAll CATOOLIB performance comparisons verified.\n");
    return summary.unavailable != 0 ? 2 : 0;
#else
    std::printf("CATOOLIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
