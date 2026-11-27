#include <cstdio>
#include <cstdlib>
#include "cattrlib.hpp"

// Native-C reference and Catalyst comparison. All platform calls are confined
// to the reference and the common wall-clock harness, never the Catalyst path.
#if TARGET_PLATFORM_WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #undef TRUE
    #undef FALSE
#elif TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
    #include <pthread.h>
    #include <unistd.h>
    #include <time.h>
#endif

using namespace catalyst;

#if (TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN) && CATALYST_SUPPORTS_32BIT

typedef CATALYST_BOOL Flag;
typedef CATALYST_BYTE Octet;
typedef CATALYST_UINT Ticks;
typedef CATALYST_STATUS_CODE Status;
typedef void (*Procedure)(POINTERW);
typedef POINTERW (*Function)(POINTERW);

static void require(Flag condition, const char* message) {
    if (condition) return;
    std::printf("\nFAIL: %s\n", message);
    std::fflush(stdout);
    std::exit(1);
}

static void expect(Status actual, Status expected) {
    if (actual == expected) return;
    std::printf("\nFAIL: expected status 0x%02X, received 0x%02X\n",
        (unsigned int) expected, (unsigned int) actual);
    std::exit(1);
}

// ------------------------------------------------------------
// Independent native-C baseline: direct thread and condition
// operations, a single bounded invocation slot, and caller
// storage for native handles. No calls into Catalyst libraries.
// ------------------------------------------------------------
#if TARGET_PLATFORM_WIN32
typedef DWORD NativePid;
struct NativeMonitor {
    SRWLOCK lock;
    CONDITION_VARIABLE changed;
};
#else
typedef pid_t NativePid;
struct NativeMonitor {
    pthread_mutex_t lock;
    pthread_cond_t changed;
};
#endif

static void native_monitor_init(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    InitializeSRWLock(&monitor->lock);
    InitializeConditionVariable(&monitor->changed);
#else
    require(pthread_mutex_init(&monitor->lock, 0) == 0 ? TRUE : FALSE, "native mutex initialization");
    require(pthread_cond_init(&monitor->changed, 0) == 0 ? TRUE : FALSE, "native condition initialization");
#endif
}

static void native_monitor_free(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    (void) monitor;
#else
    require(pthread_cond_destroy(&monitor->changed) == 0 ? TRUE : FALSE, "native condition destruction");
    require(pthread_mutex_destroy(&monitor->lock) == 0 ? TRUE : FALSE, "native mutex destruction");
#endif
}

static void native_lock(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    AcquireSRWLockExclusive(&monitor->lock);
#else
    require(pthread_mutex_lock(&monitor->lock) == 0 ? TRUE : FALSE, "native mutex lock");
#endif
}

static void native_unlock(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    ReleaseSRWLockExclusive(&monitor->lock);
#else
    require(pthread_mutex_unlock(&monitor->lock) == 0 ? TRUE : FALSE, "native mutex unlock");
#endif
}

static void native_notify(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    WakeAllConditionVariable(&monitor->changed);
#else
    require(pthread_cond_broadcast(&monitor->changed) == 0 ? TRUE : FALSE, "native condition broadcast");
#endif
}

static void native_wait(NativeMonitor* monitor) {
#if TARGET_PLATFORM_WIN32
    require(SleepConditionVariableSRW(&monitor->changed, &monitor->lock, INFINITE, 0) ? TRUE : FALSE, "native condition wait");
#else
    require(pthread_cond_wait(&monitor->changed, &monitor->lock) == 0 ? TRUE : FALSE, "native condition wait");
#endif
}

struct NativeProcess {
    volatile NativePid id;
};

struct NativeThread {
#if TARGET_PLATFORM_WIN32
    HANDLE handle;
    DWORD id;
    SRWLOCK gate;
#else
    pthread_t handle;
    pthread_mutex_t gate;
#endif
    Procedure procedure;
    POINTERW arguments;
    Flag captured;
    Flag started;
};

static Status native_process_current(NativeProcess* process) {
#if TARGET_PLATFORM_WIN32
    process->id = GetCurrentProcessId();
#else
    process->id = getpid();
#endif
    return STATUS_CODE_SUCCESS;
}

static Status native_thread_current(NativeThread* thread) {
    thread->started = FALSE;
    thread->captured = TRUE;
#if TARGET_PLATFORM_WIN32
    thread->id = GetCurrentThreadId();
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
            &thread->handle, 0, FALSE, DUPLICATE_SAME_ACCESS)) return STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
#else
    thread->handle = pthread_self();
#endif
    return STATUS_CODE_SUCCESS;
}

static Status native_thread_create(NativeThread* thread, Procedure procedure, POINTERW arguments) {
    thread->captured = FALSE;
    thread->started = FALSE;
    thread->procedure = procedure;
    thread->arguments = arguments;
    return STATUS_CODE_SUCCESS;
}

