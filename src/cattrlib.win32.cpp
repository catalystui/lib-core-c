#include "cmakecfg.h"
#if TARGET_PLATFORM_WIN32
#include "cattrlib.internal.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

using namespace catalyst;

struct CATTRLIB_NATIVE_THREAD {
    HANDLE handle;
    DWORD id;
    SRWLOCK gate;
    PROCEDURE procedure;
    POINTERW arguments;
};

struct CATTRLIB_NATIVE_MONITOR {
    SRWLOCK lock;
    CONDITION_VARIABLE changed;
};

NUINT cattr_platform_pid() {
    return (NUINT) GetCurrentProcessId();
}

static unsigned __stdcall cattr_thread_entry(void* value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    AcquireSRWLockExclusive(&state->gate);
    ReleaseSRWLockExclusive(&state->gate);
    ((void (*)(POINTERW)) state->procedure)(state->arguments);
    return 0;
}

CATALYST_RESULT cattr_platform_thread_capture(POINTERW* output) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x08, 0x01, allocation.detail);
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) *output;
    state->handle = 0;
    state->id = GetCurrentThreadId();
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &state->handle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x08, 0x03, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x08, 0x02, 0);
}

CATALYST_RESULT cattr_platform_thread_start(POINTERW* output, PROCEDURE procedure, POINTERW arguments) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_THREAD));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x09, 0x01, allocation.detail);
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) *output;
    state->procedure = procedure;
    state->arguments = arguments;
    InitializeSRWLock(&state->gate);
    AcquireSRWLockExclusive(&state->gate);
    unsigned id = 0;
    state->handle = (HANDLE) _beginthreadex(0, 0, cattr_thread_entry, state, 0, &id);
    state->id = (DWORD) id;
    ReleaseSRWLockExclusive(&state->gate);
    if (state->handle == 0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x09, 0x03, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x09, 0x04, 0);
}

CATALYST_RESULT cattr_platform_thread_wait(POINTERW value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    if (WaitForSingleObject(state->handle, INFINITE) != WAIT_OBJECT_0) return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x0A, 0x01, 0);
    return RESULT(STATUS_CODE_SUCCESS, 0x0A, 0x02, 0);
}

CATALYST_BOOL cattr_platform_thread_equals(POINTER first, POINTER second) {
    const CATTRLIB_NATIVE_THREAD* a = (const CATTRLIB_NATIVE_THREAD*) first;
    const CATTRLIB_NATIVE_THREAD* b = (const CATTRLIB_NATIVE_THREAD*) second;
    if (a->id != b->id) return CATALYST_FALSE;
    return WaitForSingleObject(a->handle, 0) == WAIT_TIMEOUT && WaitForSingleObject(b->handle, 0) == WAIT_TIMEOUT ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_BOOL cattr_platform_thread_is_current(POINTER value) {
    const CATTRLIB_NATIVE_THREAD* state = (const CATTRLIB_NATIVE_THREAD*) value;
    return state->id == GetCurrentThreadId() && WaitForSingleObject(state->handle, 0) == WAIT_TIMEOUT ? CATALYST_TRUE : CATALYST_FALSE;
}

CATALYST_RESULT cattr_platform_thread_free(POINTERW value) {
    CATTRLIB_NATIVE_THREAD* state = (CATTRLIB_NATIVE_THREAD*) value;
    if (state->handle != 0) {
        if (!CloseHandle(state->handle)) return RESULT(STATUS_CODE_ERROR_INVALID_STATE, 0x0B, 0x01, 0);
        state->handle = 0;
    }
    RESULT release = catmm_free(value);
    return RESULT(release.status, 0x0B, 0x02, release.detail);
}

CATALYST_RESULT cattr_platform_monitor_create(POINTERW* output) {
    RESULT allocation = catmm_alloc(output, (NUINT) sizeof(CATTRLIB_NATIVE_MONITOR));
    if (!status_code_is_success(allocation.status)) return RESULT(allocation.status, 0x0C, 0x01, allocation.detail);
    CATTRLIB_NATIVE_MONITOR* state = (CATTRLIB_NATIVE_MONITOR*) *output;
    InitializeSRWLock(&state->lock);
    InitializeConditionVariable(&state->changed);
    return RESULT(STATUS_CODE_SUCCESS, 0x0C, 0x05, 0);
}

void cattr_platform_monitor_lock(POINTERW value) {
    AcquireSRWLockExclusive(&((CATTRLIB_NATIVE_MONITOR*) value)->lock);
}

void cattr_platform_monitor_unlock(POINTERW value) {
    ReleaseSRWLockExclusive(&((CATTRLIB_NATIVE_MONITOR*) value)->lock);
}

void cattr_platform_monitor_notify(POINTERW value) {
    WakeAllConditionVariable(&((CATTRLIB_NATIVE_MONITOR*) value)->changed);
}

CATALYST_RESULT cattr_platform_monitor_wait(POINTERW value, NUINT timeout) {
    CATTRLIB_NATIVE_MONITOR* state = (CATTRLIB_NATIVE_MONITOR*) value;
    DWORD duration = timeout == CATALYST_NUINT_MAX ? INFINITE : (DWORD) timeout;
    if (!SleepConditionVariableSRW(&state->changed, &state->lock, duration, 0)) {
        if (GetLastError() == ERROR_TIMEOUT) return RESULT(STATUS_CODE_ERROR_TIMEOUT, 0x0D, 0x02, 0);
        return RESULT(STATUS_CODE_ERROR_DEPENDENCY_FAILURE, 0x0D, 0x03, 0);
    }
    return RESULT(STATUS_CODE_SUCCESS, 0x0D, 0x04, 0);
}

CATALYST_RESULT cattr_platform_monitor_free(POINTERW value) {
    RESULT release = catmm_free(value);
    return RESULT(release.status, 0x0E, 0x03, release.detail);
}

void cattr_platform_yield() {
    SwitchToThread();
}

NUINT cattr_platform_ticks() {
    return (NUINT) GetTickCount64();
}
#endif /* TARGET_PLATFORM_WIN32 */
