#include "catalyst.hpp"
#include "catrllib.h"
#include "catmmlib.h"

using namespace catalyst;

/* Address-sized arithmetic is performed on explicitly ordered bytes, least
 * significant first. NUINT is materialized only at ABI/address boundaries.
 */
struct catrl_size {
    BYTE bytes[TARGET_SIZEOF_VOID_P];
};

static catrl_size size_byte(BYTE value) {
    catrl_size size = {{0}};
    size.bytes[0] = value;
    return size;
}

#if TARGET_SIZEOF_VOID_P != 1
/* The ABI's unsigned object representation is inspected, not assumed.
 * These constant probes identify each native value bit by its significance.
 * The normal byte-permutation path also supports mixed-endian ABIs.
 */
#if TARGET_SIZEOF_VOID_P == 2
static const NUINT size_layout[] = {0xAAAAu, 0xCCCCu, 0xF0F0u, 0xFF00u, 0, 0};
#elif TARGET_SIZEOF_VOID_P == 4
static const NUINT size_layout[] = {0xAAAAAAAAu, 0xCCCCCCCCu, 0xF0F0F0F0u, 0xFF00FF00u, 0xFFFF0000u, 0};
#elif TARGET_SIZEOF_VOID_P == 8
static const NUINT size_layout[] = {
    UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xCCCCCCCCCCCCCCCC),
    UINT64_C(0xF0F0F0F0F0F0F0F0), UINT64_C(0xFF00FF00FF00FF00),
    UINT64_C(0xFFFF0000FFFF0000), UINT64_C(0xFFFFFFFF00000000)
};
#endif

static BYTE layout_byte(BYTE probe, BYTE offset) {
    return (BYTE) ((const unsigned char*) &size_layout[probe])[offset];
}

static BOOL native_byte_index(BYTE offset, BYTE* index) {
    if (layout_byte(0, offset) != 0xAA || layout_byte(1, offset) != 0xCC || layout_byte(2, offset) != 0xF0) return FALSE;
    BYTE digit = 0;
    BYTE mask = 1;
    for (BYTE i = 3; i < 6; ++i) {
        BYTE value = layout_byte(i, offset);
        if (value == 0xFF) digit = (BYTE) (digit | mask);
        else if (value != 0) return FALSE;
        mask = (BYTE) (mask << 1);
    }
    *index = digit;
    return TRUE;
}

static BYTE native_bit_index(BYTE offset, BYTE mask) {
    BYTE position = 0;
    BYTE flag = 1;
    for (BYTE i = 0; i < 6; ++i) {
        if ((layout_byte(i, offset) & mask) != 0) position = (BYTE) (position | flag);
        flag = (BYTE) (flag << 1);
    }
    return position;
}
#endif

/* The foundational ABI defines NUINT to be exactly pointer-width. */
typedef char catrl_size_requires_exact_nuint[(sizeof(NUINT) == TARGET_SIZEOF_VOID_P) ? 1 : -1];

static catrl_size size_from_abi(NUINT value) {
#if TARGET_SIZEOF_VOID_P == 1
    return size_byte(value);
#else
    catrl_size size = {{0}};
    const unsigned char* native = (const unsigned char*) &value;
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        BYTE digit;
        if (native_byte_index(i, &digit)) {
            size.bytes[digit] = (BYTE) native[i];
        } else {
            for (BYTE mask = 1; mask != 0; mask = (BYTE) ((mask & 0x7F) << 1)) {
                if ((native[i] & mask) == 0) continue;
                BYTE position = native_bit_index(i, mask);
                BYTE index = (BYTE) (position >> 3);
                BYTE bit = (BYTE) (1u << (position & 0x07));
                size.bytes[index] = (BYTE) (size.bytes[index] | bit);
            }
        }
    }
    return size;
#endif
}

