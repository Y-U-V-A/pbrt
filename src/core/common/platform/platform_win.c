#include "platform.h"

#ifdef PLATFORM_WINDOWS
#    include <windows.h>
#    include "logger.h"
#    include "zthread.h"
#    include "zmutex.h"

//    ██████  ██       █████  ████████ ███████  ██████  ██████  ███    ███
//    ██   ██ ██      ██   ██    ██    ██      ██    ██ ██   ██ ████  ████
//    ██████  ██      ███████    ██    █████   ██    ██ ██████  ██ ████ ██
//    ██      ██      ██   ██    ██    ██      ██    ██ ██   ██ ██  ██  ██
//    ██      ███████ ██   ██    ██    ██       ██████  ██   ██ ██      ██
//
//

static LARGE_INTEGER ticks_per_sec;
f64 platform_time() {
    if (ticks_per_sec.QuadPart == 0) {
        QueryPerformanceFrequency(&ticks_per_sec);
    }
    LARGE_INTEGER curr_tick;
    QueryPerformanceCounter(&curr_tick);
    return curr_tick.QuadPart / (f64)ticks_per_sec.QuadPart;
}

u32 platform_processor_count() {
    SYSTEM_INFO sys;
    GetSystemInfo(&sys);
    return sys.dwNumberOfProcessors;
}
/***
 *    ███████ ████████ ██   ██ ██████  ███████  █████  ██████
 *       ███     ██    ██   ██ ██   ██ ██      ██   ██ ██   ██
 *      ███      ██    ███████ ██████  █████   ███████ ██   ██
 *     ███       ██    ██   ██ ██   ██ ██      ██   ██ ██   ██
 *    ███████    ██    ██   ██ ██   ██ ███████ ██   ██ ██████
 *
 *
 */

void zthread_create(zthread_func_return_type (*start_func)(void*), void* params, zthread* thread) {
    ASSERT(start_func && thread);
    thread->internal_data = CreateThread(0, 0, start_func, params, 0, 0);
    ASSERT(thread->internal_data);
}

void zthread_destroy(zthread* thread) {
    ASSERT(thread && CloseHandle(thread->internal_data));
}

void zthread_wait(zthread* thread) {
    ASSERT(thread);
    u32 result = WaitForSingleObject(thread->internal_data, INFINITE);
    ASSERT(WAIT_ABANDONED != result && WAIT_TIMEOUT != result && WAIT_FAILED != result);
}

void zthread_wait_on_all(zthread* threads, u32 count) {
    ASSERT(threads && count);
    u32 result = WaitForMultipleObjects(count, (HANDLE*)threads, TRUE, INFINITE);
    ASSERT(WAIT_TIMEOUT != result && WAIT_FAILED != result);
}

/***
 *    ███████ ███    ███ ██    ██ ████████ ███████ ██   ██
 *       ███  ████  ████ ██    ██    ██    ██       ██ ██
 *      ███   ██ ████ ██ ██    ██    ██    █████     ███
 *     ███    ██  ██  ██ ██    ██    ██    ██       ██ ██
 *    ███████ ██      ██  ██████     ██    ███████ ██   ██
 *
 *
 */

void zmutex_create(zmutex* mutex) {
    ASSERT(mutex);
    mutex->internal_data = CreateMutex(0, FALSE, 0);
    ASSERT(mutex->internal_data);
}

void zmutex_destroy(zmutex* mutex) {
    ASSERT(mutex && CloseHandle(mutex->internal_data));
}

void zmutex_lock(zmutex* mutex) {
    ASSERT(mutex);
    u32 result = WaitForSingleObject(mutex->internal_data, INFINITE);
    ASSERT(WAIT_ABANDONED != result && WAIT_TIMEOUT != result && WAIT_FAILED != result);
}

void zmutex_unlock(zmutex* mutex) {
    ASSERT(mutex && ReleaseMutex(mutex->internal_data));
}

#endif