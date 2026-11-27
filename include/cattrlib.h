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

/** A reference to an executable array of instructions. For CATTRLIB entry and submitted procedures, restore void (*)(CATMMLIB_POINTERW) before invocation. */
typedef void (*CATTRLIB_PROCEDURE)(void);

/** A reference to a procedure whose execution produces a result. Submitted functions use CATMMLIB_POINTERW (*)(CATMMLIB_POINTERW); dispatch rules use CATMMLIB_POINTERW (*)(const CATRLLIB_SET*). */
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

/** Native process launch is not implemented; leaves a freeable empty process. */
CATTRLIB_API CATALYST_RESULT cattr_process_create(CATTRLIB_PROCESS* process, CATTRLIB_FUNCTION function, CATMMLIB_POINTERW input);

/** Capture the current native process into unused storage. Its function is unknown. */
CATTRLIB_API CATALYST_RESULT cattr_process_current(CATTRLIB_PROCESS* process);

/** Native process launch is not implemented. */
CATTRLIB_API CATALYST_RESULT cattr_process_start(CATTRLIB_PROCESS* process);

/** Native process waiting is not implemented; waiting for the current process is invalid. */
CATTRLIB_API CATALYST_RESULT cattr_process_wait(CATTRLIB_PROCESS* process);

/** Determine whether two processes represent the same instance of execution. */
CATTRLIB_API CATALYST_BOOL cattr_process_equals(const CATTRLIB_PROCESS* process1, const CATTRLIB_PROCESS* process2);

/** Native process termination is not implemented; terminating the captured current process is invalid. */
CATTRLIB_API CATALYST_RESULT cattr_process_terminate(CATTRLIB_PROCESS* process);

/** Release the resources associated with a process. */
CATTRLIB_API CATALYST_RESULT cattr_process_free(CATTRLIB_PROCESS* process);

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a single-start thread in the current process. Keep process and arguments live through completion; failed creation permits free. */
CATTRLIB_API CATALYST_RESULT cattr_thread_create(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process, CATTRLIB_PROCEDURE procedure, CATMMLIB_POINTERW arguments);

/** Capture the current thread in the current process into unused storage; procedure is unknown. Keep the captured execution live when comparing or using its identity. Failed capture permits free. */
CATTRLIB_API CATALYST_RESULT cattr_thread_current(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process);

/** Start a created thread once. Serialize lifecycle changes and publish associated dispatchers before starting. Failed start permits free only. Windows uses native CreateThread without CRT-specific thread initialization; code requiring CRT-managed thread startup must arrange it outside CATTRLIB. */
CATTRLIB_API CATALYST_RESULT cattr_thread_start(CATTRLIB_THREAD* thread);

/** Join a started thread from another thread. A previously joined thread is a no-op; serialize lifecycle changes. */
CATTRLIB_API CATALYST_RESULT cattr_thread_wait(CATTRLIB_THREAD* thread);

/** Compare execution identity; distinct handles may be compared while their native identities remain live. Serialize lifecycle changes. */
CATTRLIB_API CATALYST_BOOL cattr_thread_equals(const CATTRLIB_THREAD* thread1, const CATTRLIB_THREAD* thread2);

/** Release a captured, unstarted, joined or failed thread after all uses cease; does not terminate execution. */
CATTRLIB_API CATALYST_RESULT cattr_thread_free(CATTRLIB_THREAD* thread);

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish bounded work storage of nonzero capacity in unused storage. Null rule selects FIFO; otherwise the rule returns the address of a member in the read-only set of CATTRLIB_PROCEDURE references. The rule must not reenter the dispatcher or retain member addresses. Failed creation permits free only. Keep the associated thread live. Serialize its start and lifecycle changes with external dispatcher access. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_create(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_THREAD* thread, CATTRLIB_FUNCTION rule, CATALYST_NUINT capacity);

/** Submit or execute immediately on the associated thread. Timeout is milliseconds (NUINT_MAX means indefinite); pending timed-out awaited work is removed, running awaited work completes before return. Async arguments must live through execution or cancellation. Async duplicate pending operations are no-ops (arguments are not replaced); awaited duplicates are rejected. CATTRLIB does not retain arguments or output storage after a completed awaited call. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit_procedure(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_PROCEDURE procedure, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout);

/** As for procedure submission. Awaited calls require output storage, cleared on entry and written only on success; async calls require null output. Returned storage remains owned by the function/caller. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit_function(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_FUNCTION function, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout, CATMMLIB_POINTERW* output);

/** Return the synchronized pending work count; excludes executing and completed work. Do not access work/state directly during concurrent use. */
CATTRLIB_API CATALYST_NUINT cattr_dispatcher_count(CATTRLIB_DISPATCHER* dispatcher);

/** On the associated thread, wait for pending work; timeout is milliseconds (NUINT_MAX means indefinite). Close interrupts the wait. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_wait(CATTRLIB_DISPATCHER* dispatcher, CATALYST_NUINT timeout);

/** Execute one pending operation on the associated thread, or return SUCCESS_NOOP when empty. Non-reentrant; execute outside the monitor. Close interrupts polling. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_poll(CATTRLIB_DISPATCHER* dispatcher);

/** Close to further submissions, cancel pending work and awaken waiters; running work completes. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_close(CATTRLIB_DISPATCHER* dispatcher);

/** Release a closed or failed-init dispatcher after all users have returned. Failed release permits only a subsequent free. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_free(CATTRLIB_DISPATCHER* dispatcher);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTRLIB_H */
