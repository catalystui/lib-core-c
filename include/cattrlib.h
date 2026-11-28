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
 * C API :: EXECUTION UNITS
 * ------------------------------------------------------------------------------------------------- */

/** A reference to executable instructions. */
typedef void (*CATTRLIB_POINTERX)(void);

/* -------------------------------------------------------------------------------------------------
 * C API :: THREADING STRUCTURES
 * ------------------------------------------------------------------------------------------------- */

/** A bounded flow of execution which transforms input into output. Owns its captured state; do not copy or free while a thread borrows it. */
typedef struct CATTRLIB_PROCESS {
    CATMMLIB_POINTERW state;
} CATTRLIB_PROCESS;

/** A path of execution contained within a process. Owns its state and borrows its process. Keep initialized objects and entry arguments live through execution and cleanup; do not copy or mutate them. */
typedef struct CATTRLIB_THREAD {
    CATTRLIB_PROCESS* process;
    CATTRLIB_POINTERX entry;
    CATMMLIB_POINTERW arguments;
    CATMMLIB_POINTERW state;
} CATTRLIB_THREAD;

/** A mechanism which selects work for execution on an associated thread. Owns its internal storage; borrows the thread until free. Do not copy or mutate an initialized dispatcher. Before its thread stops servicing work, close admission if other callers may still submit or wait. The caller coordinates thread startup and shutdown. */
typedef struct CATTRLIB_DISPATCHER {
    CATTRLIB_THREAD* thread;
    CATRLLIB_SET work;
    CATMMLIB_POINTER (*rule)(const CATRLLIB_SET*);
    CATMMLIB_POINTERW state;
} CATTRLIB_DISPATCHER;

/* -------------------------------------------------------------------------------------------------
 * C API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Retrieve the process associated with the current execution. */
CATTRLIB_API CATALYST_RESULT cattr_process_current(CATTRLIB_PROCESS* process);

/** Determine whether two processes represent the same instance of execution. */
CATTRLIB_API CATALYST_BOOL cattr_process_equals(const CATTRLIB_PROCESS* process1, const CATTRLIB_PROCESS* process2);

/** Release the resources associated with a process. */
CATTRLIB_API CATALYST_RESULT cattr_process_free(CATTRLIB_PROCESS* process);

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a thread within the current process. Entry must reference a void (CATMMLIB_POINTERW) function and return normally. Failed creation permits free only. */
CATTRLIB_API CATALYST_RESULT cattr_thread_create(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process, CATTRLIB_POINTERX entry, CATMMLIB_POINTERW arguments);

/** Capture the thread associated with the current execution. The captured native identity must remain live until its last use (except free). */
CATTRLIB_API CATALYST_RESULT cattr_thread_current(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process);

/** Begin execution of a created thread. Serialize startup against other callers; entry may use the thread after native identity publication. Failed startup permits free only. */
CATTRLIB_API CATALYST_RESULT cattr_thread_start(CATTRLIB_THREAD* thread);

/** Join a started thread from another thread. Serialize joining against external thread and dispatcher operations; the executing entry may continue until joined. */
CATTRLIB_API CATALYST_RESULT cattr_thread_wait(CATTRLIB_THREAD* thread);

/** Determine whether two threads represent the same instance of execution. Distinct captures are comparable only while their native identities remain live; a joined object equals only itself. */
CATTRLIB_API CATALYST_BOOL cattr_thread_equals(const CATTRLIB_THREAD* thread1, const CATTRLIB_THREAD* thread2);

/** Release a captured, unstarted, joined or failed thread after all users and borrowing dispatchers are finished. */
CATTRLIB_API CATALYST_RESULT cattr_thread_free(CATTRLIB_THREAD* thread);

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a dispatcher with nonzero capacity and a dispatch rule. A null rule selects FIFO order. The rule receives a nonempty Set of opaque submission references and returns one member (not its storage address). It runs under the monitor and must not reenter the dispatcher or block on it. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_create(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_THREAD* thread, CATMMLIB_POINTER (*rule)(const CATRLLIB_SET*), CATALYST_NUINT capacity);

/** Submit an independent invocation for execution. Operation must reference a void (CATMMLIB_POINTERW) function. On the associated thread, awaited submission is invalid and asynchronous admission is nonblocking. Callers must avoid cyclic awaited dependencies between execution threads. Timeout is milliseconds (zero polls, NUINT_MAX is indefinite), covering admission and the pending interval; once an awaited callback starts, it completes before return. Readiness observed on reacquiring the monitor wins a race with expiry; the timeout excludes acquiring the monitor. Arguments remain valid through execution or cancellation. Callbacks must return normally. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_POINTERX operation, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout);

/** Inspect a live submission reference during a dispatch rule. Outputs its executable reference and borrowed arguments. */
CATTRLIB_API void cattr_dispatcher_inspect(CATMMLIB_POINTER submission, CATTRLIB_POINTERX* operation, CATMMLIB_POINTERW* arguments);

/** Retrieve the number of submissions awaiting execution. */
CATTRLIB_API CATALYST_NUINT cattr_dispatcher_count(const CATTRLIB_DISPATCHER* dispatcher);

/** Wait on the associated thread until work becomes available or the dispatcher closes. Do not wait on work that requires the waiting callback to finish. Timeout is milliseconds; zero polls and NUINT_MAX is indefinite. Readiness observed on reacquiring the monitor wins a race with expiry. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_wait(CATTRLIB_DISPATCHER* dispatcher, CATALYST_NUINT timeout);

/** Select and execute one pending invocation on the associated thread. The callback must return normally; recursive polling is invalid. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_poll(CATTRLIB_DISPATCHER* dispatcher);

/** Close admission, cancel pending submissions and awaken waiters. Running callbacks are not interrupted; close does not establish quiescence. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_close(CATTRLIB_DISPATCHER* dispatcher);

/** Release the resources associated with a closed dispatcher after all submissions, waits, polls and other accesses have returned. Failed creation or release permits a subsequent free. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_free(CATTRLIB_DISPATCHER* dispatcher);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTRLIB_H */
