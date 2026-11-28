#include <cstdio>
#include <cstdlib>
#include "cattrlib.hpp"

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

#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN

typedef CATALYST_BOOL Flag;
typedef CATALYST_BYTE Octet;
typedef CATALYST_STATUS_CODE Status;
typedef void (*Entry)(POINTERW);

static void require(Flag condition, const char* message) {
    if (condition) return;
    std::printf("\nFAIL: %s\n", message);
    std::exit(1);
}

static void expect(Status actual, Status expected) {
    if (actual == expected) return;
    std::printf("\nFAIL: expected status 0x%02X, got 0x%02X\n",
        (unsigned int) expected, (unsigned int) actual);
    std::exit(1);
}

// ------------------------------------------------------------
// Native-C baseline. Caller-owned native handles and a bounded
// FIFO ring; no Catalyst allocation or collection calls.
// Only zero and infinite timeouts are needed by these workloads.
// One submitting caller and one associated execution thread.
// ------------------------------------------------------------
#if TARGET_PLATFORM_WIN32
typedef DWORD NativePid;
typedef DWORD NativeId;
struct NativeMonitor {
    SRWLOCK lock;
    CONDITION_VARIABLE changed;
};
#else
typedef pid_t NativePid;
typedef pthread_t NativeId;
struct NativeMonitor {
    pthread_mutex_t lock;
    pthread_cond_t changed;
};
#endif

static void monitor_init(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    InitializeSRWLock(&m->lock);
    InitializeConditionVariable(&m->changed);
#else
    require(pthread_mutex_init(&m->lock, 0) == 0, "native mutex init");
    require(pthread_cond_init(&m->changed, 0) == 0, "native condition init");
#endif
}

static void monitor_free(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    (void) m;
#else
    require(pthread_cond_destroy(&m->changed) == 0, "native condition free");
    require(pthread_mutex_destroy(&m->lock) == 0, "native mutex free");
#endif
}

static void monitor_lock(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    AcquireSRWLockExclusive(&m->lock);
#else
    require(pthread_mutex_lock(&m->lock) == 0, "native lock");
#endif
}

static void monitor_unlock(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    ReleaseSRWLockExclusive(&m->lock);
#else
    require(pthread_mutex_unlock(&m->lock) == 0, "native unlock");
#endif
}

static void monitor_notify(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    WakeAllConditionVariable(&m->changed);
#else
    require(pthread_cond_broadcast(&m->changed) == 0, "native notify");
#endif
}

static void monitor_wait(NativeMonitor* m) {
#if TARGET_PLATFORM_WIN32
    require(SleepConditionVariableSRW(&m->changed, &m->lock, INFINITE, 0) != 0, "native wait");
#else
    require(pthread_cond_wait(&m->changed, &m->lock) == 0, "native wait");
#endif
}

struct NativeProcess {
    NativePid id;
};

struct NativeThread {
    NativeId id;
#if TARGET_PLATFORM_WIN32
    HANDLE handle;
#endif
#if TARGET_PLATFORM_WIN32
    SRWLOCK gate;
#else
    pthread_mutex_t gate;
#endif
    Entry entry;
    POINTERW arguments;
    Flag started;
    Flag captured;
};

