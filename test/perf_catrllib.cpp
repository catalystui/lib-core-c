#include <cstdio>
#include <ctime>
#include <climits>
#include <stdlib.h>
#include <string.h>

#include "catalyst.hpp"
#include "catrllib.h"

using namespace catalyst;

#if defined(CLOCKS_PER_SEC)

// ------------------------------------------------------------
// Independent C-style contiguous collection reference.
// Native sizes and C allocation are restricted to the baseline.
// ------------------------------------------------------------
struct NativeCollection {
    unsigned char* keys;
    unsigned char* values;
    size_t size, valueSize, count, capacity;
};

static void native_init(NativeCollection& c, size_t size, size_t valueSize) {
    c.keys = 0; c.values = 0; c.size = size; c.valueSize = valueSize;
    c.count = 0; c.capacity = 0;
}

static BOOL native_buffer(unsigned char** p, size_t oldBytes, size_t bytes) {
    if (bytes == 0) { ::free(*p); *p = 0; return TRUE; }
    void* resized = ::realloc(*p, bytes);
    if (resized == 0) return FALSE;
    *p = (unsigned char*) resized;
    if (bytes > oldBytes) ::memset(*p + oldBytes, 0, bytes - oldBytes);
    return TRUE;
}

static BOOL native_capacity(NativeCollection& c, size_t capacity) {
    if (capacity == c.capacity) return TRUE;
    if (capacity > (size_t) -1 / c.size ||
        (c.valueSize != 0 && capacity > (size_t) -1 / c.valueSize)) return FALSE;
    size_t old = c.capacity;
    if (!native_buffer(&c.keys, old * c.size, capacity * c.size)) return FALSE;
    if (capacity < old) c.capacity = capacity;
    if (c.valueSize != 0 && !native_buffer(&c.values, old * c.valueSize, capacity * c.valueSize))
        return FALSE;
    c.capacity = capacity;
    return TRUE;
}

static BOOL native_expand(NativeCollection& c, size_t capacity) {
    return capacity <= c.capacity ? TRUE : native_capacity(c, capacity);
}

static BOOL native_shrink(NativeCollection& c) {
    size_t capacity = c.capacity / 2;
    if (capacity < c.count) capacity = c.count;
    return native_capacity(c, capacity);
}

static BOOL native_free(NativeCollection& c) {
    if (c.keys != 0) ::free(c.keys);
    if (c.values != 0) ::free(c.values);
    c.keys = 0; c.values = 0; c.count = 0; c.capacity = 0;
    return TRUE;
}

static size_t native_find(const NativeCollection& c, const unsigned char* key) {
    for (size_t i = 0; i < c.count; ++i)
        if (::memcmp(c.keys + i * c.size, key, c.size) == 0) return i;
    return c.count;
}

static BOOL native_add(NativeCollection& c, const unsigned char* key, const unsigned char* value) {
    if (native_find(c, key) != c.count) return FALSE;
    ::memcpy(c.keys + c.count * c.size, key, c.size);
    if (c.valueSize != 0) ::memcpy(c.values + c.count * c.valueSize, value, c.valueSize);
    ++c.count;
    return TRUE;
}

static BOOL native_remove(NativeCollection& c, const unsigned char* key) {
    size_t i = native_find(c, key);
    if (i == c.count) return FALSE;
    --c.count;
    if (i != c.count) {
        ::memcpy(c.keys + i * c.size, c.keys + c.count * c.size, c.size);
        if (c.valueSize != 0)
            ::memcpy(c.values + i * c.valueSize, c.values + c.count * c.valueSize, c.valueSize);
    }
    return TRUE;
}

static void native_insert(NativeCollection& c, size_t index, const unsigned char* key) {
    unsigned char* p = c.keys + index * c.size;
    if (index < c.count) ::memmove(p + c.size, p, (c.count - index) * c.size);
    ::memcpy(p, key, c.size);
    ++c.count;
}

static void native_erase(NativeCollection& c, size_t index) {
    --c.count;
    if (index < c.count) {
        unsigned char* p = c.keys + index * c.size;
        ::memmove(p, p + c.size, (c.count - index) * c.size);
    }
}

static BOOL succeeded(CATALYST_RESULT r) { return status_code_is_success(r.status); }

// These thin benchmark adapters keep each timed path independent.
struct NativeSet {
    typedef NativeCollection Data;
    static void init(Data& c, NUINT size) { native_init(c, size, 0); }
    static BOOL expand(Data& c, NUINT n) { return native_expand(c, n); }
    static BOOL shrink(Data& c) { return native_shrink(c); }
    static BOOL release(Data& c) { return native_free(c); }
    static NUINT count(const Data& c) { return (NUINT) c.count; }
    static NUINT capacity(const Data& c) { return (NUINT) c.capacity; }
    static const BYTE* keys(const Data& c) { return c.keys; }
    static const BYTE* values(const Data& c) { return c.values; }
    static void clear(Data& c) { c.count = 0; }
    static BOOL contains(Data& c, const BYTE* k) { return native_find(c, k) != c.count ? TRUE : FALSE; }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { return native_add(c, k, v); }
    static BOOL remove(Data& c, const BYTE* k) { return native_remove(c, k); }
};

