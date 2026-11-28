#ifndef CATRLLIB_INTERNAL_HPP
#define CATRLLIB_INTERNAL_HPP
#include "catmmlib.hpp"
#include "catrllib.h"

namespace catalyst {

/* Nonzero size, count * size readable data bytes when count is nonzero, and size readable element bytes. Return count for absence; extents were checked when capacity was established. */
static inline NUINT catrl_find(POINTER data, NUINT count, NUINT size, POINTER element) {
    if (count == 0) return 0;
    const BYTE* current = (const BYTE*) data;
    const BYTE* target = (const BYTE*) element;
    const BYTE first = target[0];
    NUINT index = 0;
    if (size == (NUINT) 1) {
#if CATALYST_SUPPORTS_64BIT || CATALYST_SUPPORTS_32BIT
        if (count >= (NUINT) 32) {
            if (current[0] == first) return 0;
#if CATALYST_SUPPORTS_64BIT
            typedef ULONG CATRL_SEARCH_WORD;
            const CATRL_SEARCH_WORD ones = (CATRL_SEARCH_WORD) (CATALYST_ULONG_MAX / (ULONG) CATALYST_BYTE_MAX);
#else
            typedef UINT CATRL_SEARCH_WORD;
            const CATRL_SEARCH_WORD ones = (CATRL_SEARCH_WORD) (CATALYST_UINT_MAX / (UINT) CATALYST_BYTE_MAX);
#endif
            const CATRL_SEARCH_WORD high = (CATRL_SEARCH_WORD) (ones << 7);
            const CATRL_SEARCH_WORD pattern = (CATRL_SEARCH_WORD) (ones * (CATRL_SEARCH_WORD) first);
            const NUINT width = (NUINT) sizeof(CATRL_SEARCH_WORD);
            const NUINT group = (NUINT) (width * (NUINT) 4);
            /* Four independent word tests share one branch in the miss path.
             * All loads are within the checked extent and need no alignment. */
            while ((NUINT) (count - index) >= group) {
                CATRL_SEARCH_WORD a, b, c, d;
#if defined(__GNUC__) || defined(__clang__)
                __builtin_memcpy(&a, current, sizeof(a));
                __builtin_memcpy(&b, current + width, sizeof(b));
                __builtin_memcpy(&c, current + (NUINT) (width * (NUINT) 2), sizeof(c));
                __builtin_memcpy(&d, current + (NUINT) (width * (NUINT) 3), sizeof(d));
#else
                for (BYTE i = 0; i < (BYTE) sizeof(a); ++i) {
                    ((BYTE*) &a)[i] = current[i];
                    ((BYTE*) &b)[i] = current[width + i];
                    ((BYTE*) &c)[i] = current[(NUINT) (width * (NUINT) 2) + i];
                    ((BYTE*) &d)[i] = current[(NUINT) (width * (NUINT) 3) + i];
                }
#endif
                a ^= pattern; b ^= pattern; c ^= pattern; d ^= pattern;
                a = (CATRL_SEARCH_WORD) ((a - ones) & ~a & high);
                b = (CATRL_SEARCH_WORD) ((b - ones) & ~b & high);
                c = (CATRL_SEARCH_WORD) ((c - ones) & ~c & high);
                d = (CATRL_SEARCH_WORD) ((d - ones) & ~d & high);
                if ((a | b | c | d) != 0) {
                    NUINT offset = 0;
                    CATRL_SEARCH_WORD matches = a;
                    if (matches == 0) {
                        offset = width; matches = b;
                        if (matches == 0) {
                            offset = (NUINT) (width * (NUINT) 2); matches = c;
                            if (matches == 0) {
                                offset = (NUINT) (width * (NUINT) 3); matches = d;
                            }
                        }
                    }
                    /* Borrow propagation cannot mark a byte before the first
                     * true match in little-endian memory order. */
#if (defined(__GNUC__) || defined(__clang__)) && defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if CATALYST_SUPPORTS_64BIT && defined(__SIZEOF_LONG_LONG__) && __SIZEOF_LONG_LONG__ == 8
                    return (NUINT) (index + offset + (NUINT) ((NUINT) __builtin_ctzll(matches) / (NUINT) 8));
#elif !CATALYST_SUPPORTS_64BIT && CATALYST_SUPPORTS_32BIT && defined(__SIZEOF_INT__) && __SIZEOF_INT__ == 4
                    return (NUINT) (index + offset + (NUINT) ((NUINT) __builtin_ctz(matches) / (NUINT) 8));
#endif
#endif
                    for (NUINT i = 0; i < width; ++i) {
                        if (current[offset + i] == first) return (NUINT) (index + offset + i);
                    }
                }
                current += group;
                index = (NUINT) (index + group);
            }
        }
#endif
        while ((NUINT) (count - index) >= (NUINT) 4) {
            if (current[0] == first) return index;
            if (current[1] == first) return (NUINT) (index + (NUINT) 1);
            if (current[2] == first) return (NUINT) (index + (NUINT) 2);
            if (current[3] == first) return (NUINT) (index + (NUINT) 3);
            current += 4;
            index = (NUINT) (index + (NUINT) 4);
        }
        while (index < count) {
            if (*current++ == first) return index;
            ++index;
        }
        return count;
    }
    while (index < count) {
        if (current[0] == first && catmm_compare(current + 1, target + 1, (NUINT) (size - (NUINT) 1)) == 0) return index;
        current += size;
        ++index;
    }
    return count;
}

/* Store one small element without a general-purpose transfer call. All input
 * and output bytes obey the same non-overlap contract as catmm_copy. */
static inline void catrl_store(POINTERW destination, POINTER source, NUINT size) {
    BYTE* output = (BYTE*) destination;
    const BYTE* input = (const BYTE*) source;
    if (size == (NUINT) 1) {
        output[0] = input[0];
    } else if (size == (NUINT) 2) {
        output[0] = input[0];
        output[1] = input[1];
    } else if (size == (NUINT) 4) {
#if defined(__GNUC__) || defined(__clang__)
        __builtin_memcpy(output, input, 4);
#else
        for (BYTE i = 0; i < (BYTE) 4; ++i) output[i] = input[i];
#endif
#if defined(__GNUC__) || defined(__clang__)
    } else if (size == (NUINT) 8) {
        __builtin_memcpy(output, input, 8);
    } else if (size == (NUINT) 16) {
        __builtin_memcpy(output, input, 16);
#else
    } else if (size == (NUINT) 8) {
        for (BYTE i = 0; i < (BYTE) 8; ++i) output[i] = input[i];
#endif
    } else {
        catmm_copy(destination, source, size);
    }
}

/* Distinct key, readable external key/value and one reserved slot. */
static inline void catrl_map_append_known(CATRLLIB_MAP* map, POINTER key, POINTER value) {
    catrl_store((BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), key, map->ksize);
    catrl_store((BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), value, map->vsize);
    ++map->cardinality;
}

/* Remove a resolved live slot. No search and no allocation. */
static inline void catrl_map_remove_at(CATRLLIB_MAP* map, NUINT index) {
    --map->cardinality;
    if (index != map->cardinality) {
        catrl_store((BYTE*) map->keys + (NUINT) (index * map->ksize), (const BYTE*) map->keys + (NUINT) (map->cardinality * map->ksize), map->ksize);
        catrl_store((BYTE*) map->values + (NUINT) (index * map->vsize), (const BYTE*) map->values + (NUINT) (map->cardinality * map->vsize), map->vsize);
    }
}

} /* namespace catalyst */
#endif /* CATRLLIB_INTERNAL_HPP */