static NUINT size_to_abi(const catrl_size* size) {
#if TARGET_SIZEOF_VOID_P == 1
    return size->bytes[0];
#else
    NUINT value = 0;
    unsigned char* native = (unsigned char*) &value;
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        BYTE digit;
        if (native_byte_index(i, &digit)) {
            native[i] = size->bytes[digit];
        } else {
            BYTE octet = 0;
            for (BYTE mask = 1; mask != 0; mask = (BYTE) ((mask & 0x7F) << 1)) {
                BYTE position = native_bit_index(i, mask);
                BYTE index = (BYTE) (position >> 3);
                BYTE bit = (BYTE) (1u << (position & 0x07));
                if ((size->bytes[index] & bit) != 0) octet = (BYTE) (octet | mask);
            }
            native[i] = octet;
        }
    }
    return value;
#endif
}

static BOOL size_zero(const catrl_size* size) {
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        if (size->bytes[i] != 0) return FALSE;
    }
    return TRUE;
}

static SBYTE size_compare(const catrl_size* left, const catrl_size* right) {
    BYTE i = TARGET_SIZEOF_VOID_P;
    while (i != 0) {
        --i;
        if (left->bytes[i] < right->bytes[i]) return (SBYTE) -1;
        if (left->bytes[i] > right->bytes[i]) return (SBYTE) 1;
    }
    return 0;
}

/* Add/subtract a digit without an intermediate exceeding one byte.
 * Output may alias either operand. FALSE is the ordinary arithmetic carry/
 * borrow flag; callers decide whether it represents an operation failure.
 */
static BOOL size_add(const catrl_size* left, const catrl_size* right, catrl_size* output) {
    BOOL carry = FALSE;
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        BYTE a = left->bytes[i];
        BYTE b = right->bytes[i];
        BYTE room = (BYTE) (0xFF - a);
        BOOL next = b > room ? TRUE : FALSE;
        BYTE digit = next ? (BYTE) (b - room - 1) : (BYTE) (a + b);
        if (carry) {
            if (digit == 0xFF) {
                digit = 0;
                next = TRUE;
            } else {
                ++digit;
            }
        }
        output->bytes[i] = digit;
        carry = next;
    }
    return carry ? FALSE : TRUE;
}

static BOOL size_subtract(const catrl_size* left, const catrl_size* right, catrl_size* output) {
    BOOL borrow = FALSE;
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        BYTE a = left->bytes[i];
        BYTE b = right->bytes[i];
        BOOL next = a < b ? TRUE : FALSE;
        BYTE digit = next ? (BYTE) (0xFF - (b - a - 1)) : (BYTE) (a - b);
        if (borrow) {
            if (digit == 0) {
                digit = 0xFF;
                next = TRUE;
            } else {
                --digit;
            }
        }
        output->bytes[i] = digit;
        borrow = next;
    }
    return borrow ? FALSE : TRUE;
}

static void size_half(catrl_size* size) {
    BYTE carry = 0;
    BYTE i = TARGET_SIZEOF_VOID_P;
    while (i != 0) {
        --i;
        BYTE digit = size->bytes[i];
        size->bytes[i] = (BYTE) ((digit >> 1) | carry);
        carry = (digit & 1) != 0 ? (BYTE) 0x80 : (BYTE) 0;
    }
}

static BOOL size_multiply(const catrl_size* left, const catrl_size* right, catrl_size* output) {
    // Iterate the smaller factor to keep common small extents inexpensive.
    BOOL leftSmaller = size_compare(left, right) < 0 ? TRUE : FALSE;
    catrl_size term = leftSmaller ? *right : *left;
    catrl_size multiplier = leftSmaller ? *left : *right;
    *output = size_byte(0);
    if (size_zero(&term) || size_zero(&multiplier)) return TRUE;
    while (!size_zero(&multiplier)) {
        if ((multiplier.bytes[0] & 1) != 0 && !size_add(output, &term, output)) return FALSE;
        size_half(&multiplier);
        if (!size_zero(&multiplier) && !size_add(&term, &term, &term)) return FALSE;
    }
    return TRUE;
}

/* Divide the largest representable extent by a nonzero stride. The carry
 * out of doubling the remainder is retained separately from its digits.
 */
