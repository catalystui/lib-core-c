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

int main() {
    int failed = 0;
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

    /* A captured current thread can wait and poll without starting a worker.
     * Exercise a real finite timeout, empty polling, and inline execution. */
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
                    STATUS_CODE_SUCCESS, 0x0B, 0x02) && local_state.executed == 1, "same-thread submission");
                check(&failed, status_code_is_success(dispatcher_close(&local).status), "close local dispatcher");
            }
            check(&failed, status_code_is_success(dispatcher_free(&local).status), "release local dispatcher");
        }
        check(&failed, status_code_is_success(thread_free(&current).status), "release current thread");
    }

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
    check(&failed, matches(process_free(&process), STATUS_CODE_SUCCESS, 0x03, 0x02), "release process");
    return failed != 0 ? 1 : 0;
}
