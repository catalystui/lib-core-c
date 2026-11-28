#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN
#include "cattrlib.internal.hpp"
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>

using namespace catalyst;

/* Backend results are remapped at the shared API boundary. Native error
 * numbers are not Catalyst detail codes. */
static STATUS_CODE cattr_posix_status(int error) {
    switch (error) {
        case EAGAIN:
        case ENOMEM: return STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case EPERM:
        case EACCES: return STATUS_CODE_ERROR_ACCESS_DENIED;
        case ETIMEDOUT: return STATUS_CODE_ERROR_TIMEOUT;
        case EINTR: return STATUS_CODE_ERROR_INTERRUPTED;
        case ENOTSUP: return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        default: return STATUS_CODE_ERROR_INVALID_STATE;
    }
}

static RESULT cattr_posix_release(POINTERW* state) {
    RESULT result = catmm_heap_free(*state);
    if (status_code_is_success(result.status)) *state = 0;
    return result;
}

/* -------------------------------------------------------------------------------------------------
 * PROCESS IDENTITY
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT cattr_platform_process_current(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(pid_t));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x01, 0x01, allocation.detail);
    *(pid_t*) *state = getpid();
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x02, 0);
}

CATALYST_BOOL cattr_platform_process_equals(POINTER first, POINTER second) {
    return *(const pid_t*) first == *(const pid_t*) second ? TRUE : FALSE;
}

CATALYST_BOOL cattr_platform_process_is_current(POINTER state) {
    return *(const pid_t*) state == getpid() ? TRUE : FALSE;
}

CATALYST_RESULT cattr_platform_process_free(POINTERW* state) {
    RESULT release = cattr_posix_release(state);
    return RESULT(release.status, 0x02, 0x01, release.detail);
}

/* -------------------------------------------------------------------------------------------------
 * THREAD EXECUTION
 * ------------------------------------------------------------------------------------------------- */

struct CATTRLIB_POSIX_THREAD {
    CATTRLIB_THREAD_STATE common;
    pthread_t identity;
    pthread_mutex_t gate;
    BOOL gate_initialized;
};

/* The creator holds the gate until pthread_create publishes the identity.
 * The entry borrows state through completion; free follows join. */
static void* cattr_posix_entry(void* arguments) {
    THREAD* thread = (THREAD*) arguments;
    CATTRLIB_POSIX_THREAD* state = (CATTRLIB_POSIX_THREAD*) thread->state;
    pthread_mutex_lock(&state->gate);
    pthread_mutex_unlock(&state->gate);
    ((void (*)(POINTERW)) thread->entry)(thread->arguments);
    return 0;
}

CATALYST_RESULT cattr_platform_thread_create(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(CATTRLIB_POSIX_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0A, 0x01, allocation.detail);
    CATTRLIB_POSIX_THREAD* thread = (CATTRLIB_POSIX_THREAD*) *state;
    thread->gate_initialized = FALSE;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_current(POINTERW* state) {
    RESULT allocation = cattr_platform_thread_create(state);
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x03, 0x01, allocation.detail);
    CATTRLIB_POSIX_THREAD* thread = (CATTRLIB_POSIX_THREAD*) *state;
    thread->identity = pthread_self();
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_start(THREAD* owner) {
    CATTRLIB_POSIX_THREAD* thread = (CATTRLIB_POSIX_THREAD*) owner->state;
    thread->gate_initialized = FALSE;
    int error = pthread_mutex_init(&thread->gate, 0);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x04, 0x02, 0);
    thread->gate_initialized = TRUE;
    error = pthread_mutex_lock(&thread->gate);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x04, 0x05, 0);
    error = pthread_create(&thread->identity, 0, cattr_posix_entry, owner);
    pthread_mutex_unlock(&thread->gate);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x04, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x04, 0);
}

CATALYST_RESULT cattr_platform_thread_wait(POINTERW state) {
    int error = pthread_join(((CATTRLIB_POSIX_THREAD*) state)->identity, 0);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x05, 0x01, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x02, 0);
}

CATALYST_BOOL cattr_platform_thread_equals(POINTER first, POINTER second) {
    return pthread_equal(((const CATTRLIB_POSIX_THREAD*) first)->identity,
        ((const CATTRLIB_POSIX_THREAD*) second)->identity) ? TRUE : FALSE;
}

