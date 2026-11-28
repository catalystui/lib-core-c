#ifndef CATTRLIB_INTERNAL_HPP
#define CATTRLIB_INTERNAL_HPP
#include "cattrlib.hpp"

/* Private backend boundaries. Capture/start publish a live native identity
 * on success. Failure leaves null or a state releasable by the matching
 * free operation; release failure retains a releasable state, success clears
 * the slot. Equality/current predicates receive live identities only.
 * Native types and time representations remain inside the backend.
 */
#if !TARGET_PLATFORM_WIN32 && (defined(__GNUC__) || defined(__clang__))
#define CATTRLIB_PRIVATE __attribute__((visibility("hidden")))
#else
#define CATTRLIB_PRIVATE
#endif

CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_process_current(CATMMLIB_POINTERW* state);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_process_equals(CATMMLIB_POINTER first, CATMMLIB_POINTER second);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_process_is_current(CATMMLIB_POINTER state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_process_free(CATMMLIB_POINTERW* state);

CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_current(CATMMLIB_POINTERW* state);
/* Publish identity before entry can execute. Failure starts no entry. */
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_start(CATMMLIB_POINTERW* state, void (*entry)(CATMMLIB_POINTERW), CATMMLIB_POINTERW arguments);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_wait(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_thread_equals(CATMMLIB_POINTER first, CATMMLIB_POINTER second);
CATTRLIB_PRIVATE CATALYST_BOOL cattr_platform_thread_is_current(CATMMLIB_POINTER state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_thread_free(CATMMLIB_POINTERW* state);

/* A monitor serializes access and provides change notification. Lock acquires
 * exclusive ownership of a valid initialized monitor; unlock/notify require
 * ownership. Nonreporting lock operations require native invariants to hold.
 * Wait is entered and
 * returns locked, including on failure. Evaluate ready before blocking and
 * after every wake; true completes successfully. The predicate may advance
 * shared state under the lock, but must not block or release the lock.
 * One monotonic deadline covers the entire call, including spurious wakes;
 * zero polls and NUINT_MAX waits indefinitely. Reevaluate before reporting
 * timeout. Notify wakes all waiters, which may have different predicates.
 * Free requires external quiescence; partial creation remains freeable.
 */
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_create(CATMMLIB_POINTERW* state);
CATTRLIB_PRIVATE void cattr_platform_monitor_lock(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE void cattr_platform_monitor_unlock(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE void cattr_platform_monitor_notify(CATMMLIB_POINTERW state);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_wait(CATMMLIB_POINTERW state, CATALYST_NUINT timeout, CATALYST_BOOL (*ready)(CATMMLIB_POINTERW), CATMMLIB_POINTERW arguments);
CATTRLIB_PRIVATE CATALYST_RESULT cattr_platform_monitor_free(CATMMLIB_POINTERW* state);
/* Give a running callback an opportunity to complete after wait failure. */
CATTRLIB_PRIVATE void cattr_platform_yield();

#undef CATTRLIB_PRIVATE
#endif /* CATTRLIB_INTERNAL_HPP */
