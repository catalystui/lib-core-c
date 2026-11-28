#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "cattrlib.internal.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

using namespace catalyst;

/* Use explicit CATALYST_BOOL where Win32 also defines BOOL. */
static STATUS_CODE cattr_win32_status(DWORD error) {
    switch (error) {
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
        case ERROR_NO_SYSTEM_RESOURCES: return STATUS_CODE_ERROR_ALLOCATION_FAILED;
        case ERROR_ACCESS_DENIED: return STATUS_CODE_ERROR_ACCESS_DENIED;
        case ERROR_TIMEOUT: return STATUS_CODE_ERROR_TIMEOUT;
        case ERROR_OPERATION_ABORTED: return STATUS_CODE_ERROR_INTERRUPTED;
        case ERROR_NOT_SUPPORTED: return STATUS_CODE_ERROR_SYSTEM_NOT_SUPPORTED;
        default: return STATUS_CODE_ERROR_INVALID_STATE;
    }
}

static RESULT cattr_win32_release(POINTERW* state) {
    RESULT result = catmm_heap_free(*state);
    if (status_code_is_success(result.status)) *state = 0;
    return result;
}

/* -------------------------------------------------------------------------------------------------
 * PROCESS IDENTITY
 * ------------------------------------------------------------------------------------------------- */

CATALYST_RESULT cattr_platform_process_current(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(DWORD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x01, 0x01, allocation.detail);
    *(DWORD*) *state = GetCurrentProcessId();
    return RESULT(STATUS_CODE_SUCCESS, 0x01, 0x02, 0);
}

