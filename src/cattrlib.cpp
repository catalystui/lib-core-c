#include "cattrlib.internal.hpp"

using namespace catalyst;

/* Contexts are assigned in declaration order: process 01-06, thread 07-0B,
 * dispatcher 0C-10 (create 0C, wait 0D, submits 0F/10), poll/close/free 11-13.
 * Non-reporting predicates/count have no context.
 * Native reporting sites use 01-05; portable sites begin at 10.
 * Private clock read and initialization have contexts 14 and 15.
 * The public objects own their state and must not be shallow-copied. */

static const BYTE THREAD_CREATED = 0;
static const BYTE THREAD_STARTED = 1;
static const BYTE THREAD_CAPTURED = 2;
static const BYTE THREAD_JOINED = 3;
static const BYTE THREAD_CLEANUP = 4;

struct CATTRLIB_THREAD_STATE {
    POINTERW native;
    POINTERW arguments;
    BYTE stage;
};

static BOOL cattr_thread_is_current(const THREAD* thread) {
    const CATTRLIB_THREAD_STATE* state = (const CATTRLIB_THREAD_STATE*) thread->state;
    return (state->stage == THREAD_STARTED || state->stage == THREAD_CAPTURED)
        && cattr_platform_thread_is_current(state->native) ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------------------------------
 * C API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Native process launch needs an executable/bootstrap and an input transfer
 * contract. A function pointer is not a portable native process entrypoint.
 * Do not substitute a thread or fork-and-call arbitrary user code. */
CATALYST_RESULT cattr_process_create(PROCESS* process, FUNCTION function, POINTERW input) {
    (void) input;
    process->function = function;
    process->state = 0;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x01, 0x10, 0);
}

CATALYST_RESULT cattr_process_current(PROCESS* process) {
    process->function = 0;
    RESULT capture = cattr_platform_process_capture(&process->state);
    if (!status_code_is_success(capture.status)) return RESULT(capture.status, 0x02, 0x10, capture.detail);
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x11, 0);
}

CATALYST_RESULT cattr_process_start(PROCESS* process) {
    (void) process;
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x03, 0x10, 0);
}

CATALYST_RESULT cattr_process_wait(PROCESS* process) {
    if (process->state != 0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x04, 0x10, 0);
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x04, 0x11, 0);
}

CATALYST_BOOL cattr_process_equals(const PROCESS* process1, const PROCESS* process2) {
    if (process1->state == 0 || process2->state == 0) return FALSE;
    return cattr_platform_process_equals(process1->state, process2->state);
}

CATALYST_RESULT cattr_process_terminate(PROCESS* process) {
    if (process->state != 0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x05, 0x10, 0);
    return RESULT(STATUS_CODE_ERROR_NOT_IMPLEMENTED, 0x05, 0x11, 0);
}

CATALYST_RESULT cattr_process_free(PROCESS* process) {
    if (process->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x06, 0x10, 0);
    RESULT release = catmm_free(process->state);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x06, 0x11, release.detail);
    process->state = 0;
    process->function = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x12, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT cattr_thread_create(THREAD* thread, PROCESS* process, PROCEDURE procedure, POINTERW arguments) {
    thread->process = process;
    thread->procedure = procedure;
    thread->state = 0;
    if (process->state == 0 || !cattr_platform_process_is_current(process->state)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x07, 0x10, 0);
    }
    RESULT allocation = catmm_alloc(&thread->state, (NUINT) sizeof(CATTRLIB_THREAD_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x07, 0x11, allocation.detail);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    state->native = 0;
    state->arguments = arguments;
    state->stage = THREAD_CREATED;
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x12, 0);
}

