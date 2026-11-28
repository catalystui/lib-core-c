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

/** A bounded flow of execution which transforms input into output. */
typedef struct CATTRLIB_PROCESS {
    CATMMLIB_POINTERW state;
} CATTRLIB_PROCESS;

/** A path of execution contained within a process. */
typedef struct CATTRLIB_THREAD {
    CATTRLIB_PROCESS* process;
    CATTRLIB_POINTERX entry;
    CATMMLIB_POINTERW arguments;
    CATMMLIB_POINTERW state;
} CATTRLIB_THREAD;

/** A mechanism which selects work for execution on an associated thread. Owns its internal storage; borrows the thread until free. Do not copy an initialized dispatcher or mutate its work directly. */
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

/** Establish a thread within the specified process. Entry must reference a void (CATMMLIB_POINTERW) function. */
CATTRLIB_API CATALYST_RESULT cattr_thread_create(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process, CATTRLIB_POINTERX entry, CATMMLIB_POINTERW arguments);

/** Retrieve the thread associated with the current execution. */
CATTRLIB_API CATALYST_RESULT cattr_thread_current(CATTRLIB_THREAD* thread, CATTRLIB_PROCESS* process);

/** Begin execution of a thread. */
CATTRLIB_API CATALYST_RESULT cattr_thread_start(CATTRLIB_THREAD* thread);

/** Wait for a thread to complete execution. */
CATTRLIB_API CATALYST_RESULT cattr_thread_wait(CATTRLIB_THREAD* thread);

/** Determine whether two threads represent the same instance of execution. */
CATTRLIB_API CATALYST_BOOL cattr_thread_equals(const CATTRLIB_THREAD* thread1, const CATTRLIB_THREAD* thread2);

/** Release the resources associated with a thread. */
CATTRLIB_API CATALYST_RESULT cattr_thread_free(CATTRLIB_THREAD* thread);

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/** Establish a dispatcher with nonzero capacity and a dispatch rule. A null rule selects FIFO order. The rule receives a nonempty Set of opaque submission references and returns one member (not its storage address). */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_create(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_THREAD* thread, CATMMLIB_POINTER (*rule)(const CATRLLIB_SET*), CATALYST_NUINT capacity);

/** Submit an independent invocation for execution. Operation must reference a void (CATMMLIB_POINTERW) function. On the associated thread, awaited submission is invalid and asynchronous admission is nonblocking. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_submit(CATTRLIB_DISPATCHER* dispatcher, CATTRLIB_POINTERX operation, CATMMLIB_POINTERW arguments, CATALYST_BOOL wait, CATALYST_NUINT timeout);

/** Inspect a live submission reference during a dispatch rule. Outputs its executable reference and borrowed arguments. */
CATTRLIB_API void cattr_dispatcher_inspect(CATMMLIB_POINTER submission, CATTRLIB_POINTERX* operation, CATMMLIB_POINTERW* arguments);

/** Retrieve the number of submissions awaiting execution. */
CATTRLIB_API CATALYST_NUINT cattr_dispatcher_count(const CATTRLIB_DISPATCHER* dispatcher);

/** Wait until work becomes available for execution. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_wait(CATTRLIB_DISPATCHER* dispatcher, CATALYST_NUINT timeout);

/** Select and execute pending work according to the dispatch rule. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_poll(CATTRLIB_DISPATCHER* dispatcher);

/** Close admission, cancel pending submissions and awaken waiters. Running callbacks are not interrupted; close does not establish quiescence. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_close(CATTRLIB_DISPATCHER* dispatcher);

/** Release the resources associated with a closed dispatcher after all submissions, waits, polls and other accesses have returned. Failed creation or release permits a subsequent free. */
CATTRLIB_API CATALYST_RESULT cattr_dispatcher_free(CATTRLIB_DISPATCHER* dispatcher);

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* CATTRLIB_H */
