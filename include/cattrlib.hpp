#ifndef CATTRLIB_HPP
#define CATTRLIB_HPP
#include "catrllib.hpp"
#include "cattrlib.h"

namespace catalyst {

/* -------------------------------------------------------------------------------------------------
 * C++ API :: OPERATION STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A reference to an executable array of instructions. */
typedef ::CATTRLIB_PROCEDURE PROCEDURE;

/** A reference to a procedure whose execution produces a result. */
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

/** Establish a process associated with the specified function. */
inline RESULT process_create(PROCESS* process, FUNCTION function, POINTERW input) {
    return ::cattr_process_create(process, function, input);
}

/** Retrieve the process associated with the current execution. */
inline RESULT process_current(PROCESS* process) {
    return ::cattr_process_current(process);
}

/** Begin execution of a process. */
inline RESULT process_start(PROCESS* process) {
    return ::cattr_process_start(process);
}

/** Wait for a process to complete execution. */
inline RESULT process_wait(PROCESS* process) {
    return ::cattr_process_wait(process);
}

/** Determine whether two processes represent the same instance of execution. */
inline BOOL process_equals(const PROCESS* process1, const PROCESS* process2) {
    return ::cattr_process_equals(process1, process2);
}

/** Terminate execution of a process. */
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

/** Establish a thread within the specified process. */
inline RESULT thread_create(THREAD* thread, PROCESS* process, PROCEDURE procedure, POINTERW arguments) {
    return ::cattr_thread_create(thread, process, procedure, arguments);
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

/** Determine whether two threads represent the same execution path. */
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

/** Establish a dispatcher associated with the specified thread. */
inline RESULT dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, FUNCTION rule, NUINT capacity) {
    return ::cattr_dispatcher_create(dispatcher, thread, rule, capacity);
}

/** Submit a procedure for execution. */
inline RESULT dispatcher_submit_procedure(DISPATCHER* dispatcher, PROCEDURE procedure, POINTERW arguments, BOOL wait, NUINT timeout) {
    return ::cattr_dispatcher_submit_procedure(dispatcher, procedure, arguments, wait, timeout);
}

/** Submit a function for execution. */
inline RESULT dispatcher_submit_function(DISPATCHER* dispatcher, FUNCTION function, POINTERW arguments, BOOL wait, NUINT timeout, POINTERW* output) {
    return ::cattr_dispatcher_submit_function(dispatcher, function, arguments, wait, timeout, output);
}

/** Retrieve the number of operations awaiting execution. */
inline NUINT dispatcher_count(DISPATCHER* dispatcher) {
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