static catrl_size size_limit(const catrl_size* stride) {
    catrl_size quotient = size_byte(0);
    catrl_size remainder = size_byte(0);
    BYTE i = TARGET_SIZEOF_VOID_P;
    while (i != 0) {
        --i;
        for (BYTE bit = 0x80; bit != 0; bit = (BYTE) (bit >> 1)) {
            BOOL carry = size_add(&remainder, &remainder, &remainder) ? FALSE : TRUE;
            remainder.bytes[0] = (BYTE) (remainder.bytes[0] | 1);
            if (carry || size_compare(&remainder, stride) >= 0) {
                // With carry, subtraction modulo the extent restores the exact remainder.
                size_subtract(&remainder, stride, &remainder);
                quotient.bytes[i] = (BYTE) (quotient.bytes[i] | bit);
            }
        }
    }
    return quotient;
}

static void size_increment(catrl_size* size) {
    // Callers establish that the count is below the representable capacity.
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        if (size->bytes[i] != 0xFF) {
            ++size->bytes[i];
            return;
        }
        size->bytes[i] = 0;
    }
}

static catrl_size size_previous(const catrl_size* size) {
    catrl_size previous = *size;
    // Callers establish that the count is nonzero.
    for (BYTE i = 0; i < TARGET_SIZEOF_VOID_P; ++i) {
        if (previous.bytes[i] != 0) {
            --previous.bytes[i];
            return previous;
        }
        previous.bytes[i] = 0xFF;
    }
    return previous;
}

/* Private storage shared by the four collection implementations. */
struct catrl_storage {
    BYTE* bytes;
    catrl_size stride;
    catrl_size count;
    catrl_size capacity;
};

struct CATALYST_ARRAY { catrl_storage storage; };
struct CATALYST_SET { catrl_storage storage; };
struct CATALYST_MAP {
    catrl_storage storage;
    catrl_size key_size;
};
struct CATALYST_STACK { catrl_storage storage; };

/* Fault details are scoped to the public method's invariant-check opcode.
 * 1: zero stride, 2: count exceeds capacity, 3: storage/capacity mismatch,
 * 4: unrepresentable extent, 5: invalid map key extent, 6: array count differs
 * from capacity. The checks cannot validate arbitrary foreign pointers.
 */
/* Classify storage invariants. The enclosing operation owns reporting. */
static DETAIL_CODE storage_fault(const catrl_storage* storage) {
    if (size_zero(&storage->stride)) return 1;
    if (size_compare(&storage->count, &storage->capacity) > 0) return 2;
    if ((storage->bytes == 0) != (size_zero(&storage->capacity) != 0)) return 3;
    catrl_size extent;
    if (!size_multiply(&storage->capacity, &storage->stride, &extent)) return 4;
    return 0;
}

static DETAIL_CODE array_fault(const CATALYST_ARRAY* array) {
    DETAIL_CODE fault = storage_fault(&array->storage);
    if (fault != 0) return fault;
    if (size_compare(&array->storage.count, &array->storage.capacity) != 0) return 6;
    return 0;
}

static DETAIL_CODE map_fault(const CATALYST_MAP* map) {
    DETAIL_CODE fault = storage_fault(&map->storage);
    if (fault != 0) return fault;
    if (size_zero(&map->key_size) || size_compare(&map->key_size, &map->storage.stride) >= 0) return 5;
    return 0;
}

/* Callers validate the storage extent and ensure index < capacity. This
 * guarantees non-null storage and a representable element offset.
 */
static BYTE* element_at(const catrl_storage* storage, const catrl_size* index) {
    catrl_size offset;
    size_multiply(index, &storage->stride, &offset);
    return storage->bytes + size_to_abi(&offset);
}

/* A validated map entry has a nonzero key and value extent. */
static BYTE* value_at(const CATALYST_MAP* map, BYTE* entry) {
    return entry + size_to_abi(&map->key_size);
}

static NUINT map_value_size(const CATALYST_MAP* map) {
    catrl_size size;
    size_subtract(&map->storage.stride, &map->key_size, &size);
    return size_to_abi(&size);
}

