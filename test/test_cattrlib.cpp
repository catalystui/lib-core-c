#include <cstdio>
#include "cattrlib.hpp"

using namespace catalyst;

static BOOL matches(CATALYST_RESULT result, STATUS_CODE status, CONTEXT_CODE context, OPERATION_CODE operation, DETAIL_CODE detail = 0) {
    return result.status == status && result.context == context && result.operation == operation && result.detail == detail ? TRUE : FALSE;
}

static BOOL check(int* failed, BOOL passed, const char* name) {
    std::printf("%s: %s\n", passed ? "PASS" : "FAIL", name);
    if (!passed) ++*failed;
    return passed;
}

struct TEST_STATE {
    DISPATCHER* dispatcher;
    PROCESS* process;
    THREAD* thread;
    BOOL identity;
    BYTE executed;
    RESULT outcome;
};

static void increment(POINTERW arguments) {
    ++((TEST_STATE*) arguments)->executed;
}

static void worker(POINTERW arguments) {
    TEST_STATE* state = (TEST_STATE*) arguments;
    THREAD current;
    RESULT capture = thread_current(&current, state->process);
    if (status_code_is_success(capture.status)) {
        state->identity = thread_equals(&current, state->thread);
    }
    RESULT release = thread_free(&current);
    if (!status_code_is_success(capture.status)) {
        state->outcome = capture;
        return;
    }
    if (!status_code_is_success(release.status)) {
        state->outcome = release;
        return;
    }
    /* Idle timeout bounds the test even if a submission is lost. */
    for (;;) {
        state->outcome = dispatcher_wait(state->dispatcher, 5000);
        if (!status_code_is_success(state->outcome.status)) return;
        state->outcome = dispatcher_poll(state->dispatcher);
        if (!status_code_is_success(state->outcome.status)) return;
    }
}

/* Results and observations are read only after join. All queued arguments
 * remain alive until execution or cancellation. No platform timing helpers. */
struct ORDER_STATE {
    DISPATCHER* dispatcher;
    BYTE values[3];
    BYTE length;
    RESULT outcome;
};

struct ORDER_ARGUMENT {
    ORDER_STATE* state;
    BYTE value;
};

static void append_value(POINTERW arguments) {
    ORDER_ARGUMENT* item = (ORDER_ARGUMENT*) arguments;
    ORDER_STATE* state = item->state;
    if (state->length < (BYTE) 3) {
        state->values[state->length] = item->value;
    }
    ++state->length;
}

/* Select highest priority using only the public rule inspection contract. */
static POINTER select_last(const SET* work) {
    POINTERW selected = 0;
    BYTE priority = 0;
    for (NUINT i = 0; i < work->cardinality; ++i) {
        POINTERW candidate;
        catmm_copy(&candidate, (const BYTE*) work->data + (NUINT) (i * work->size), (NUINT) sizeof(candidate));
        POINTERX operation;
        POINTERW arguments;
        dispatcher_inspect(candidate, &operation, &arguments);
        if (operation != (POINTERX) append_value) continue;
        BYTE value = ((const ORDER_ARGUMENT*) arguments)->value;
        if (selected == 0 || value > priority) {
            selected = candidate;
            priority = value;
        }
    }
    return selected;
}

static void drain_three(POINTERW arguments) {
    ORDER_STATE* state = (ORDER_STATE*) arguments;
    for (BYTE i = 0; i < (BYTE) 3; ++i) {
        state->outcome = dispatcher_wait(state->dispatcher, 5000);
        if (!matches(state->outcome, STATUS_CODE_SUCCESS, 0x0D, 0x04)) return;
        state->outcome = dispatcher_poll(state->dispatcher);
        if (!matches(state->outcome, STATUS_CODE_SUCCESS, 0x0E, 0x06)) return;
    }
}

/* Fill before starting the worker, so saturation and rule selection cannot
 * depend on scheduling. Use both C ABI and C++ wrapper entry points. */
