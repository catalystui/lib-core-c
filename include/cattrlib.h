#ifndef CATTRLIB_H
#define CATTRLIB_H
#include "catalyst.h"
#include "catmmlib.h"
#include "catrllib.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Export API */
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(CATTRLIB_EXPORTS)
        #define CATTRLIB_API __declspec(dllexport)
    #elif defined(CATTRLIB_USE_DLL)
        #define CATTRLIB_API __declspec(dllimport)
    #else
        #define CATTRLIB_API
    #endif
#else
    #define CATTRLIB_API
#endif

/* -------------------------------------------------------------------------------------------------
 * C API :: OPERATION STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A reference to an executable array of instructions. */
typedef void (*CATTRLIB_PROCEDURE)(void);

/** A reference to a procedure whose execution produces a result. */
typedef CATMMLIB_POINTERW (*CATTRLIB_FUNCTION)(void);

/* -------------------------------------------------------------------------------------------------
 * C API :: THREADING STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A bounded flow of execution which transforms input into output. */
typedef struct CATTRLIB_PROCESS {
    CATTRLIB_FUNCTION function;
    CATMMLIB_POINTERW state;
} CATTRLIB_PROCESS;

/** A path of execution contained within a process. */
typedef struct CATTRLIB_THREAD {
    CATTRLIB_PROCESS* process;
    CATTRLIB_PROCEDURE procedure;
    CATMMLIB_POINTERW state;
} CATTRLIB_THREAD;

/** A mechanism which selects work for execution on an associated thread. */
typedef struct CATTRLIB_DISPATCHER {
    CATTRLIB_THREAD* thread;
    CATTRLIB_FUNCTION rule;
    CATRLLIB_SET work;
    CATMMLIB_POINTERW state;
} CATTRLIB_DISPATCHER;

/* -------------------------------------------------------------------------------------------------
 * C API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a process associated with the specified function. */
CATTRLIB_API CATALYST_RESULT cattr_process_create(CATTRLIB_PROCESS* process, CATTRLIB_FUNCTION function, CATMMLIB_POINTERW input);

/** Retrieve the process associated with the current execution. */
CATTRLIB_API CATALYST_RESULT cattr_process_current(CATTRLIB_PROCESS* process);

/** Begin execution of a process. */
CATTRLIB_API CATALYST_RESULT cattr_process_start(CATTRLIB_PROCESS* process);

/** Wait for a process to complete execution. */
CATTRLIB_API CATALYST_RESULT cattr_process_wait(CATTRLIB_PROCESS* process);

/** Determine whether two processes represent the same instance of execution. */
CATTRLIB_API CATALYST_BOOL cattr_process_equals(const CATTRLIB_PROCESS* process1, const CATTRLIB_PROCESS* process2);

/** Terminate execution of a process. */
CATTRLIB_API CATALYST_RESULT cattr_process_terminate(CATTRLIB_PROCESS* process);

/** Release the resources associated with a process. */
CATTRLIB_API CATALYST_RESULT cattr_process_free(CATTRLIB_PROCESS* process);

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a thread within the specified process. */
CATTRLIB_API CATALYST_RESULT cattr_thread_create(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process, CATTRLIB_PROCEDURE procedure, CATMMLIB_POINTERW arguments);

/** Retrieve the thread associated with the current execution. */
CATTRLIB_API CATALYST_RESULT cattr_thread_current(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process);

/** Begin execution of a thread. */
CATTRLIB_API CATALYST_RESULT cattr_thread_start(CATTRLIB_THREAD* thread);

/** Wait for a thread to complete execution. */
CATTRLIB_API CATALYST_RESULT cattr_thread_wait(CATTRLIB_THREAD* thread);

/** Determine whether two threads represent the same execution path. */
CATTRLIB_API CATALYST_BOOL cattr_thread_equals(const CATTRLIB_THREAD* thread1, const CATTRLIB_THREAD* thread2);

/** Release the resources associated with a thread. */
CATTRLIB_API CATALYST_RESULT cattr_thread_free(CATTRLIB_THREAD* thread);

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a dispatcher associated with the specified thread. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_create(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_THREAD* thread, CATTRLIB_FUNCTION rule, CATALYST_NUINT capacity);

/** Submit a procedure for execution. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit_procedure(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_PROCEDURE procedure, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout);

/** Submit a function for execution. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit_function(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_FUNCTION function, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout, CATMMLIB_POINTERW* output);

/** Retrieve the number of operations awaiting execution. */
CATTRLIB_API CATALYST_NUINT cattr_dispatcher_count(CATTRLIB_DISPATCHER* dispatcher);

/** Wait until work becomes available for execution. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_wait(CATTRLIB_DISPATCHER* dispatcher, CATALYST_NUINT timeout);

/** Select and execute pending work according to the dispatch rule. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_poll(CATTRLIB_DISPATCHER* dispatcher);

/** Close the dispatcher to further submissions and awaken waiting threads. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_close(CATTRLIB_DISPATCHER* dispatcher);

/** Release the resources associated with a dispatcher. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_free(CATTRLIB_DISPATCHER* dispatcher);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTRLIB_H */