CATALYST_BOOL cattr_platform_process_equals(POINTER first, POINTER second) {
    return *(const DWORD*) first == *(const DWORD*) second ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_BOOL cattr_platform_process_is_current(POINTER state) {
    return *(const DWORD*) state == GetCurrentProcessId() ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_RESULT cattr_platform_process_free(POINTERW* state) {
    RESULT release = cattr_win32_release(state);
    return RESULT(release.status, 0x02, 0x01, release.detail);
}

/* -------------------------------------------------------------------------------------------------
 * THREAD EXECUTION
 * ------------------------------------------------------------------------------------------------- */

struct CATTRLIB_WIN32_THREAD {
    CATTRLIB_THREAD_STATE common;
    HANDLE handle;
    DWORD identity;
    SRWLOCK gate;
    void (*entry)(POINTERW);
    POINTERW arguments;
};

/* Publish the handle and identity before the entry can run. A captured
 * current thread borrows its identity; only created threads own handles. */
static DWORD WINAPI cattr_win32_entry(LPVOID arguments) {
    CATTRLIB_WIN32_THREAD* state = (CATTRLIB_WIN32_THREAD*) arguments;
    AcquireSRWLockExclusive(&state->gate);
    ReleaseSRWLockExclusive(&state->gate);
    state->entry(state->arguments);
    return 0;
}

CATALYST_RESULT cattr_platform_thread_create(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(CATTRLIB_WIN32_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0A, 0x01, allocation.detail);
    CATTRLIB_WIN32_THREAD* thread = (CATTRLIB_WIN32_THREAD*) *state;
    thread->handle = 0;
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_current(POINTERW* state) {
    RESULT allocation = cattr_platform_thread_create(state);
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x03, 0x01, allocation.detail);
    CATTRLIB_WIN32_THREAD* thread = (CATTRLIB_WIN32_THREAD*) *state;
    thread->identity = GetCurrentThreadId();
    return RESULT(STATUS_CODE_SUCCESS, 0x03, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_start(POINTERW state, void (*entry)(POINTERW), POINTERW arguments) {
    CATTRLIB_WIN32_THREAD* thread = (CATTRLIB_WIN32_THREAD*) state;
    thread->entry = entry;
    thread->arguments = arguments;
    InitializeSRWLock(&thread->gate);
    AcquireSRWLockExclusive(&thread->gate);
    thread->handle = CreateThread(0, 0, cattr_win32_entry, thread, 0, &thread->identity);
    DWORD error = thread->handle == 0 ? GetLastError() : ERROR_SUCCESS;
    ReleaseSRWLockExclusive(&thread->gate);
    if (thread->handle == 0) return RESULT(cattr_win32_status(error), 0x04, 0x02, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x04, 0x03, 0);
}

CATALYST_RESULT cattr_platform_thread_wait(POINTERW state) {
    DWORD wait = WaitForSingleObject(((CATTRLIB_WIN32_THREAD*) state)->handle, INFINITE);
    if (wait == WAIT_FAILED) return RESULT(cattr_win32_status(GetLastError()), 0x05, 0x01, 0);
    if (wait != WAIT_OBJECT_0) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x05, 0x02, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x05, 0x03, 0);
}

/* A Windows ID can be reused once a thread exits, even before its handle
 * is closed. Do not identify an exited created thread as a live capture.
 * Captured identities are borrowed and their original thread must be live. */
static CATALYST_BOOL cattr_win32_thread_live(const CATTRLIB_WIN32_THREAD* thread) {
    return thread->handle == 0 || WaitForSingleObject(thread->handle, 0) == WAIT_TIMEOUT
        ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_BOOL cattr_platform_thread_equals(POINTER first, POINTER second) {
    const CATTRLIB_WIN32_THREAD* a = (const CATTRLIB_WIN32_THREAD*) first;
    const CATTRLIB_WIN32_THREAD* b = (const CATTRLIB_WIN32_THREAD*) second;
    return a->identity == b->identity && cattr_win32_thread_live(a) && cattr_win32_thread_live(b)
        ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_BOOL cattr_platform_thread_is_current(POINTER state) {
    const CATTRLIB_WIN32_THREAD* thread = (const CATTRLIB_WIN32_THREAD*) state;
    return thread->identity == GetCurrentThreadId() && cattr_win32_thread_live(thread)
        ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_RESULT cattr_platform_thread_free(POINTERW* state) {
    CATTRLIB_WIN32_THREAD* thread = (CATTRLIB_WIN32_THREAD*) *state;
    if (thread->handle != 0) {
        if (!CloseHandle(thread->handle)) return RESULT(cattr_win32_status(GetLastError()), 0x06, 0x01, 0);
        thread->handle = 0;
    }
    RESULT release = cattr_win32_release(state);
    return RESULT(release.status, 0x06, 0x02, release.detail);
}

/* -------------------------------------------------------------------------------------------------
 * MONITOR
 * ------------------------------------------------------------------------------------------------- */

struct CATTRLIB_WIN32_MONITOR {
    SRWLOCK lock;
    CONDITION_VARIABLE condition;
};

CATALYST_RESULT cattr_platform_monitor_create(POINTERW* state) {
    RESULT allocation = catmm_heap_alloc(state, (NUINT) sizeof(CATTRLIB_WIN32_MONITOR));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x07, 0x01, allocation.detail);
    CATTRLIB_WIN32_MONITOR* monitor = (CATTRLIB_WIN32_MONITOR*) *state;
    InitializeSRWLock(&monitor->lock);
    InitializeConditionVariable(&monitor->condition);
    return RESULT(STATUS_CODE_SUCCESS, 0x07, 0x02, 0);
}

void cattr_platform_monitor_lock(POINTERW state) {
    AcquireSRWLockExclusive(&((CATTRLIB_WIN32_MONITOR*) state)->lock);
}

void cattr_platform_monitor_unlock(POINTERW state) {
    ReleaseSRWLockExclusive(&((CATTRLIB_WIN32_MONITOR*) state)->lock);
}

void cattr_platform_monitor_notify(POINTERW state) {
    WakeAllConditionVariable(&((CATTRLIB_WIN32_MONITOR*) state)->condition);
}

CATALYST_RESULT cattr_platform_monitor_wait(POINTERW state, NUINT timeout, CATALYST_BOOL (*ready)(POINTERW), POINTERW arguments) {
    CATTRLIB_WIN32_MONITOR* monitor = (CATTRLIB_WIN32_MONITOR*) state;
    if (ready(arguments)) return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x01, 0);
    if (timeout == 0) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x08, 0x02, 0);
    ULONGLONG previous = GetTickCount64();
    NUINT remaining = timeout;
    for (;;) {
        /* INFINITE is reserved by Win32. Split wider Catalyst timeouts into
         * finite slices and charge actual monotonic elapsed time to the
         * original budget across wakeups. */
        DWORD interval = INFINITE;
        if (timeout != CATALYST_NUINT_MAX) {
            ULONGLONG now = GetTickCount64();
            ULONGLONG elapsed = now - previous;
            previous = now;
            if (elapsed >= remaining) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x08, 0x03, 0);
            remaining = (NUINT) (remaining - (NUINT) elapsed);
            interval = remaining >= (NUINT) INFINITE ? INFINITE - 1 : (DWORD) remaining;
        }
        if (!SleepConditionVariableSRW(&monitor->condition, &monitor->lock, interval, 0)) {
            DWORD error = GetLastError();
            if (error != ERROR_TIMEOUT) return RESULT(cattr_win32_status(error), 0x08, 0x04, 0);
        }
        if (ready(arguments)) return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x05, 0);
    }
}

CATALYST_RESULT cattr_platform_monitor_free(POINTERW* state) {
    RESULT release = cattr_win32_release(state);
    return RESULT(release.status, 0x09, 0x01, release.detail);
}

void cattr_platform_yield() {
    if (!SwitchToThread()) Sleep(1);
}
#endif /* TARGET_PLATFORM_WIN32 */