// Context IDs follow CATRLLIB public declaration order (0x01 through 0x1D).
// Reporting private helpers use 0x1E through 0x20; 0x21 is retired.
// A caller reports its own operation and uses the helper opcode as failure detail.
static CATALYST_RESULT create_storage(NUINT objectSize, const catrl_size* stride, const catrl_size* count, void** output) {
    *output = 0;
    void* object = 0;
    void* bytes = 0;
    RESULT status;
    catrl_size extent;
    if (!size_multiply(count, stride, &extent)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x1E, 0x01, 0);
    }
    status = catmm_alloc(&object, objectSize);
    if (!status_code_is_success(status.status)) {
        return RESULT(status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status, 0x1E, 0x02, status.operation);
    }
    if (!size_zero(count)) {
        status = catmm_alloc(&bytes, size_to_abi(&extent));
        if (!status_code_is_success(status.status)) {
            RESULT cleanup;
            cleanup = catmm_free(object);
            STATUS_CODE failure = status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status;
            if (!status_code_is_success(cleanup.status) && !status_code_is_fatal(failure)) {
                return RESULT(cleanup.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : cleanup.status, 0x1E, 0x03, cleanup.operation);
            }
            return RESULT(failure, 0x1E, 0x04, status.operation);
        }
    }
    catrl_storage* storage = (catrl_storage*) object;
    storage->bytes = (BYTE*) bytes;
    storage->stride = *stride;
    storage->count = *count;
    storage->capacity = *count;
    *output = object;
    return RESULT(STATUS_CODE_SUCCESS, 0x1E, 0x05, 0);
}

static CATALYST_RESULT destroy_storage(void* object, catrl_storage* storage) {
    RESULT bytesStatus;
    RESULT objectStatus;
    if (storage->bytes != 0) {
        bytesStatus = catmm_free(storage->bytes);
    }
    objectStatus = catmm_free(object);
    STATUS_CODE bytesFailure = bytesStatus.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : bytesStatus.status;
    STATUS_CODE objectFailure = objectStatus.status == STATUS_CODE_ERROR_INVALID_ARGUMENT ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : objectStatus.status;
    if (!status_code_is_success(bytesFailure) && (status_code_is_fatal(bytesFailure) || !status_code_is_fatal(objectFailure))) {
        return RESULT(bytesFailure, 0x1F, 0x01, bytesStatus.operation);
    } else if (!status_code_is_success(objectFailure)) {
        return RESULT(objectFailure, 0x1F, 0x02, objectStatus.operation);
    } else {
        return RESULT(STATUS_CODE_SUCCESS, 0x1F, 0x03, 0);
    }
}

/* Reserve room for one more element, with bounded geometric growth. */
static CATALYST_RESULT grow(catrl_storage* storage) {
    catrl_size limit = size_limit(&storage->stride);
    if (size_compare(&storage->count, &limit) == 0) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x20, 0x01, 0);
    }
    catrl_size capacity;
    BOOL capacityValid = TRUE;
    catrl_size half = limit;
    size_half(&half);
    if (size_zero(&storage->capacity)) {
        capacity = size_byte(1);
    } else if (size_compare(&storage->capacity, &half) > 0) {
        capacity = limit;
    } else {
        capacityValid = size_add(&storage->capacity, &storage->capacity, &capacity);
    }
    catrl_size extent;
    if (!capacityValid || !size_multiply(&capacity, &storage->stride, &extent)) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x20, 0x04, 0);
    }
    void* bytes = storage->bytes;
    RESULT status;
    status = catmm_realloc(&bytes, size_to_abi(&extent));
    if (!status_code_is_success(status.status)) {
        // A failed old-block release can invalidate the collection's storage.
        return RESULT(status.status == STATUS_CODE_ERROR_INVALID_ARGUMENT || (status.status == STATUS_CODE_ERROR_INVALID_STATE && status.operation == 0x06) ? STATUS_CODE_FATAL_INVARIANT_VIOLATION : status.status, 0x20, 0x02, status.operation);
    }
    storage->bytes = (BYTE*) bytes;
    storage->capacity = capacity;
    return RESULT(STATUS_CODE_SUCCESS, 0x20, 0x03, 0);
}

