#include "cattrlib.internal.hpp"

using namespace catalyst;

/* Contexts follow the public declaration order (01-10). Each public
 * reporting site has a distinct operation ID. Native failures retain their
 * status and detail at the public reporting boundary.
 * Objects own their state and may not be shallow-copied. Create/current
 * require unused storage; failed initialization permits free only.
 */

static const BYTE THREAD_CREATED = 0;
static const BYTE THREAD_STARTED = 1;
static const BYTE THREAD_CAPTURED = 2;
static const BYTE THREAD_JOINED = 3;
static const BYTE THREAD_CLEANUP = 4;

struct CATTRLIB_THREAD_STATE {
    POINTERW native;
    BYTE stage;
};

/* Lifecycle changes are externally serialized with all uses. A captured
 * identity must remain live; the backend publishes identity before entry. */
static BOOL cattr_thread_is_current(const THREAD* thread) {
    const CATTRLIB_THREAD_STATE* state = (const CATTRLIB_THREAD_STATE*) thread->state;
    return (state->stage == THREAD_STARTED || state->stage == THREAD_CAPTURED)
        && cattr_platform_thread_is_current(state->native) ? TRUE : FALSE;
}

/* -------------------------------------------------------------------------------------------------
 * C API :: PROCESS OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT cattr_process_current(PROCESS* process) {
    process->state = 0;
    RESULT capture = cattr_platform_process_current(&process->state);
    return RESULT(capture.status, 0x01, 0x01, capture.detail);
}

CATALYST_BOOL cattr_process_equals(const PROCESS* process1, const PROCESS* process2) {
    return cattr_platform_process_equals(process1->state, process2->state);
}

CATALYST_RESULT cattr_process_free(PROCESS* process) {
    if (process->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x03, 0x01, 0);
    RESULT release = cattr_platform_process_free(&process->state);
    return RESULT(release.status, 0x03, 0x02, release.detail);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: THREAD OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT cattr_thread_create(THREAD* thread, PROCESS* process, POINTERX entry, POINTERW arguments) {
    thread->process = process;
    thread->entry = entry;
    thread->arguments = arguments;
    thread->state = 0;
    if (!cattr_platform_process_is_current(process->state)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x04, 0x01, 0);
    RESULT allocation = catmm_heap_alloc(&thread->state, (NUINT) sizeof(CATTRLIB_THREAD_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x04, 0x02, allocation.detail);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    state->native = 0;
    state->stage = THREAD_CREATED;
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x03, 0);
}

CATALYST_RESULT cattr_thread_current(THREAD* thread, PROCESS* process) {
    thread->process = process;
    thread->entry = 0;
    thread->arguments = 0;
    thread->state = 0;
    if (!cattr_platform_process_is_current(process->state)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x05, 0x01, 0);
    RESULT allocation = catmm_heap_alloc(&thread->state, (NUINT) sizeof(CATTRLIB_THREAD_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x05, 0x02, allocation.detail);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    state->native = 0;
    state->stage = THREAD_CLEANUP;
    RESULT capture = cattr_platform_thread_current(&state->native);
    if (!status_code_is_success(capture.status)) return RESULT(capture.status, 0x05, 0x03, capture.detail);
    state->stage = THREAD_CAPTURED;
    return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x04, 0);
}

CATALYST_RESULT cattr_thread_start(THREAD* thread) {
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage != THREAD_CREATED) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x06, 0x01, 0);
    state->stage = THREAD_STARTED;
    RESULT start = cattr_platform_thread_start(&state->native, (void (*)(POINTERW)) thread->entry, thread->arguments);
    if (!status_code_is_success(start.status)) {
        state->stage = THREAD_CLEANUP;
        return RESULT(start.status, 0x06, 0x02, start.detail);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x06, 0x03, 0);
}

CATALYST_RESULT cattr_thread_wait(THREAD* thread) {
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage == THREAD_JOINED) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01, 0);
    if (state->stage != THREAD_STARTED || cattr_thread_is_current(thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x07, 0x02, 0);
    RESULT wait = cattr_platform_thread_wait(state->native);
    if (!status_code_is_success(wait.status)) return RESULT(wait.status, 0x07, 0x03, wait.detail);
    state->stage = THREAD_JOINED;
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x04, 0);
}

CATALYST_BOOL cattr_thread_equals(const THREAD* thread1, const THREAD* thread2) {
    if (thread1->state == thread2->state) return TRUE;
    if (!cattr_process_equals(thread1->process, thread2->process)) return FALSE;
    const CATTRLIB_THREAD_STATE* first = (const CATTRLIB_THREAD_STATE*) thread1->state;
    const CATTRLIB_THREAD_STATE* second = (const CATTRLIB_THREAD_STATE*) thread2->state;
    if ((first->stage != THREAD_STARTED && first->stage != THREAD_CAPTURED)
        || (second->stage != THREAD_STARTED && second->stage != THREAD_CAPTURED)) return FALSE;
    return cattr_platform_thread_equals(first->native, second->native);
}

CATALYST_RESULT cattr_thread_free(THREAD* thread) {
    if (thread->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x09, 0x01, 0);
    CATTRLIB_THREAD_STATE* state = (CATTRLIB_THREAD_STATE*) thread->state;
    if (state->stage == THREAD_STARTED) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x09, 0x02, 0);
    state->stage = THREAD_CLEANUP;
    if (state->native != 0) {
        RESULT release = cattr_platform_thread_free(&state->native);
        if (!status_code_is_success(release.status)) return RESULT(release.status, 0x09, 0x03, release.detail);
    }
    RESULT release = catmm_heap_free(state);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x09, 0x04, release.detail);
    thread->process = 0;
    thread->entry = 0;
    thread->arguments = 0;
    thread->state = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x05, 0);
}

/* -------------------------------------------------------------------------------------------------
 * C API :: DISPATCHER OPERATIONS
 * ------------------------------------------------------------------------------------------------- */

