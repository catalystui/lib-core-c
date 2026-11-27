#include "cmakecfg.h"
#if TARGET_PLATFORM_DARWIN
#include "cattrlib.pthread.hpp"

/* Darwin provides a native relative condition wait. It does not need a
 * realtime clock conversion or an additional framework dependency. */
static RESULT cattr_native_condition_init(pthread_cond_t* condition) {
    if (pthread_cond_init(condition, 0) != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x17, 0x01, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x17, 0x03, 0);
}

static RESULT cattr_native_condition_timedwait(pthread_cond_t* condition, pthread_mutex_t* lock, NUINT timeout) {
    struct timespec duration;
    duration.tv_sec = (time_t) (timeout / (NUINT) 1000);
    duration.tv_nsec = (long) (timeout % (NUINT) 1000) * 1000000L;
    int error = pthread_cond_timedwait_relative_np(condition, lock, &duration);
    if (error == ETIMEDOUT) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x18, 0x02, 0);
    if (error != 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x18, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x18, 0x04, 0);
}
#endif /* TARGET_PLATFORM_DARWIN */
