#ifndef CATMMLIB_INTERNAL_HPP
#define CATMMLIB_INTERNAL_HPP
#include "catalyst.hpp"
#include "catmmlib.h"

#if !TARGET_PLATFORM_WIN32 && !TARGET_PLATFORM_LINUX && !TARGET_PLATFORM_DARWIN
#error "CATMMLIB requires a supported native memory-management backend."
#endif

/* Private object representation copied to/from the allocation prefix. Never
 * interpret mapped bytes as a constructed or suitably aligned header object.
 * requested_size excludes the prefix. On mapping backends, pending is
 * null unless both old-block release and replacement cleanup failed during
 * resize. Such a replacement remains owned and reachable from the old block.
 * Its own header supplies its extent; it must be released before another
 * resize or release of the old block.
 */
typedef struct CATMMLIB_MEMORY_HEADER {
    CATALYST_NUINT requested_size;
#if !TARGET_PLATFORM_WIN32
    CATMMLIB_POINTERW pending;
#endif
} CATMMLIB_MEMORY_HEADER;

/* Platform boundary: size is nonzero, includes the prefix, and is representable
 * in the native extent type. Output slots are valid; release/resize receive a
 * live native base and its original requested extent. Allocate writes null on
 * failure; resize/release failure leaves the input allocation unchanged.
 * Allocate zero-fills the native extent; Windows resize zero-fills growth.
 * Prefix size is a multiple of native fundamental alignment and holds the
 * header. Extended alignment is not promised. Native types stay in backends.
 * Contexts: alloc 01, resize 02, release 03. Backend operation 01 = failure,
 * 02 = success; allocation 06 = page-rounding overflow, 07 = unsupported
 * native page-size representation.
 * Shared reporting sites start at 03 (allocation skips backend operation 06).
 */
/* These implementation boundaries are not part of the exported C API. */
#if !TARGET_PLATFORM_WIN32 && (defined(__GNUC__) || defined(__clang__))
#define CATMMLIB_PRIVATE __attribute__((visibility("hidden")))
#else
#define CATMMLIB_PRIVATE
#endif

CATMMLIB_PRIVATE CATALYST_NUINT catmm_platform_prefix_size();
CATMMLIB_PRIVATE CATALYST_RESULT catmm_platform_alloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size);
#if TARGET_PLATFORM_DARWIN
/* Nonzero extents include the prefix; old_size belongs to a live allocation.
 * Return whether both sizes occupy the same native page extent. No OS call or
 * mutation occurs; representable page size was established on allocation.
 */
CATMMLIB_PRIVATE CATALYST_BOOL catmm_platform_extent_matches(CATALYST_NUINT old_size, CATALYST_NUINT new_size);
#endif
#if TARGET_PLATFORM_WIN32
CATMMLIB_PRIVATE CATALYST_RESULT catmm_platform_realloc(CATMMLIB_POINTERW* memory, CATALYST_NUINT size);
#endif
CATMMLIB_PRIVATE CATALYST_RESULT catmm_platform_free(CATMMLIB_POINTERW memory, CATALYST_NUINT size);
#undef CATMMLIB_PRIVATE
#endif /* CATMMLIB_INTERNAL_HPP */