/* Provisional private representation. The fixed-capacity Pool owns records;
 * the public Set stores distinct POINTERW references to pending records.
 * FIFO uses an Array of the same references only when no rule is supplied.
 * Set order is not relied upon. An awaited record stays acquired until its
 * submitting caller observes completion or cancellation.
 */
static const BYTE INVOCATION_PENDING = 0;
static const BYTE INVOCATION_RUNNING = 1;
static const BYTE INVOCATION_FINISHED = 2;
static const BYTE INVOCATION_CANCELLED = 3;

struct CATTRLIB_INVOCATION {
    POINTERX operation;
    POINTERW arguments;
    POINTERW identity;
    NUINT index;
    BOOL awaited;
    BYTE stage;
};

struct CATTRLIB_DISPATCHER_STATE {
    POINTERW monitor;
    POOL pool;
    ARRAY order;
    BOOL closed;
    BOOL polling;
};

/* Read a stored pointer representation without treating raw bytes as a
 * constructed pointer object. The input identifies one live collection member. */
static CATTRLIB_INVOCATION* cattr_invocation_at(POINTER member) {
    POINTERW value;
    catmm_copy(&value, member, (NUINT) sizeof(value));
    return (CATTRLIB_INVOCATION*) value;
}

/* Monitor held. Invocation is pending and belongs to this dispatcher.
 * Remove using the exact identity representation originally inserted. */
static void cattr_remove_pending(DISPATCHER* dispatcher, CATTRLIB_INVOCATION* invocation) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    catrl_set_remove(&dispatcher->work, &invocation->identity);
    if (dispatcher->rule == 0) {
        for (NUINT i = 0; i < state->order.length; ++i) {
            if (cattr_invocation_at(catrl_array_get(&state->order, i)) == invocation) {
                catrl_array_remove(&state->order, i);
                break;
            }
        }
    }
}

/* Monitor held, pending set nonempty. The rule is read-only/non-reentrant
 * and returns an opaque record reference, not the address of its Set slot.
 * Validate membership before dereferencing a rule result. */