#if TARGET_PLATFORM_WIN32
static DWORD WINAPI native_thread_entry(LPVOID value) {
    NativeThread* thread = (NativeThread*) value;
    AcquireSRWLockExclusive(&thread->gate);
    ReleaseSRWLockExclusive(&thread->gate);
    thread->procedure(thread->arguments);
    return 0;
}
#else
static void* native_thread_entry(void* value) {
    NativeThread* thread = (NativeThread*) value;
    require(pthread_mutex_lock(&thread->gate) == 0 ? TRUE : FALSE, "native start gate lock");
    require(pthread_mutex_unlock(&thread->gate) == 0 ? TRUE : FALSE, "native start gate unlock");
    thread->procedure(thread->arguments);
    return 0;
}
#endif

static Status native_thread_start(NativeThread* thread) {
#if TARGET_PLATFORM_WIN32
    InitializeSRWLock(&thread->gate);
    AcquireSRWLockExclusive(&thread->gate);
    thread->started = TRUE;
    thread->handle = CreateThread(0, 0, native_thread_entry, thread, 0, &thread->id);
    ReleaseSRWLockExclusive(&thread->gate);
    if (thread->handle == 0) return STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
#else
    if (pthread_mutex_init(&thread->gate, 0) != 0) return STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
    require(pthread_mutex_lock(&thread->gate) == 0 ? TRUE : FALSE, "native start gate lock");
    thread->started = TRUE;
    int error = pthread_create(&thread->handle, 0, native_thread_entry, thread);
    require(pthread_mutex_unlock(&thread->gate) == 0 ? TRUE : FALSE, "native start gate unlock");
    if (error != 0) return STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
#endif
    return STATUS_CODE_SUCCESS;
}

static Status native_thread_join(NativeThread* thread) {
#if TARGET_PLATFORM_WIN32
    return WaitForSingleObject(thread->handle, INFINITE) == WAIT_OBJECT_0
        ? STATUS_CODE_SUCCESS : STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
#else
    return pthread_join(thread->handle, 0) == 0 ? STATUS_CODE_SUCCESS : STATUS_CODE_ERROR_DEPENDENCY_FAILURE;
#endif
}

static Status native_thread_free(NativeThread* thread) {
#if TARGET_PLATFORM_WIN32
    if (thread->started || thread->captured) {
        if (!CloseHandle(thread->handle)) return STATUS_CODE_ERROR_INVALID_STATE;
    }
#else
    if (thread->started && pthread_mutex_destroy(&thread->gate) != 0) return STATUS_CODE_ERROR_INVALID_STATE;
#endif
    thread->started = FALSE;
    thread->captured = FALSE;
    return STATUS_CODE_SUCCESS;
}

static Flag native_thread_equals(const NativeThread* first, const NativeThread* second) {
#if TARGET_PLATFORM_WIN32
    return first->id == second->id && WaitForSingleObject(first->handle, 0) == WAIT_TIMEOUT
        && WaitForSingleObject(second->handle, 0) == WAIT_TIMEOUT ? TRUE : FALSE;
#else
    return pthread_equal(first->handle, second->handle) ? TRUE : FALSE;
#endif
}

static Flag native_is_current(const NativeThread* thread) {
    if (!thread->started && !thread->captured) return FALSE;
#if TARGET_PLATFORM_WIN32
    return thread->id == GetCurrentThreadId() && WaitForSingleObject(thread->handle, 0) == WAIT_TIMEOUT ? TRUE : FALSE;
#else
    return pthread_equal(thread->handle, pthread_self()) ? TRUE : FALSE;
#endif
}

static const Octet SLOT_FREE = 0;
static const Octet SLOT_PENDING = 1;
static const Octet SLOT_RUNNING = 2;
static const Octet SLOT_DONE = 3;
static const Octet SLOT_CANCELLED = 4;

struct NativeDispatcher {
    NativeThread* thread;
    NativeMonitor monitor;
    Procedure procedure;
    Function function;
    POINTERW arguments;
    POINTERW output;
    Octet stage;
    Flag awaited;
    Flag closed;
};

static Status native_dispatcher_create(NativeDispatcher* dispatcher, NativeThread* thread) {
    native_monitor_init(&dispatcher->monitor);
    dispatcher->thread = thread;
    dispatcher->stage = SLOT_FREE;
    dispatcher->closed = FALSE;
    return STATUS_CODE_SUCCESS;
}

