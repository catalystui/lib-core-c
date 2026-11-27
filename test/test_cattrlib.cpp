#include "cattrlib.hpp"
#include <cstdio>

using namespace catalyst;

#if TARGET_PLATFORM_WIN32 || TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN

static RESULT worker_result;
static DISPATCHER dispatcher;
static PROCESS process;
static THREAD worker;
static BYTE input = 7;
static BYTE observed = 0;

static void record(POINTERW arguments) {
    observed = *(BYTE*) arguments;
}

static void record_after(POINTERW arguments) {
    observed = observed == *(BYTE*) arguments ? (BYTE) 9 : (BYTE) 0;
}

static POINTERW identity(POINTERW arguments) {
    return arguments;
}

static void run(POINTERW arguments) {
    (void) arguments;
    for (;;) {
        RESULT result = dispatcher_wait(&dispatcher, CATALYST_NUINT_MAX);
        if (result.status == STATUS_CODE_ERROR_INTERRUPTED) return;
        if (!status_code_is_success(result.status)) {
            worker_result = result;
            return;
        }
        result = dispatcher_poll(&dispatcher);
        if (!status_code_is_success(result.status)) {
            if (result.status == STATUS_CODE_ERROR_INTERRUPTED) return;
            worker_result = result;
            return;
        }
    }
}

static POINTERW last_member(const SET* work) {
    return ((PROCEDURE*) work->data) + (NUINT) (work->cardinality - (NUINT) 1);
}

static void submit_two(POINTERW arguments) {
    DISPATCHER* local = (DISPATCHER*) arguments;
    worker_result = dispatcher_submit_procedure(local, (PROCEDURE) record, &input, FALSE, 0);
    if (!status_code_is_success(worker_result.status)) return;
    worker_result = dispatcher_submit_procedure(local, (PROCEDURE) record_after, &input, FALSE, 0);
}

static BOOL check(BOOL condition, const char* message) {
    std::printf("%s: %s\n", condition ? "PASS" : "FAIL", message);
    std::fflush(stdout);
    return condition;
}

#define CHECK(condition, name) if (!check((condition) ? TRUE : FALSE, name)) return 1