/* Search validated storage with a non-null value and a nonzero size no
 * greater than the stride. Return the matching index, or count if absent.
 * Callers establish storage invariants and validate the search argument.
 */
static inline catrl_size find(const catrl_storage* storage, const void* value, const catrl_size* size) {
    catrl_size i = size_byte(0);
    const BYTE* element = storage->bytes;
    NUINT stride = size_to_abi(&storage->stride);
    NUINT length = size_to_abi(size);
    while (size_compare(&i, &storage->count) < 0) {
        // Equality is an ordinary value; no Result is needed for each comparison.
        const BYTE* first = element;
        const BYTE* second = (const BYTE*) value;
        const BYTE* end = first + length;
        if (first != second) {
            while (first != end && *first == *second) {
                ++first;
                ++second;
            }
        }
        if (element == value || first == end) {
            return i;
        }
        size_increment(&i);
        if (size_compare(&i, &storage->count) < 0) element += stride;
    }
    return storage->count;
}

/* ARRAY */
CATALYST_RESULT catrl_array_create(CATALYST_ARRAY** array, CATALYST_NUINT element_size, CATALYST_NUINT length) {
    if (array == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x01, 0);
    }
    *array = 0;
    catrl_size stride = size_from_abi(element_size);
    if (size_zero(&stride)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x01, 0x02, 1);
    }
    RESULT storageResult;
    catrl_size count = size_from_abi(length);
    void* object;
    storageResult = create_storage(sizeof(CATALYST_ARRAY), &stride, &count, &object);
    *array = (CATALYST_ARRAY*) object;
    return RESULT(storageResult.status, 0x01, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_array_destroy(CATALYST_ARRAY* array) {
    if (array == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x02, 0x01, 0);
    }
    DETAIL_CODE fault = array_fault(array);
    RESULT storageResult;
    storageResult = destroy_storage(array, &array->storage);
    if (fault != 0 && !status_code_is_fatal(storageResult.status)) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x02, 0x03, fault);
    }
    return RESULT(storageResult.status, 0x02, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_array_get(const CATALYST_ARRAY* array, CATALYST_NUINT index, void* value) {
    if (array == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x01, 0);
    }
    DETAIL_CODE fault = array_fault(array);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x03, 0x06, fault);
    }
    catrl_size position = size_from_abi(index);
    if (size_compare(&position, &array->storage.count) >= 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x02, 1);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x03, 0x03, 2);
    }
    catmm_copy(value, element_at(&array->storage, &position), size_to_abi(&array->storage.stride));
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x05, 0);
}

CATALYST_RESULT catrl_array_set(CATALYST_ARRAY* array, CATALYST_NUINT index, const void* value) {
    if (array == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x01, 0);
    }
    DETAIL_CODE fault = array_fault(array);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x04, 0x06, fault);
    }
    catrl_size position = size_from_abi(index);
    if (size_compare(&position, &array->storage.count) >= 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x02, 1);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x04, 0x03, 2);
    }
    catmm_copy(element_at(&array->storage, &position), value, size_to_abi(&array->storage.stride));
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x05, 0);
}

CATALYST_RESULT catrl_array_length(const CATALYST_ARRAY* array, CATALYST_NUINT* length) {
    if (array == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x01, 0);
    }
    DETAIL_CODE fault = array_fault(array);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x05, 0x04, fault);
    }
    if (length == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x05, 0x02, 1);
    }
    *length = size_to_abi(&array->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x03, 0);
}