static CATTRLIB_INVOCATION* cattr_select(DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    if (dispatcher->rule == 0) return cattr_invocation_at(catrl_array_get(&state->order, 0));
    POINTER selected = dispatcher->rule(&dispatcher->work);
    const BYTE* members = (const BYTE*) dispatcher->work.data;
    for (NUINT i = 0; i < dispatcher->work.cardinality; ++i) {
        CATTRLIB_INVOCATION* invocation = cattr_invocation_at(members + (NUINT) (i * dispatcher->work.size));
        if (selected == invocation) return invocation;
    }
    return 0;
}

CATALYST_RESULT cattr_dispatcher_create(DISPATCHER* dispatcher, THREAD* thread, POINTER (*rule)(const SET*), NUINT capacity) {
    dispatcher->thread = thread;
    dispatcher->rule = rule;
    dispatcher->state = 0;
    catrl_set_init(&dispatcher->work, (NUINT) sizeof(POINTERW));
    if (capacity == 0) return RESULT(STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 3);
    RESULT allocation = catmm_heap_alloc(&dispatcher->state, (NUINT) sizeof(CATTRLIB_DISPATCHER_STATE));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0A, 0x02, allocation.detail);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    state->monitor = 0;
    state->closed = TRUE;
    state->polling = FALSE;
    catrl_pool_init(&state->pool, (NUINT) sizeof(CATTRLIB_INVOCATION));
    catrl_array_init(&state->order, (NUINT) sizeof(POINTERW));
    RESULT pool = catrl_pool_expand(&state->pool, capacity);
    if (!status_code_is_success(pool.status)) return RESULT(pool.status, 0x0A, 0x03, pool.detail);
    RESULT set = catrl_set_expand(&dispatcher->work, capacity);
    if (!status_code_is_success(set.status)) return RESULT(set.status, 0x0A, 0x04, set.detail);
    if (rule == 0) {
        RESULT order = catrl_array_expand(&state->order, capacity);
        if (!status_code_is_success(order.status)) return RESULT(order.status, 0x0A, 0x05, order.detail);
    }
    RESULT monitor = cattr_platform_monitor_create(&state->monitor);
    if (!status_code_is_success(monitor.status)) return RESULT(monitor.status, 0x0A, 0x06, monitor.detail);
    state->closed = FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x07, 0);
}

struct CATTRLIB_SUBMISSION {
    DISPATCHER* dispatcher;
    POINTERX operation;
    POINTERW arguments;
    BOOL awaited;
    CATTRLIB_INVOCATION* invocation;
};

/* Monitor held. Admit once when a slot is available, then wait for execution
 * to start (awaited) or admission alone (asynchronous). A single backend
 * deadline spans both capacity and pending waits.
 */
static BOOL cattr_submission_ready(POINTERW arguments) {
    CATTRLIB_SUBMISSION* submission = (CATTRLIB_SUBMISSION*) arguments;
    DISPATCHER* dispatcher = submission->dispatcher;
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    if (submission->invocation == 0) {
        if (state->closed) return TRUE;
        NUINT index;
        if (!catrl_pool_acquire(&state->pool, &index)) return FALSE;
        CATTRLIB_INVOCATION* invocation = (CATTRLIB_INVOCATION*) catrl_pool_get(&state->pool, index);
        invocation->operation = submission->operation;
        invocation->arguments = submission->arguments;
        invocation->identity = invocation;
        invocation->index = index;
        invocation->awaited = submission->awaited;
        invocation->stage = INVOCATION_PENDING;
        submission->invocation = invocation;
        catrl_set_add(&dispatcher->work, &invocation->identity);
        if (dispatcher->rule == 0) catrl_array_add(&state->order, &invocation->identity);
        cattr_platform_monitor_notify(state->monitor);
    }
    return !submission->awaited || submission->invocation->stage != INVOCATION_PENDING ? TRUE : FALSE;
}