CATALYST_RESULT cattr_thread_current(THREAD* thread, PROCESS* process) {
    thread->process = process;
    thread->procedure = 0;
    thread->state = 0;
    if (process->state == 0 || !cattr_platform_process_is_current(process->state)) {
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x08, 0x10, 0);
    }
    RESULT allocation = catmm_alloc(&thread->state, (NUINT) sizeof(CATTRLIB_THREAD_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x08, 0x11, allocation.detail);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    state->native = 0;
    state->arguments = 0;
    state->stage = THREAD_CLEANUP;
    RESULT capture = cattr_platform_thread_capture(&state->native);
    if (!status_code_is_success(capture.status)) return RESULT(capture.status, 0x08, 0x12, capture.detail);
    state->stage = THREAD_CAPTURED;
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x13, 0);
}

CATALYST_RESULT cattr_thread_start(THREAD* thread) {
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage != THREAD_CREATED) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x09, 0x10, 0);
    /* The platform entry gate prevents user execution before native identity
     * is available. The caller serializes lifecycle changes. */
    state->stage = THREAD_STARTED;
    RESULT start = cattr_platform_thread_start(&state->native, thread->procedure, state->arguments);
    if (!status_code_is_success(start.status)) {
        state->stage = THREAD_CLEANUP;
        return RESULT(start.status, 0x09, 0x11, start.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x12, 0);
}

CATALYST_RESULT cattr_thread_wait(THREAD* thread) {
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage == THREAD_JOINED) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0A, 0x10, 0);
    if (state->stage != THREAD_STARTED || cattr_thread_is_current(thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0A, 0x11, 0);
    RESULT wait = cattr_platform_thread_wait(state->native);
    if (!status_code_is_success(wait.status)) return RESULT(wait.status, 0x0A, 0x12, wait.detail);
    state->stage = THREAD_JOINED;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x13, 0);
}

CATALYST_BOOL cattr_thread_equals(const THREAD* thread1, const THREAD* thread2) {
    if (thread1->state == 0 || thread2->state == 0) return FALSE;
    if (thread1->state == thread2->state) return TRUE;
    if (!cattr_process_equals(thread1->process, thread2->process)) return FALSE;
    const CATTRLIB_THREAD_STATE* first = (const CATTRLIB_THREAD_STATE*) thread1->state;
    const CATTRLIB_THREAD_STATE* second = (const CATTRLIB_THREAD_STATE*) thread2->state;
    if ((first->stage != THREAD_STARTED && first->stage != THREAD_CAPTURED)
        || (second->stage != THREAD_STARTED && second->stage != THREAD_CAPTURED)) return FALSE;
    return cattr_platform_thread_equals(first->native, second->native);
}

CATALYST_RESULT cattr_thread_free(THREAD* thread) {
    if (thread->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0B, 0x10, 0);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage == THREAD_STARTED) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x11, 0);
    state->stage = THREAD_CLEANUP;
    if (state->native != 0) {
        RESULT release = cattr_platform_thread_free(state->native);
        if (!status_code_is_success(release.status)) return RESULT(release.status, 0x0B, 0x12, release.detail);
        state->native = 0;
    }
    RESULT release = catmm_free(state);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x0B, 0x13, release.detail);
    thread->state = 0;
    thread->procedure = 0;
    thread->process = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x14, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Like DelegateQueue, the bounded work storage is established once and guarded
 * by one monitor. Slots remain stable while a submitting thread awaits them.
 * Unlike the old FIFO of invocations, the public set contains distinct
 * pending executable references. Its storage is maintained in arrival order
 * for the default FIFO rule; pointer identity is compared as a function
 * pointer, never as an integer, data pointer or object representation. */
static const BYTE WORK_FREE = 0;
static const BYTE WORK_PENDING = 1;
static const BYTE WORK_RUNNING = 2;
static const BYTE WORK_FINISHED = 3;
static const BYTE WORK_CANCELLED = 4;

struct CATTRLIB_WORK {
    PROCEDURE operation;
    POINTERW arguments;
    POINTERW output;
    BOOL function;
    BOOL awaited;
    BYTE stage;
};

struct CATTRLIB_DISPATCHER_STATE {
    POINTERW monitor;
    CATTRLIB_WORK* slots;
    NUINT capacity;
    BOOL closed;
    BOOL polling;
    BOOL ready;
};