/* SET */
CATALYST_RESULT catrl_set_create(CATALYST_SET** set, CATALYST_NUINT element_size) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x01, 0);
    }
    *set = 0;
    catrl_size stride = size_from_abi(element_size);
    if (size_zero(&stride)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x06, 0x02, 1);
    }
    RESULT storageResult;
    catrl_size count = size_byte(0);
    void* object;
    storageResult = create_storage(sizeof(CATALYST_SET), &stride, &count, &object);
    *set = (CATALYST_SET*) object;
    return RESULT(storageResult.status, 0x06, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_set_destroy(CATALYST_SET* set) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x07, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    RESULT storageResult;
    storageResult = destroy_storage(set, &set->storage);
    if (fault != 0 && !status_code_is_fatal(storageResult.status)) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x07, 0x03, fault);
    }
    return RESULT(storageResult.status, 0x07, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_set_add(CATALYST_SET* set, const void* value) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x08, 0x08, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x08, 0x02, 1);
    }
    catrl_size index = find(&set->storage, value, &set->storage.stride);
    if (size_compare(&index, &set->storage.count) != 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x08, 0x04, 0);
    }
    if (size_compare(&set->storage.count, &set->storage.capacity) == 0) {
        RESULT growthResult;
        growthResult = grow(&set->storage);
        if (!status_code_is_success(growthResult.status)) {
            return RESULT(growthResult.status, 0x08, 0x05, growthResult.operation);
        }
    }
    catmm_copy(element_at(&set->storage, &set->storage.count), value, size_to_abi(&set->storage.stride));
    size_increment(&set->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x07, 0);
}

CATALYST_RESULT catrl_set_remove(CATALYST_SET* set, const void* value) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x09, 0x07, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x09, 0x02, 1);
    }
    catrl_size index = find(&set->storage, value, &set->storage.stride);
    if (size_compare(&index, &set->storage.count) == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x09, 0x04, 0);
    }
    catrl_size last = size_previous(&set->storage.count);
    if (size_compare(&index, &last) != 0) {
        catmm_copy(element_at(&set->storage, &index), element_at(&set->storage, &last), size_to_abi(&set->storage.stride));
    }
    set->storage.count = last;
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x06, 0);
}

CATALYST_RESULT catrl_set_contains(const CATALYST_SET* set, const void* value, CATALYST_BOOL* contains) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x0A, 0x06, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x02, 1);
    }
    if (contains == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x03, 2);
    }
    catrl_size index = find(&set->storage, value, &set->storage.stride);
    *contains = size_compare(&index, &set->storage.count) != 0 ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x05, 0);
}

CATALYST_RESULT catrl_set_count(const CATALYST_SET* set, CATALYST_NUINT* count) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x0B, 0x04, fault);
    }
    if (count == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0B, 0x02, 1);
    }
    *count = size_to_abi(&set->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x03, 0);
}

CATALYST_RESULT catrl_set_at(const CATALYST_SET* set, CATALYST_NUINT index, void* value) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x0C, 0x06, fault);
    }
    catrl_size position = size_from_abi(index);
    if (size_compare(&position, &set->storage.count) >= 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x02, 1);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x03, 2);
    }
    catmm_copy(value, element_at(&set->storage, &position), size_to_abi(&set->storage.stride));
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x05, 0);
}

CATALYST_RESULT catrl_set_clear(CATALYST_SET* set) {
    if (set == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0D, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&set->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x0D, 0x04, fault);
    }
    if (size_zero(&set->storage.count)) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0D, 0x02, 0);
    }
    set->storage.count = size_byte(0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x03, 0);
}

/* MAP */
CATALYST_RESULT catrl_map_create(CATALYST_MAP** map, CATALYST_NUINT key_size, CATALYST_NUINT value_size) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x01, 0);
    }
    *map = 0;
    catrl_size key = size_from_abi(key_size);
    catrl_size value = size_from_abi(value_size);
    if (size_zero(&key)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x02, 1);
    }
    if (size_zero(&value)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0E, 0x03, 2);
    }
    catrl_size stride;
    if (!size_add(&key, &value, &stride)) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0E, 0x04, 0);
    }
    RESULT storageResult;
    catrl_size count = size_byte(0);
    void* object;
    storageResult = create_storage(sizeof(CATALYST_MAP), &stride, &count, &object);
    *map = (CATALYST_MAP*) object;
    if (*map != 0) (*map)->key_size = key;
    return RESULT(storageResult.status, 0x0E, 0x05, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_map_destroy(CATALYST_MAP* map) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0F, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    RESULT storageResult;
    storageResult = destroy_storage(map, &map->storage);
    if (fault != 0 && !status_code_is_fatal(storageResult.status)) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x0F, 0x03, fault);
    }
    return RESULT(storageResult.status, 0x0F, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_map_get(const CATALYST_MAP* map, const void* key, void* value) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x10, 0x08, fault);
    }
    if (key == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x02, 1);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x03, 2);
    }
    catrl_size index = find(&map->storage, key, &map->key_size);
    if (size_compare(&index, &map->storage.count) == 0) {
        return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x10, 0x05, 0);
    }
    catmm_copy(value, value_at(map, element_at(&map->storage, &index)), map_value_size(map));
    return RESULT(STATUS_CODE_SUCCESS, 0x10, 0x07, 0);
}

