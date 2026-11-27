#include "cmakecfg.h"
#if TARGET_PLATFORM_LINUX
#include "cattrlib.pthread.hpp"

/* Linux condition variables support absolute monotonic deadlines. */
static RESULT cattr_native_condition_init(pthread_cond_t* condition) {
    pthread_condattr_t attributes;
    if (pthread_condattr_init(&attributes) != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x17, 0x01, 0);
    int error = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    if (error == 0) error = pthread_cond_init(condition, &attributes);
    pthread_condattr_destroy(&attributes);
    if (error != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x17, 0x02, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x17, 0x03, 0);
}

static RESULT cattr_native_condition_timedwait(pthread_cond_t* condition, pthread_mutex_t* lock, NUINT timeout) {
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x18, 0x01, 0);
    /* Convert the short Catalyst timeout to the native time representation. */
    deadline.tv_nsec += (long) (timeout % (NUINT) 1000) * 1000000L;
    deadline.tv_sec += (time_t) (timeout / (NUINT) 1000) + deadline.tv_nsec / 1000000000L;
    deadline.tv_nsec %= 1000000000L;
    int error = pthread_cond_timedwait(condition, lock, &deadline);
    if (error == ETIMEDOUT) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x18, 0x02, 0);
    if (error != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x18, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x18, 0x04, 0);
}
#endif /* TARGET_PLATFORM_LINUX */