static NativeId native_current_id() {
#if TARGET_PLATFORM_WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

static Flag native_id_equals(NativeId first, NativeId second) {
#if TARGET_PLATFORM_WIN32
    return first == second ? TRUE : FALSE;
#else
    return pthread_equal(first, second) ? TRUE : FALSE;
#endif
}

static Flag native_is_current(const NativeThread* thread) {
    if (!thread->started && !thread->captured) return FALSE;
    return native_id_equals(thread->id, native_current_id());
}

#if TARGET_PLATFORM_WIN32
static DWORD WINAPI native_entry(LPVOID arguments) {
#else
static void* native_entry(void* arguments) {
#endif
    NativeThread* thread = (NativeThread*) arguments;
#if TARGET_PLATFORM_WIN32
    AcquireSRWLockExclusive(&thread->gate);
    ReleaseSRWLockExclusive(&thread->gate);
#else
    require(pthread_mutex_lock(&thread->gate) == 0, "native gate lock");
    require(pthread_mutex_unlock(&thread->gate) == 0, "native gate unlock");
#endif
    thread->entry(thread->arguments);
    return 0;
}

static const Octet native_max_capacity = 32;
static const Octet completion_pending = 0;
static const Octet completion_done = 1;
static const Octet completion_cancelled = 2;

struct NativeInvocation {
    Entry entry;
    POINTERW arguments;
    Octet* completion;
};

struct NativeDispatcher {
    NativeThread* thread;
    NativeMonitor monitor;
    NativeInvocation entries[native_max_capacity];
    NUINT capacity;
    NUINT head;
    NUINT length;
    NUINT occupied;
    Flag closed;
};

// Benchmark adapters normalize outcomes for identical verification.
// Native lifecycle uses caller storage, not managed Catalyst handles.
struct Native {
    typedef NativeProcess Process;
    typedef NativeThread Thread;
    typedef NativeDispatcher Dispatcher;

    static Status process_current(Process* p) {
#if TARGET_PLATFORM_WIN32
        p->id = GetCurrentProcessId();
#else
        p->id = getpid();
#endif
        return STATUS_CODE_SUCCESS;
    }
    static Flag process_equals(const Process* a, const Process* b) {
        // Read each identity on every iteration, even under Release IPO.
        const volatile NativePid* first = &a->id;
        const volatile NativePid* second = &b->id;
        return *first == *second ? TRUE : FALSE;
    }
    static Status process_free(Process*) { return STATUS_CODE_SUCCESS; }

    static Status thread_current(Thread* t, Process*) {
        t->id = native_current_id();
        t->started = FALSE;
        t->captured = TRUE;
        return STATUS_CODE_SUCCESS;
    }
    static Flag thread_equals(const Thread* a, const Thread* b) {
        const volatile NativeId* first = &a->id;
        const volatile NativeId* second = &b->id;
        return native_id_equals(*first, *second);
    }
    static Status thread_create(Thread* t, Process*, Entry entry, POINTERW args) {
        t->entry = entry;
        t->arguments = args;
        t->started = FALSE;
        t->captured = FALSE;
        return STATUS_CODE_SUCCESS;
    }
    static Status thread_start(Thread* t) {
        t->started = TRUE;
#if TARGET_PLATFORM_WIN32
        InitializeSRWLock(&t->gate);
        AcquireSRWLockExclusive(&t->gate);
        t->handle = CreateThread(0, 0, native_entry, t, 0, &t->id);
        require(t->handle != 0, "native CreateThread");
        ReleaseSRWLockExclusive(&t->gate);
#else
        require(pthread_mutex_init(&t->gate, 0) == 0, "native gate init");
        require(pthread_mutex_lock(&t->gate) == 0, "native gate lock");
        require(pthread_create(&t->id, 0, native_entry, t) == 0, "native pthread_create");
        require(pthread_mutex_unlock(&t->gate) == 0, "native gate unlock");
#endif
        return STATUS_CODE_SUCCESS;
    }
    static Status thread_wait(Thread* t) {
#if TARGET_PLATFORM_WIN32
        require(WaitForSingleObject(t->handle, INFINITE) == WAIT_OBJECT_0, "native join");
#else
        require(pthread_join(t->id, 0) == 0, "native join");
#endif
        return STATUS_CODE_SUCCESS;
    }
    static Status thread_free(Thread* t) {
        if (t->started) {
#if TARGET_PLATFORM_WIN32
            require(CloseHandle(t->handle) != 0, "native close handle");
#else
            require(pthread_mutex_destroy(&t->gate) == 0, "native gate free");
#endif
        }
        t->started = FALSE;
        t->captured = FALSE;
        return STATUS_CODE_SUCCESS;
    }
    static Status dispatcher_create(Dispatcher* d, Thread* t, NUINT capacity) {
        require(capacity != 0 && capacity <= native_max_capacity, "native capacity");
        monitor_init(&d->monitor);
        d->thread = t;
        d->capacity = capacity;
        d->head = 0;
        d->length = 0;
        d->occupied = 0;
        d->closed = FALSE;
        return STATUS_CODE_SUCCESS;
    }
    static Status dispatcher_submit(Dispatcher* d, Entry entry, POINTERW args, Flag wait, NUINT timeout) {
        monitor_lock(&d->monitor);
        if (d->closed) {
            monitor_unlock(&d->monitor);
            return STATUS_CODE_ERROR_INVALID_STATE;
        }
        if (native_is_current(d->thread)) {
            monitor_unlock(&d->monitor);
            entry(args);
            return STATUS_CODE_SUCCESS;
        }
        while (d->occupied == d->capacity && !d->closed) {
            if (timeout == 0) {
                monitor_unlock(&d->monitor);
                return STATUS_CODE_ERROR_TIMEOUT;
            }
            monitor_wait(&d->monitor);
        }
        if (d->closed) {
            monitor_unlock(&d->monitor);
            return STATUS_CODE_ERROR_INTERRUPTED;
        }
        Octet completion = completion_pending;
        NUINT index = (NUINT) ((d->head + d->length) % d->capacity);
        d->entries[index].entry = entry;
        d->entries[index].arguments = args;
        d->entries[index].completion = wait ? &completion : 0;
        ++d->length;
        ++d->occupied;
        monitor_notify(&d->monitor);
        if (!wait) {
            monitor_unlock(&d->monitor);
            return STATUS_CODE_SUCCESS;
        }
        // A zero-timeout synchronous submission cannot begin while this
        // lock is held. Remove the tail and return without borrowing args.
        if (timeout == 0) {
            --d->length;
            --d->occupied;
            monitor_notify(&d->monitor);
            monitor_unlock(&d->monitor);
            return STATUS_CODE_ERROR_TIMEOUT;
        }
        while (completion == completion_pending) monitor_wait(&d->monitor);
        --d->occupied;
        monitor_notify(&d->monitor);
        monitor_unlock(&d->monitor);
        return completion == completion_done ? STATUS_CODE_SUCCESS : STATUS_CODE_ERROR_INTERRUPTED;
    }
    static NUINT dispatcher_count(Dispatcher* d) {
        monitor_lock(&d->monitor);
        NUINT count = d->length;
        monitor_unlock(&d->monitor);
        return count;
    }
    static Status dispatcher_wait(Dispatcher* d, NUINT timeout) {
        require(native_is_current(d->thread), "native wait on associated thread");
        monitor_lock(&d->monitor);
        while (d->length == 0 && !d->closed) {
            if (timeout == 0) {
                monitor_unlock(&d->monitor);
                return STATUS_CODE_ERROR_TIMEOUT;
            }
            monitor_wait(&d->monitor);
        }
        Status result = d->closed ? STATUS_CODE_ERROR_INTERRUPTED : STATUS_CODE_SUCCESS;
        monitor_unlock(&d->monitor);
        return result;
    }
    static Status dispatcher_poll(Dispatcher* d) {
        require(native_is_current(d->thread), "native poll on associated thread");
        monitor_lock(&d->monitor);
        if (d->closed || d->length == 0) {
            Status result = d->closed ? STATUS_CODE_ERROR_INTERRUPTED : STATUS_CODE_SUCCESS_NOOP;
            monitor_unlock(&d->monitor);
            return result;
        }
        NativeInvocation work = d->entries[d->head];
        d->head = (NUINT) ((d->head + (NUINT) 1) % d->capacity);
        --d->length;
        monitor_notify(&d->monitor);
        monitor_unlock(&d->monitor);
        work.entry(work.arguments);
        monitor_lock(&d->monitor);
        if (work.completion != 0) *work.completion = completion_done;
        else --d->occupied;
        monitor_notify(&d->monitor);
        monitor_unlock(&d->monitor);
        return STATUS_CODE_SUCCESS;
    }
    static Status dispatcher_close(Dispatcher* d) {
        monitor_lock(&d->monitor);
        if (d->closed) {
            monitor_unlock(&d->monitor);
            return STATUS_CODE_SUCCESS_NOOP;
        }
        d->closed = TRUE;
        while (d->length != 0) {
            NativeInvocation* work = &d->entries[d->head];
            if (work->completion != 0) *work->completion = completion_cancelled;
            else --d->occupied;
            d->head = (NUINT) ((d->head + (NUINT) 1) % d->capacity);
            --d->length;
        }
        monitor_notify(&d->monitor);
        monitor_unlock(&d->monitor);
        return STATUS_CODE_SUCCESS;
    }
    static Status dispatcher_free(Dispatcher* d) {
        require(d->closed && d->occupied == 0, "native dispatcher quiescence");
        monitor_free(&d->monitor);
        return STATUS_CODE_SUCCESS;
    }
};

// The comparison uses only exported Catalyst functionality.
struct Catalyst {
    typedef PROCESS Process;
    typedef THREAD Thread;
    typedef DISPATCHER Dispatcher;
    static Status process_current(Process* p) { return catalyst::process_current(p).status; }
    static Flag process_equals(const Process* a, const Process* b) { return catalyst::process_equals(a, b); }
    static Status process_free(Process* p) { return catalyst::process_free(p).status; }
    static Status thread_current(Thread* t, Process* p) { return catalyst::thread_current(t, p).status; }
    static Flag thread_equals(const Thread* a, const Thread* b) { return catalyst::thread_equals(a, b); }
    static Status thread_create(Thread* t, Process* p, Entry e, POINTERW a) { return catalyst::thread_create(t, p, (POINTERX) e, a).status; }
    static Status thread_start(Thread* t) { return catalyst::thread_start(t).status; }
    static Status thread_wait(Thread* t) { return catalyst::thread_wait(t).status; }
    static Status thread_free(Thread* t) { return catalyst::thread_free(t).status; }
    static Status dispatcher_create(Dispatcher* d, Thread* t, NUINT c) { return catalyst::dispatcher_create(d, t, 0, c).status; }
    static Status dispatcher_submit(Dispatcher* d, Entry e, POINTERW a, Flag w, NUINT timeout) {
        return catalyst::dispatcher_submit(d, (POINTERX) e, a, w, timeout).status;
    }
    static NUINT dispatcher_count(Dispatcher* d) { return catalyst::dispatcher_count(d); }
    static Status dispatcher_wait(Dispatcher* d, NUINT timeout) { return catalyst::dispatcher_wait(d, timeout).status; }
    static Status dispatcher_poll(Dispatcher* d) { return catalyst::dispatcher_poll(d).status; }
    static Status dispatcher_close(Dispatcher* d) { return catalyst::dispatcher_close(d).status; }
    static Status dispatcher_free(Dispatcher* d) { return catalyst::dispatcher_free(d).status; }
};

// ------------------------------------------------------------
// Common fixtures. Native clocks and floating-point formatting
// are measurement-only, not part of either implementation.
// ------------------------------------------------------------
static double now_seconds() {
#if TARGET_PLATFORM_WIN32
    LARGE_INTEGER frequency, now;
    require(QueryPerformanceFrequency(&frequency) != 0 && frequency.QuadPart > 0, "performance frequency");
    require(QueryPerformanceCounter(&now) != 0, "performance counter");
    return (double) now.QuadPart / (double) frequency.QuadPart;
#else
    timespec now;
    require(clock_gettime(CLOCK_MONOTONIC, &now) == 0, "monotonic clock");
    return (double) now.tv_sec + (double) now.tv_nsec / 1000000000.0;
#endif
}

// Exact callback counts, represented as bytes so the callback requires no
// optional-width Catalyst integer type. Three bytes cover all sample limits.
struct CallbackCount {
    volatile Octet bytes[3];
};

static void increment(POINTERW args) {
    CallbackCount* count = (CallbackCount*) args;
    for (Octet i = 0; i < (Octet) 3; ++i) {
        count->bytes[i] = (Octet) (count->bytes[i] + (Octet) 1);
        if (count->bytes[i] != 0) return;
    }
}

static Flag count_matches(const CallbackCount* count, unsigned long expected) {
    for (Octet i = 0; i < (Octet) 3; ++i) {
        if (count->bytes[i] != (Octet) (expected % 256UL)) return FALSE;
        expected /= 256UL;
    }
    return expected == 0 ? TRUE : FALSE;
}

template<class API> struct Worker {
    typename API::Dispatcher* dispatcher;
    Status outcome;
};

template<class API> static void worker_entry(POINTERW args) {
    Worker<API>* worker = (Worker<API>*) args;
    for (;;) {
        worker->outcome = API::dispatcher_wait(worker->dispatcher, CATALYST_NUINT_MAX);
        if (worker->outcome != STATUS_CODE_SUCCESS) return;
        worker->outcome = API::dispatcher_poll(worker->dispatcher);
        if (worker->outcome != STATUS_CODE_SUCCESS && worker->outcome != STATUS_CODE_SUCCESS_NOOP) return;
    }
}

enum Kind {
    PROCESS_CAPTURE, PROCESS_EQUAL, THREAD_CAPTURE, THREAD_EQUAL, THREAD_LIFECYCLE,
    DISPATCHER_LIFECYCLE, COUNT_EMPTY, WAIT_EMPTY, POLL_EMPTY, INLINE_SUBMIT,
    PENDING_TIMEOUT, FULL_ADMISSION, SYNC_ROUNDTRIP, BATCH_FIFO, CLOSE_JOIN
};

struct Benchmark {
    const char* name;
    Kind kind;
    NUINT capacity;
    NUINT operations;
    unsigned long limit;
};

static const Benchmark benchmarks[] = {
    { "process current / free", PROCESS_CAPTURE, 1, 1, 262144UL },
    { "process equals", PROCESS_EQUAL, 1, 1, 262144UL },
    { "thread current / free", THREAD_CAPTURE, 1, 1, 262144UL },
    { "thread equals", THREAD_EQUAL, 1, 1, 262144UL },
    { "thread create/start/wait/free", THREAD_LIFECYCLE, 1, 1, 1024UL },
    { "dispatcher create/close/free (1)", DISPATCHER_LIFECYCLE, 1, 1, 32768UL },
    { "dispatcher create/close/free (32)", DISPATCHER_LIFECYCLE, 32, 1, 32768UL },
    { "dispatcher count (empty)", COUNT_EMPTY, 1, 1, 262144UL },
    { "dispatcher wait (zero timeout)", WAIT_EMPTY, 1, 1, 262144UL },
    { "dispatcher poll (empty)", POLL_EMPTY, 1, 1, 262144UL },
    { "dispatcher submit (same thread)", INLINE_SUBMIT, 1, 1, 262144UL },
    { "dispatcher submit (pending timeout)", PENDING_TIMEOUT, 1, 1, 32768UL },
    { "dispatcher submit (full, zero wait)", FULL_ADMISSION, 1, 1, 32768UL },
    { "dispatcher synchronous round trip", SYNC_ROUNDTRIP, 1, 1, 4096UL },
    { "dispatcher FIFO batch (8)", BATCH_FIFO, 8, 8, 4096UL },
    { "dispatcher FIFO batch (32)", BATCH_FIFO, 32, 32, 4096UL },
    { "dispatcher close / worker join", CLOSE_JOIN, 1, 1, 1024UL }
};

template<class API> static double measure(const Benchmark* b, unsigned long calls) {
    typedef typename API::Process Process;
    typedef typename API::Thread Thread;
    typedef typename API::Dispatcher Dispatcher;
    Process process, other;
    Thread thread, current;
    Dispatcher dispatcher;
    CallbackCount value = { { 0, 0, 0 } };
    double start, end;
    expect(API::process_current(&process), STATUS_CODE_SUCCESS);

    if (b->kind == PROCESS_CAPTURE || b->kind == PROCESS_EQUAL) {
        if (b->kind == PROCESS_EQUAL) expect(API::process_current(&other), STATUS_CODE_SUCCESS);
        start = now_seconds();
        for (unsigned long i = 0; i < calls; ++i) {
            if (b->kind == PROCESS_CAPTURE) {
                expect(API::process_current(&other), STATUS_CODE_SUCCESS);
                expect(API::process_free(&other), STATUS_CODE_SUCCESS);
            } else {
                require(API::process_equals(&process, &other), "process equality");
            }
        }
        end = now_seconds();
        if (b->kind == PROCESS_EQUAL) expect(API::process_free(&other), STATUS_CODE_SUCCESS);
    } else if (b->kind == THREAD_CAPTURE || b->kind == THREAD_EQUAL || b->kind == THREAD_LIFECYCLE) {
        if (b->kind == THREAD_EQUAL) {
            expect(API::thread_current(&current, &process), STATUS_CODE_SUCCESS);
            expect(API::thread_current(&thread, &process), STATUS_CODE_SUCCESS);
        }
        start = now_seconds();
        for (unsigned long i = 0; i < calls; ++i) {
            if (b->kind == THREAD_CAPTURE) {
                expect(API::thread_current(&thread, &process), STATUS_CODE_SUCCESS);
                expect(API::thread_free(&thread), STATUS_CODE_SUCCESS);
            } else if (b->kind == THREAD_EQUAL) {
                require(API::thread_equals(&current, &thread), "thread equality");
            } else {
                expect(API::thread_create(&thread, &process, increment, &value), STATUS_CODE_SUCCESS);
                expect(API::thread_start(&thread), STATUS_CODE_SUCCESS);
                expect(API::thread_wait(&thread), STATUS_CODE_SUCCESS);
                expect(API::thread_free(&thread), STATUS_CODE_SUCCESS);
            }
        }
        end = now_seconds();
        if (b->kind == THREAD_EQUAL) {
            expect(API::thread_free(&thread), STATUS_CODE_SUCCESS);
            expect(API::thread_free(&current), STATUS_CODE_SUCCESS);
        }
        if (b->kind == THREAD_LIFECYCLE) require(count_matches(&value, calls), "thread callback output");
    } else if (b->kind == CLOSE_JOIN) {
        // Measure close/join alone; setup and resource release are outside
        // each interval. The worker may already be waiting or about to wait.
        double total = 0.0;
        for (unsigned long i = 0; i < calls; ++i) {
            Worker<API> worker;
            worker.dispatcher = &dispatcher;
            worker.outcome = STATUS_CODE_ERROR;
            expect(API::thread_create(&thread, &process, worker_entry<API>, &worker), STATUS_CODE_SUCCESS);
            expect(API::dispatcher_create(&dispatcher, &thread, b->capacity), STATUS_CODE_SUCCESS);
            expect(API::thread_start(&thread), STATUS_CODE_SUCCESS);
            start = now_seconds();
            expect(API::dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS);
            expect(API::thread_wait(&thread), STATUS_CODE_SUCCESS);
            end = now_seconds();
            require(end >= start, "monotonic elapsed");
            total += end - start;
            require(worker.outcome == STATUS_CODE_ERROR_INTERRUPTED, "worker close outcome");
            expect(API::dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS);
            expect(API::thread_free(&thread), STATUS_CODE_SUCCESS);
        }
        expect(API::process_free(&process), STATUS_CODE_SUCCESS);
        return total;
    } else {
        Worker<API> worker;
        worker.dispatcher = &dispatcher;
        worker.outcome = STATUS_CODE_ERROR;
        Flag remote = b->kind == SYNC_ROUNDTRIP || b->kind == BATCH_FIFO ? TRUE : FALSE;
        Flag pending = b->kind == PENDING_TIMEOUT || b->kind == FULL_ADMISSION ? TRUE : FALSE;
        if (remote || pending) {
            expect(API::thread_create(&thread, &process, worker_entry<API>, &worker), STATUS_CODE_SUCCESS);
        } else {
            expect(API::thread_current(&thread, &process), STATUS_CODE_SUCCESS);
        }
        if (b->kind != DISPATCHER_LIFECYCLE) {
            expect(API::dispatcher_create(&dispatcher, &thread, b->capacity), STATUS_CODE_SUCCESS);
            if (b->kind == FULL_ADMISSION) {
                expect(API::dispatcher_submit(&dispatcher, increment, &value, FALSE, 0), STATUS_CODE_SUCCESS);
            }
            if (remote) expect(API::thread_start(&thread), STATUS_CODE_SUCCESS);
        }

        start = now_seconds();
        for (unsigned long i = 0; i < calls; ++i) {
            switch (b->kind) {
                case DISPATCHER_LIFECYCLE:
                    expect(API::dispatcher_create(&dispatcher, &thread, b->capacity), STATUS_CODE_SUCCESS);
                    expect(API::dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS);
                    expect(API::dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS);
                    break;
                case COUNT_EMPTY:
                    require(API::dispatcher_count(&dispatcher) == 0, "empty count");
                    break;
                case WAIT_EMPTY:
                    expect(API::dispatcher_wait(&dispatcher, 0), STATUS_CODE_ERROR_TIMEOUT);
                    break;
                case POLL_EMPTY:
                    expect(API::dispatcher_poll(&dispatcher), STATUS_CODE_SUCCESS_NOOP);
                    break;
                case INLINE_SUBMIT:
                    expect(API::dispatcher_submit(&dispatcher, increment, &value, TRUE, 0), STATUS_CODE_SUCCESS);
                    break;
                case PENDING_TIMEOUT:
                    expect(API::dispatcher_submit(&dispatcher, increment, &value, TRUE, 0), STATUS_CODE_ERROR_TIMEOUT);
                    break;
                case FULL_ADMISSION:
                    expect(API::dispatcher_submit(&dispatcher, increment, &value, FALSE, 0), STATUS_CODE_ERROR_TIMEOUT);
                    break;
                case SYNC_ROUNDTRIP:
                    expect(API::dispatcher_submit(&dispatcher, increment, &value, TRUE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                    break;
                case BATCH_FIFO:
                    // The last synchronous invocation is the completion
                    // barrier for all earlier FIFO submissions in this batch.
                    for (NUINT j = 1; j < b->operations; ++j) {
                        expect(API::dispatcher_submit(&dispatcher, increment, &value, FALSE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                    }
                    expect(API::dispatcher_submit(&dispatcher, increment, &value, TRUE, CATALYST_NUINT_MAX), STATUS_CODE_SUCCESS);
                    break;
                default: require(FALSE, "unrecognized workload");
            }
        }
        end = now_seconds();

        if (b->kind != DISPATCHER_LIFECYCLE) {
            require(API::dispatcher_count(&dispatcher) == (b->kind == FULL_ADMISSION ? 1 : 0), "final pending count");
            expect(API::dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS);
            if (remote) {
                expect(API::thread_wait(&thread), STATUS_CODE_SUCCESS);
                require(worker.outcome == STATUS_CODE_ERROR_INTERRUPTED, "worker shutdown");
            }
            expect(API::dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS);
        }
        expect(API::thread_free(&thread), STATUS_CODE_SUCCESS);
        if (remote || b->kind == INLINE_SUBMIT) {
            require(count_matches(&value, calls * (unsigned long) b->operations), "dispatcher callback output");
        } else {
            require(count_matches(&value, 0), "cancelled or absent callback");
        }
    }

    expect(API::process_free(&process), STATUS_CODE_SUCCESS);
    require(end >= start, "monotonic elapsed");
    return end - start;
}

static double median(double a, double b, double c) {
    if (a > b) { double t = a; a = b; b = t; }
    if (b > c) { double t = b; b = c; c = t; }
    return a > b ? a : b;
}

static void pair(const Benchmark* b, unsigned long calls, Flag reverse, double* native, double* comparison) {
    if (reverse) {
        *comparison = measure<Catalyst>(b, calls);
        *native = measure<Native>(b, calls);
    } else {
        *native = measure<Native>(b, calls);
        *comparison = measure<Catalyst>(b, calls);
    }
}

int main() {
    std::printf("CATTRLIB Performance (Release recommended)\n");
    std::printf("Native-C / OS baseline versus public Catalyst API. Lower time is better.\n");
    std::printf("Monotonic wall-clock, median of three paired samples; adaptive equal work counts.\n");
    std::printf("Native caller-owned handles and fixed FIFO storage; Catalyst owns its bookkeeping.\n");
    std::printf("Cross-thread rows include scheduling and completion; batch rows divide by completed invocations.\n");
    std::printf("Lifecycle rows report the named group, not individual calls. Close/join includes clock overhead.\n");
    std::printf("Zero-timeout rows measure nonblocking outcomes, not timer accuracy.\n");
    std::printf("Relative difference includes algorithm and ownership costs, not Result reporting alone.\n\n");
    std::printf("%-42s %13s %13s %12s %13s %s\n", "Workload", "Native ns/op", "Catalyst ns/op", "Difference", "Catalyst op/s", "Assessment");
    const NUINT count = (NUINT) (sizeof(benchmarks) / sizeof(benchmarks[0]));
    NUINT review = 0, short_samples = 0;
    for (NUINT i = 0; i < count; ++i) {
        const Benchmark* b = &benchmarks[i];
        unsigned long calls = 8;
        double native, comparison;
        for (;;) {
            pair(b, calls, FALSE, &native, &comparison);
            if ((native >= 0.005 && comparison >= 0.005) || native >= 0.1 || comparison >= 0.1 || calls >= b->limit) break;
            calls *= 2;
        }
        double n[3], c[3];
        for (Octet sample = 0; sample < (Octet) 3; ++sample) {
            pair(b, calls, sample == (Octet) 1 ? TRUE : FALSE, &n[sample], &c[sample]);
        }
        native = median(n[0], n[1], n[2]);
        comparison = median(c[0], c[1], c[2]);
        double operations = (double) calls * (double) b->operations;
        std::printf("%-42s %13.2f %13.2f", b->name, native * 1e9 / operations, comparison * 1e9 / operations);
        if (native <= 0.0 || comparison <= 0.0) {
            ++short_samples;
            std::printf(" %12s %13s  Clock resolution", "-", "-");
        } else {
            double change = (comparison / native - 1.0) * 100.0;
            std::printf(" %+11.1f%% %13.0f", change, operations / comparison);
            if (native < 0.001 || comparison < 0.001) {
                ++short_samples;
                std::printf("  Short sample");
            } else if (change >= 20.0) {
                ++review;
                std::printf("  Review");
            } else if (change > 5.0) {
                std::printf("  Slower");
            } else if (change < -5.0) {
                std::printf("  Faster");
            } else {
                std::printf("  Comparable");
            }
        }
        std::printf(" (%lu cycles)\n", calls);
        std::fflush(stdout);
    }
    std::printf("\n%u workload(s) >=20%% slower; %u short/unresolved sample(s).\n",
        (unsigned int) review, (unsigned int) short_samples);
    std::printf("All measured outcomes and callback outputs verified.\n");
    return 0;
}

#else
int main() {
    std::printf("CATTRLIB performance unavailable: no native threading backend for this target.\n");
    return 2;
}
#endif