/* Finite timeouts are milliseconds; NUINT_MAX is infinite. Native wait slices
 * avoid narrowing a pointer-width duration to an OS timeout field. */
struct CATTRLIB_TIMEOUT {
    NUINT remaining;
    NUINT previous;
};

static RESULT cattr_timeout_init(CATTRLIB_TIMEOUT* clock, NUINT timeout) {
    clock->remaining = timeout;
    clock->previous = 0;
    if (timeout == CATALYST_NUINT_MAX) return RESULT(STATUS_CODE_SUCCESS, 0x15, 0x01, 0);
    RESULT read = cattr_platform_ticks(&clock->previous);
    return RESULT(read.status, 0x15, 0x02, read.detail);
}

static RESULT cattr_wait_change(CATTRLIB_DISPATCHER_STATE* state, CATTRLIB_TIMEOUT* clock) {
    if (clock->remaining == CATALYST_NUINT_MAX) {
        return cattr_platform_monitor_wait(state->monitor, CATALYST_NUINT_MAX);
    }
    NUINT now = 0;
    RESULT read = cattr_platform_ticks(&now);
    if (!status_code_is_success(read.status)) return RESULT(read.status, 0x0D, 0x14, read.detail);
    NUINT elapsed = (NUINT) (now - clock->previous);
    clock->previous = now;
    if (elapsed >= clock->remaining) {
        clock->remaining = 0;
        return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x0D, 0x10, 0);
    }
    clock->remaining = (NUINT) (clock->remaining - elapsed);
    NUINT slice = clock->remaining;
#if TARGET_SIZEOF_VOID_P >= 2
    if (slice > (NUINT) 1000) slice = (NUINT) 1000;
#endif
    RESULT wait = cattr_platform_monitor_wait(state->monitor, slice);
    if (wait.status == STATUS_CODE_ERROR_TIMEOUT) {
        read = cattr_platform_ticks(&now);
        if (!status_code_is_success(read.status)) return RESULT(read.status, 0x0D, 0x15, read.detail);
        elapsed = (NUINT) (now - clock->previous);
        clock->previous = now;
        if (elapsed < slice) elapsed = slice;
        if (elapsed >= clock->remaining) {
            clock->remaining = 0;
            return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x0D, 0x11, 0);
        }
        clock->remaining = (NUINT) (clock->remaining - elapsed);
        return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x12, 0);
    }
    return RESULT(wait.status, 0x0D, 0x13, wait.detail);
}

/* Requires monitor held. The set is maintained in FIFO order without
 * catrl_set_remove's swap removal, and identity uses typed equality. */
static NUINT cattr_find_pending(DISPATCHER* dispatcher, PROCEDURE operation) {
    PROCEDURE* members = (PROCEDURE*) dispatcher->work.data;
    for (NUINT i = 0; i < dispatcher->work.cardinality; ++i) {
        if (members[i] == operation) return i;
    }
    return dispatcher->work.cardinality;
}

static void cattr_remove_pending(DISPATCHER* dispatcher, NUINT index) {
    PROCEDURE* members = (PROCEDURE*) dispatcher->work.data;
    --dispatcher->work.cardinality;
    while (index < dispatcher->work.cardinality) {
        members[index] = members[(NUINT) (index + (NUINT) 1)];
        ++index;
    }
}

static CATTRLIB_WORK* cattr_pending_slot(CATTRLIB_DISPATCHER_STATE* state, PROCEDURE operation) {
    for (NUINT i = 0; i < state->capacity; ++i) {
        CATTRLIB_WORK* slot = state->slots + i;
        if (slot->stage == WORK_PENDING && slot->operation == operation) return slot;
    }
    return 0;
}

static CATTRLIB_WORK* cattr_free_slot(CATTRLIB_DISPATCHER_STATE* state) {
    for (NUINT i = 0; i < state->capacity; ++i) {
        if (state->slots[i].stage == WORK_FREE) return state->slots + i;
    }
    return 0;
}