/* An awaited running invocation cannot release its borrowed arguments until
 * completion, even if a native wait fails or its original deadline expires. */
static BOOL cattr_invocation_finished(POINTERW arguments) {
    return ((CATTRLIB_INVOCATION*) arguments)->stage != INVOCATION_RUNNING ? TRUE : FALSE;
}

CATALYST_RESULT cattr_dispatcher_submit(DISPATCHER* dispatcher, POINTERX operation, POINTERW arguments, BOOL wait, NUINT timeout) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x01, 0);
    }
    /* The associated thread cannot block awaiting its own dispatch.
     * Asynchronous submissions still enter the Set and obey the rule. */
    if (cattr_thread_is_current(dispatcher->thread)) {
        if (wait) {
            cattr_platform_monitor_unlock(state->monitor);
            return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x02, 0);
        }
        timeout = 0;
    }
    CATTRLIB_SUBMISSION submission;
    submission.dispatcher = dispatcher;
    submission.operation = operation;
    submission.arguments = arguments;
    submission.awaited = wait;
    submission.invocation = 0;
    RESULT admission = cattr_platform_monitor_wait(state->monitor, timeout, cattr_submission_ready, &submission);
    CATTRLIB_INVOCATION* invocation = submission.invocation;
    if (invocation == 0) {
        cattr_platform_monitor_unlock(state->monitor);
        if (!status_code_is_success(admission.status)) return RESULT(admission.status, 0x0B, 0x03, admission.detail);
        return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x0B, 0x04, 0);
    }
    if (!wait) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x05, 0);
    }
    if (invocation->stage == INVOCATION_PENDING) {
        cattr_remove_pending(dispatcher, invocation);
        catrl_pool_release(&state->pool, invocation->index);
        cattr_platform_monitor_notify(state->monitor);
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(admission.status, 0x0B, 0x06, admission.detail);
    }
    RESULT completion = admission;
    if (completion.status == STATUS_CODE_ERROR_TIMEOUT) completion = RESULT(STATUS_CODE_SUCCESS, 0, 0, 0);
    while (invocation->stage == INVOCATION_RUNNING) {
        RESULT change = cattr_platform_monitor_wait(state->monitor, CATALYST_NUINT_MAX, cattr_invocation_finished, invocation);
        if (!status_code_is_success(change.status)) {
            if (status_code_is_success(completion.status) || change.status > completion.status) completion = change;
            if (invocation->stage == INVOCATION_RUNNING) {
                cattr_platform_monitor_unlock(state->monitor);
                cattr_platform_yield();
                cattr_platform_monitor_lock(state->monitor);
            }
        }
    }
    BOOL cancelled = invocation->stage == INVOCATION_CANCELLED ? TRUE : FALSE;
    catrl_pool_release(&state->pool, invocation->index);
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    if (!status_code_is_success(completion.status)) return RESULT(completion.status, 0x0B, 0x07, completion.detail);
    if (cancelled) return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x0B, 0x08, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0B, 0x09, 0);
}

void cattr_dispatcher_inspect(POINTER submission, POINTERX* operation, POINTERW* arguments) {
    const CATTRLIB_INVOCATION* invocation = (const CATTRLIB_INVOCATION*) submission;
    *operation = invocation->operation;
    *arguments = invocation->arguments;
}

CATALYST_NUINT cattr_dispatcher_count(const DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    NUINT count = dispatcher->work.cardinality;
    cattr_platform_monitor_unlock(state->monitor);
    return count;
}

/* Monitor held. Close also satisfies the predicate, interrupting the wait. */
static BOOL cattr_work_available(POINTERW arguments) {
    DISPATCHER* dispatcher = (DISPATCHER*) arguments;
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    return state->closed || dispatcher->work.cardinality != 0 ? TRUE : FALSE;
}

