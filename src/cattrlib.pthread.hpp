#ifndef CATTRLIB_PTHREAD_HPP
#define CATTRLIB_PTHREAD_HPP

/* Private native implementation included only by cattrlib.linux.cpp and
 * cattrlib.darwin.mm. Both systems expose pthreads as their supported
 * user-space thread ABI. Platform-specific wait and condition setup are
 * supplied by those files; no pthread types enter portable CATTRLIB. */
#include "cattrlib.internal.hpp"
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sched.h>

using namespace catalyst;

static RESULT cattr_native_condition_init(pthread_cond_t* condition);
static RESULT cattr_native_condition_timedwait(pthread_cond_t* condition, pthread_mutex_t* lock, NUINT timeout);

struct CATTRLIB_NATIVE_THREAD {
    pthread_t handle;
    pthread_mutex_t gate;
    PROCEDURE procedure;
    POINTERW arguments;
    BOOL gated;
};

struct CATTRLIB_NATIVE_MONITOR {
    pthread_mutex_t lock;
    pthread_cond_t changed;
    BOOL condition_live;
    BOOL lock_live;
};

CATALYST_RESULT cattr_platform_process_capture(POINTERW* output) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(pid_t));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x02, 0x01, allocation.detail);
    *(pid_t*) *output = getpid();
    return RESULT(STATUS_CODE_SUCCESS, 0x02, 0x02, 0);
}

CATALYST_BOOL cattr_platform_process_equals(POINTER first, POINTER second) {
    return *(const pid_t*) first == *(const pid_t*) second ? TRUE : FALSE;
}

CATALYST_BOOL cattr_platform_process_is_current(POINTER value) {
    return *(const pid_t*) value == getpid() ? TRUE : FALSE;
}

/* The creation gate publishes the native handle before user code runs. */
static void* cattr_thread_entry(void* value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    pthread_mutex_lock(&state->gate);
    pthread_mutex_unlock(&state->gate);
    ((void (*)(POINTERW)) state->procedure)(state->arguments);
    return 0;
}

CATALYST_RESULT cattr_platform_thread_capture(POINTERW* output) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x08, 0x01, allocation.detail);
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) *output;
    state->handle = pthread_self();
    state->gated = FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_start(POINTERW* output, PROCEDURE procedure, POINTERW arguments) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x09, 0x01, allocation.detail);
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) *output;
    state->procedure = procedure;
    state->arguments = arguments;
    if (pthread_mutex_init(&state->gate, 0) != 0) {
        return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x09, 0x02, 0);
    }
    state->gated = TRUE;
    pthread_mutex_lock(&state->gate);
    int error = pthread_create(&state->handle, 0, cattr_thread_entry, state);
    pthread_mutex_unlock(&state->gate);
    if (error != 0) {
        return RESULT(error == EAGAIN ? STATUS_CODE_ERROR_ALLOCATION_FAILED : STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x09, 0x03, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x04, 0);
}

CATALYST_RESULT cattr_platform_thread_wait(POINTERW value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    if (pthread_join(state->handle, 0) != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x0A, 0x01, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

BOOL cattr_platform_thread_equals(POINTER first, POINTER second) {
    return pthread_equal(((const CATTRLIB_NATIVE_THREAD*) first)->handle, ((const CATTRLIB_NATIVE_THREAD*) second)->handle) ? TRUE : FALSE;
}

BOOL cattr_platform_thread_is_current(POINTER value) {
    return pthread_equal(((const CATTRLIB_NATIVE_THREAD*) value)->handle, pthread_self()) ? TRUE : FALSE;
}

CATALYST_RESULT cattr_platform_thread_free(POINTERW value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    if (state->gated) {
        if (pthread_mutex_destroy(&state->gate) != 0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x01, 0);
        state->gated = FALSE;
    }
    RESULT release = catmm_free(value);
    return RESULT(release.status, 0x0B, 0x02, release.detail);
}

CATALYST_RESULT cattr_platform_monitor_create(POINTERW* output) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_MONITOR));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0C, 0x01, allocation.detail);
    CATTRLIB_NATIVE_MONITOR* state = (CATTRLIB_NATIVE_MONITOR*) *output;
    if (pthread_mutex_init(&state->lock, 0) != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x0C, 0x02, 0);
    state->lock_live = TRUE;
    RESULT condition = cattr_native_condition_init(&state->changed);
    if (!status_code_is_success(condition.status)) return RESULT(condition.status, 0x0C, 0x06, condition.detail);
    state->condition_live = TRUE;
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x05, 0);
}

void cattr_platform_monitor_lock(POINTERW value) {
    pthread_mutex_lock(&((CATTRLIB_NATIVE_MONITOR*) value)->lock);
}

void cattr_platform_monitor_unlock(POINTERW value) {
    pthread_mutex_unlock(&((CATTRLIB_NATIVE_MONITOR*) value)->lock);
}

void cattr_platform_monitor_notify(POINTERW value) {
    pthread_cond_broadcast(&((CATTRLIB_NATIVE_MONITOR*) value)->changed);
}

CATALYST_RESULT cattr_platform_monitor_wait(POINTERW value, NUINT timeout) {
    CATTRLIB_NATIVE_MONITOR* state = (CATTRLIB_NATIVE_MONITOR*) value;
    if (timeout != CATALYST_NUINT_MAX) {
        RESULT wait = cattr_native_condition_timedwait(&state->changed, &state->lock, timeout);
        return RESULT(wait.status, 0x0D, 0x06, wait.detail);
    }
    int error = pthread_cond_wait(&state->changed, &state->lock);
    if (error == ETIMEDOUT) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x0D, 0x02, 0);
    if (error != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x0D, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x04, 0);
}

CATALYST_RESULT cattr_platform_monitor_free(POINTERW value) {
    CATTRLIB_NATIVE_MONITOR* state = (CATTRLIB_NATIVE_MONITOR*) value;
    if (state->condition_live) {
        if (pthread_cond_destroy(&state->changed) != 0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x01, 0);
        state->condition_live = FALSE;
    }
    if (state->lock_live) {
        if (pthread_mutex_destroy(&state->lock) != 0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0E, 0x02, 0);
        state->lock_live = FALSE;
    }
    RESULT release = catmm_free(value);
    return RESULT(release.status, 0x0E, 0x03, release.detail);
}

void cattr_platform_yield() {
    sched_yield();
}

CATALYST_RESULT cattr_platform_ticks(NUINT* ticks) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x14, 0x01, 0);
    }
    /* Native time fields are converted only here. The portable caller
     * measures short deltas in modulo pointer-width milliseconds. */
    *ticks = (NUINT) ((NUINT) now.tv_sec * (NUINT) 1000 + (NUINT) (now.tv_nsec / 1000000L));
    return RESULT(STATUS_CODE_SUCCESS, 0x14, 0x02, 0);
}
#endif /* CATTRLIB_PTHREAD_HPP */