static BOOL test_order(int* failed, PROCESS* process, POINTER (*rule)(const SET*)) {
    std::printf("\n%s selection\n", rule == 0 ? "FIFO" : "Custom");
    THREAD thread;
    DISPATCHER dispatcher;
    ORDER_STATE state;
    state.dispatcher = &dispatcher;
    state.length = 0;
    state.outcome = RESULT(STATUS_CODE_ERROR, 0, 0, 0);
    ORDER_ARGUMENT items[3];
    for (BYTE i = 0; i < (BYTE) 3; ++i) {
        items[i].state = &state;
        items[i].value = (BYTE) (i + (BYTE) 1);
    }

    RESULT result = cattr_thread_create(&thread, process, (POINTERX) drain_three, &state);
    if (!check(failed, matches(result, STATUS_CODE_SUCCESS, 0x04, 0x03), "create ordering thread")) {
        thread_free(&thread);
        return TRUE;
    }
    result = cattr_dispatcher_create(&dispatcher, &thread, rule, 3);
    if (!check(failed, matches(result, STATUS_CODE_SUCCESS, 0x0A, 0x07), "create bounded dispatcher")) {
        dispatcher_free(&dispatcher);
        thread_free(&thread);
        return TRUE;
    }

    check(failed, matches(cattr_dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[0], FALSE, 0),
        STATUS_CODE_SUCCESS, 0x0B, 0x05), "enqueue first value");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[1], TRUE, 0),
        STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x06), "cancel pending tail with earlier work retained");
    check(failed, dispatcher_count(&dispatcher) == 1, "cancellation preserves earlier pending work");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[1], FALSE, 0),
        STATUS_CODE_SUCCESS, 0x0B, 0x05), "enqueue second value");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[2], FALSE, 0),
        STATUS_CODE_SUCCESS, 0x0B, 0x05), "enqueue third value");
    check(failed, cattr_dispatcher_count(&dispatcher) == 3, "dispatcher at capacity");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[0], FALSE, 0),
        STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x03), "full dispatcher rejects nonblocking admission");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[0], TRUE, 10),
        STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x03), "full dispatcher times out awaiting admission");
    check(failed, dispatcher_count(&dispatcher) == 3 && state.length == 0, "failed admissions preserve pending work");

    result = thread_start(&thread);
    if (check(failed, matches(result, STATUS_CODE_SUCCESS, 0x06, 0x03), "start ordering thread")) {
        if (!check(failed, matches(cattr_thread_wait(&thread), STATUS_CODE_SUCCESS, 0x07, 0x04), "join ordering thread")) return FALSE;
        check(failed, matches(state.outcome, STATUS_CODE_SUCCESS, 0x0E, 0x06), "worker completes three polls");
        check(failed, state.length == 3 && state.values[0] == (rule == 0 ? 1 : 3) &&
            state.values[1] == 2 && state.values[2] == (rule == 0 ? 3 : 1), "selected execution order");
        check(failed, dispatcher_count(&dispatcher) == 0, "all pending work removed");
        check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[0], FALSE, 0),
            STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x0A), "joined thread rejects submission");
    }
    check(failed, matches(dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS, 0x0F, 0x02), "close ordering dispatcher");
    check(failed, matches(dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS_NOOP, 0x0F, 0x01), "repeat close");
    check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[0], FALSE, 0),
        STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x01), "closed dispatcher rejects submission");
    check(failed, matches(dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS, 0x10, 0x08), "free ordering dispatcher");
    check(failed, matches(dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01), "repeat dispatcher free");
    check(failed, matches(thread_free(&thread), STATUS_CODE_SUCCESS, 0x09, 0x05), "free ordering thread");
    check(failed, matches(thread_free(&thread), STATUS_CODE_SUCCESS_NOOP, 0x09, 0x01), "repeat thread free");
    return TRUE;
}

/* The associated thread observes pending work, then closes without polling.
 * The awaiting submitter must return interrupted, not execute the callback. */
static void cancel_worker(POINTERW arguments) {
    TEST_STATE* state = (TEST_STATE*) arguments;
    state->outcome = dispatcher_wait(state->dispatcher, 5000);
    if (matches(state->outcome, STATUS_CODE_SUCCESS, 0x0D, 0x04)) {
        state->outcome = dispatcher_close(state->dispatcher);
    }
}