CATALYST_RESULT catrl_map_set(CATALYST_MAP* map, const void* key, const void* value) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x11, 0x09, fault);
    }
    if (key == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x02, 1);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x11, 0x03, 2);
    }
    BYTE* entry;
    catrl_size index = find(&map->storage, key, &map->key_size);
    BOOL insert = size_compare(&index, &map->storage.count) == 0 ? TRUE : FALSE;
    if (insert) {
        if (size_compare(&map->storage.count, &map->storage.capacity) == 0) {
            RESULT growthResult;
            growthResult = grow(&map->storage);
            if (!status_code_is_success(growthResult.status)) {
                return RESULT(growthResult.status, 0x11, 0x05, growthResult.operation);
            }
        }
        entry = element_at(&map->storage, &index);
        catmm_copy(entry, key, size_to_abi(&map->key_size));
    } else {
        entry = element_at(&map->storage, &index);
    }
    catmm_copy(value_at(map, entry), value, map_value_size(map));
    if (insert) size_increment(&map->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x11, 0x08, 0);
}

CATALYST_RESULT catrl_map_remove(CATALYST_MAP* map, const void* key) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x12, 0x07, fault);
    }
    if (key == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x12, 0x02, 1);
    }
    catrl_size index = find(&map->storage, key, &map->key_size);
    if (size_compare(&index, &map->storage.count) == 0) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x12, 0x04, 0);
    }
    catrl_size last = size_previous(&map->storage.count);
    if (size_compare(&index, &last) != 0) {
        catmm_copy(element_at(&map->storage, &index), element_at(&map->storage, &last), size_to_abi(&map->storage.stride));
    }
    map->storage.count = last;
    return RESULT(STATUS_CODE_SUCCESS, 0x12, 0x06, 0);
}

CATALYST_RESULT catrl_map_contains(const CATALYST_MAP* map, const void* key, CATALYST_BOOL* contains) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x13, 0x06, fault);
    }
    if (key == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x02, 1);
    }
    if (contains == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x13, 0x03, 2);
    }
    catrl_size index = find(&map->storage, key, &map->key_size);
    *contains = size_compare(&index, &map->storage.count) != 0 ? TRUE : FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0x13, 0x05, 0);
}

CATALYST_RESULT catrl_map_count(const CATALYST_MAP* map, CATALYST_NUINT* count) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x14, 0x04, fault);
    }
    if (count == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x14, 0x02, 1);
    }
    *count = size_to_abi(&map->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x14, 0x03, 0);
}

CATALYST_RESULT catrl_map_at(const CATALYST_MAP* map, CATALYST_NUINT index, void* key, void* value) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x15, 0x08, fault);
    }
    catrl_size position = size_from_abi(index);
    if (size_compare(&position, &map->storage.count) >= 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x02, 1);
    }
    if (key == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x03, 2);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x15, 0x04, 3);
    }
    BYTE* entry = element_at(&map->storage, &position);
    BYTE* entryValue = value_at(map, entry);
    catmm_copy(key, entry, size_to_abi(&map->key_size));
    catmm_copy(value, entryValue, map_value_size(map));
    return RESULT(STATUS_CODE_SUCCESS, 0x15, 0x07, 0);
}