int main() {
    PROCESS current;
    PROCESS unsupported;
    CHECK(process_create(&unsupported, (FUNCTION) identity, &input).status == STATUS_CODE_ERROR_NOT_IMPLEMENTED, "process_create reports unsupported native launch");
    CHECK(process_free(&unsupported).status == STATUS_CODE_SUCCESS_NOOP, "process_free empty process is a no-op");
    CHECK(status_code_is_success(process_current(&process).status), "process_current captures the current process");
    CHECK(status_code_is_success(process_current(&current).status), "process_current captures a second handle");
    CHECK(process_equals(&process, &current), "process_equals identifies the same process");
    CHECK(status_code_is_success(process_free(&current).status), "process_free second process handle");

    THREAD main_thread;
    THREAD same_thread;
    DISPATCHER local;
    CHECK(status_code_is_success(thread_current(&main_thread, &process).status), "thread_current captures the main thread");
    CHECK(status_code_is_success(thread_current(&same_thread, &process).status), "thread_current captures a second thread handle");
    CHECK(thread_equals(&main_thread, &same_thread), "thread_equals identifies the same thread");
    CHECK(status_code_is_success(thread_free(&same_thread).status), "thread_free second thread handle");
    CHECK(status_code_is_success(dispatcher_create(&local, &main_thread, 0, 1).status), "dispatcher_create with capacity one");
    CHECK(dispatcher_poll(&local).status == STATUS_CODE_SUCCESS_NOOP, "dispatcher_poll empty work set is a no-op");
    CHECK(dispatcher_wait(&local, 0).status == STATUS_CODE_ERROR_TIMEOUT, "dispatcher_wait empty work set times out");
    POINTERW output = 0;
    CHECK(status_code_is_success(dispatcher_submit_function(&local, (FUNCTION) identity, &input, TRUE, 0, &output).status), "dispatcher_submit_function executes immediately on the associated thread");
    CHECK(output == &input, "dispatcher_submit_function returns the function output");
    CHECK(dispatcher_count(&local) == 0, "dispatcher_count remains zero after immediate execution");
    CHECK(status_code_is_success(dispatcher_close(&local).status), "dispatcher_close closes the local dispatcher");
    CHECK(dispatcher_submit_procedure(&local, (PROCEDURE) record, &input, FALSE, 0).status == STATUS_CODE_ERROR_INVALID_STATE, "dispatcher_submit_procedure rejects submission after close");
    CHECK(status_code_is_success(dispatcher_free(&local).status), "dispatcher_free closed dispatcher");
    CHECK(status_code_is_success(dispatcher_create(&local, &main_thread, (FUNCTION) last_member, 2).status), "dispatcher_create with custom dispatch rule");
    THREAD producer;
    CHECK(status_code_is_success(thread_create(&producer, &process, (PROCEDURE) submit_two, &local).status), "thread_create producer thread");
    CHECK(status_code_is_success(thread_start(&producer).status), "thread_start producer thread");
    CHECK(status_code_is_success(thread_wait(&producer).status), "thread_wait producer thread");
    CHECK(status_code_is_success(worker_result.status), "producer submits both operations successfully");
    CHECK(dispatcher_count(&local) == 2, "dispatcher_count reports two pending operations");
    observed = 0;
    CHECK(status_code_is_success(dispatcher_poll(&local).status), "dispatcher_poll selects the last member");
    CHECK(observed == 0 && dispatcher_count(&local) == 1, "custom rule executes the second operation first");
    CHECK(status_code_is_success(dispatcher_poll(&local).status), "dispatcher_poll executes remaining work");
    CHECK(observed == input && dispatcher_count(&local) == 0, "custom rule leaves an empty work set");
    CHECK(status_code_is_success(dispatcher_close(&local).status), "dispatcher_close custom dispatcher");
    CHECK(status_code_is_success(dispatcher_free(&local).status), "dispatcher_free custom dispatcher");
    CHECK(status_code_is_success(thread_free(&producer).status), "thread_free joined producer thread");
    CHECK(status_code_is_success(thread_free(&main_thread).status), "thread_free captured main thread");

    CHECK(status_code_is_success(thread_create(&worker, &process, (PROCEDURE) run, 0).status), "thread_create worker thread");
    CHECK(status_code_is_success(dispatcher_create(&dispatcher, &worker, 0, 2).status), "dispatcher_create bounded worker dispatcher");
    output = &input;
    CHECK(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, 0, &output).status == STATUS_CODE_ERROR_TIMEOUT, "dispatcher_submit_function times out before execution");
    CHECK(output == 0 && dispatcher_count(&dispatcher) == 0, "timed-out submission clears output and removes pending work");
    observed = 0;
    CHECK(status_code_is_success(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, FALSE, 0).status), "dispatcher_submit_procedure accepts asynchronous work");
    CHECK(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, FALSE, 0).status == STATUS_CODE_SUCCESS_NOOP, "duplicate asynchronous submission is a no-op");
    CHECK(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, TRUE, 0).status == STATUS_CODE_ERROR_INVALID_STATE, "duplicate synchronous submission is rejected");
    CHECK(dispatcher_count(&dispatcher) == 1, "dispatcher_count excludes duplicate work");
    CHECK(status_code_is_success(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record_after, &input, FALSE, 0).status), "dispatcher_submit_procedure fills remaining capacity");
    CHECK(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, 0, &output).status == STATUS_CODE_ERROR_TIMEOUT, "dispatcher_submit_function times out when capacity is exhausted");
    CHECK(dispatcher_count(&dispatcher) == 2, "dispatcher_count preserves the full work set");
    CHECK(status_code_is_success(thread_start(&worker).status), "thread_start dispatcher worker");
    output = 0;
    CHECK(status_code_is_success(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, CATALYST_NUINT_MAX, &output).status), "dispatcher_submit_function waits for cross-thread completion");
    CHECK(output == &input && observed == 9, "cross-thread output and FIFO execution order");
    CHECK(status_code_is_success(dispatcher_close(&dispatcher).status), "dispatcher_close signals worker shutdown");
    CHECK(status_code_is_success(thread_wait(&worker).status), "thread_wait joins dispatcher worker");
    CHECK(status_code_is_success(worker_result.status), "worker exits without execution errors");
    CHECK(status_code_is_success(dispatcher_free(&dispatcher).status), "dispatcher_free worker dispatcher");
    CHECK(status_code_is_success(thread_free(&worker).status), "thread_free joined worker thread");
    CHECK(status_code_is_success(process_free(&process).status), "process_free current process handle");
    std::printf("\nAll CATTRLIB tests passed.\n");
    return 0;
}

#else
int main() {
    std::printf("CATTRLIB native threading backend unavailable.\n");
    return 0;
}
#endif