static BOOL test_cancel(int* failed, PROCESS* process) {
    std::printf("\nAwaited cancellation\n");
    THREAD thread;
    DISPATCHER dispatcher;
    TEST_STATE state;
    state.dispatcher = &dispatcher;
    state.executed = 0;
    state.outcome = RESULT(STATUS_CODE_ERROR, 0, 0, 0);

    RESULT result = thread_create(&thread, process, (POINTERX) cancel_worker, &state);
    if (!check(failed, matches(result, STATUS_CODE_SUCCESS, 0x04, 0x03), "create cancellation thread")) {
        thread_free(&thread);
        return TRUE;
    }
    result = dispatcher_create(&dispatcher, &thread, 0, 1);
    if (!check(failed, matches(result, STATUS_CODE_SUCCESS, 0x0A, 0x07), "create cancellation dispatcher")) {
        dispatcher_free(&dispatcher);
        thread_free(&thread);
        return TRUE;
    }
    result = thread_start(&thread);
    if (check(failed, matches(result, STATUS_CODE_SUCCESS, 0x06, 0x03), "start cancellation thread")) {
        check(failed, matches(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, TRUE, 5000),
            STATUS_CODE_ERROR_INTERRUPTED, 0x0B, 0x08), "close interrupts awaited submission");
        if (!check(failed, matches(thread_wait(&thread), STATUS_CODE_SUCCESS, 0x07, 0x04), "join cancellation thread")) return FALSE;
        check(failed, matches(state.outcome, STATUS_CODE_SUCCESS, 0x0F, 0x02), "worker closed dispatcher");
        check(failed, dispatcher_count(&dispatcher) == 0 && state.executed == 0, "cancelled invocation never executed");
    }
    check(failed, status_code_is_success(dispatcher_close(&dispatcher).status), "ensure dispatcher closed");
    check(failed, matches(dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS, 0x10, 0x08), "free cancellation dispatcher");
    check(failed, matches(thread_free(&thread), STATUS_CODE_SUCCESS, 0x09, 0x05), "free cancellation thread");
    return TRUE;
}

/* A separate submitting thread attempts synchronous zero-timeout work
 * while the associated thread deliberately does not poll. */
struct WRAP_CANCEL {
    DISPATCHER* dispatcher;
    ORDER_ARGUMENT* argument;
    RESULT outcome;
};

static void cancel_wrapped(POINTERW arguments) {
    WRAP_CANCEL* state = (WRAP_CANCEL*) arguments;
    state->outcome = dispatcher_submit(state->dispatcher, (POINTERX) append_value, state->argument, TRUE, 0);
}

static BOOL test_wrapped_cancel(int* failed, PROCESS* process) {
    std::printf("\nWrapped FIFO cancellation\n");
    THREAD current;
    if (!check(failed, status_code_is_success(thread_current(&current, process).status), "capture wrapped FIFO thread")) {
        thread_free(&current);
        return TRUE;
    }
    DISPATCHER dispatcher;
    if (!check(failed, status_code_is_success(dispatcher_create(&dispatcher, &current, 0, 3).status), "create wrapped FIFO")) {
        dispatcher_free(&dispatcher);
        thread_free(&current);
        return TRUE;
    }
    ORDER_STATE state;
    state.dispatcher = &dispatcher;
    state.length = 0;
    ORDER_ARGUMENT items[3];
    for (BYTE i = 0; i < (BYTE) 3; ++i) {
        items[i].state = &state;
        items[i].value = (BYTE) (i + (BYTE) 1);
    }
    for (BYTE i = 0; i < (BYTE) 2; ++i) {
        check(failed, status_code_is_success(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[i], FALSE, 0).status), "fill FIFO");
    }
    check(failed, status_code_is_success(dispatcher_poll(&dispatcher).status), "advance FIFO head");
    check(failed, status_code_is_success(dispatcher_submit(&dispatcher, (POINTERX) append_value, &items[2], FALSE, 0).status), "enqueue behind advanced head");
    WRAP_CANCEL cancellation;
    cancellation.dispatcher = &dispatcher;
    cancellation.argument = &items[0];
    THREAD submitter;
    RESULT created = thread_create(&submitter, process, (POINTERX) cancel_wrapped, &cancellation);
    if (check(failed, status_code_is_success(created.status), "create wrapped canceller") &&
        check(failed, status_code_is_success(thread_start(&submitter).status), "start wrapped canceller")) {
        if (!check(failed, status_code_is_success(thread_wait(&submitter).status), "join wrapped canceller")) return FALSE;
        check(failed, matches(cancellation.outcome, STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x06), "cancel wrapped tail");
    }
    check(failed, status_code_is_success(thread_free(&submitter).status), "free wrapped canceller");
    check(failed, dispatcher_count(&dispatcher) == 2, "retain two pending members");
    check(failed, status_code_is_success(dispatcher_poll(&dispatcher).status), "poll retained second member");
    check(failed, status_code_is_success(dispatcher_poll(&dispatcher).status), "poll retained third member");
    check(failed, state.length == 3 && state.values[0] == 1 && state.values[1] == 2 && state.values[2] == 3, "wrapped cancellation preserves FIFO order");
    check(failed, status_code_is_success(dispatcher_close(&dispatcher).status), "close wrapped FIFO");
    check(failed, status_code_is_success(dispatcher_free(&dispatcher).status), "free wrapped FIFO");
    check(failed, status_code_is_success(thread_free(&current).status), "free wrapped FIFO thread");
    return TRUE;
}