static POINTERW cattr_invoke(PROCEDURE operation, BOOL function, POINTERW arguments) {
    if (function) return ((POINTERW (*)(POINTERW)) operation)(arguments);
    ((void (*)(POINTERW)) operation)(arguments);
    return 0;
}

CATALYST_RESULT cattr_dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, FUNCTION rule, NUINT capacity) {
    dispatcher->thread = thread;
    dispatcher->rule = rule;
    dispatcher->state = 0;
    catrl_set_init(&dispatcher->work, (NUINT) sizeof(PROCEDURE));
    if (capacity == 0) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0C, 0x10, 3);
    if (capacity > (NUINT) (CATALYST_NUINT_MAX / (NUINT) sizeof(CATTRLIB_WORK))) {
        return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x0C, 0x11, 0);
    }
    RESULT allocation = catmm_alloc(&dispatcher->state, (NUINT) sizeof(CATTRLIB_DISPATCHER_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0C, 0x12, allocation.detail);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    state->monitor = 0;
    state->slots = 0;
    state->capacity = capacity;
    state->polling = FALSE;
    state->closed = TRUE;
    state->ready = FALSE;
    POINTERW slots = 0;
    allocation = catmm_alloc(&slots, (NUINT) (capacity * (NUINT) sizeof(CATTRLIB_WORK)));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0C, 0x13, allocation.detail);
    state->slots = (CATTRLIB_WORK*) slots;
    for (NUINT i = 0; i < capacity; ++i) state->slots[i].stage = WORK_FREE;
    RESULT expansion = catrl_set_expand(&dispatcher->work, capacity);
    if (!status_code_is_success(expansion.status)) return RESULT(expansion.status, 0x0C, 0x14, expansion.detail);
    RESULT monitor = cattr_platform_monitor_create(&state->monitor);
    if (!status_code_is_success(monitor.status)) return RESULT(monitor.status, 0x0C, 0x15, monitor.detail);
    state->closed = FALSE;
    state->ready = TRUE;
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x16, 0);
}

/* Submission owns no argument or output storage. A timed-out pending request
 * is removed before returning; once running, an awaited request completes
 * even after the deadline, so borrowed arguments cannot expire mid-call.
 * Asynchronous requests require caller-managed argument lifetime. */