CATALYST_RESULT cattr_dispatcher_wait(DISPATCHER* dispatcher, NUINT timeout) {
    if (!cattr_thread_is_current(dispatcher->thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0D, 0x01, 0);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    RESULT wait = cattr_platform_monitor_wait(state->monitor, timeout, cattr_work_available, dispatcher);
    BOOL closed = state->closed;
    cattr_platform_monitor_unlock(state->monitor);
    if (!status_code_is_success(wait.status)) return RESULT(wait.status, 0x0D, 0x02, wait.detail);
    if (closed) return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x0D, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x04, 0);
}

CATALYST_RESULT cattr_dispatcher_poll(DISPATCHER* dispatcher) {
    if (!cattr_thread_is_current(dispatcher->thread)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x01, 0);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INTERRUPTED, 0x0E, 0x02, 0);
    }
    if (state->polling) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x03, 0);
    }
    if (dispatcher->work.cardinality == 0) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0E, 0x04, 0);
    }
    CATTRLIB_INVOCATION* invocation = cattr_select(dispatcher);
    if (invocation == 0) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x05, 0);
    }
    cattr_remove_pending(dispatcher, invocation);
    invocation->stage = INVOCATION_RUNNING;
    state->polling = TRUE;
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    ((void (*)(POINTERW)) invocation->operation)(invocation->arguments);
    cattr_platform_monitor_lock(state->monitor);
    invocation->stage = INVOCATION_FINISHED;
    if (!invocation->awaited) catrl_pool_release(&state->pool, invocation->index);
    state->polling = FALSE;
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    return RESULT(STATUS_CODE_SUCCESS, 0x0E, 0x06, 0);
}

CATALYST_RESULT cattr_dispatcher_close(DISPATCHER* dispatcher) {
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    cattr_platform_monitor_lock(state->monitor);
    if (state->closed) {
        cattr_platform_monitor_unlock(state->monitor);
        return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x0F, 0x01, 0);
    }
    state->closed = TRUE;
    const BYTE* members = (const BYTE*) dispatcher->work.data;
    for (NUINT i = 0; i < dispatcher->work.cardinality; ++i) {
        CATTRLIB_INVOCATION* invocation = cattr_invocation_at(members + (NUINT) (i * dispatcher->work.size));
        if (invocation->awaited) invocation->stage = INVOCATION_CANCELLED;
        else catrl_pool_release(&state->pool, invocation->index);
    }
    catrl_set_clear(&dispatcher->work);
    catrl_array_clear(&state->order);
    cattr_platform_monitor_notify(state->monitor);
    cattr_platform_monitor_unlock(state->monitor);
    return RESULT(STATUS_CODE_SUCCESS, 0x0F, 0x02, 0);
}

CATALYST_RESULT cattr_dispatcher_free(DISPATCHER* dispatcher) {
    if (dispatcher->state == 0) return RESULT(STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01, 0);
    CATTRLIB_DISPATCHER_STATE* state = (CATTRLIB_DISPATCHER_STATE*) dispatcher->state;
    /* Close and external quiescence precede free; partial creation/release
     * remains retryable, and no further dispatcher access is permitted. */
    if (!state->closed) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x10, 0x02, 0);
    if (state->monitor != 0) {
        RESULT monitor = cattr_platform_monitor_free(&state->monitor);
        if (!status_code_is_success(monitor.status)) return RESULT(monitor.status, 0x10, 0x03, monitor.detail);
    }
    RESULT set = catrl_set_free(&dispatcher->work);
    if (!status_code_is_success(set.status)) return RESULT(set.status, 0x10, 0x04, set.detail);
    RESULT order = catrl_array_free(&state->order);
    if (!status_code_is_success(order.status)) return RESULT(order.status, 0x10, 0x05, order.detail);
    RESULT pool = catrl_pool_free(&state->pool);
    if (!status_code_is_success(pool.status)) return RESULT(pool.status, 0x10, 0x06, pool.detail);
    RESULT release = catmm_heap_free(state);
    if (!status_code_is_success(release.status)) return RESULT(release.status, 0x10, 0x07, release.detail);
    dispatcher->thread = 0;
    dispatcher->rule = 0;
    dispatcher->state = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x10, 0x08, 0);
}
