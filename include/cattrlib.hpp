#ifndef CATTRLIB_HPP
#define CATTRLIB_HPP
#include "catrllib.hpp"
#include "cattrlib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: EXECUTION UNITS
 * ------------------------------------------------------------------------------------------------- */

/** A reference to executable instructions. */
typedef ::CATTRLIB_POINTERX POINTERX;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: THREADING STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A bounded flow of execution which transforms input into output. Owns its captured state; do not copy or free while a thread borrows it. */
typedef ::CATTRLIB_PROCESS PROCESS;
/** A path of execution contained within a process. Owns its state and borrows its process. Keep initialized objects and entry arguments live through execution and cleanup; do not copy or mutate them. */
typedef ::CATTRLIB_THREAD THREAD;
/** A mechanism which selects work for execution on an associated thread. Owns its internal storage; borrows the thread until free. Do not copy or mutate an initialized dispatcher. Before its thread stops servicing work, close admission if other callers may still submit or wait. The caller coordinates thread startup and shutdown. */
typedef ::CATTRLIB_DISPATCHER DISPATCHER;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Retrieve the process associated with the current execution. */
inline RESULT process_current(PROCESS* process) {
    return ::cattr_process_current(process);
}

/** Determine whether two processes represent the same instance of execution. */
inline BOOL process_equals(const PROCESS* process1, const PROCESS* process2) {
    return ::cattr_process_equals(process1, process2);
}

/** Release the resources associated with a process. */
inline RESULT process_free(PROCESS* process) {
    return ::cattr_process_free(process);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a thread within the current process. Entry must reference a void (CATMMLIB_POINTERW) function and return normally. Failed creation permits free only. */
inline RESULT thread_create(THREAD* thread, PROCESS* process, POINTERX entry, POINTERW arguments) {
    return ::cattr_thread_create(thread, process, entry, arguments);
}

/** Capture the thread associated with the current execution. The captured native identity must remain live until its last use (except free). */
inline RESULT thread_current(THREAD* thread, PROCESS* process) {
    return ::cattr_thread_current(thread, process);
}

/** Begin execution of a created thread. Serialize startup against other callers; entry may use the thread after native identity publication. Failed startup permits free only. */
inline RESULT thread_start(THREAD* thread) {
    return ::cattr_thread_start(thread);
}

/** Join a started thread from another thread. Serialize joining against external thread and dispatcher operations; the executing entry may continue until joined. */
inline RESULT thread_wait(THREAD* thread) {
    return ::cattr_thread_wait(thread);
}

/** Determine whether two threads represent the same instance of execution. Distinct captures are comparable only while their native identities remain live; a joined object equals only itself. */
inline BOOL thread_equals(const THREAD* thread1, const THREAD* thread2) {
    return ::cattr_thread_equals(thread1, thread2);
}

/** Release a captured, unstarted, joined or failed thread after all users and borrowing dispatchers are finished. */
inline RESULT thread_free(THREAD* thread) {
    return ::cattr_thread_free(thread);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a dispatcher with nonzero capacity and a dispatch rule. A null rule selects FIFO order. The rule receives a nonempty HashSet of opaque submission references and returns one member (not its storage address). Capacity must fit the collection's storage and hash index. It runs under the monitor and must not reenter the dispatcher or block on it. */
inline RESULT dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, POINTER (*rule)(const HASHSET*), NUINT capacity) {
    return ::cattr_dispatcher_create(dispatcher, thread, rule, capacity);
}

/** Submit an independent invocation for execution. Operation must reference a void (CATMMLIB_POINTERW) function. On the associated thread, awaited submission is invalid and asynchronous admission is nonblocking. Callers must avoid cyclic awaited dependencies between execution threads. Timeout is milliseconds (zero polls, NUINT_MAX is indefinite), covering admission and the pending interval; once an awaited callback starts, it completes before return. Readiness observed on reacquiring the monitor wins a race with expiry; the timeout excludes acquiring the monitor. Arguments remain valid through execution or cancellation. Callbacks must return normally. */
inline RESULT dispatcher_submit(DISPATCHER* dispatcher, POINTERX operation, POINTERW arguments, BOOL wait, NUINT timeout) {
    return ::cattr_dispatcher_submit(dispatcher, operation, arguments, wait, timeout);
}

/** Inspect a live submission reference during a dispatch rule. Outputs its executable reference and borrowed arguments. */
inline void dispatcher_inspect(POINTER submission, POINTERX* operation, POINTERW* arguments) {
    ::cattr_dispatcher_inspect(submission, operation, arguments);
}

/** Retrieve the number of submissions awaiting execution. */
inline NUINT dispatcher_count(const DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_count(dispatcher);
}

/** Wait on the associated thread until work becomes available or the dispatcher closes. Do not wait on work that requires the waiting callback to finish. Timeout is milliseconds; zero polls and NUINT_MAX is indefinite. Readiness observed on reacquiring the monitor wins a race with expiry. */
inline RESULT dispatcher_wait(DISPATCHER* dispatcher, NUINT timeout) {
    return ::cattr_dispatcher_wait(dispatcher, timeout);
}

/** Select and execute one pending invocation on the associated thread. The callback must return normally; recursive polling is invalid. */
inline RESULT dispatcher_poll(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_poll(dispatcher);
}

/** Close admission, cancel pending submissions and awaken waiters. Running callbacks are not interrupted; close does not establish quiescence. */
inline RESULT dispatcher_close(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_close(dispatcher);
}

/** Release the resources associated with a closed dispatcher after all submissions, waits, polls and other accesses have returned. Failed creation or release permits a subsequent free. */
inline RESULT dispatcher_free(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_free(dispatcher);
}

} /* namespace catalyst */
#endif /* CATTRLIB_HPP */
