#ifndef CATTRLIB_HPP
#define CATTRLIB_HPP
#include "catrllib.hpp"
#include "cattrlib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: OPERATION STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A reference to an executable array of instructions. For CATTRLIB entry and submitted procedures, restore void (*)(CATMMLIB_POINTERW) before invocation. */
typedef ::CATTRLIB_PROCEDURE PROCEDURE;

/** A reference to a procedure whose execution produces a result. Submitted functions use CATMMLIB_POINTERW (*)(CATMMLIB_POINTERW); dispatch rules use CATMMLIB_POINTERW (*)(const CATRLLIB_SET*). */
typedef ::CATTRLIB_FUNCTION FUNCTION;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: THREADING STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A bounded flow of execution which transforms input into output. */
typedef ::CATTRLIB_PROCESS PROCESS;

/** A path of execution contained within a process. */
typedef ::CATTRLIB_THREAD THREAD;

/** A mechanism which selects work for execution on an associated thread. */
typedef ::CATTRLIB_DISPATCHER DISPATCHER;

/* -------------------------------------------------------------------------------------------------
 * C++ API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Native process launch is not implemented; leaves a freeable empty process. */
inline RESULT process_create(PROCESS* process, FUNCTION function, POINTERW input) {
    return ::cattr_process_create(process, function, input);
}

/** Capture the current native process into unused storage. Its function is unknown. */
inline RESULT process_current(PROCESS* process) {
    return ::cattr_process_current(process);
}

/** Native process launch is not implemented. */
inline RESULT process_start(PROCESS* process) {
    return ::cattr_process_start(process);
}

/** Native process waiting is not implemented; waiting for the current process is invalid. */
inline RESULT process_wait(PROCESS* process) {
    return ::cattr_process_wait(process);
}

/** Determine whether two processes represent the same instance of execution. */
inline BOOL process_equals(const PROCESS* process1, const PROCESS* process2) {
    return ::cattr_process_equals(process1, process2);
}

/** Native process termination is not implemented; terminating the captured current process is invalid. */
inline RESULT process_terminate(PROCESS* process) {
    return ::cattr_process_terminate(process);
}

/** Release the resources associated with a process. */
inline RESULT process_free(PROCESS* process) {
    return ::cattr_process_free(process);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a single-start thread in the current process. Keep process and arguments live through completion; failed creation permits free. */
inline RESULT thread_create(THREAD* thread, PROCESS* process, PROCEDURE procedure, POINTERW arguments) {
    return ::cattr_thread_create(thread, process, procedure, arguments);
}

/** Capture the current thread in the current process into unused storage; procedure is unknown. Keep the captured execution live when comparing or using its identity. Failed capture permits free. */
inline RESULT thread_current(THREAD* thread, PROCESS* process) {
    return ::cattr_thread_current(thread, process);
}

/** Start a created thread once. Serialize lifecycle changes and publish associated dispatchers before starting. Failed start permits free only. Windows uses native CreateThread without CRT-specific thread initialization; code requiring CRT-managed thread startup must arrange it outside CATTRLIB. */
inline RESULT thread_start(THREAD* thread) {
    return ::cattr_thread_start(thread);
}

/** Join a started thread from another thread. A previously joined thread is a no-op; serialize lifecycle changes. */
inline RESULT thread_wait(THREAD* thread) {
    return ::cattr_thread_wait(thread);
}

/** Compare execution identity; distinct handles may be compared while their native identities remain live. Serialize lifecycle changes. */
inline BOOL thread_equals(const THREAD* thread1, const THREAD* thread2) {
    return ::cattr_thread_equals(thread1, thread2);
}

/** Release a captured, unstarted, joined or failed thread after all uses cease; does not terminate execution. */
inline RESULT thread_free(THREAD* thread) {
    return ::cattr_thread_free(thread);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish bounded work storage of nonzero capacity in unused storage. Null rule selects FIFO; otherwise the rule returns the address of a member in the read-only set of CATTRLIB_PROCEDURE references. The rule must not reenter the dispatcher or retain member addresses. Failed creation permits free only. Keep the associated thread live. Serialize its start and lifecycle changes with external dispatcher access. */
inline RESULT dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, FUNCTION rule, NUINT capacity) {
    return ::cattr_dispatcher_create(dispatcher, thread, rule, capacity);
}

/** Submit or execute immediately on the associated thread. Timeout is milliseconds (NUINT_MAX means indefinite); pending timed-out awaited work is removed, running awaited work completes before return. Async arguments must live through execution or cancellation. Async duplicate pending operations are no-ops (arguments are not replaced); awaited duplicates are rejected. CATTRLIB does not retain arguments or output storage after a completed awaited call. */
inline RESULT dispatcher_submit_procedure(DISPATCHER* dispatcher, PROCEDURE procedure, POINTERW arguments, BOOL wait, NUINT timeout) {
    return ::cattr_dispatcher_submit_procedure(dispatcher, procedure, arguments, wait, timeout);
}

/** As for procedure submission. Awaited calls require output storage, cleared on entry and written only on success; async calls require null output. Returned storage remains owned by the function/caller. */
inline RESULT dispatcher_submit_function(DISPATCHER* dispatcher, FUNCTION function, POINTERW arguments, BOOL wait, NUINT timeout, POINTERW* output) {
    return ::cattr_dispatcher_submit_function(dispatcher, function, arguments, wait, timeout, output);
}

/** Return the synchronized pending work count; excludes executing and completed work. Do not access work/state directly during concurrent use. */
inline NUINT dispatcher_count(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_count(dispatcher);
}

/** On the associated thread, wait for pending work; timeout is milliseconds (NUINT_MAX means indefinite). Close interrupts the wait. */
inline RESULT dispatcher_wait(DISPATCHER* dispatcher, NUINT timeout) {
    return ::cattr_dispatcher_wait(dispatcher, timeout);
}

/** Execute one pending operation on the associated thread, or return SUCCESS_NOOP when empty. Non-reentrant; execute outside the monitor. Close interrupts polling. */
inline RESULT dispatcher_poll(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_poll(dispatcher);
}

/** Close to further submissions, cancel pending work and awaken waiters; running work completes. */
inline RESULT dispatcher_close(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_close(dispatcher);
}

/** Release a closed or failed-init dispatcher after all users have returned. Failed release permits only a subsequent free. */
inline RESULT dispatcher_free(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_free(dispatcher);
}

} /* namespace catalyst */
#endif /* CATTRLIB_HPP */
