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

/** A bounded flow of execution which transforms input into output. */
typedef ::CATTRLIB_PROCESS PROCESS;
/** A path of execution contained within a process. */
typedef ::CATTRLIB_THREAD THREAD;
/** A mechanism which selects work for execution on an associated thread. */
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

/** Establish a thread within the specified process. Entry must reference a void (CATMMLIB_POINTERW) function. */
inline RESULT thread_create(THREAD* thread, PROCESS* process, POINTERX entry, POINTERW arguments) {
    return ::cattr_thread_create(thread, process, entry, arguments);
}

/** Retrieve the thread associated with the current execution. */
inline RESULT thread_current(THREAD* thread, PROCESS* process) {
    return ::cattr_thread_current(thread, process);
}

/** Begin execution of a thread. */
inline RESULT thread_start(THREAD* thread) {
    return ::cattr_thread_start(thread);
}

/** Wait for a thread to complete execution. */
inline RESULT thread_wait(THREAD* thread) {
    return ::cattr_thread_wait(thread);
}

/** Determine whether two threads represent the same instance of execution. */
inline BOOL thread_equals(const THREAD* thread1, const THREAD* thread2) {
    return ::cattr_thread_equals(thread1, thread2);
}

/** Release the resources associated with a thread. */
inline RESULT thread_free(THREAD* thread) {
    return ::cattr_thread_free(thread);
}

/* -------------------------------------------------------------------------------------------------
 * C++ API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a dispatcher with the specified capacity and dispatch rule. A null rule selects FIFO order. */
inline RESULT dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, POINTER (*rule)(const SET*), NUINT capacity) {
    return ::cattr_dispatcher_create(dispatcher, thread, rule, capacity);
}

/** Submit an independent invocation for execution. Operation must reference a void (CATMMLIB_POINTERW) function. On the associated thread, awaited submission is invalid and asynchronous admission is nonblocking. */
inline RESULT dispatcher_submit(DISPATCHER* dispatcher, POINTERX operation, POINTERW arguments, BOOL wait, NUINT timeout) {
    return ::cattr_dispatcher_submit(dispatcher, operation, arguments, wait, timeout);
}

/** Retrieve the number of submissions awaiting execution. */
inline NUINT dispatcher_count(const DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_count(dispatcher);
}

/** Wait until work becomes available for execution. */
inline RESULT dispatcher_wait(DISPATCHER* dispatcher, NUINT timeout) {
    return ::cattr_dispatcher_wait(dispatcher, timeout);
}

/** Select and execute pending work according to the dispatch rule. */
inline RESULT dispatcher_poll(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_poll(dispatcher);
}

/** Close the dispatcher to further submissions and awaken waiting threads. */
inline RESULT dispatcher_close(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_close(dispatcher);
}

/** Release the resources associated with a dispatcher. */
inline RESULT dispatcher_free(DISPATCHER* dispatcher) {
    return ::cattr_dispatcher_free(dispatcher);
}

} /* namespace catalyst */
#endif /* CATTRLIB_HPP */