static Status native_submit(NativeDispatcher* dispatcher, Procedure procedure, Function function,
        POINTERW arguments, Flag wait, NUINT timeout, POINTERW* output) {
    if (output != 0) *output = 0;
    native_lock(&dispatcher->monitor);
    if (dispatcher->closed) {
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_ERROR_INVALID_STATE;
    }
    if (native_is_current(dispatcher->thread)) {
        native_unlock(&dispatcher->monitor);
        if (function != 0) {
            POINTERW value = function(arguments);
            if (output != 0) *output = value;
        } else {
            procedure(arguments);
        }
        return STATUS_CODE_SUCCESS;
    }
    // Workloads use either zero or infinite timeout, with one bounded slot.
    for (;;) {
        if (dispatcher->closed) {
            native_unlock(&dispatcher->monitor);
            return STATUS_CODE_ERROR_INTERRUPTED;
        }
        if (dispatcher->stage == SLOT_PENDING &&
            dispatcher->procedure == procedure && dispatcher->function == function) {
            native_unlock(&dispatcher->monitor);
            return wait ? STATUS_CODE_ERROR_INVALID_STATE : STATUS_CODE_SUCCESS_NOOP;
        }
        if (dispatcher->stage == SLOT_FREE) break;
        if (timeout == 0) {
            native_unlock(&dispatcher->monitor);
            return STATUS_CODE_ERROR_TIMEOUT;
        }
        native_wait(&dispatcher->monitor);
    }
    dispatcher->procedure = procedure;
    dispatcher->function = function;
    dispatcher->arguments = arguments;
    dispatcher->output = 0;
    dispatcher->awaited = wait;
    dispatcher->stage = SLOT_PENDING;
    native_notify(&dispatcher->monitor);
    if (!wait) {
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_SUCCESS;
    }
    if (timeout == 0) {
        dispatcher->stage = SLOT_FREE;
        native_notify(&dispatcher->monitor);
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_ERROR_TIMEOUT;
    }
    while (dispatcher->stage == SLOT_PENDING || dispatcher->stage == SLOT_RUNNING) native_wait(&dispatcher->monitor);
    Flag cancelled = dispatcher->stage == SLOT_CANCELLED ? TRUE : FALSE;
    if (!cancelled && output != 0) *output = dispatcher->output;
    dispatcher->stage = SLOT_FREE;
    native_notify(&dispatcher->monitor);
    native_unlock(&dispatcher->monitor);
    return cancelled ? STATUS_CODE_ERROR_INTERRUPTED : STATUS_CODE_SUCCESS;
}

static NUINT native_dispatcher_count(NativeDispatcher* dispatcher) {
    native_lock(&dispatcher->monitor);
    NUINT count = dispatcher->stage == SLOT_PENDING ? (NUINT) 1 : (NUINT) 0;
    native_unlock(&dispatcher->monitor);
    return count;
}

static Status native_dispatcher_wait(NativeDispatcher* dispatcher, NUINT timeout) {
    if (!native_is_current(dispatcher->thread)) return STATUS_CODE_ERROR_INVALID_STATE;
    native_lock(&dispatcher->monitor);
    while (!dispatcher->closed && dispatcher->stage != SLOT_PENDING) {
        if (timeout == 0) {
            native_unlock(&dispatcher->monitor);
            return STATUS_CODE_ERROR_TIMEOUT;
        }
        native_wait(&dispatcher->monitor);
    }
    Flag closed = dispatcher->closed;
    native_unlock(&dispatcher->monitor);
    return closed ? STATUS_CODE_ERROR_INTERRUPTED : STATUS_CODE_SUCCESS;
}

static Status native_dispatcher_poll(NativeDispatcher* dispatcher) {
    if (!native_is_current(dispatcher->thread)) return STATUS_CODE_ERROR_INVALID_STATE;
    native_lock(&dispatcher->monitor);
    if (dispatcher->closed) {
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_ERROR_INTERRUPTED;
    }
    if (dispatcher->stage != SLOT_PENDING) {
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_SUCCESS_NOOP;
    }
    dispatcher->stage = SLOT_RUNNING;
    native_unlock(&dispatcher->monitor);
    POINTERW output = 0;
    if (dispatcher->function != 0) output = dispatcher->function(dispatcher->arguments);
    else dispatcher->procedure(dispatcher->arguments);
    native_lock(&dispatcher->monitor);
    dispatcher->output = output;
    dispatcher->stage = dispatcher->awaited ? SLOT_DONE : SLOT_FREE;
    native_notify(&dispatcher->monitor);
    native_unlock(&dispatcher->monitor);
    return STATUS_CODE_SUCCESS;
}

static Status native_dispatcher_close(NativeDispatcher* dispatcher) {
    native_lock(&dispatcher->monitor);
    if (dispatcher->closed) {
        native_unlock(&dispatcher->monitor);
        return STATUS_CODE_SUCCESS_NOOP;
    }
    dispatcher->closed = TRUE;
    if (dispatcher->stage == SLOT_PENDING) dispatcher->stage = dispatcher->awaited ? SLOT_CANCELLED : SLOT_FREE;
    native_notify(&dispatcher->monitor);
    native_unlock(&dispatcher->monitor);
    return STATUS_CODE_SUCCESS;
}

