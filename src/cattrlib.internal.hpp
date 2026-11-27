#ifndef CATTRLIB_INTERNAL_HPP
#define CATTRLIB_INTERNAL_HPP
#include "cattrlib.hpp"

/* Native objects remain opaque outside the platform backend. Native waits
 * return with the monitor locked, including on timeout. A monitor is used
 * only after initialization and destroyed only after external quiescence. */
#if !TARGET_PLATFORM_WIN32 && (defined(__GNUC__) || defined(__clang__))
#define CATTRLIB_PRIVATE __attribute__((visibility("hidden")))
#else
#define CATTRLIB_PRIVATE
#endif

/* Capture a native process identity without narrowing it to a portable size
 * or exposing a native handle/identifier in the public structure. */
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_process_capture(CATMMLIB_POINTERW* state);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_process_equals(CATMMLIB_POINTER state1, CATMMLIB_POINTER state2);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_process_is_current(CATMMLIB_POINTER state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_capture(CATMMLIB_POINTERW* state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_start(CATMMLIB_POINTERW* state, CATTRLIB_PROCEDURE procedure, CATMMLIB_POINTERW arguments);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_wait(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_thread_equals(CATMMLIB_POINTER state1, CATMMLIB_POINTER state2);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_thread_is_current(CATMMLIB_POINTER state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_free(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_create(CATMMLIB_POINTERW* state);
CATTRLIB_PRIVATE void cattr_platform_monitor_lock(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE void cattr_platform_monitor_unlock(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE void cattr_platform_monitor_notify(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_wait(CATMMLIB_POINTERW state, CATALYST_NUINT timeout);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_free(CATMMLIB_POINTERW state);
/* Read monotonic milliseconds modulo the Catalyst pointer-width range. */
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_ticks(CATALYST_NUINT* ticks);
CATTRLIB_PRIVATE void cattr_platform_yield();

#undef CATTRLIB_PRIVATE
#endif /* CATTRLIB_INTERNAL_HPP */