static RESULT cattr_submit(DISPATCHER* dispatcher, PROCEDURE operation, BOOL function, POINTERW arguments,
                           BOOL wait, NUINT timeout, POINTERW* output, CONTEXT_CODE context) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    CATTRLIB_TIMEOUT clock;
    RESULT timer = cattr_timeout_init(&clock, timeout);
    if (!status_code_is_success(timer.status)) return RESULT(timer.status, context, 0x1B, timer.detail);
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, context, 0x10, 0);
    }
    if (cattr_thread_is_current(dispatcher->thread)) {
        cattr_platform_monitor_unlock(state->monitor);
        POINTERW value = cattr_invoke(operation, function, arguments);
        if (wait && output != 0) *output = value;
        return RESULT(STATUS_CODE_SUCCESS, context, 0x11, 0);
    }
    CATTRLIB_WORK* slot;
    for (;;) {
        if (state->closed) {
            cattr_platform_monitor_unlock(state->monitor);
            return RESULT(STATUS_CODE_ERROR_INTERRUPTED, context, 0x12, 0);
        }
        if (cattr_find_pending(dispatcher, operation) != dispatcher->work.cardinality) {
            cattr_platform_monitor_unlock(state->monitor);
            /* A synchronous duplicate cannot claim another invocation's output. */
            return RESULT(wait ? STATUS_CODE_ERROR_INVALID_STATE : STATUS_CODE_SUCCESS_NOOP, context, 0x13, 0);
        }
        slot = cattr_free_slot(state);
        if (slot != 0) break;
        RESULT change = cattr_wait_change(state, &clock);
        if (!status_code_is_success(change.status)) {
            cattr_platform_monitor_unlock(state->monitor);
            return RESULT(change.status, context, 0x14, change.detail);
        }
    }
    slot->operation = operation;
    slot->arguments = arguments;
    slot->output = 0;
    slot->function = function;
    slot->awaited = wait;
    slot->stage = WORK_PENDING;
    ((PROCEDURE*) dispatcher->work.data)[dispatcher->work.cardinality] = operation;
    ++dispatcher->work.cardinality;
    cattr_platform_monitor_notify(state->monitor);
    if (!wait) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS, context, 0x15, 0);
    }
    STATUS_CODE wait_failure = STATUS_CODE_SUCCESS;
    DETAIL_CODE wait_detail = 0;
    while (slot->stage == WORK_PENDING || slot->stage == WORK_RUNNING) {
        if (slot->stage == WORK_RUNNING) clock.remaining = CATALYST_NUINT_MAX;
        RESULT change = cattr_wait_change(state, &clock);
        if (!status_code_is_success(change.status)) {
            if (slot->stage == WORK_PENDING) {
                cattr_remove_pending(dispatcher, cattr_find_pending(dispatcher, operation));
                slot->stage = WORK_FREE;
                cattr_platform_monitor_notify(state->monitor);
                cattr_platform_monitor_unlock(state->monitor);
                return RESULT(change.status, context, 0x16, change.detail);
            }
            if (slot->stage == WORK_RUNNING) {
                /* A running callback cannot be cancelled safely. Keep waiting
                 * with borrowed arguments live; the native monitor is valid. */
                clock.remaining = CATALYST_NUINT_MAX;
                if (change.status != STATUS_CODE_ERROR_TIMEOUT) {
                    wait_failure = change.status;
                    wait_detail = change.detail;
                    /* An unexpected native wait failure must not retain the
                     * lock and prevent completion of the executing callback. */
                    cattr_platform_monitor_unlock(state->monitor);
                    cattr_platform_yield();
                    cattr_platform_monitor_lock(state->monitor);
                }
            }
        }
    }
    BOOL cancelled = slot->stage == WORK_CANCELLED ? TRUE : FALSE;
    if (!cancelled && status_code_is_success(wait_failure) && output != 0) *output = slot->output;
    slot->stage = WORK_FREE;
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    if (cancelled) return RESULT(STATUS_CODE_ERROR_INTERRUPTED, context, 0x17, 0);
    if (!status_code_is_success(wait_failure)) return RESULT(wait_failure, context, 0x1A, wait_detail);
    return RESULT(STATUS_CODE_SUCCESS, context, 0x18, 0);
}

CATALYST_RESULT cattr_dispatcher_submit_procedure(DISPATCHER* dispatcher, PROCEDURE procedure, POINTERW arguments, BOOL wait, NUINT timeout) {
    return cattr_submit(dispatcher, procedure, FALSE, arguments, wait, timeout, 0, 0x0F);
}

CATALYST_RESULT cattr_dispatcher_submit_function(DISPATCHER* dispatcher, FUNCTION function, POINTERW arguments, BOOL wait, NUINT timeout, POINTERW* output) {
    if ((!wait && output != 0) || (wait && output == 0)) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x10, 0x19, 5);
    if (output != 0) *output = 0;
    return cattr_submit(dispatcher, (PROCEDURE) function, TRUE, arguments, wait, timeout, output, 0x10);
}

CATALYST_NUINT cattr_dispatcher_count(DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    NUINT count = dispatcher->work.cardinality;
    cattr_platform_monitor_unlock(state->monitor);
    return count;
}