// The adapters expose the same operations to the measurement harness.
// The native path returns the outcome only, not Catalyst diagnostic bytes.
struct Native {
    typedef NativeProcess Process;
    typedef NativeThread Thread;
    typedef NativeDispatcher Dispatcher;
    static Status capture_process(Process* p) { return native_process_current(p); }
    static Flag equal_process(const Process* a, const Process* b) { return a->id == b->id ? TRUE : FALSE; }
    static Status release_process(Process*) { return STATUS_CODE_SUCCESS; }
    static Status capture_thread(Thread* t, Process*) { return native_thread_current(t); }
    static Status prepare_thread(Thread* t, Process*, Procedure p, POINTERW a) { return native_thread_create(t, p, a); }
    static Status start_thread(Thread* t) { return native_thread_start(t); }
    static Status join_thread(Thread* t) { return native_thread_join(t); }
    static Flag equal_thread(const Thread* a, const Thread* b) { return native_thread_equals(a, b); }
    static Status release_thread(Thread* t) { return native_thread_free(t); }
    static Status create_dispatcher(Dispatcher* d, Thread* t) { return native_dispatcher_create(d, t); }
    static Status submit_procedure(Dispatcher* d, Procedure p, POINTERW a, Flag w, NUINT t) { return native_submit(d, p, 0, a, w, t, 0); }
    static Status submit_function(Dispatcher* d, Function f, POINTERW a, Flag w, NUINT t, POINTERW* o) { return native_submit(d, 0, f, a, w, t, o); }
    static NUINT count(Dispatcher* d) { return native_dispatcher_count(d); }
    static Status wait(Dispatcher* d, NUINT t) { return native_dispatcher_wait(d, t); }
    static Status poll(Dispatcher* d) { return native_dispatcher_poll(d); }
    static Status close(Dispatcher* d) { return native_dispatcher_close(d); }
    static Status release_dispatcher(Dispatcher* d) { native_monitor_free(&d->monitor); return STATUS_CODE_SUCCESS; }
};

struct Catalyst {
    typedef PROCESS Process;
    typedef THREAD Thread;
    typedef DISPATCHER Dispatcher;
    static Status capture_process(Process* p) { return process_current(p).status; }
    static Flag equal_process(const Process* a, const Process* b) { return process_equals(a, b); }
    static Status release_process(Process* p) { return process_free(p).status; }
    static Status capture_thread(Thread* t, Process* p) { return thread_current(t, p).status; }
    static Status prepare_thread(Thread* t, Process* p, Procedure f, POINTERW a) { return thread_create(t, p, (PROCEDURE) f, a).status; }
    static Status start_thread(Thread* t) { return thread_start(t).status; }
    static Status join_thread(Thread* t) { return thread_wait(t).status; }
    static Flag equal_thread(const Thread* a, const Thread* b) { return thread_equals(a, b); }
    static Status release_thread(Thread* t) { return thread_free(t).status; }
    static Status create_dispatcher(Dispatcher* d, Thread* t) { return dispatcher_create(d, t, 0, (NUINT) 1).status; }
    static Status submit_procedure(Dispatcher* d, Procedure p, POINTERW a, Flag w, NUINT t) { return dispatcher_submit_procedure(d, (PROCEDURE) p, a, w, t).status; }
    static Status submit_function(Dispatcher* d, Function f, POINTERW a, Flag w, NUINT t, POINTERW* o) { return dispatcher_submit_function(d, (FUNCTION) f, a, w, t, o).status; }
    static NUINT count(Dispatcher* d) { return dispatcher_count(d); }
    static Status wait(Dispatcher* d, NUINT t) { return dispatcher_wait(d, t).status; }
    static Status poll(Dispatcher* d) { return dispatcher_poll(d).status; }
    static Status close(Dispatcher* d) { return dispatcher_close(d).status; }
    static Status release_dispatcher(Dispatcher* d) { return dispatcher_free(d).status; }
};

// ------------------------------------------------------------
// Monotonic wall-clock timing. CPU clocks cannot measure blocking
// dispatch latency. Native time fields stay at this boundary.
// Store elapsed nanoseconds in an optional Catalyst UINT; samples
// longer than four seconds are unavailable, never silently wrapped.
// ------------------------------------------------------------
#if TARGET_PLATFORM_WIN32
typedef LARGE_INTEGER Stamp;
static LARGE_INTEGER frequency;
static Flag read_clock(Stamp* stamp) {
    return QueryPerformanceCounter(stamp) ? TRUE : FALSE;
}

static Flag elapsed(Stamp start, Stamp finish, Ticks* ticks) {
    if (finish.QuadPart < start.QuadPart || frequency.QuadPart <= 0 ||
        frequency.QuadPart > 4000000000UL) return FALSE;
    LONGLONG delta = finish.QuadPart - start.QuadPart;
    LONGLONG seconds = delta / frequency.QuadPart;
    if (seconds > 3) return FALSE;
    *ticks = (Ticks) seconds * (Ticks) 1000000000
        + (Ticks) ((delta % frequency.QuadPart) * 1000000000UL / frequency.QuadPart);
    return TRUE;
}
#else
typedef struct timespec Stamp;
static Flag read_clock(Stamp* stamp) {
    return clock_gettime(CLOCK_MONOTONIC, stamp) == 0 ? TRUE : FALSE;
}