CATALYST_BOOL cattr_platform_thread_is_current(POINTER state) {
    return pthread_equal(((const CATTRLIB_POSIX_THREAD*) state)->identity, pthread_self()) ? TRUE : FALSE;
}

CATALYST_RESULT cattr_platform_thread_free(POINTERW* state) {
    CATTRLIB_POSIX_THREAD* thread = (CATTRLIB_POSIX_THREAD*) *state;
    if (thread->gate_initialized) {
        int error = pthread_mutex_destroy(&thread->gate);
        if (error != 0) return RESULT(cattr_posix_status(error), 0x06, 0x01, 0);
        thread->gate_initialized = FALSE;
    }
    RESULT release = cattr_posix_release(state);
    return RESULT(release.status, 0x06, 0x02, release.detail);
}

/* -------------------------------------------------------------------------------------------------
 * MONITOR
 * ------------------------------------------------------------------------------------------------- */

struct CATTRLIB_POSIX_MONITOR {
    CATTRLIB_DISPATCHER_STATE common;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
#if TARGET_PLATFORM_LINUX
    pthread_condattr_t attributes;
    BOOL attributes_initialized;
#endif
    BOOL mutex_initialized;
    BOOL condition_initialized;
};

CATALYST_RESULT cattr_platform_monitor_create(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(CATTRLIB_POSIX_MONITOR));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x07, 0x01, allocation.detail);
    CATTRLIB_POSIX_MONITOR* monitor = (CATTRLIB_POSIX_MONITOR*) *state;
    monitor->mutex_initialized = FALSE;
    monitor->condition_initialized = FALSE;
#if TARGET_PLATFORM_LINUX
    monitor->attributes_initialized = FALSE;
#endif
    int error = pthread_mutex_init(&monitor->mutex, 0);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x07, 0x02, 0);
    monitor->mutex_initialized = TRUE;
#if TARGET_PLATFORM_LINUX
    error = pthread_condattr_init(&monitor->attributes);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x07, 0x03, 0);
    monitor->attributes_initialized = TRUE;
    error = pthread_condattr_setclock(&monitor->attributes, CLOCK_MONOTONIC);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x07, 0x04, 0);
    error = pthread_cond_init(&monitor->condition, &monitor->attributes);
#else
    error = pthread_cond_init(&monitor->condition, 0);
#endif
    if (error != 0) return RESULT(cattr_posix_status(error), 0x07, 0x05, 0);
    monitor->condition_initialized = TRUE;
#if TARGET_PLATFORM_LINUX
    error = pthread_condattr_destroy(&monitor->attributes);
    if (error != 0) return RESULT(cattr_posix_status(error), 0x07, 0x06, 0);
    monitor->attributes_initialized = FALSE;
#endif
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x07, 0);
}

void cattr_platform_monitor_lock(POINTERW state) {
    pthread_mutex_lock(&((CATTRLIB_POSIX_MONITOR*) state)->mutex);
}

void cattr_platform_monitor_unlock(POINTERW state) {
    pthread_mutex_unlock(&((CATTRLIB_POSIX_MONITOR*) state)->mutex);
}

void cattr_platform_monitor_notify(POINTERW state) {
    pthread_cond_broadcast(&((CATTRLIB_POSIX_MONITOR*) state)->condition);
}

/* Finite timeout is milliseconds. Compute elapsed time from the original
 * monotonic sample, not from the last wake. Use bounded native wait slices
 * so a pointer-width timeout need never fit into a native absolute deadline.
 * Native seconds/nanoseconds are confined to the POSIX boundary.
 */