struct PRODUCER {
    DISPATCHER* dispatcher;
    BYTE count;
    RESULT outcome;
};

static void count_producer(POINTERW arguments) {
    PRODUCER* producer = (PRODUCER*) arguments;
    ++producer->count;
}

static void submit_many(POINTERW arguments) {
    PRODUCER* producer = (PRODUCER*) arguments;
    /* Alternate asynchronous and awaited work; final awaited work establishes
     * completion of all earlier FIFO submissions from this producer. */
    for (BYTE i = 0; i < (BYTE) 64; ++i) {
        producer->outcome = dispatcher_submit(producer->dispatcher, (POINTERX) count_producer,
            producer, (BOOL) (i & (BYTE) 1), 5000);
        if (!status_code_is_success(producer->outcome.status)) return;
    }
}

static BOOL test_contention(int* failed, PROCESS* process, NUINT capacity) {
    std::printf("\nConcurrent producers\n");
    THREAD execution;
    DISPATCHER dispatcher;
    TEST_STATE state;
    state.dispatcher = &dispatcher;
    state.process = process;
    state.thread = &execution;
    state.identity = FALSE;
    RESULT created = thread_create(&execution, process, (POINTERX) worker, &state);
    if (!check(failed, status_code_is_success(created.status), "create contended worker")) {
        thread_free(&execution);
        return TRUE;
    }
    if (!check(failed, status_code_is_success(dispatcher_create(&dispatcher, &execution, 0, capacity).status), "create contended dispatcher")) {
        dispatcher_free(&dispatcher);
        thread_free(&execution);
        return TRUE;
    }
    if (check(failed, status_code_is_success(thread_start(&execution).status), "start contended worker")) {
        THREAD threads[4];
        PRODUCER producers[4];
        BYTE started = 0;
        for (BYTE i = 0; i < (BYTE) 4; ++i) {
            producers[i].dispatcher = &dispatcher;
            producers[i].count = 0;
            if (!check(failed, status_code_is_success(thread_create(&threads[i], process, (POINTERX) submit_many, &producers[i]).status), "create producer") ||
                !check(failed, status_code_is_success(thread_start(&threads[i]).status), "start producer")) {
                thread_free(&threads[i]);
                break;
            }
            ++started;
        }
        for (BYTE i = 0; i < started; ++i) {
            if (!check(failed, status_code_is_success(thread_wait(&threads[i]).status), "join producer")) return FALSE;
        }
        check(failed, status_code_is_success(dispatcher_close(&dispatcher).status), "close contended dispatcher");
        if (!check(failed, status_code_is_success(thread_wait(&execution).status), "join contended worker")) return FALSE;
        for (BYTE i = 0; i < started; ++i) {
            check(failed, status_code_is_success(producers[i].outcome.status) && producers[i].count == (BYTE) 64, "producer completed exactly 64 callbacks");
            check(failed, status_code_is_success(thread_free(&threads[i]).status), "free producer");
        }
        check(failed, dispatcher_count(&dispatcher) == 0, "no pending work after shutdown");
        check(failed, state.outcome.status == STATUS_CODE_ERROR_INTERRUPTED, "contended worker closed normally");
    } else {
        dispatcher_close(&dispatcher);
    }
    check(failed, status_code_is_success(dispatcher_free(&dispatcher).status), "free contended dispatcher");
    check(failed, status_code_is_success(thread_free(&execution).status), "free contended worker");
    return TRUE;
}

static POINTER reject_selection(const SET* work) {
    (void) work;
    return 0;
}