static Flag elapsed(Stamp start, Stamp finish, Ticks* ticks) {
    if (finish.tv_sec < start.tv_sec ||
        (finish.tv_sec == start.tv_sec && finish.tv_nsec < start.tv_nsec)) return FALSE;
    time_t seconds = finish.tv_sec - start.tv_sec;
    long nanoseconds = finish.tv_nsec - start.tv_nsec;
    if (nanoseconds < 0) {
        --seconds;
        nanoseconds += 1000000000L;
    }
    if (seconds > 3) return FALSE;
    *ticks = (Ticks) seconds * (Ticks) 1000000000 + (Ticks) nanoseconds;
    return TRUE;
}
#endif

// ------------------------------------------------------------
// Matched workloads. Persistent fixtures are established outside
// the timed region, except for the lifecycle being measured.
// ------------------------------------------------------------
static void touch(POINTERW arguments) {
    volatile NUINT* value = (volatile NUINT*) arguments;
    *value = (NUINT) (*value + (NUINT) 1);
}

static POINTERW touch_function(POINTERW arguments) {
    touch(arguments);
    return arguments;
}

template<class B>
static void worker(POINTERW arguments) {
    typename B::Dispatcher* dispatcher = (typename B::Dispatcher*) arguments;
    for (;;) {
        Status status = B::wait(dispatcher, CATALYST_NUINT_MAX);
        if (status == STATUS_CODE_ERROR_INTERRUPTED) return;
        expect(status, STATUS_CODE_SUCCESS);
        status = B::poll(dispatcher);
        if (status == STATUS_CODE_ERROR_INTERRUPTED) return;
        expect(status, STATUS_CODE_SUCCESS);
    }
}

enum Workload {
    PROCESS_CAPTURE, PROCESS_EQUAL,
    THREAD_CAPTURE, THREAD_EQUAL, THREAD_PREPARE, THREAD_EXECUTE,
    DISPATCHER_LIFECYCLE, EMPTY_COUNT, EMPTY_POLL, EMPTY_WAIT, IMMEDIATE_PROCEDURE,
    IMMEDIATE_FUNCTION, CLOSED_NOOP,
    PENDING_COUNT, DUPLICATE, PENDING_TIMEOUT, FULL_TIMEOUT,
    CROSS_PROCEDURE, CROSS_FUNCTION, ASYNC_PROCEDURE, ASYNC_FUNCTION
};

struct Benchmark {
    const char* name;
    Workload workload;
};

static const Benchmark benchmarks[] = {
    { "process_current / free", PROCESS_CAPTURE },
    { "process_equals (same process)", PROCESS_EQUAL },
    { "thread_current / free", THREAD_CAPTURE },
    { "thread_equals (same thread)", THREAD_EQUAL },
    { "thread_create / free", THREAD_PREPARE },
    { "thread_create / start / wait / free", THREAD_EXECUTE },
    { "dispatcher_create / close / free", DISPATCHER_LIFECYCLE },
    { "dispatcher_count (empty)", EMPTY_COUNT },
    { "dispatcher_poll (empty)", EMPTY_POLL },
    { "dispatcher_wait (zero timeout)", EMPTY_WAIT },
    { "submit_procedure (same thread)", IMMEDIATE_PROCEDURE },
    { "submit_function (same thread)", IMMEDIATE_FUNCTION },
    { "dispatcher_close (already closed)", CLOSED_NOOP },
    { "dispatcher_count (pending)", PENDING_COUNT },
    { "submit_procedure (duplicate)", DUPLICATE },
    { "submit_function (pending timeout)", PENDING_TIMEOUT },
    { "submit_function (full timeout)", FULL_TIMEOUT },
    { "submit_procedure (cross-thread)", CROSS_PROCEDURE },
    { "submit_function (cross-thread)", CROSS_FUNCTION },
    { "async procedure + sync function", ASYNC_PROCEDURE },
    { "async function + sync procedure", ASYNC_FUNCTION }
};