CATALYST_RESULT catrl_map_clear(CATALYST_MAP* map) {
    if (map == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x16, 0x01, 0);
    }
    DETAIL_CODE fault = map_fault(map);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x16, 0x04, fault);
    }
    if (size_zero(&map->storage.count)) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x16, 0x02, 0);
    }
    map->storage.count = size_byte(0);
    return RESULT(STATUS_CODE_SUCCESS, 0x16, 0x03, 0);
}

/* STACK */
CATALYST_RESULT catrl_stack_create(CATALYST_STACK** stack, CATALYST_NUINT element_size) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x01, 0);
    }
    *stack = 0;
    catrl_size stride = size_from_abi(element_size);
    if (size_zero(&stride)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x17, 0x02, 1);
    }
    RESULT storageResult;
    catrl_size count = size_byte(0);
    void* object;
    storageResult = create_storage(sizeof(CATALYST_STACK), &stride, &count, &object);
    *stack = (CATALYST_STACK*) object;
    return RESULT(storageResult.status, 0x17, 0x03, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_stack_destroy(CATALYST_STACK* stack) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x18, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    RESULT storageResult;
    storageResult = destroy_storage(stack, &stack->storage);
    if (fault != 0 && !status_code_is_fatal(storageResult.status)) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x18, 0x03, fault);
    }
    return RESULT(storageResult.status, 0x18, 0x02, status_code_is_success(storageResult.status) ? 0 : storageResult.operation);
}

CATALYST_RESULT catrl_stack_push(CATALYST_STACK* stack, const void* value) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x19, 0x06, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x19, 0x02, 1);
    }
    if (size_compare(&stack->storage.count, &stack->storage.capacity) == 0) {
        RESULT growthResult;
        growthResult = grow(&stack->storage);
        if (!status_code_is_success(growthResult.status)) {
            return RESULT(growthResult.status, 0x19, 0x03, growthResult.operation);
        }
    }
    catmm_copy(element_at(&stack->storage, &stack->storage.count), value, size_to_abi(&stack->storage.stride));
    size_increment(&stack->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x19, 0x05, 0);
}

CATALYST_RESULT catrl_stack_pop(CATALYST_STACK* stack, void* value) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1A, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x1A, 0x06, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1A, 0x02, 1);
    }
    if (size_zero(&stack->storage.count)) {
        return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x1A, 0x03, 0);
    }
    catrl_size last = size_previous(&stack->storage.count);
    catmm_copy(value, element_at(&stack->storage, &last), size_to_abi(&stack->storage.stride));
    stack->storage.count = last;
    return RESULT(STATUS_CODE_SUCCESS, 0x1A, 0x05, 0);
}

CATALYST_RESULT catrl_stack_peek(const CATALYST_STACK* stack, void* value) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1B, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x1B, 0x06, fault);
    }
    if (value == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1B, 0x02, 1);
    }
    if (size_zero(&stack->storage.count)) {
        return RESULT(STATUS_CODE_ERROR_NOT_FOUND, 0x1B, 0x03, 0);
    }
    catrl_size last = size_previous(&stack->storage.count);
    catmm_copy(value, element_at(&stack->storage, &last), size_to_abi(&stack->storage.stride));
    return RESULT(STATUS_CODE_SUCCESS, 0x1B, 0x05, 0);
}

CATALYST_RESULT catrl_stack_count(const CATALYST_STACK* stack, CATALYST_NUINT* count) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1C, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x1C, 0x04, fault);
    }
    if (count == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1C, 0x02, 1);
    }
    *count = size_to_abi(&stack->storage.count);
    return RESULT(STATUS_CODE_SUCCESS, 0x1C, 0x03, 0);
}

CATALYST_RESULT catrl_stack_clear(CATALYST_STACK* stack) {
    if (stack == 0) {
        return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x1D, 0x01, 0);
    }
    DETAIL_CODE fault = storage_fault(&stack->storage);
    if (fault != 0) {
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x1D, 0x04, fault);
    }
    if (size_zero(&stack->storage.count)) {
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x1D, 0x02, 0);
    }
    stack->storage.count = size_byte(0);
    return RESULT(STATUS_CODE_SUCCESS, 0x1D, 0x03, 0);
}