struct CatalystSet {
    typedef CATRLLIB_SET Data;
    static void init(Data& c, NUINT size) { catrl_set_init(&c, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catrl_set_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catrl_set_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catrl_set_free(&c)); }
    static NUINT count(const Data& c) { return c.cardinality; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.data; }
    static const BYTE* values(const Data& c) { (void) c; return 0; }
    static void clear(Data& c) { catrl_set_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catrl_set_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { (void) v; return catrl_set_add(&c, k); }
    static BOOL remove(Data& c, const BYTE* k) { return catrl_set_remove(&c, k); }
};

struct NativeMap {
    typedef NativeCollection Data;
    static void init(Data& c, NUINT size) { native_init(c, size, size); }
    static BOOL expand(Data& c, NUINT n) { return native_expand(c, n); }
    static BOOL shrink(Data& c) { return native_shrink(c); }
    static BOOL release(Data& c) { return native_free(c); }
    static NUINT count(const Data& c) { return (NUINT) c.count; }
    static NUINT capacity(const Data& c) { return (NUINT) c.capacity; }
    static const BYTE* keys(const Data& c) { return c.keys; }
    static const BYTE* values(const Data& c) { return c.values; }
    static void clear(Data& c) { c.count = 0; }
    static BOOL contains(Data& c, const BYTE* k) { return native_find(c, k) != c.count ? TRUE : FALSE; }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { return native_add(c, k, v); }
    static BOOL remove(Data& c, const BYTE* k) { return native_remove(c, k); }
    static BYTE* get(Data& c, const BYTE* k) { size_t i = native_find(c, k); return i == c.count ? 0 : c.values + i * c.valueSize; }
    static BOOL set(Data& c, const BYTE* k, const BYTE* v) { BYTE* p = get(c, k); if (p == 0) return FALSE; ::memcpy(p, v, c.valueSize); return TRUE; }
};

struct CatalystMap {
    typedef CATRLLIB_MAP Data;
    static void init(Data& c, NUINT size) { catrl_map_init(&c, size, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catrl_map_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catrl_map_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catrl_map_free(&c)); }
    static NUINT count(const Data& c) { return c.cardinality; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.keys; }
    static const BYTE* values(const Data& c) { return (const BYTE*) c.values; }
    static void clear(Data& c) { catrl_map_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catrl_map_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { return catrl_map_add(&c, k, v); }
    static BOOL remove(Data& c, const BYTE* k) { return catrl_map_remove(&c, k); }
    static BYTE* get(Data& c, const BYTE* k) { return (BYTE*) catrl_map_get(&c, k); }
    static BOOL set(Data& c, const BYTE* k, const BYTE* v) { return catrl_map_set(&c, k, v); }
};

struct NativeArray {
    typedef NativeCollection Data;
    static void init(Data& c, NUINT size) { native_init(c, size, 0); }
    static BOOL expand(Data& c, NUINT n) { return native_expand(c, n); }
    static BOOL shrink(Data& c) { return native_shrink(c); }
    static BOOL release(Data& c) { return native_free(c); }
    static NUINT count(const Data& c) { return (NUINT) c.count; }
    static NUINT capacity(const Data& c) { return (NUINT) c.capacity; }
    static const BYTE* keys(const Data& c) { return c.keys; }
    static const BYTE* values(const Data& c) { return c.values; }
    static void clear(Data& c) { c.count = 0; }
    static BOOL contains(Data& c, const BYTE* k) { return native_find(c, k) != c.count ? TRUE : FALSE; }
    static BOOL add(Data& c, const BYTE* k, const BYTE*) { native_insert(c, c.count, k); return TRUE; }
    static BOOL remove(Data& c, NUINT i) { native_erase(c, i); return TRUE; }
    static BOOL insert(Data& c, NUINT i, const BYTE* k) { native_insert(c, i, k); return TRUE; }
    static BYTE* get(Data& c, NUINT i) { return c.keys + (size_t) i * c.size; }
    static BOOL set(Data& c, NUINT i, const BYTE* k) { ::memcpy(get(c, i), k, c.size); return TRUE; }
};

struct CatalystArray {
    typedef CATRLLIB_ARRAY Data;
    static void init(Data& c, NUINT size) { catrl_array_init(&c, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catrl_array_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catrl_array_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catrl_array_free(&c)); }
    static NUINT count(const Data& c) { return c.length; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.data; }
    static const BYTE* values(const Data& c) { (void) c; return 0; }
    static void clear(Data& c) { catrl_array_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catrl_array_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE*) { catrl_array_add(&c, k); return TRUE; }
    static BOOL remove(Data& c, NUINT i) { catrl_array_remove(&c, i); return TRUE; }
    static BOOL insert(Data& c, NUINT i, const BYTE* k) { catrl_array_insert(&c, i, k); return TRUE; }
    static BYTE* get(Data& c, NUINT i) { return (BYTE*) catrl_array_get(&c, i); }
    static BOOL set(Data& c, NUINT i, const BYTE* k) { catrl_array_set(&c, i, k); return TRUE; }
};

// ------------------------------------------------------------
// Fixtures and timed operations. A cycle restores the collection
// to its initial logical contents; setup and verification are untimed.
// ------------------------------------------------------------
static volatile BYTE observed = 0;
enum Measurement { MEASURE_OK, MEASURE_CLOCK, MEASURE_FAIL };

static void element(BYTE* bytes, NUINT size, BYTE key) {
    for (NUINT i = 0; i < size; ++i) bytes[i] = (BYTE) (key ^ (BYTE) i);
}

template<typename API>
struct CollectionTrial {
    typename API::Data data;
    NUINT n, width, index;
    BYTE key[8], value[8], alternate[8], lastKey[8], lastValue[8];
    BOOL flipped;
    BOOL (*operation)(CollectionTrial*);
    BOOL populate;
    BOOL mapped;
};

template<typename API>
static BOOL populate(CollectionTrial<API>* t) {
    BYTE key[8], value[8];
    for (NUINT i = 0; i < t->n; ++i) {
        element(key, t->width, (BYTE) i);
        element(value, t->width, (BYTE) ((BYTE) i ^ (BYTE) 0xA5));
        if (!API::add(t->data, key, value)) return FALSE;
    }
    return TRUE;
}

template<typename API>
static BOOL verify_collection(CollectionTrial<API>& t) {
    NUINT n = t.populate ? t.n : 0;
    if (API::count(t.data) != n || API::capacity(t.data) != (NUINT) (t.n + 1)) return FALSE;
    BYTE key[8], value[8];
    for (NUINT i = 0; i < n; ++i) {
        NUINT expected = t.flipped && i == 0 ? (NUINT) (n - 1) :
            t.flipped && i == n - 1 ? (NUINT) 0 : i;
        element(key, t.width, (BYTE) expected);
        if (::memcmp(API::keys(t.data) + i * t.width, key, t.width) != 0) return FALSE;
        if (t.mapped) {
            element(value, t.width, (BYTE) ((BYTE) expected ^ (BYTE) 0xA5));
            if (::memcmp(API::values(t.data) + i * t.width, value, t.width) != 0) return FALSE;
        }
    }
    return TRUE;
}

template<typename API>
static BOOL op_lifecycle(CollectionTrial<API>* t) {
    typename API::Data c;
    API::init(c, t->width);
    BOOL ok = API::expand(c, (NUINT) (t->n + 1));
    BOOL released = API::release(c);
    return ok && released && API::capacity(c) == 0 && API::count(c) == 0 &&
        API::keys(c) == 0 && API::values(c) == 0 ? TRUE : FALSE;
}

template<typename API>
static BOOL op_empty_lifecycle(CollectionTrial<API>* t) {
    typename API::Data c;
    API::init(c, t->width);
    return API::release(c);
}

template<typename API>
static BOOL op_capacity(CollectionTrial<API>* t) {
    return API::expand(t->data, (NUINT) ((t->n + 1) * 2)) && API::shrink(t->data) ? TRUE : FALSE;
}

template<typename API>
static BOOL op_expand_noop(CollectionTrial<API>* t) { return API::expand(t->data, (NUINT) (t->n + 1)); }

template<typename API>
static BOOL op_clear(CollectionTrial<API>* t) {
    API::clear(t->data);
    return populate(t);
}

template<typename API>
static BOOL op_contains_hit(CollectionTrial<API>* t) { return API::contains(t->data, t->key); }
template<typename API>
static BOOL op_contains_miss(CollectionTrial<API>* t) { return !API::contains(t->data, t->key) ? TRUE : FALSE; }
template<typename API>
static BOOL op_add_duplicate(CollectionTrial<API>* t) { return !API::add(t->data, t->key, t->value) ? TRUE : FALSE; }
template<typename API>
static BOOL op_remove_missing(CollectionTrial<API>* t) { return !API::remove(t->data, t->key) ? TRUE : FALSE; }
template<typename API>
static BOOL op_add_remove(CollectionTrial<API>* t) {
    return API::add(t->data, t->key, t->value) && API::remove(t->data, t->key) ? TRUE : FALSE;
}
template<typename API>
static BOOL op_remove_front(CollectionTrial<API>* t) {
    // Removing the first element moves the last into its place. Re-adding
    // the removed element swaps those positions; the next cycle reverses it.
    const BYTE* key = t->flipped ? t->lastKey : t->key;
    const BYTE* value = t->flipped ? t->lastValue : t->value;
    if (!API::remove(t->data, key) || !API::add(t->data, key, value)) return FALSE;
    t->flipped = t->flipped ? FALSE : TRUE;
    return TRUE;
}
template<typename API>
static BOOL op_map_get(CollectionTrial<API>* t) {
    BYTE* p = API::get(t->data, t->key);
    if (p == 0) return FALSE;
    observed = p[0];
    return p[0] == t->value[0] ? TRUE : FALSE;
}
template<typename API>
static BOOL op_map_get_miss(CollectionTrial<API>* t) { return API::get(t->data, t->key) == 0 ? TRUE : FALSE; }
template<typename API>
static BOOL op_map_set(CollectionTrial<API>* t) {
    return API::set(t->data, t->key, t->alternate) && API::set(t->data, t->key, t->value) ? TRUE : FALSE;
}
template<typename API>
static BOOL op_map_set_miss(CollectionTrial<API>* t) { return !API::set(t->data, t->key, t->value) ? TRUE : FALSE; }
template<typename API>
static BOOL op_array_add(CollectionTrial<API>* t) {
    return API::add(t->data, t->key, t->value) && API::remove(t->data, t->n) ? TRUE : FALSE;
}
template<typename API>
static BOOL op_array_insert(CollectionTrial<API>* t) {
    return API::insert(t->data, t->index, t->key) && API::remove(t->data, t->index) ? TRUE : FALSE;
}
template<typename API>
static BOOL op_array_get(CollectionTrial<API>* t) {
    BYTE* p = API::get(t->data, t->index);
    if (p == 0) return FALSE;
    observed = p[0];
    return p[0] == t->key[0] ? TRUE : FALSE;
}
template<typename API>
static BOOL op_array_set(CollectionTrial<API>* t) {
    return API::set(t->data, t->index, t->alternate) && API::set(t->data, t->index, t->key) ? TRUE : FALSE;
}

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


template<typename API>
static Measurement measure_collection(CollectionTrial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    t.flipped = FALSE;
    API::init(t.data, t.width);
    BOOL ok = API::expand(t.data, (NUINT) (t.n + 1));
    if (ok && t.populate) ok = populate(&t);
    BOOL (*volatile operation)(CollectionTrial<API>*) = t.operation;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        ok = operation(&t);
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify) ok = verify_collection(t);
    if (!API::release(t.data)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

// ------------------------------------------------------------
// File: ownership transfer and callback-resolved borrowed views.
// ------------------------------------------------------------
struct NativeFile {
    NativeCollection metadata, data;
    BOOL (*resolver)(const NativeFile*, size_t, NativeCollection*);
};

static BOOL native_resolve(const NativeFile* f, size_t index, NativeCollection* out) {
    if (index >= f->metadata.count) return FALSE;
    size_t offset = f->metadata.keys[index];
    if (offset >= f->data.count || f->data.count - offset < 2) return FALSE;
    native_init(*out, 1, 0);
    out->keys = f->data.keys + offset;
    out->count = out->capacity = 2;
    return TRUE;
}
static void native_file_init(NativeFile& f, NativeCollection& metadata, NativeCollection& data) {
    f.metadata = metadata; f.data = data; f.resolver = native_resolve;
    native_init(metadata, metadata.size, 0);
    native_init(data, data.size, 0);
}
static BOOL native_file_free(NativeFile& f) {
    native_free(f.metadata); native_free(f.data); f.resolver = 0; return TRUE;
}
static CATALYST_RESULT catalyst_resolve(const CATRLLIB_FILE* f, NUINT index, CATRLLIB_ARRAY* out) {
    if (index >= f->metadata.length) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0xA1, 1, 1);
    NUINT offset = ((const BYTE*) f->metadata.data)[index];
    if (offset >= f->data.length || f->data.length - offset < (NUINT) 2)
        return RESULT(STATUS_CODE_ERROR_MALFORMED_INPUT, 0xA1, 2, 0);
    out->data = (BYTE*) f->data.data + offset;
    out->size = 1; out->length = 2; out->capacity = 2;
    return RESULT(STATUS_CODE_SUCCESS, 0xA1, 3, 0);
}

enum FileKind { FILE_LIFECYCLE, FILE_HIT, FILE_MISS };
struct NativeFileAPI {
    typedef NativeCollection Array;
    typedef NativeFile Data;
    static void array_init(Array& a) { native_init(a, 1, 0); }
    static BOOL expand(Array& a, NUINT n) { return native_expand(a, n); }
    static void add(Array& a, BYTE v) { native_insert(a, a.count, &v); }
    static BOOL release_array(Array& a) { return native_free(a); }
    static void init(Data& f, Array& m, Array& d) { native_file_init(f, m, d); }
    static BOOL release(Data& f) { return native_file_free(f); }
    static BOOL get(Data& f, NUINT i, Array& out) { return f.resolver(&f, i, &out); }
    static const BYTE* bytes(const Array& a) { return a.keys; }
    static NUINT count(const Array& a) { return (NUINT) a.count; }
};
struct CatalystFileAPI {
    typedef CATRLLIB_ARRAY Array;
    typedef CATRLLIB_FILE Data;
    static void array_init(Array& a) { catrl_array_init(&a, 1); }
    static BOOL expand(Array& a, NUINT n) { return succeeded(catrl_array_expand(&a, n)); }
    static void add(Array& a, BYTE v) { catrl_array_add(&a, &v); }
    static BOOL release_array(Array& a) { return succeeded(catrl_array_free(&a)); }
    static void init(Data& f, Array& m, Array& d) { catrl_file_init(&f, &m, &d, catalyst_resolve); }
    static BOOL release(Data& f) { return succeeded(catrl_file_free(&f)); }
    static BOOL get(Data& f, NUINT i, Array& out) { return succeeded(catrl_file_get(&f, i, &out)); }
    static const BYTE* bytes(const Array& a) { return (const BYTE*) a.data; }
    static NUINT count(const Array& a) { return a.length; }
};

template<typename API>
static BOOL file_sources(typename API::Array& m, typename API::Array& d, NUINT n) {
    API::array_init(m); API::array_init(d);
    if (!API::expand(m, n) || !API::expand(d, (NUINT) (n + 1))) return FALSE;
    for (NUINT i = 0; i < n; ++i) API::add(m, (BYTE) i);
    for (NUINT i = 0; i <= n; ++i) API::add(d, (BYTE) (i ^ (NUINT) 0xA5));
    return TRUE;
}

template<typename API>
struct FileTrial { FileKind kind; NUINT n; };

template<typename API>
static BOOL file_cycle(FileTrial<API>* t) {
    typename API::Array m, d;
    BOOL ok = file_sources<API>(m, d, t->n);
    if (ok) {
        typename API::Data f;
        API::init(f, m, d);
        ok = API::bytes(m) == 0 && API::bytes(d) == 0;
        if (!API::release(f)) ok = FALSE;
    }
    if (!API::release_array(m)) ok = FALSE;
    if (!API::release_array(d)) ok = FALSE;
    return ok;
}

template<typename API>
static Measurement measure_file(FileTrial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    BOOL ok = TRUE;
    typename API::Array m, d, out;
    typename API::Data f;
    API::array_init(m); API::array_init(d); API::array_init(out);
    BOOL active = FALSE;
    if (t.kind != FILE_LIFECYCLE) {
        ok = file_sources<API>(m, d, t.n);
        if (ok) { API::init(f, m, d); active = TRUE; }
    }
    BOOL (*volatile cycle)(FileTrial<API>*) = file_cycle<API>;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        if (t.kind == FILE_LIFECYCLE) ok = cycle(&t);
        else if (t.kind == FILE_MISS) ok = !API::get(f, t.n, out) ? TRUE : FALSE;
        else {
            ok = API::get(f, (NUINT) (t.n - 1), out);
            if (ok && API::bytes(out) != 0) observed = API::bytes(out)[0];
            else ok = FALSE;
        }
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify && t.kind == FILE_HIT)
        ok = API::count(out) == 2 && API::bytes(out)[0] == (BYTE) ((t.n - 1) ^ 0xA5) &&
            API::bytes(out)[1] == (BYTE) (t.n ^ 0xA5) ? TRUE : FALSE;
    if (ok && verify && t.kind == FILE_MISS) ok = API::bytes(out) == 0 && API::count(out) == 0 ? TRUE : FALSE;
    if (active && !API::release(f)) ok = FALSE;
    if (!API::release_array(m)) ok = FALSE;
    if (!API::release_array(d)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

// ------------------------------------------------------------
// Stream: C heap ring and spare buffer with equivalent retention.
// ------------------------------------------------------------
struct NativeChunk { unsigned char* data; size_t capacity; };
struct NativeStream {
    size_t lifetime, cursor;
    BYTE counter;
    NativeChunk spare;
    NativeChunk* slots;
    BOOL (*produce)(NativeStream*, unsigned char*, size_t);
};
static BOOL native_produce(NativeStream* s, unsigned char* chunk, size_t size) {
    ++s->counter;
    ::memset(chunk, s->counter, size);
    return TRUE;
}
static BOOL native_stream_init(NativeStream** out, size_t lifetime) {
    *out = (NativeStream*) ::calloc(1, sizeof(NativeStream) + lifetime * sizeof(NativeChunk));
    if (*out == 0) return FALSE;
    NativeStream* s = *out;
    s->lifetime = lifetime; s->cursor = 0; s->counter = 0;
    s->spare.data = 0; s->spare.capacity = 0;
    s->slots = (NativeChunk*) (s + 1);
    s->produce = native_produce;
    for (size_t i = 0; i < lifetime; ++i) { s->slots[i].data = 0; s->slots[i].capacity = 0; }
    return TRUE;
}
static BOOL native_stream_next(NativeStream* s, size_t size, BYTE** out) {
    size_t required = size == 0 ? 1 : size;
    size_t capacity = s->spare.capacity == 0 ? 1 : s->spare.capacity;
    while (capacity < required) {
        if (capacity > (size_t) -1 / 2) { capacity = required; break; }
        capacity *= 2;
    }
    if (capacity != s->spare.capacity) {
        if (!native_buffer(&s->spare.data, s->spare.capacity, capacity)) return FALSE;
        s->spare.capacity = capacity;
    }
    if (!s->produce(s, s->spare.data, size)) return FALSE;
    NativeChunk expired = s->slots[s->cursor];
    s->slots[s->cursor] = s->spare;
    *out = s->spare.data;
    s->spare = expired;
    if (++s->cursor == s->lifetime) s->cursor = 0;
    if (required <= s->spare.capacity / 4) {
        size_t smaller = s->spare.capacity / 2;
        if (!native_buffer(&s->spare.data, s->spare.capacity, smaller)) return FALSE;
        s->spare.capacity = smaller;
    }
    return TRUE;
}
static BOOL native_stream_free(NativeStream** out) {
    NativeStream* s = *out;
    if (s != 0) {
        for (size_t i = 0; i < s->lifetime; ++i) ::free(s->slots[i].data);
        ::free(s->spare.data); ::free(s); *out = 0;
    }
    return TRUE;
}
static CATALYST_RESULT produce(CATRLLIB_STREAM* stream, CATMMLIB_POINTERW chunk, NUINT size) {
    BYTE* counter = (BYTE*) stream->state;
    ++*counter;
    catmm_fill(chunk, *counter, size);
    return RESULT(STATUS_CODE_SUCCESS, 0xA2, 1, 0);
}
struct NativeStreamAPI {
    typedef NativeStream* Data;
    static BOOL init(Data& s, NUINT lifetime) { return native_stream_init(&s, lifetime); }
    static BOOL next(Data& s, NUINT size, BYTE** out) { return native_stream_next(s, size, out); }
    static BOOL release(Data& s) { return native_stream_free(&s); }
};
struct CatalystStreamAPI {
    typedef CATRLLIB_STREAM Data;
    static BOOL init(Data& s, NUINT lifetime) { return succeeded(catrl_stream_init(&s, 1, lifetime, produce)); }
    static BOOL next(Data& s, NUINT size, BYTE** out) {
        CATMMLIB_POINTERW p = 0;
        CATALYST_RESULT r = catrl_stream_next(&s, size, &p);
        *out = (BYTE*) p;
        return succeeded(r);
    }
    static BOOL release(Data& s) { return succeeded(catrl_stream_free(&s)); }
};
enum StreamKind { STREAM_LIFECYCLE, STREAM_STEADY, STREAM_VARIABLE };
template<typename API>
struct StreamTrial { StreamKind kind; NUINT size, lifetime; };

template<typename API>
static BOOL stream_cycle(StreamTrial<API>* t) {
    typename API::Data s;
    BOOL ok = API::init(s, t->lifetime);
    if (ok) ok = API::release(s);
    return ok;
}

template<typename API>
static Measurement measure_stream(StreamTrial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    typename API::Data s;
    BOOL active = FALSE, ok = TRUE;
    BYTE* retained[3] = { 0, 0, 0 };
    BYTE expected[3] = { 0, 0, 0 };
    NUINT lengths[3] = { 0, 0, 0 };
    NUINT cursor = 0;
    BYTE counter = 0;
    if (t.kind != STREAM_LIFECYCLE) {
        ok = API::init(s, t.lifetime);
        active = ok;
        // Prime all retained slots and the spare before measuring reuse.
        for (NUINT i = 0; ok && i <= t.lifetime; ++i) {
            BYTE* p = 0;
            ok = API::next(s, t.size, &p);
            ++counter;
        }
    }
    BOOL (*volatile cycle)(StreamTrial<API>*) = stream_cycle<API>;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        if (t.kind == STREAM_LIFECYCLE) ok = cycle(&t);
        else {
            NUINT size = t.kind == STREAM_VARIABLE && (i & 1) ? (NUINT) (t.size / 4) : t.size;
            BYTE* p = 0;
            ok = API::next(s, size, &p);
            ++counter;
            if (ok && p != 0) {
                observed = p[0];
                if (size != 0 && (p[0] != counter || p[size - 1] != counter)) ok = FALSE;
                retained[cursor] = p; expected[cursor] = counter; lengths[cursor] = size;
                if (++cursor == t.lifetime) cursor = 0;
            } else ok = FALSE;
        }
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify && active) {
        for (NUINT i = 0; i < t.lifetime; ++i) {
            if (retained[i] == 0) continue;
            for (NUINT j = 0; j < lengths[i]; ++j)
                if (retained[i][j] != expected[i]) ok = FALSE;
            for (NUINT j = 0; j < i; ++j)
                if (retained[i] == retained[j]) ok = FALSE;
        }
    }
    if (active && !API::release(s)) ok = FALSE;
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


// ------------------------------------------------------------
// Workloads and output.
// ------------------------------------------------------------
static const NUINT counts[] = {
    4,
#if TARGET_SIZEOF_ADDRESS >= 2
    32, 128,
#else
    8,
#endif
};

static const NUINT widths[] = {
    1,
#if TARGET_SIZEOF_ADDRESS >= 2
    8,
#endif
};

enum Position { FIRST, MIDDLE, LAST, ABSENT };

template<typename API>
static void collection_fixture(CollectionTrial<API>& t, NUINT n, NUINT width, Position position,
                               BOOL mapped, BOOL populated, BOOL (*operation)(CollectionTrial<API>*)) {
    t.n = n; t.width = width; t.mapped = mapped; t.populate = populated; t.operation = operation;
    t.index = position == FIRST ? 0 : position == MIDDLE ? (NUINT) (n / 2) :
        position == LAST ? (NUINT) (n - 1) : n;
    element(t.key, width, (BYTE) t.index);
    element(t.value, width, (BYTE) ((BYTE) t.index ^ (BYTE) 0xA5));
    element(t.alternate, width, (BYTE) ((BYTE) t.index ^ (BYTE) 0xFF));
    element(t.lastKey, width, (BYTE) (n - 1));
    element(t.lastValue, width, (BYTE) ((BYTE) (n - 1) ^ (BYTE) 0xA5));
}

template<typename Native, typename Catalyst>
static void collection_cases(const char* heading, Position position, BOOL mapped, BOOL populated,
                             BOOL (*nOp)(CollectionTrial<Native>*),
                             BOOL (*cOp)(CollectionTrial<Catalyst>*), Summary& summary) {
    std::printf("\n%s\n", heading);
    for (BYTE w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
        for (BYTE i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i) {
            if (nOp == op_empty_lifecycle<Native> && (w != 0 || i != 0)) continue;
            NUINT n = counts[i], width = widths[w];
            CollectionTrial<Native> native;
            CollectionTrial<Catalyst> comparison;
            collection_fixture(native, n, width, position, mapped, populated, nOp);
            collection_fixture(comparison, n, width, position, mapped, populated, cOp);
            char label[40];
            std::sprintf(label, "%lu elements x %lu B", (unsigned long) (populated ? n : 0), (unsigned long) width);
            if (nOp == op_lifecycle<Native>) std::sprintf(label, "capacity %lu x %lu B", (unsigned long) (n + 1), (unsigned long) width);
            benchmark_case(label, native, comparison, measure_collection<Native>, measure_collection<Catalyst>, 67108864UL, summary);
        }
    }
}

static void benchmark_set(Summary& summary) {
    std::printf("\nSet (contiguous C reference)\n");
    collection_cases<NativeSet, CatalystSet>("catrl_set_init + free (empty)", FIRST, FALSE, FALSE, op_empty_lifecycle<NativeSet>, op_empty_lifecycle<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_init + expand + free", FIRST, FALSE, FALSE, op_lifecycle<NativeSet>, op_lifecycle<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_expand + shrink (preserve contents)", FIRST, FALSE, TRUE, op_capacity<NativeSet>, op_capacity<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_expand (no-op)", FIRST, FALSE, TRUE, op_expand_noop<NativeSet>, op_expand_noop<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_clear + refill", FIRST, FALSE, TRUE, op_clear<NativeSet>, op_clear<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_contains (first)", FIRST, FALSE, TRUE, op_contains_hit<NativeSet>, op_contains_hit<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_contains (last)", LAST, FALSE, TRUE, op_contains_hit<NativeSet>, op_contains_hit<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_contains (absent)", ABSENT, FALSE, TRUE, op_contains_miss<NativeSet>, op_contains_miss<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_add + remove (new key)", ABSENT, FALSE, TRUE, op_add_remove<NativeSet>, op_add_remove<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_remove + add (first)", FIRST, FALSE, TRUE, op_remove_front<NativeSet>, op_remove_front<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_add (duplicate)", LAST, FALSE, TRUE, op_add_duplicate<NativeSet>, op_add_duplicate<CatalystSet>, summary);
    collection_cases<NativeSet, CatalystSet>("catrl_set_remove (absent)", ABSENT, FALSE, TRUE, op_remove_missing<NativeSet>, op_remove_missing<CatalystSet>, summary);
}

static void benchmark_map(Summary& summary) {
    std::printf("\nMap (contiguous C reference)\n");
    collection_cases<NativeMap, CatalystMap>("catrl_map_init + free (empty)", FIRST, TRUE, FALSE, op_empty_lifecycle<NativeMap>, op_empty_lifecycle<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_init + expand + free", FIRST, TRUE, FALSE, op_lifecycle<NativeMap>, op_lifecycle<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_expand + shrink (preserve contents)", FIRST, TRUE, TRUE, op_capacity<NativeMap>, op_capacity<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_expand (no-op)", FIRST, TRUE, TRUE, op_expand_noop<NativeMap>, op_expand_noop<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_clear + refill", FIRST, TRUE, TRUE, op_clear<NativeMap>, op_clear<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_contains (first)", FIRST, TRUE, TRUE, op_contains_hit<NativeMap>, op_contains_hit<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_contains (last)", LAST, TRUE, TRUE, op_contains_hit<NativeMap>, op_contains_hit<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_contains (absent)", ABSENT, TRUE, TRUE, op_contains_miss<NativeMap>, op_contains_miss<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_add + remove (new key)", ABSENT, TRUE, TRUE, op_add_remove<NativeMap>, op_add_remove<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_remove + add (first)", FIRST, TRUE, TRUE, op_remove_front<NativeMap>, op_remove_front<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_add (duplicate)", LAST, TRUE, TRUE, op_add_duplicate<NativeMap>, op_add_duplicate<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_remove (absent)", ABSENT, TRUE, TRUE, op_remove_missing<NativeMap>, op_remove_missing<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_get (first)", FIRST, TRUE, TRUE, op_map_get<NativeMap>, op_map_get<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_get (last)", LAST, TRUE, TRUE, op_map_get<NativeMap>, op_map_get<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_get (absent)", ABSENT, TRUE, TRUE, op_map_get_miss<NativeMap>, op_map_get_miss<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_set + restore (last)", LAST, TRUE, TRUE, op_map_set<NativeMap>, op_map_set<CatalystMap>, summary);
    collection_cases<NativeMap, CatalystMap>("catrl_map_set (absent)", ABSENT, TRUE, TRUE, op_map_set_miss<NativeMap>, op_map_set_miss<CatalystMap>, summary);
}

static void benchmark_array(Summary& summary) {
    std::printf("\nArray (contiguous C reference)\n");
    collection_cases<NativeArray, CatalystArray>("catrl_array_init + free (empty)", FIRST, FALSE, FALSE, op_empty_lifecycle<NativeArray>, op_empty_lifecycle<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_init + expand + free", FIRST, FALSE, FALSE, op_lifecycle<NativeArray>, op_lifecycle<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_expand + shrink (preserve contents)", FIRST, FALSE, TRUE, op_capacity<NativeArray>, op_capacity<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_expand (no-op)", FIRST, FALSE, TRUE, op_expand_noop<NativeArray>, op_expand_noop<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_clear + refill", FIRST, FALSE, TRUE, op_clear<NativeArray>, op_clear<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_contains (first)", FIRST, FALSE, TRUE, op_contains_hit<NativeArray>, op_contains_hit<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_contains (last)", LAST, FALSE, TRUE, op_contains_hit<NativeArray>, op_contains_hit<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_contains (absent)", ABSENT, FALSE, TRUE, op_contains_miss<NativeArray>, op_contains_miss<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_add + remove (append)", ABSENT, FALSE, TRUE, op_array_add<NativeArray>, op_array_add<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_insert + remove (front)", FIRST, FALSE, TRUE, op_array_insert<NativeArray>, op_array_insert<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_insert + remove (middle)", MIDDLE, FALSE, TRUE, op_array_insert<NativeArray>, op_array_insert<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_get (middle)", MIDDLE, FALSE, TRUE, op_array_get<NativeArray>, op_array_get<CatalystArray>, summary);
    collection_cases<NativeArray, CatalystArray>("catrl_array_set + restore (middle)", MIDDLE, FALSE, TRUE, op_array_set<NativeArray>, op_array_set<CatalystArray>, summary);
}

static void benchmark_files(Summary& summary) {
    std::printf("\nFile (C reference with borrowed-view resolver)\n");
    const char* labels[] = {
        "catrl_file_init + free (including source array creation)",
        "catrl_file_get (valid index)",
        "catrl_file_get (invalid index)"
    };
    for (BYTE kind = 0; kind < 3; ++kind) {
        std::printf("\n%s\n", labels[kind]);
        for (BYTE i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i) {
            NUINT n = counts[i];
            char label[40];
            std::sprintf(label, "%lu metadata entries", (unsigned long) n);
            FileTrial<NativeFileAPI> native = { (FileKind) kind, n };
            FileTrial<CatalystFileAPI> comparison = { (FileKind) kind, n };
            benchmark_case(label, native, comparison, measure_file<NativeFileAPI>, measure_file<CatalystFileAPI>, 1048576UL, summary);
        }
    }
}

static void benchmark_streams(Summary& summary) {
    std::printf("\nStream (C reference with retained chunks and spare buffer)\n");
    const char* labels[] = {
        "catrl_stream_init + free",
        "catrl_stream_next (warm, constant chunk size)",
        "catrl_stream_next (alternating chunk sizes)"
    };
    const NUINT chunkSizes[] = {
        8, 16,
#if TARGET_SIZEOF_ADDRESS >= 2
        128, 1024,
#endif
    };
    for (BYTE kind = 0; kind < 3; ++kind) {
        std::printf("\n%s\n", labels[kind]);
        for (NUINT lifetime = 1; lifetime <= 3; lifetime = (NUINT) (lifetime + 2)) {
            for (BYTE i = 0; i < sizeof(chunkSizes) / sizeof(chunkSizes[0]); ++i) {
                if (kind == STREAM_LIFECYCLE && i != 0) continue;
                NUINT size = chunkSizes[i];
                char label[48];
                if (kind == STREAM_LIFECYCLE)
                    std::sprintf(label, "retain %lu chunks", (unsigned long) lifetime);
                else if (kind == STREAM_VARIABLE)
                    std::sprintf(label, "%lu/%lu B; retain %lu", (unsigned long) size, (unsigned long) (size / 4), (unsigned long) lifetime);
                else
                    std::sprintf(label, "%lu B; retain %lu", (unsigned long) size, (unsigned long) lifetime);
                StreamTrial<NativeStreamAPI> native = { (StreamKind) kind, size, lifetime };
                StreamTrial<CatalystStreamAPI> comparison = { (StreamKind) kind, size, lifetime };
                benchmark_case(label, native, comparison, measure_stream<NativeStreamAPI>, measure_stream<CatalystStreamAPI>, 1048576UL, summary);
            }
        }
    }
}
#endif

int main() {
#if defined(CLOCKS_PER_SEC)
    if (CLOCKS_PER_SEC <= 0 || std::clock() == (std::clock_t) -1) {
        std::printf("CATRLLIB performance unavailable: no usable standard C clock.\n");
        return 2;
    }
    std::printf("CATRLLIB Performance (Release recommended)\n");
    std::printf("Native-C is the benchmark. Lower time is better.\n");
    std::printf("Difference: +10%% means Catalyst takes 10%% longer; -10%% means 10%% less time.\n");
    std::printf("Time per operation or named cycle: median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("Native references use contiguous arrays, linear search, C heap allocation and matching stream retention.\n");
    std::printf("Collection counts/widths vary; map keys and values have the same width.\n");
    std::printf("Named cycles include every listed operation; fixtures and final verification are untimed.\n");
    std::printf("Stream measurements include production and identical endpoint checks; file get returns a borrowed view.\n");
    std::printf("Both paths consume outcomes; Native-C has no diagnostic metadata.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-28s %14s %14s %12s  %s\n", "Workload", "Native-C", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
    benchmark_set(summary);
    benchmark_map(summary);
    benchmark_array(summary);
    benchmark_files(summary);
    benchmark_streams(summary);
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
    std::printf("\nAll CATRLLIB performance comparisons verified.\n");
    return summary.unavailable != 0 ? 2 : 0;
#else
    std::printf("CATRLLIB performance unavailable: standard C clock not provided.\n");
    return 2;
#endif
}
