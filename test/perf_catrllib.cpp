#include <cstdio>
#include <ctime>
#include <climits>
#include <stdlib.h>
#include <string.h>
#include "catalyst.hpp"
#include "catrllib.hpp"

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
    if (c.count == 0) return 0;
    if (c.size == 1) {
        const unsigned char* match = (const unsigned char*) ::memchr(c.keys, key[0], c.count);
        return match == 0 ? c.count : (size_t) (match - c.keys);
    }
    const unsigned char* current = c.keys;
    for (size_t i = 0; i < c.count; ++i, current += c.size)
        if (current[0] == key[0] && ::memcmp(current + 1, key + 1, c.size - 1) == 0) return i;
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
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        if (::memcmp(keys(c) + i * width, key, width) != 0) return FALSE;
        (void) value; return TRUE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
};

struct CatalystSet {
    typedef SET Data;
    static void init(Data& c, NUINT size) { catalyst::set_init(&c, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catalyst::set_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catalyst::set_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catalyst::set_free(&c)); }
    static NUINT count(const Data& c) { return c.cardinality; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.data; }
    static const BYTE* values(const Data& c) { (void) c; return 0; }
    static void clear(Data& c) { catalyst::set_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catalyst::set_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { (void) v; return catalyst::set_add(&c, k); }
    static BOOL remove(Data& c, const BYTE* k) { return catalyst::set_remove(&c, k); }
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        if (::memcmp(keys(c) + i * width, key, width) != 0) return FALSE;
        (void) value; return TRUE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
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
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        if (::memcmp(keys(c) + i * width, key, width) != 0) return FALSE;
        return ::memcmp(values(c) + i * width, value, width) == 0 ? TRUE : FALSE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
};

struct CatalystMap {
    typedef MAP Data;
    static void init(Data& c, NUINT size) { catalyst::map_init(&c, size, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catalyst::map_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catalyst::map_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catalyst::map_free(&c)); }
    static NUINT count(const Data& c) { return c.cardinality; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.keys; }
    static const BYTE* values(const Data& c) { return (const BYTE*) c.values; }
    static void clear(Data& c) { catalyst::map_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catalyst::map_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE* v) { return catalyst::map_add(&c, k, v); }
    static BOOL remove(Data& c, const BYTE* k) { return catalyst::map_remove(&c, k); }
    static BYTE* get(Data& c, const BYTE* k) { return (BYTE*) catalyst::map_get(&c, k); }
    static BOOL set(Data& c, const BYTE* k, const BYTE* v) { return catalyst::map_set(&c, k, v); }
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        if (::memcmp(keys(c) + i * width, key, width) != 0) return FALSE;
        return ::memcmp(values(c) + i * width, value, width) == 0 ? TRUE : FALSE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
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
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        (void) value;
        return ::memcmp(keys(c) + i * width, key, width) == 0 ? TRUE : FALSE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
};

struct CatalystArray {
    typedef ARRAY Data;
    static void init(Data& c, NUINT size) { catalyst::array_init(&c, size); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(catalyst::array_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(catalyst::array_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(catalyst::array_free(&c)); }
    static NUINT count(const Data& c) { return c.length; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* keys(const Data& c) { return (const BYTE*) c.data; }
    static const BYTE* values(const Data& c) { (void) c; return 0; }
    static void clear(Data& c) { catalyst::array_clear(&c); }
    static BOOL contains(Data& c, const BYTE* k) { return catalyst::array_contains(&c, k); }
    static BOOL add(Data& c, const BYTE* k, const BYTE*) { catalyst::array_add(&c, k); return TRUE; }
    static BOOL remove(Data& c, NUINT i) { catalyst::array_remove(&c, i); return TRUE; }
    static BOOL insert(Data& c, NUINT i, const BYTE* k) { catalyst::array_insert(&c, i, k); return TRUE; }
    static BYTE* get(Data& c, NUINT i) { return (BYTE*) catalyst::array_get(&c, i); }
    static BOOL set(Data& c, NUINT i, const BYTE* k) { catalyst::array_set(&c, i, k); return TRUE; }
    static BOOL matches_item(Data& c, NUINT i, const BYTE* key, const BYTE* value, NUINT width) {
        if (::memcmp(keys(c) + i * width, key, width) != 0) return FALSE;
        (void) value; return TRUE;
    }
    static BOOL released(const Data& c) { return keys(c) == 0 && values(c) == 0 ? TRUE : FALSE; }
};

// ------------------------------------------------------------
// Fixtures and timed operations. A cycle restores the collection
// to its initial logical contents; setup and verification are untimed.
// ------------------------------------------------------------
static volatile BYTE observed = 0;
static const BYTE* volatile observedStorage = 0;

// Escape the allocation and access both endpoints before release. The
// pointer store and volatile reads cannot be optimized away with the cycle.
static BOOL observe_storage(const BYTE* data, NUINT capacity, NUINT width) {
    if (data == 0) return FALSE;
    observedStorage = data;
    const volatile BYTE* bytes = data;
    observed = bytes[0];
    observed = bytes[(NUINT) (capacity * width) - (NUINT) 1];
    observedStorage = 0;
    return TRUE;
}
enum Measurement { MEASURE_OK, MEASURE_CLOCK, MEASURE_FAIL };

static void element(BYTE* bytes, NUINT size, BYTE key) {
    // Sixteen-byte fixtures differ only in the last byte; the shorter
    // fixtures retain early mismatches.
    for (NUINT i = 0; i < size; ++i)
        bytes[i] = size == 16 && i != (NUINT) (size - (NUINT) 1)
            ? (BYTE) i : (BYTE) (key ^ (BYTE) i);
}

template<typename API>
struct CollectionTrial {
    typename API::Data data;
    NUINT n, width, index;
    BYTE key[16], value[16], alternate[16], lastKey[16], lastValue[16];
    BOOL flipped;
    BOOL (*operation)(CollectionTrial*);
    BOOL populate;
    BOOL mapped;
};

template<typename API>
static BOOL populate(CollectionTrial<API>* t) {
    BYTE key[16], value[16];
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
    BYTE key[16], value[16];
    for (NUINT i = 0; i < n; ++i) {
        NUINT expected = t.flipped && i == 0 ? (NUINT) (n - 1) :
            t.flipped && i == n - 1 ? (NUINT) 0 : i;
        element(key, t.width, (BYTE) expected);
        element(value, t.width, (BYTE) ((BYTE) expected ^ (BYTE) 0xA5));
        if (!API::matches_item(t.data, i, key, value, t.width)) return FALSE;
    }
    return TRUE;
}

template<typename API>
static BOOL op_lifecycle(CollectionTrial<API>* t) {
    typename API::Data c;
    API::init(c, t->width);
    BOOL ok = API::expand(c, (NUINT) (t->n + 1));
    if (ok) {
        ok = observe_storage(API::keys(c), (NUINT) (t->n + 1), t->width);
        if (ok && t->mapped)
            ok = observe_storage(API::values(c), (NUINT) (t->n + 1), t->width);
    }
    BOOL released = API::release(c);
    return ok && released && API::capacity(c) == 0 && API::count(c) == 0 &&
        API::released(c) ? TRUE : FALSE;
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
    if (!API::set(t->data, t->key, t->alternate)) return FALSE;
    if (!observe_storage(API::values(t->data) + (NUINT) (t->index * t->width), 1, t->width)) return FALSE;
    return API::set(t->data, t->key, t->value);
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
    if (!API::set(t->data, t->index, t->alternate)) return FALSE;
    if (!observe_storage(API::keys(t->data) + (NUINT) (t->index * t->width), 1, t->width)) return FALSE;
    return API::set(t->data, t->index, t->key);
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
static CATALYST_RESULT catalyst_resolve(const catalyst::FILE* f, NUINT index, ARRAY* out) {
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
    typedef ARRAY Array;
    typedef catalyst::FILE Data;
    static void array_init(Array& a) { catalyst::array_init(&a, 1); }
    static BOOL expand(Array& a, NUINT n) { return succeeded(catalyst::array_expand(&a, n)); }
    static void add(Array& a, BYTE v) { catalyst::array_add(&a, &v); }
    static BOOL release_array(Array& a) { return succeeded(catalyst::array_free(&a)); }
    static void init(Data& f, Array& m, Array& d) { catalyst::file_init(&f, &m, &d, catalyst_resolve); }
    static BOOL release(Data& f) { return succeeded(catalyst::file_free(&f)); }
    static BOOL get(Data& f, NUINT i, Array& out) { return succeeded(catalyst::file_get(&f, i, &out)); }
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
static CATALYST_RESULT produce(STREAM* stream, POINTERW chunk, NUINT size) {
    BYTE* counter = (BYTE*) stream->state;
    ++*counter;
    catalyst::fill(chunk, *counter, size);
    return RESULT(STATUS_CODE_SUCCESS, 0xA2, 1, 0);
}
struct NativeStreamAPI {
    typedef NativeStream* Data;
    static BOOL init(Data& s, NUINT lifetime) { return native_stream_init(&s, lifetime); }
    static BOOL next(Data& s, NUINT size, BYTE** out) { return native_stream_next(s, size, out); }
    static BOOL release(Data& s) { return native_stream_free(&s); }
};
struct CatalystStreamAPI {
    typedef STREAM Data;
    static BOOL init(Data& s, NUINT lifetime) { return succeeded(catalyst::stream_init(&s, 1, lifetime, produce)); }
    static BOOL next(Data& s, NUINT size, BYTE** out) {
        POINTERW p = 0;
        CATALYST_RESULT r = catalyst::stream_next(&s, size, &p);
        *out = (BYTE*) p;
        return succeeded(r);
    }
    static BOOL release(Data& s) { return succeeded(catalyst::stream_free(&s)); }
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
    8, 16,
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
                             BOOL (*cOp)(CollectionTrial<Catalyst>*), Summary& summary, NUINT onlyWidth = 0) {
    if (nOp == op_empty_lifecycle<Native> && onlyWidth > 1) return;
    std::printf("\n%s\n", heading);
    for (BYTE w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
        if (onlyWidth != 0 && widths[w] != onlyWidth) continue;
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

/* Independent C reference for a contiguous stack or circular queue. */
struct NativeSequence {
    NativeCollection storage;
    size_t head;
};

static void native_rotate_bytes(unsigned char* data, size_t left, size_t right) {
    unsigned char scratch[32];
    while (left > sizeof(scratch) && right > sizeof(scratch)) {
        size_t count = left < right ? left : right;
        unsigned char* other = data + (left < right ? right : left);
        for (size_t offset = 0; offset < count;) {
            size_t size = count - offset;
            if (size > sizeof(scratch)) size = sizeof(scratch);
            ::memcpy(scratch, data + offset, size);
            ::memcpy(data + offset, other + offset, size);
            ::memcpy(other + offset, scratch, size);
            offset += size;
        }
        if (left == right) return;
        if (left < right) right -= left;
        else { data += right; left -= right; }
    }
    if (left <= right) {
        ::memcpy(scratch, data, left);
        ::memmove(data, data + left, right);
        ::memcpy(data + right, scratch, left);
    } else {
        ::memcpy(scratch, data + left, right);
        ::memmove(data + right, data, left);
        ::memcpy(data, scratch, right);
    }
}

template<BOOL Queue>
struct NativeSequenceAPI {
    typedef NativeSequence Data;
    static void init(Data& c, NUINT width) {
        native_init(c.storage, width, 0); c.head = 0;
    }
    static void normalize(Data& c, size_t oldCapacity, size_t extra) {
        NativeCollection& a = c.storage;
        if (c.head == 0) return;
        size_t offset = c.head * a.size, extent = oldCapacity * a.size;
        size_t live = a.count * a.size, tail = extent - offset;
        if (live <= tail) ::memmove(a.keys, a.keys + offset, live);
        else {
            size_t prefix = live - tail;
            if (prefix <= extra && (prefix <= tail || tail > extra)) {
                ::memcpy(a.keys + extent, a.keys, prefix);
                ::memmove(a.keys, a.keys + offset, tail);
                ::memcpy(a.keys + tail, a.keys + extent, prefix);
            } else if (tail <= extra) {
                ::memcpy(a.keys + extent, a.keys + offset, tail);
                ::memmove(a.keys + tail, a.keys, prefix);
                ::memcpy(a.keys, a.keys + extent, tail);
            } else {
                if (prefix != offset) ::memmove(a.keys + prefix, a.keys + offset, tail);
                native_rotate_bytes(a.keys, prefix, tail);
            }
        }
        c.head = 0;
    }
    static BOOL expand(Data& c, NUINT n) {
        NativeCollection& a = c.storage;
        if (n <= a.capacity) return TRUE;
        size_t old = a.capacity;
        if (!native_expand(a, n)) return FALSE;
        if (Queue) normalize(c, old, (n - old) * a.size);
        return TRUE;
    }
    static BOOL shrink(Data& c) {
        NativeCollection& a = c.storage;
        size_t target = a.capacity / 2;
        if (target < a.count) target = a.count;
        if (target == a.capacity) return TRUE;
        if (Queue) normalize(c, a.capacity, 0);
        return native_capacity(a, target);
    }
    static BOOL release(Data& c) { c.head = 0; return native_free(c.storage); }
    static NUINT count(const Data& c) { return (NUINT) c.storage.count; }
    static NUINT capacity(const Data& c) { return (NUINT) c.storage.capacity; }
    static const BYTE* data(const Data& c) { return c.storage.keys; }
    static void clear(Data& c) { c.storage.count = 0; c.head = 0; }
    static void put(Data& c, const BYTE* value) {
        NativeCollection& a = c.storage;
        size_t tail = a.count;
        if (Queue) {
            size_t remaining = a.capacity - c.head;
            tail = a.count < remaining ? c.head + a.count : a.count - remaining;
        }
        ::memcpy(a.keys + tail * a.size, value, a.size);
        ++a.count;
    }
    static const BYTE* peek(Data& c) {
        NativeCollection& a = c.storage;
        if (a.count == 0) return 0;
        return a.keys + (Queue ? c.head : a.count - 1) * a.size;
    }
    static BOOL take(Data& c, BYTE* output) {
        NativeCollection& a = c.storage;
        const BYTE* p = peek(c);
        if (p == 0) return FALSE;
        ::memcpy(output, p, a.size);
        --a.count;
        if (Queue && (++c.head == a.capacity || a.count == 0)) c.head = 0;
        return TRUE;
    }
};

struct CatalystQueueAPI {
    typedef QUEUE Data;
    static void init(Data& c, NUINT width) { queue_init(&c, width); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(queue_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(queue_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(queue_free(&c)); }
    static NUINT count(const Data& c) { return c.length; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* data(const Data& c) { return (const BYTE*) c.data; }
    static void clear(Data& c) { queue_clear(&c); }
    static void put(Data& c, const BYTE* v) { queue_enqueue(&c, v); }
    static BOOL take(Data& c, BYTE* v) { return queue_dequeue(&c, v); }
    static const BYTE* peek(Data& c) { return (const BYTE*) queue_peek(&c); }
};

struct CatalystStackAPI {
    typedef STACK Data;
    static void init(Data& c, NUINT width) { stack_init(&c, width); }
    static BOOL expand(Data& c, NUINT n) { return succeeded(stack_expand(&c, n)); }
    static BOOL shrink(Data& c) { return succeeded(stack_shrink(&c)); }
    static BOOL release(Data& c) { return succeeded(stack_free(&c)); }
    static NUINT count(const Data& c) { return c.length; }
    static NUINT capacity(const Data& c) { return c.capacity; }
    static const BYTE* data(const Data& c) { return (const BYTE*) c.data; }
    static void clear(Data& c) { stack_clear(&c); }
    static void put(Data& c, const BYTE* v) { stack_push(&c, v); }
    static BOOL take(Data& c, BYTE* v) { return stack_pop(&c, v); }
    static const BYTE* peek(Data& c) { return (const BYTE*) stack_peek(&c); }
};

enum SequenceKind { SEQ_LIFECYCLE, SEQ_PEEK, SEQ_EMPTY, SEQ_CYCLE, SEQ_RESIZE, SEQ_RESIZE_STRAIGHT, SEQ_NOOP, SEQ_CLEAR };
template<typename API>
struct SequenceTrial { SequenceKind kind; NUINT n, width; BOOL fifo; };

template<typename API>
static void sequence_fill(typename API::Data& c, NUINT n, NUINT width) {
    BYTE value[16];
    for (NUINT i = 0; i < n; ++i) {
        element(value, width, (BYTE) i);
        API::put(c, value);
    }
}

template<typename API>
static BOOL sequence_lifecycle(SequenceTrial<API>* t) {
    typename API::Data c;
    API::init(c, t->width);
    BOOL ok = API::expand(c, t->n);
    if (ok) ok = observe_storage(API::data(c), t->n, t->width);
    BOOL released = API::release(c);
    return ok && released && API::data(c) == 0 && API::count(c) == 0 &&
        API::capacity(c) == 0 ? TRUE : FALSE;
}

template<typename API>
static BOOL sequence_step(SequenceTrial<API>& t, typename API::Data& c, NUINT* head, BYTE* output) {
    BOOL ok = TRUE;
    switch (t.kind) {
    case SEQ_LIFECYCLE:
        ok = sequence_lifecycle<API>(&t);
        break;
    case SEQ_PEEK: {
        const BYTE* p = API::peek(c);
        ok = p != 0 ? TRUE : FALSE;
        if (ok) observed = p[t.width - 1];
        break;
    }
    case SEQ_EMPTY:
        output[0] = 0xA5;
        ok = !API::take(c, output) && output[0] == 0xA5 && API::peek(c) == 0 ? TRUE : FALSE;
        break;
    case SEQ_CYCLE:
    case SEQ_RESIZE:
        ok = API::take(c, output);
        if (ok) {
            observed = output[t.width - 1];
            API::put(c, output);
            if (t.fifo && ++*head == t.n) *head = 0;
            if (t.kind == SEQ_RESIZE)
                ok = API::expand(c, (NUINT) (t.n * (NUINT) 2)) && API::shrink(c) ? TRUE : FALSE;
        }
        break;
    case SEQ_RESIZE_STRAIGHT:
        ok = API::expand(c, (NUINT) (t.n * (NUINT) 2)) && API::shrink(c) ? TRUE : FALSE;
        break;
    case SEQ_NOOP:
        ok = API::expand(c, t.n) && API::shrink(c) ? TRUE : FALSE;
        break;
    case SEQ_CLEAR:
        API::clear(c);
        sequence_fill<API>(c, t.n, t.width);
        *head = 0;
        break;
    }
    return ok;
}

template<typename API>
static Measurement measure_sequence(SequenceTrial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    typename API::Data c;
    API::init(c, t.width);
    BOOL populated = t.kind != SEQ_EMPTY && t.kind != SEQ_LIFECYCLE ? TRUE : FALSE;
    BOOL ok = TRUE;
    if (populated) {
        ok = API::expand(c, t.n);
        if (ok) sequence_fill<API>(c, t.n, t.width);
    }
    NUINT head = 0;
    BYTE output[16] = {0};
    BYTE expected[16];
    BOOL (*volatile operation)(SequenceTrial<API>&, typename API::Data&, NUINT*, BYTE*) = sequence_step<API>;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        ok = operation(t, c, &head, output);
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify) {
        ok = API::count(c) == (populated ? t.n : 0) &&
            API::capacity(c) == (populated ? t.n : 0) ? TRUE : FALSE;
        if (populated) {
            for (NUINT i = 0; ok && i < t.n; ++i) {
                NUINT key = t.fifo ? head : (NUINT) (t.n - 1 - i);
                element(expected, t.width, (BYTE) key);
                ok = API::take(c, output) && ::memcmp(output, expected, t.width) == 0 ? TRUE : FALSE;
                if (t.fifo && ++head == t.n) head = 0;
            }
            if (ok) ok = API::peek(c) == 0 ? TRUE : FALSE;
        }
    }
    if (!API::release(c)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

/* Independent C pool with the same bit order, slot reuse and capacity policy. */
struct NativePool {
    unsigned char* data;
    unsigned char* bits;
    size_t size, count, capacity;
};

struct NativePoolAPI {
    typedef NativePool Data;
    static size_t bytes(size_t n) { return n / 8 + (n % 8 != 0); }
    static unsigned char mask(size_t i) { return (unsigned char) (1U << (i % 8)); }
    static void init(Data& p, NUINT width) {
        p.data = 0; p.bits = 0; p.size = width; p.count = 0; p.capacity = 0;
    }
    static BOOL expand(Data& p, NUINT n) {
        if (n <= p.capacity) return TRUE;
        if (bytes(n) != bytes(p.capacity) && !native_buffer(&p.bits, bytes(p.capacity), bytes(n))) return FALSE;
        if (!native_buffer(&p.data, p.capacity * p.size, (size_t) n * p.size)) return FALSE;
        p.capacity = n;
        return TRUE;
    }
    static BOOL contains(const Data& p, NUINT i) {
        return i < p.capacity && (p.bits[i / 8] & mask(i)) != 0 ? TRUE : FALSE;
    }
    static BOOL shrink(Data& p) {
        size_t target = p.capacity / 2;
        if (target < p.count) target = p.count;
        if (p.count != 0) {
            size_t end = p.capacity;
            while (end > target) {
                size_t last = end - 1, byte = last / 8;
                if (p.bits[byte] == 0) {
                    size_t first = byte * 8;
                    end = first < target ? target : first;
                } else if ((p.bits[byte] & mask(last)) != 0) { target = end; break; }
                else --end;
            }
        }
        if (target == p.capacity) return TRUE;
        size_t old = p.capacity;
        if (!native_buffer(&p.data, old * p.size, target * p.size)) return FALSE;
        p.capacity = target;
        if (bytes(old) != bytes(target) && !native_buffer(&p.bits, bytes(old), bytes(target))) return FALSE;
        return TRUE;
    }
    static BOOL release(Data& p) {
        ::free(p.data); ::free(p.bits); p.data = 0; p.bits = 0;
        p.count = 0; p.capacity = 0; return TRUE;
    }
    static BOOL acquire(Data& p, NUINT* index) {
        if (p.count == p.capacity) return FALSE;
        for (size_t i = 0; i < bytes(p.capacity); ++i) {
            if (p.bits[i] == 0xFF) continue;
            for (unsigned char bit = 0; bit < 8; ++bit) {
                unsigned char m = (unsigned char) (1U << bit);
                if ((p.bits[i] & m) != 0) continue;
                p.bits[i] |= m; ++p.count; *index = (NUINT) (i * 8 + bit); return TRUE;
            }
        }
        return FALSE;
    }
    static BOOL relinquish(Data& p, NUINT i) {
        if (!contains(p, i)) return FALSE;
        p.bits[i / 8] &= (unsigned char) ~mask(i); --p.count; return TRUE;
    }
    static BYTE* get(Data& p, NUINT i) { return contains(p, i) ? p.data + (size_t) i * p.size : 0; }
    static void clear(Data& p) {
        if (p.capacity != 0) ::memset(p.bits, 0, bytes(p.capacity));
        p.count = 0;
    }
    static NUINT count(const Data& p) { return (NUINT) p.count; }
    static NUINT capacity(const Data& p) { return (NUINT) p.capacity; }
    static const BYTE* data(const Data& p) { return p.data; }
    static const BYTE* bits(const Data& p) { return p.bits; }
};

struct CatalystPoolAPI {
    typedef POOL Data;
    static void init(Data& p, NUINT width) { pool_init(&p, width); }
    static BOOL expand(Data& p, NUINT n) { return succeeded(pool_expand(&p, n)); }
    static BOOL shrink(Data& p) { return succeeded(pool_shrink(&p)); }
    static BOOL release(Data& p) { return succeeded(pool_free(&p)); }
    static BOOL acquire(Data& p, NUINT* index) { return pool_acquire(&p, index); }
    static BOOL relinquish(Data& p, NUINT index) { return pool_release(&p, index); }
    static BOOL contains(const Data& p, NUINT index) { return pool_contains(&p, index); }
    static BYTE* get(Data& p, NUINT index) { return (BYTE*) pool_get(&p, index); }
    static void clear(Data& p) { pool_clear(&p); }
    static NUINT count(const Data& p) { return p.cardinality; }
    static NUINT capacity(const Data& p) { return p.capacity; }
    static const BYTE* data(const Data& p) { return (const BYTE*) p.data; }
    static const BYTE* bits(const Data& p) { return (const BYTE*) p.occupied; }
};

enum PoolKind { POOL_LIFECYCLE, POOL_CYCLE, POOL_CYCLE_FIRST, POOL_CYCLE_MIDDLE, POOL_HIT, POOL_MISS, POOL_FULL, POOL_RESIZE, POOL_NOOP, POOL_CLEAR };
template<typename API>
struct PoolTrial { PoolKind kind; NUINT n, width; };

template<typename API>
static BOOL pool_fill(typename API::Data& p, NUINT n, NUINT width) {
    for (NUINT i = 0; i < n; ++i) {
        NUINT index;
        if (!API::acquire(p, &index) || index != i) return FALSE;
        BYTE* slot = API::get(p, index);
        if (slot == 0) return FALSE;
        element(slot, width, (BYTE) i);
    }
    return TRUE;
}

template<typename API>
static BOOL pool_lifecycle(PoolTrial<API>* t) {
    typename API::Data p;
    API::init(p, t->width);
    BOOL ok = API::expand(p, t->n);
    if (ok) {
        NUINT bytes = (NUINT) (t->n / (NUINT) 8 + (t->n % (NUINT) 8 != 0 ? 1 : 0));
        ok = observe_storage(API::data(p), t->n, t->width) && observe_storage(API::bits(p), bytes, 1) ? TRUE : FALSE;
    }
    BOOL released = API::release(p);
    return ok && released && API::data(p) == 0 && API::bits(p) == 0 &&
        API::capacity(p) == 0 && API::count(p) == 0 ? TRUE : FALSE;
}

template<typename API>
static BOOL pool_step(PoolTrial<API>& t, typename API::Data& p, NUINT last, NUINT freeIndex) {
    BOOL ok = TRUE;
    NUINT index = CATALYST_NUINT_MAX;
    switch (t.kind) {
    case POOL_LIFECYCLE:
        ok = pool_lifecycle<API>(&t);
        break;
    case POOL_CYCLE:
    case POOL_CYCLE_FIRST:
    case POOL_CYCLE_MIDDLE:
        ok = API::acquire(p, &index) && index == freeIndex;
        if (ok) {
            observed = (BYTE) index;
            ok = API::relinquish(p, index);
        }
        break;
    case POOL_HIT: {
        const BYTE* slot = API::get(p, last);
        ok = API::contains(p, last) && slot != 0 ? TRUE : FALSE;
        if (ok) observed = slot[t.width - 1];
        break;
    }
    case POOL_MISS:
        ok = !API::contains(p, last) && API::get(p, last) == 0 && !API::relinquish(p, last) ? TRUE : FALSE;
        break;
    case POOL_FULL:
        ok = !API::acquire(p, &index) && index == CATALYST_NUINT_MAX ? TRUE : FALSE;
        break;
    case POOL_RESIZE:
        ok = API::expand(p, (NUINT) (t.n * (NUINT) 2)) && API::shrink(p) ? TRUE : FALSE;
        break;
    case POOL_NOOP:
        ok = API::expand(p, t.n) && API::shrink(p) ? TRUE : FALSE;
        break;
    case POOL_CLEAR:
        API::clear(p);
        ok = pool_fill<API>(p, t.n, t.width);
        break;
    }
    return ok;
}

template<typename API>
static Measurement measure_pool(PoolTrial<API>& t, unsigned long calls, unsigned long* ticks, BOOL verify) {
    typename API::Data p;
    API::init(p, t.width);
    BOOL active = t.kind != POOL_LIFECYCLE ? TRUE : FALSE;
    NUINT last = (NUINT) (t.n - 1);
    BOOL sparse = t.kind == POOL_RESIZE ? TRUE : FALSE;
    BOOL missing = t.kind == POOL_CYCLE || t.kind == POOL_CYCLE_FIRST || t.kind == POOL_CYCLE_MIDDLE || t.kind == POOL_MISS ? TRUE : FALSE;
    NUINT freeIndex = t.kind == POOL_CYCLE_FIRST ? (NUINT) 0 : t.kind == POOL_CYCLE_MIDDLE ? (NUINT) (t.n / (NUINT) 2) : last;
    BOOL ok = TRUE;
    if (active) {
        ok = API::expand(p, t.n) && pool_fill<API>(p, t.n, t.width) ? TRUE : FALSE;
        if (ok && sparse) {
            for (NUINT i = 0; ok && i < last; ++i) ok = API::relinquish(p, i);
        } else if (ok && missing) ok = API::relinquish(p, freeIndex);
    }
    BOOL (*volatile operation)(PoolTrial<API>&, typename API::Data&, NUINT, NUINT) = pool_step<API>;
    const std::clock_t start = std::clock();
    for (unsigned long i = 0; ok && i < calls; ++i) {
        ok = operation(t, p, last, freeIndex);
        observed = (BYTE) ok;
    }
    BOOL clockOk = elapsed(start, ticks);
    if (ok && verify) {
        NUINT count = !active ? 0 : sparse ? 1 : missing ? last : t.n;
        ok = API::count(p) == count && API::capacity(p) == (active ? t.n : 0) ? TRUE : FALSE;
        BYTE expected[16];
        for (NUINT i = 0; ok && active && i < t.n; ++i) {
            BOOL live = sparse ? (i == last ? TRUE : FALSE) : (missing && i == freeIndex ? FALSE : TRUE);
            const BYTE* slot = API::get(p, i);
            ok = API::contains(p, i) == live && (slot != 0) == (live != FALSE) ? TRUE : FALSE;
            if (ok && live) {
                element(expected, t.width, (BYTE) i);
                ok = ::memcmp(slot, expected, t.width) == 0 ? TRUE : FALSE;
            }
        }
    }
    if (!API::release(p)) ok = FALSE;
    if (!ok) return MEASURE_FAIL;
    return clockOk ? MEASURE_OK : MEASURE_CLOCK;
}

template<typename Native, typename Catalyst>
static void benchmark_sequence(const char* heading, BOOL fifo, Summary& summary) {
    const char* labels[] = {"init + expand + free", "peek", "empty removal + peek",
        "remove + insert", "remove + insert + expand + shrink", "expand + shrink (unwrapped)", "expand + shrink (no-op)", "clear + refill"};
    const NUINT sizes[] = {4,
#if TARGET_SIZEOF_ADDRESS >= 2
        32, 128, 1024
#if TARGET_SIZEOF_ADDRESS >= 4
        , 4096
#endif
#else
        8
#endif
    };
    std::printf("\n%s\n", heading);
    for (BYTE k = 0; k < (BYTE) 8; ++k) {
        std::printf("\n%s\n", labels[k]);
        for (BYTE w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
            for (BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
                if (k == SEQ_EMPTY && (i != 0 || w != 0)) continue;
                NUINT n = sizes[i], width = widths[w];
                if (n > (NUINT) (CATALYST_NUINT_MAX / width / (NUINT) 2)) continue;
                char label[48];
                std::sprintf(label, "%lu slots; %lu B", (unsigned long) n, (unsigned long) width);
                SequenceTrial<Native> native = { (SequenceKind) k, n, width, fifo };
                SequenceTrial<Catalyst> comparison = { (SequenceKind) k, n, width, fifo };
                benchmark_case(label, native, comparison, measure_sequence<Native>, measure_sequence<Catalyst>, 67108864UL, summary);
            }
        }
    }
}

static void benchmark_pools(Summary& summary) {
    const char* labels[] = {"init + expand + free", "acquire + release (last free)",
        "acquire + release (first free)", "acquire + release (middle free)", "contains + get (hit)", "contains + get + release (miss)", "acquire (full)",
        "expand + shrink (sparse high index)", "expand + shrink (no-op)", "clear + refill"};
    const NUINT sizes[] = {5,
#if TARGET_SIZEOF_ADDRESS >= 2
        33, 129, 1025
#if TARGET_SIZEOF_ADDRESS >= 4
        , 4097
#endif
#else
        9
#endif
    };
    std::printf("\nPool (C reference with byte-scanned occupancy bitmap)\n");
    for (BYTE k = 0; k < (BYTE) 10; ++k) {
        std::printf("\n%s\n", labels[k]);
        for (BYTE w = 0; w < sizeof(widths) / sizeof(widths[0]); ++w) {
            for (BYTE i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
                NUINT n = sizes[i], width = widths[w];
                if (n > (NUINT) (CATALYST_NUINT_MAX / width / (NUINT) 2)) continue;
                char label[48];
                std::sprintf(label, "%lu slots; %lu B", (unsigned long) n, (unsigned long) width);
                PoolTrial<NativePoolAPI> native = { (PoolKind) k, n, width };
                PoolTrial<CatalystPoolAPI> comparison = { (PoolKind) k, n, width };
                benchmark_case(label, native, comparison, measure_pool<NativePoolAPI>, measure_pool<CatalystPoolAPI>, 67108864UL, summary);
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
    std::printf("Independent contiguous C reference is the baseline. Lower time is better.\n");
    std::printf("Difference: +10%% means Catalyst takes 10%% longer; -10%% means 10%% less time.\n");
    std::printf("Time per operation or named cycle: median of 3 paired samples; %lu clock ticks/second.\n", (unsigned long) CLOCKS_PER_SEC);
    std::printf("All collection references use linear search, exact requested capacity and zeroed growth.\n");
    std::printf("Collection counts/widths vary; map keys and values have the same width.\n");
    std::printf("16-byte search fixtures share a prefix and differ at the last byte.\n");
    std::printf("Set/restore cycles observe the intermediate stored value before restoring it.\n");
    std::printf("Allocation lifecycle cycles escape storage and read its endpoints before release.\n");
    std::printf("Named cycles include every listed operation; fixtures and final verification are untimed.\n");
    std::printf("Stream measurements include production and identical endpoint checks; file get returns a borrowed view.\n");
    std::printf("Both paths consume outcomes; baseline has no diagnostic metadata.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Decimal places are estimates.\n");
    std::printf("\n  %-28s %14s %14s %12s  %s\n", "Workload", "Baseline", "Catalyst", "Difference", "Assessment");
    std::printf("  -------------------------------------------------------------------------------------\n");
    Summary summary = { 0, 0, 0, 0, 0, 0, 0 };
    benchmark_set(summary);
    benchmark_map(summary);
    benchmark_array(summary);
    benchmark_files(summary);
    benchmark_streams(summary);
    benchmark_sequence<NativeSequenceAPI<TRUE>, CatalystQueueAPI>("Queue (C circular-buffer reference)", TRUE, summary);
    benchmark_sequence<NativeSequenceAPI<FALSE>, CatalystStackAPI>("Stack (C contiguous reference)", FALSE, summary);
    benchmark_pools(summary);
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