CATALYST_RESULT cattr_dispatcher_wait(DISPATCHER* dispatcher, NUINT timeout) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    if (!cattr_thread_is_current(dispatcher->thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0D, 0x20, 0);
    CATTRLIB_TIMEOUT clock;
    RESULT timer = cattr_timeout_init(&clock, timeout);
    if (!status_code_is_success(timer.status)) return RESULT(timer.status, 0x0D, 0x24, timer.detail);
    cattr_platform_monitor_lock(state->monitor);
    while (!state->closed && dispatcher->work.cardinality == 0) {
        RESULT change = cattr_wait_change(state, &clock);
        if (!status_code_is_success(change.status) && !state->closed && dispatcher->work.cardinality == 0) {
            cattr_platform_monitor_unlock(state->monitor);
            return RESULT(change.status, 0x0D, 0x21, change.detail);
        }
    }
    BOOL closed = state->closed;
    cattr_platform_monitor_unlock(state->monitor);
    if (closed) return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x0D, 0x22, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x23, 0);
}

CATALYST_RESULT cattr_dispatcher_poll(DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    if (!cattr_thread_is_current(dispatcher->thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x11, 0x10, 0);
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x11, 0x16, 0);
    }
    if (state->polling) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x11, 0x11, 0);
    }
    if (dispatcher->work.cardinality == 0) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x11, 0x12, 0);
    }
    PROCEDURE* members = (PROCEDURE*) dispatcher->work.data;
    NUINT index = 0;
    if (dispatcher->rule != 0) {
        /* A rule returns the address of one member, not its pointer value.
         * The read-only, non-reentrant callback runs under the monitor. */
        POINTERW selected = ((POINTERW (*)(const SET*)) dispatcher->rule)(&dispatcher->work);
        while (index < dispatcher->work.cardinality && selected != members + index) ++index;
        if (index == dispatcher->work.cardinality) {
            cattr_platform_monitor_unlock(state->monitor);
            return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x11, 0x13, 0);
        }
    }
    CATTRLIB_WORK* slot = cattr_pending_slot(state, members[index]);
    if (slot == 0) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_FATAL_INVARIANT_VIOLATION, 0x11, 0x14, 0);
    }
    cattr_remove_pending(dispatcher, index);
    slot->stage = WORK_RUNNING;
    state->polling = TRUE;
    cattr_platform_monitor_unlock(state->monitor);
    POINTERW output = cattr_invoke(slot->operation, slot->function, slot->arguments);
    cattr_platform_monitor_lock(state->monitor);
    slot->output = output;
    slot->stage = slot->awaited ? WORK_FINISHED : WORK_FREE;
    state->polling = FALSE;
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    return RESULT(STATUS_CODE_SUCCESS, 0x11, 0x15, 0);
}

CATALYST_RESULT cattr_dispatcher_close(DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x12, 0x10, 0);
    }
    state->closed = TRUE;
    catrl_set_clear(&dispatcher->work);
    for (NUINT i = 0; i < state->capacity; ++i) {
        CATTRLIB_WORK* slot = state->slots + i;
        if (slot->stage == WORK_PENDING) slot->stage = slot->awaited ? WORK_CANCELLED : WORK_FREE;
    }
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    return RESULT(STATUS_CODE_SUCCESS, 0x12, 0x11, 0);
}

CATALYST_RESULT cattr_dispatcher_free(DISPATCHER* dispatcher) {
    if (dispatcher->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x13, 0x10, 0);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    /* All calls must have returned before free, including awakened waiters.
     * Failed creation or release permits free only until cleanup succeeds. */
    if (state->ready && !state->closed) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x13, 0x11, 0);
    state->ready = FALSE;
    if (state->monitor != 0) {
        RESULT release = cattr_platform_monitor_free(state->monitor);
        if (!status_code_is_success(release.status)) return RESULT(release.status, 0x13, 0x12, release.detail);
        state->monitor = 0;
    }
    RESULT release = catrl_set_free(&dispatcher->work);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x13, 0x13, release.detail);
    if (state->slots != 0) {
        release = catmm_free(state->slots);
        if (!status_code_is_success(release.status)) return RESULT(release.status, 0x13, 0x14, release.detail);
        state->slots = 0;
    }
    release = catmm_free(state);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x13, 0x15, release.detail);
    dispatcher->state = 0;
    dispatcher->thread = 0;
    dispatcher->rule = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x13, 0x16, 0);
}