static BOOL test_invalid_rule(int* failed, PROCESS* process) {
    std::printf("\nInvalid custom selection\n");
    THREAD current;
    if (!check(failed, status_code_is_success(thread_current(&current, process).status), "capture rule thread")) {
        thread_free(&current);
        return TRUE;
    }
    DISPATCHER dispatcher;
    if (check(failed, status_code_is_success(dispatcher_create(&dispatcher, &current, reject_selection, 2).status), "create invalid rule dispatcher")) {
        TEST_STATE state;
        state.executed = 0;
        check(failed, status_code_is_success(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, FALSE, 0).status), "submit work to custom rule");
        check(failed, matches(dispatcher_poll(&dispatcher), STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x05), "reject nonmember rule result");
        check(failed, dispatcher_count(&dispatcher) == 1 && state.executed == 0, "rejected rule preserves pending work");
        check(failed, status_code_is_success(dispatcher_close(&dispatcher).status) && state.executed == 0, "close cancels unselected work");
    }
    check(failed, status_code_is_success(dispatcher_free(&dispatcher).status), "free invalid rule dispatcher");
    check(failed, status_code_is_success(thread_free(&current).status), "free rule thread");
    return TRUE;
}

int main() {
    int failed = 0;
    std::printf("Process and thread identity\n");
    PROCESS process;
    PROCESS other;
    RESULT result = process_current(&process);
    if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x01, 0x01), "capture process")) {
        process_free(&process);
        return 1;
    }
    result = process_current(&other);
    if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x01, 0x01), "capture same process")) {
        check(&failed, process_equals(&process, &other), "process identity");
    }
    check(&failed, status_code_is_success(process_free(&other).status), "release other process");

    /* Zero capacity is a reported creation failure, and failed creation is freeable. */
    {
        THREAD current;
        result = thread_current(&current, &process);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x05, 0x04), "capture for failed creation")) {
            THREAD same;
            result = cattr_thread_current(&same, &process);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x05, 0x04), "second current thread capture")) {
                check(&failed, thread_equals(&current, &same) && thread_equals(&same, &same), "current thread equality");
            }
            check(&failed, status_code_is_success(thread_free(&same).status), "release second capture");
            DISPATCHER invalid;
            check(&failed, matches(dispatcher_create(&invalid, &current, 0, 0),
                STATUS_CODE_ERROR_INVALID_ARGUMENT, 0x0A, 0x01, 3), "zero capacity rejected");
            check(&failed, matches(dispatcher_free(&invalid), STATUS_CODE_SUCCESS_NOOP, 0x10, 0x01), "free failed dispatcher");
        }
        check(&failed, status_code_is_success(thread_free(&current).status), "release capture");
    }

    /* A captured current thread can wait and poll without starting a worker.
     * Exercise a real finite timeout, empty polling, and queued self-submission. */
    {
        THREAD current;
        result = thread_current(&current, &process);
        if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x05, 0x04), "capture current thread")) {
            DISPATCHER local;
            result = dispatcher_create(&local, &current, 0, 2);
            if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x0A, 0x07), "create local dispatcher")) {
                check(&failed, matches(dispatcher_wait(&local, 10), STATUS_CODE_ERROR_TIMEOUT, 0x0D, 0x02), "finite monotonic wait");
                check(&failed, matches(dispatcher_poll(&local), STATUS_CODE_SUCCESS_NOOP, 0x0E, 0x04), "empty poll");
                TEST_STATE local_state;
                local_state.executed = 0;
                check(&failed, matches(dispatcher_submit(&local, (POINTERX) increment, &local_state, TRUE, 0),
                    STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x02), "same-thread awaited submission rejected");
                check(&failed, matches(dispatcher_submit(&local, (POINTERX) increment, &local_state, FALSE, CATALYST_NUINT_MAX),
                    STATUS_CODE_SUCCESS, 0x0B, 0x05), "same-thread asynchronous submission");
                check(&failed, matches(dispatcher_submit(&local, (POINTERX) increment, &local_state, FALSE, 0),
                    STATUS_CODE_SUCCESS, 0x0B, 0x05), "second same-thread submission");
                check(&failed, matches(dispatcher_submit(&local, (POINTERX) increment, &local_state, FALSE, CATALYST_NUINT_MAX),
                    STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x03), "same-thread full admission never blocks");
                check(&failed, dispatcher_count(&local) == 2 && local_state.executed == 0, "self-submissions remain pending");
                check(&failed, matches(dispatcher_poll(&local), STATUS_CODE_SUCCESS, 0x0E, 0x06), "poll first self-submission");
                check(&failed, matches(dispatcher_poll(&local), STATUS_CODE_SUCCESS, 0x0E, 0x06) &&
                    local_state.executed == 2, "poll second self-submission");
                check(&failed, status_code_is_success(dispatcher_close(&local).status), "close local dispatcher");
            }
            check(&failed, status_code_is_success(dispatcher_free(&local).status), "release local dispatcher");
        }
        check(&failed, status_code_is_success(thread_free(&current).status), "release current thread");
    }

    std::printf("\nDuplicate submissions and worker lifecycle\n");
    THREAD thread;
    DISPATCHER dispatcher;
    TEST_STATE state;
    state.dispatcher = &dispatcher;
    state.process = &process;
    state.thread = &thread;
    state.identity = FALSE;
    state.executed = 0;
    result = thread_create(&thread, &process, (POINTERX) worker, &state);
    if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x04, 0x03), "create thread")) {
        thread_free(&thread);
        process_free(&process);
        return 1;
    }
    result = dispatcher_create(&dispatcher, &thread, 0, 4);
    if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x0A, 0x07), "create dispatcher")) {
        dispatcher_free(&dispatcher);
        thread_free(&thread);
        process_free(&process);
        return 1;
    }

    check(&failed, matches(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, TRUE, 0),
        STATUS_CODE_ERROR_TIMEOUT, 0x0B, 0x06), "pending synchronous timeout");
    check(&failed, dispatcher_count(&dispatcher) == 0, "timeout removes invocation");
    check(&failed, matches(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, FALSE, 0),
        STATUS_CODE_SUCCESS, 0x0B, 0x05), "first submission");
    check(&failed, matches(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, FALSE, 0),
        STATUS_CODE_SUCCESS, 0x0B, 0x05), "duplicate executable submission");
    check(&failed, dispatcher_count(&dispatcher) == 2, "independent pending submissions");

    result = thread_start(&thread);
    if (check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x06, 0x03), "start thread")) {
        /* FIFO ensures this awaited callback follows both asynchronous ones. */
        check(&failed, matches(dispatcher_submit(&dispatcher, (POINTERX) increment, &state, TRUE, 5000),
            STATUS_CODE_SUCCESS, 0x0B, 0x09), "await third submission");
        check(&failed, matches(dispatcher_close(&dispatcher), STATUS_CODE_SUCCESS, 0x0F, 0x02), "close dispatcher");
        result = thread_wait(&thread);
        if (!check(&failed, matches(result, STATUS_CODE_SUCCESS, 0x07, 0x04), "join thread")) return 1;
        check(&failed, state.identity, "worker sees published identity");
        check(&failed, state.executed == 3, "all three invocations executed");
        check(&failed, matches(state.outcome, STATUS_CODE_ERROR_INTERRUPTED, 0x0D, 0x03)
            || matches(state.outcome, STATUS_CODE_ERROR_INTERRUPTED, 0x0E, 0x02), "close wakes worker");
        check(&failed, matches(thread_wait(&thread), STATUS_CODE_SUCCESS_NOOP, 0x07, 0x01), "repeat join");
    } else {
        dispatcher_close(&dispatcher);
    }
    check(&failed, matches(dispatcher_free(&dispatcher), STATUS_CODE_SUCCESS, 0x10, 0x08), "release dispatcher");
    check(&failed, matches(thread_free(&thread), STATUS_CODE_SUCCESS, 0x09, 0x05), "release thread");

    if (!test_order(&failed, &process, 0)) return 1;
    if (!test_order(&failed, &process, select_last)) return 1;
    if (!test_cancel(&failed, &process)) return 1;
    if (!test_invalid_rule(&failed, &process)) return 1;
    if (!test_wrapped_cancel(&failed, &process)) return 1;
    if (!test_contention(&failed, &process, 2)) return 1;
    if (!test_contention(&failed, &process, 32)) return 1;

    check(&failed, matches(process_free(&process), STATUS_CODE_SUCCESS, 0x03, 0x02), "release process");
    check(&failed, matches(process_free(&process), STATUS_CODE_SUCCESS_NOOP, 0x03, 0x01), "repeat process free");
    std::printf("\nCATTRLIB: %d failed check(s)\n", failed);
    return failed != 0 ? 1 : 0;
}