static BOOL cattr_posix_remaining(NUINT timeout, const timespec* start, const timespec* now, timespec* relative) {
    time_t elapsed_seconds = now->tv_sec - start->tv_sec;
    long elapsed_nanos = now->tv_nsec - start->tv_nsec;
    if (elapsed_nanos < 0) {
        --elapsed_seconds;
        elapsed_nanos += 1000000000L;
    }
    NUINT seconds = (NUINT) (timeout / (NUINT) 1000);
    long nanos = (long) (timeout % (NUINT) 1000) * 1000000L;
    /* Both sides are nonnegative. Avoid narrowing elapsed native seconds. */
    if (elapsed_seconds > seconds || (elapsed_seconds == seconds && elapsed_nanos >= nanos)) return FALSE;
    seconds = (NUINT) (seconds - (NUINT) elapsed_seconds);
    nanos -= elapsed_nanos;
    if (nanos < 0) {
        --seconds;
        nanos += 1000000000L;
    }
    if (seconds > (NUINT) 60) {
        relative->tv_sec = 60;
        relative->tv_nsec = 0;
    } else {
        relative->tv_sec = (time_t) seconds;
        relative->tv_nsec = nanos;
    }
    return TRUE;
}

CATALYST_RESULT cattr_platform_monitor_wait(POINTERW state, NUINT timeout, CATALYST_BOOL (*ready)(POINTERW), POINTERW arguments) {
    CATTRLIB_POSIX_MONITOR* monitor = (CATTRLIB_POSIX_MONITOR*) state;
    if (ready(arguments)) return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
    if (timeout == 0) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x08, 0x02, 0);
    timespec start;
    if (timeout != CATALYST_NUINT_MAX && clock_gettime(CLOCK_MONOTONIC, &start) != 0)
        return RESULT(cattr_posix_status(errno), 0x08, 0x03, 0);
    for (;;) {
        int error;
        if (timeout == CATALYST_NUINT_MAX) {
            error = pthread_cond_wait(&monitor->condition, &monitor->mutex);
        } else {
            timespec now;
            timespec relative;
            if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
                return RESULT(cattr_posix_status(errno), 0x08, 0x04, 0);
            if (!cattr_posix_remaining(timeout, &start, &now, &relative))
                return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x08, 0x05, 0);
#if TARGET_PLATFORM_LINUX
            timespec deadline;
            deadline.tv_nsec = now.tv_nsec + relative.tv_nsec;
            if (__builtin_add_overflow(now.tv_sec, relative.tv_sec, &deadline.tv_sec))
                return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x08, 0x06, 0);
            if (deadline.tv_nsec >= 1000000000L) {
                deadline.tv_nsec -= 1000000000L;
                if (__builtin_add_overflow(deadline.tv_sec, (time_t) 1, &deadline.tv_sec))
                    return RESULT(STATUS_CODE_ERROR_BUFFER_OVERFLOW, 0x08, 0x07, 0);
            }
            error = pthread_cond_timedwait(&monitor->condition, &monitor->mutex, &deadline);
#else
            error = pthread_cond_timedwait_relative_np(&monitor->condition, &monitor->mutex, &relative);
#endif
        }
        /* The predicate can admit work. Do not invoke it after a genuine
         * native failure; timeout/spurious wakes still recheck readiness. */
        if (error != 0 && error != ETIMEDOUT)
            return RESULT(cattr_posix_status(error), 0x08, 0x08, 0);
        if (ready(arguments)) return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x09, 0);
    }
}

CATALYST_RESULT cattr_platform_monitor_free(POINTERW* state) {
    CATTRLIB_POSIX_MONITOR* monitor = (CATTRLIB_POSIX_MONITOR*) *state;
    if (monitor->condition_initialized) {
        int error = pthread_cond_destroy(&monitor->condition);
        if (error != 0) return RESULT(cattr_posix_status(error), 0x09, 0x01, 0);
        monitor->condition_initialized = FALSE;
    }
#if TARGET_PLATFORM_LINUX
    if (monitor->attributes_initialized) {
        int error = pthread_condattr_destroy(&monitor->attributes);
        if (error != 0) return RESULT(cattr_posix_status(error), 0x09, 0x02, 0);
        monitor->attributes_initialized = FALSE;
    }
#endif
    if (monitor->mutex_initialized) {
        int error = pthread_mutex_destroy(&monitor->mutex);
        if (error != 0) return RESULT(cattr_posix_status(error), 0x09, 0x03, 0);
        monitor->mutex_initialized = FALSE;
    }
    RESULT release = cattr_posix_release(state);
    return RESULT(release.status, 0x09, 0x04, release.detail);
}

void cattr_platform_yield() {
    sched_yield();
}
#endif /* TARGET_PLATFORM_LINUX || TARGET_PLATFORM_DARWIN */
