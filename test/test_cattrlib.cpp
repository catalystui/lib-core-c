#include "cattrlib.hpp"
#include <stdio.h>

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
    if (!condition) printf("FAIL: %s\n", message);
    return condition;
}

#define CHECK(condition) if (!check((condition) ? TRUE : FALSE, #condition)) return 1

int main() {
    PROCESS current;
    PROCESS unsupported;
    CHECK(process_create(&unsupported, (FUNCTION) identity, &input).status == STATUS_CODE_ERROR_NOT_IMPLEMENTED);
    CHECK(process_free(&unsupported).status == STATUS_CODE_SUCCESS_NOOP);
    CHECK(status_code_is_success(process_current(&process).status));
    CHECK(status_code_is_success(process_current(&current).status));
    CHECK(process_equals(&process, &current));
    CHECK(status_code_is_success(process_free(&current).status));

    THREAD main_thread;
    THREAD same_thread;
    DISPATCHER local;
    CHECK(status_code_is_success(thread_current(&main_thread, &process).status));
    CHECK(status_code_is_success(thread_current(&same_thread, &process).status));
    CHECK(thread_equals(&main_thread, &same_thread));
    CHECK(status_code_is_success(thread_free(&same_thread).status));
    CHECK(status_code_is_success(dispatcher_create(&local, &main_thread, 0, 1).status));
    CHECK(dispatcher_poll(&local).status == STATUS_CODE_SUCCESS_NOOP);
    CHECK(dispatcher_wait(&local, 0).status == STATUS_CODE_ERROR_TIMEOUT);
    POINTERW output = 0;
    CHECK(status_code_is_success(dispatcher_submit_function(&local, (FUNCTION) identity, &input, TRUE, 0, &output).status));
    CHECK(output == &input);
    CHECK(dispatcher_count(&local) == 0);
    CHECK(status_code_is_success(dispatcher_close(&local).status));
    CHECK(dispatcher_submit_procedure(&local, (PROCEDURE) record, &input, FALSE, 0).status == STATUS_CODE_ERROR_INVALID_STATE);
    CHECK(status_code_is_success(dispatcher_free(&local).status));
    CHECK(status_code_is_success(dispatcher_create(&local, &main_thread, (FUNCTION) last_member, 2).status));
    THREAD producer;
    CHECK(status_code_is_success(thread_create(&producer, &process, (PROCEDURE) submit_two, &local).status));
    CHECK(status_code_is_success(thread_start(&producer).status));
    CHECK(status_code_is_success(thread_wait(&producer).status));
    CHECK(status_code_is_success(worker_result.status));
    CHECK(dispatcher_count(&local) == 2);
    observed = 0;
    CHECK(status_code_is_success(dispatcher_poll(&local).status));
    CHECK(observed == 0 && dispatcher_count(&local) == 1);
    CHECK(status_code_is_success(dispatcher_poll(&local).status));
    CHECK(observed == input && dispatcher_count(&local) == 0);
    CHECK(status_code_is_success(dispatcher_close(&local).status));
    CHECK(status_code_is_success(dispatcher_free(&local).status));
    CHECK(status_code_is_success(thread_free(&producer).status));
    CHECK(status_code_is_success(thread_free(&main_thread).status));

    CHECK(status_code_is_success(thread_create(&worker, &process, (PROCEDURE) run, 0).status));
    CHECK(status_code_is_success(dispatcher_create(&dispatcher, &worker, 0, 2).status));
    output = &input;
    CHECK(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, 0, &output).status == STATUS_CODE_ERROR_TIMEOUT);
    CHECK(output == 0 && dispatcher_count(&dispatcher) == 0);
    observed = 0;
    CHECK(status_code_is_success(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, FALSE, 0).status));
    CHECK(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, FALSE, 0).status == STATUS_CODE_SUCCESS_NOOP);
    CHECK(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record, &input, TRUE, 0).status == STATUS_CODE_ERROR_INVALID_STATE);
    CHECK(dispatcher_count(&dispatcher) == 1);
    CHECK(status_code_is_success(dispatcher_submit_procedure(&dispatcher, (PROCEDURE) record_after, &input, FALSE, 0).status));
    CHECK(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, 0, &output).status == STATUS_CODE_ERROR_TIMEOUT);
    CHECK(dispatcher_count(&dispatcher) == 2);
    CHECK(status_code_is_success(thread_start(&worker).status));
    output = 0;
    CHECK(status_code_is_success(dispatcher_submit_function(&dispatcher, (FUNCTION) identity, &input, TRUE, CATALYST_NUINT_MAX, &output).status));
    CHECK(output == &input && observed == 9);
    CHECK(status_code_is_success(dispatcher_close(&dispatcher).status));
    CHECK(status_code_is_success(thread_wait(&worker).status));
    CHECK(status_code_is_success(worker_result.status));
    CHECK(status_code_is_success(dispatcher_free(&dispatcher).status));
    CHECK(status_code_is_success(thread_free(&worker).status));
    CHECK(status_code_is_success(process_free(&process).status));
    printf("CATTRLIB tests passed.\n");
    return 0;
}

#else
int main() {
    printf("CATTRLIB native threading backend unavailable.\n");
    return 0;
}
#endif