template<class B>
static Flag measure(Workload kind, NUINT calls, Ticks* ticks) {
    typename B::Process process, other_process;
    typename B::Thread current, other_thread, target;
    typename B::Dispatcher dispatcher;
    NUINT observed = 0;
    POINTERW output = 0;
    Flag local = kind >= EMPTY_COUNT && kind <= CLOSED_NOOP ? TRUE : FALSE;
    Flag pending = kind >= PENDING_COUNT && kind <= FULL_TIMEOUT ? TRUE : FALSE;
    Flag remote = kind >= CROSS_PROCEDURE ? TRUE : FALSE;

    expect(B::capture_process(&process), STATUS_CODE_SUCCESS);
    expect(B::capture_process(&other_process), STATUS_CODE_SUCCESS);
    expect(B::capture_thread(&current, &process), STATUS_CODE_SUCCESS);
    expect(B::capture_thread(&other_thread, &process), STATUS_CODE_SUCCESS);

    if (local) {
        expect(B::create_dispatcher(&dispatcher, &current), STATUS_CODE_SUCCESS);
        if (kind == CLOSED_NOOP) expect(B::close(&dispatcher), STATUS_CODE_SUCCESS);
    }
    if (pending || remote) {
        expect(B::prepare_thread(&target, &process, worker<B>, &dispatcher), STATUS_CODE_SUCCESS);
        expect(B::create_dispatcher(&dispatcher, &target), STATUS_CODE_SUCCESS);
        if (remote) expect(B::start_thread(&target), STATUS_CODE_SUCCESS);
        if (pending && kind != PENDING_TIMEOUT) {
            expect(B::submit_procedure(&dispatcher, touch, &observed, FALSE, 0), STATUS_CODE_SUCCESS);
        }
    }

    Stamp start, finish;
    Flag timing = read_clock(&start);
    for (NUINT i = 0; i < calls; ++i) {
        switch (kind) {
            case PROCESS_CAPTURE: {
                typename B::Process temporary;
                expect(B::capture_process(&temporary), STATUS_CODE_SUCCESS);
                expect(B::release_process(&temporary), STATUS_CODE_SUCCESS);
                break;
            }
            case PROCESS_EQUAL:
                require(B::equal_process(&process, &other_process), "process identity");
                break;
            case THREAD_CAPTURE: {
                typename B::Thread temporary;
                expect(B::capture_thread(&temporary, &process), STATUS_CODE_SUCCESS);
                expect(B::release_thread(&temporary), STATUS_CODE_SUCCESS);
                break;
            }
            case THREAD_EQUAL:
                require(B::equal_thread(&current, &other_thread), "thread identity");
                break;
            case THREAD_PREPARE:
            case THREAD_EXECUTE: {
                typename B::Thread temporary;
                expect(B::prepare_thread(&temporary, &process, touch, &observed), STATUS_CODE_SUCCESS);
                if (kind == THREAD_EXECUTE) {
                    expect(B::start_thread(&temporary), STATUS_CODE_SUCCESS);
                    expect(B::join_thread(&temporary), STATUS_CODE_SUCCESS);
                }
                expect(B::release_thread(&temporary), STATUS_CODE_SUCCESS);
                break;
            }
            case DISPATCHER_LIFECYCLE:
                expect(B::create_dispatcher(&dispatcher, &current), STATUS_CODE_SUCCESS);
                expect(B::close(&dispatcher), STATUS_CODE_SUCCESS);
                expect(B::release_dispatcher(&dispatcher), STATUS_CODE_SUCCESS);
                break;
            case EMPTY_COUNT:
                require(B::count(&dispatcher) == 0 ? TRUE : FALSE, "empty pending count");
                break;
            case EMPTY_POLL:
                expect(B::poll(&dispatcher), STATUS_CODE_SUCCESS_NOOP);
                break;
            case EMPTY_WAIT:
                expect(B::wait(&dispatcher, 0), STATUS_CODE_ERROR_TIMEOUT);
                break;
            case IMMEDIATE_PROCEDURE:
            case CROSS_PROCEDURE:
                expect(B::submit_procedure(&dispatcher, touch, &observed, TRUE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                break;
            case IMMEDIATE_FUNCTION:
            case CROSS_FUNCTION:
                output = 0;
                expect(B::submit_function(&dispatcher, touch_function, &observed, TRUE, CATALYST_NUINT_MAX, &output), STATUS_CODE_SUCCESS);
                require(output == &observed ? TRUE : FALSE, "function output");
                break;
            case CLOSED_NOOP:
                expect(B::close(&dispatcher), STATUS_CODE_SUCCESS_NOOP);
                break;
            case PENDING_COUNT:
                require(B::count(&dispatcher) == 1 ? TRUE : FALSE, "pending count");
                break;
            case DUPLICATE:
                expect(B::submit_procedure(&dispatcher, touch, &observed, FALSE, 0), STATUS_CODE_SUCCESS_NOOP);
                break;
            case PENDING_TIMEOUT:
            case FULL_TIMEOUT:
                output = &observed;
                expect(B::submit_function(&dispatcher, touch_function, &observed, TRUE, 0, &output), STATUS_CODE_ERROR_TIMEOUT);
                require(output == 0 ? TRUE : FALSE, "timeout output cleared");
                break;
            case ASYNC_PROCEDURE:
                expect(B::submit_procedure(&dispatcher, touch, &observed, FALSE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                output = 0;
                expect(B::submit_function(&dispatcher, touch_function, &observed, TRUE, CATALYST_NUINT_MAX, &output), STATUS_CODE_SUCCESS);
                require(output == &observed ? TRUE : FALSE, "async procedure barrier output");
                break;
            case ASYNC_FUNCTION:
                expect(B::submit_function(&dispatcher, touch_function, &observed, FALSE, CATALYST_NUINT_MAX, 0), STATUS_CODE_SUCCESS);
                expect(B::submit_procedure(&dispatcher, touch, &observed, TRUE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                break;
        }
    }
    if (!read_clock(&finish)) timing = FALSE;
    if (timing) timing = elapsed(start, finish, ticks);

    if (pending) require(B::count(&dispatcher) == (kind == PENDING_TIMEOUT ? 0 : 1) ? TRUE : FALSE, "pending work preserved");
    if (local || pending || remote) {
        expect(B::close(&dispatcher), kind == CLOSED_NOOP ? STATUS_CODE_SUCCESS_NOOP : STATUS_CODE_SUCCESS);
        if (remote) expect(B::join_thread(&target), STATUS_CODE_SUCCESS);
        expect(B::release_dispatcher(&dispatcher), STATUS_CODE_SUCCESS);
        if (pending || remote) expect(B::release_thread(&target), STATUS_CODE_SUCCESS);
    }

    // Worker completion/join establishes visibility before checking payloads.
    NUINT expected = 0;
    if (kind == THREAD_EXECUTE || kind == IMMEDIATE_PROCEDURE || kind == IMMEDIATE_FUNCTION ||
        kind == CROSS_PROCEDURE || kind == CROSS_FUNCTION) expected = calls;
    if (kind == ASYNC_PROCEDURE || kind == ASYNC_FUNCTION) expected = (NUINT) (calls + calls);
    require(observed == expected ? TRUE : FALSE, "callback execution count");
    expect(B::release_thread(&other_thread), STATUS_CODE_SUCCESS);
    expect(B::release_thread(&current), STATUS_CODE_SUCCESS);
    expect(B::release_process(&other_process), STATUS_CODE_SUCCESS);
    expect(B::release_process(&process), STATUS_CODE_SUCCESS);
    return timing;
}

// ------------------------------------------------------------
// Paired calibration, median of three alternating samples, and
// integer reporting. No floating-point requirement. Each measured
// batch uses identical operation counts and consumes all outcomes.
// ------------------------------------------------------------
static Ticks median(Ticks a, Ticks b, Ticks c) {
    if (a > b) { Ticks temp = a; a = b; b = temp; }
    if (b > c) { Ticks temp = b; b = c; c = temp; }
    return a > b ? a : b;
}

static const Ticks minimum_sample = (Ticks) 1000000;
static const Ticks target_sample = (Ticks) 5000000;
static const Ticks maximum_sample = (Ticks) 50000000;

#if TARGET_SIZEOF_VOID_P >= 4
static const NUINT maximum_calls = (NUINT) 1048576;
#elif TARGET_SIZEOF_VOID_P >= 2
static const NUINT maximum_calls = (NUINT) 16384;
#else
static const NUINT maximum_calls = (NUINT) 64;
#endif

struct Summary {
    Octet faster, similar, slower, review, short_samples, unavailable;
};

static Flag pair(Workload kind, NUINT calls, Flag reverse, Ticks* native, Ticks* comparison) {
    Flag first, second;
    if (reverse) {
        first = measure<Catalyst>(kind, calls, comparison);
        second = measure<Native>(kind, calls, native);
    } else {
        first = measure<Native>(kind, calls, native);
        second = measure<Catalyst>(kind, calls, comparison);
    }
    return first && second ? TRUE : FALSE;
}

static void print_time(Ticks ticks, NUINT calls) {
    Ticks divisor = (Ticks) calls;
    Ticks whole = ticks / divisor;
    Ticks fraction = (Ticks) (((ticks % divisor) * (Ticks) 1000) / divisor);
    if (whole == 0 && fraction == 0) std::printf(" %14s", "<0.001 ns");
    else std::printf(" %7lu.%03lu ns", (unsigned long) whole, (unsigned long) fraction);
}

// Floor 1000 * delta / baseline (tenths of a percent), without overflow.
static Ticks percentage(Ticks delta, Ticks baseline) {
    Ticks whole = delta / baseline;
    if (whole > (Ticks) 999) return (Ticks) 1000000;
    Ticks remainder = delta % baseline;
    Ticks accumulated = 0, fraction = 0;
    for (Ticks i = 0; i < (Ticks) 1000; ++i) {
        if (accumulated >= baseline - remainder) {
            accumulated -= baseline - remainder;
            ++fraction;
        } else {
            accumulated += remainder;
        }
    }
    return whole * (Ticks) 1000 + fraction;
}

static void benchmark(const Benchmark* entry, Summary* summary) {
    Ticks native = 0, comparison = 0;
    NUINT calls = (NUINT) 16;
    for (;;) {
        if (!pair(entry->workload, calls, FALSE, &native, &comparison)) {
            std::printf("  %-36s %14s %14s %12s  Timing unavailable\n", entry->name, "-", "-", "-");
            ++summary->unavailable;
            return;
        }
        if ((native >= target_sample && comparison >= target_sample) ||
            native >= maximum_sample || comparison >= maximum_sample ||
            calls >= maximum_calls) break;
        calls = (NUINT) (calls * (NUINT) 2);
    }
    Ticks natives[3], comparisons[3];
    for (Octet sample = 0; sample < (Octet) 3; ++sample) {
        if (!pair(entry->workload, calls, sample == (Octet) 1 ? TRUE : FALSE,
                &natives[sample], &comparisons[sample])) {
            std::printf("  %-36s %14s %14s %12s  Timing unavailable\n", entry->name, "-", "-", "-");
            ++summary->unavailable;
            return;
        }
    }
    native = median(natives[0], natives[1], natives[2]);
    comparison = median(comparisons[0], comparisons[1], comparisons[2]);
    std::printf("  %-36s", entry->name);
    print_time(native, calls);
    print_time(comparison, calls);
    Ticks difference = 0;
    if (native != 0) {
        difference = percentage(comparison >= native ? comparison - native : native - comparison, native);
        char label[20];
        if (difference >= (Ticks) 1000000) std::printf(" %12s", ">99999.9%");
        else {
            std::sprintf(label, "%c%lu.%lu%%", comparison < native ? '-' : '+',
                (unsigned long) (difference / (Ticks) 10), (unsigned long) (difference % (Ticks) 10));
            std::printf(" %12s", label);
        }
    } else {
        std::printf(" %12s", "-");
    }
    if (native < minimum_sample || comparison < minimum_sample) {
        ++summary->short_samples;
        std::printf("  Short sample *");
    } else if (difference <= (Ticks) 50) {
        ++summary->similar;
        std::printf("  Comparable");
    } else if (comparison < native) {
        ++summary->faster;
        std::printf("  Faster");
    } else {
        ++summary->slower;
        if (difference >= (Ticks) 200) {
            ++summary->review;
            std::printf("  Review");
        } else {
            std::printf("  Slower");
        }
    }
    std::printf("  (%lu calls)\n", (unsigned long) calls);
    std::fflush(stdout);
}

#endif

int main() {
#if (TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN) && CATALYST_SUPPORTS_32BIT
#if TARGET_PLATFORM_WIN32
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        std::printf("CATTRLIB performance unavailable: no usable monotonic clock.\n");
        return 2;
    }
#endif
    std::printf("CATTRLIB Performance (Release recommended)\n");
    std::printf("Native-C / OS APIs are the baseline. Lower time is better.\n");
    std::printf("Wall-clock time per operation or named group: median of 3 paired samples.\n");
    std::printf("Native handles and a single bounded work slot use caller storage; CATTRLIB manages its own state.\n");
    std::printf("Both paths consume statuses, verify function outputs and verify completed callback counts.\n");
    std::printf("Cross-thread rows include scheduling, waits and callback execution. Async rows include the synchronous completion barrier.\n");
    std::printf("Timeout rows use zero timeout; custom-rule and multi-slot workloads are not measured.\n");
    std::printf("Comparable = within 5%%; Review = at least 20%% slower. Difference is relative to Native-C.\n");
    std::printf("\n  %-36s %14s %14s %12s  %s\n", "Workload", "Native-C", "Catalyst", "Difference", "Assessment");
    std::printf("  ------------------------------------------------------------------------------------------------\n");

    Summary summary = { 0, 0, 0, 0, 0, 0 };
    const NUINT count = (NUINT) (sizeof(benchmarks) / sizeof(benchmarks[0]));
    for (NUINT i = 0; i < count; ++i) benchmark(benchmarks + i, &summary);

    std::printf("\nNative process launch\n");
    std::printf("  process_create / start / wait / terminate: not implemented; no comparison.\n");
    std::printf("\nPerformance summary\n");
    std::printf("  Faster (>5%%):                  %3u\n", (unsigned int) summary.faster);
    std::printf("  Comparable (within 5%%):        %3u\n", (unsigned int) summary.similar);
    std::printf("  Slower (>5%%):                  %3u\n", (unsigned int) summary.slower);
    std::printf("    At least 20%% slower:          %3u (review)\n", (unsigned int) summary.review);
    std::printf("  Short samples (*):             %3u\n", (unsigned int) summary.short_samples);
    std::printf("  Timing unavailable:            %3u\n", (unsigned int) summary.unavailable);
    std::printf("  Unsupported process methods:     4\n");
    if (summary.short_samples != 0) std::printf("  * Less than 1 ms; excluded from performance classification.\n");
    std::printf("\nAll measured CATTRLIB performance comparisons verified.\n");
    return summary.unavailable != 0 ? 2 : 0;
#else
    std::printf("CATTRLIB performance unavailable: native threading backend or 32-bit timing representation not provided.\n");
    return 2;
#endif
}
