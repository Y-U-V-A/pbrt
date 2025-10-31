#include "platform.h"

#ifdef PLATFORM_LINUX

#    include "zmutex.h"
#    include "zthread.h"
#    include <stdlib.h>
#    include <sys/sysinfo.h> // For get_nprocs_conf
#    include <pthread.h>
#    include <time.h>
#    include "logger.h"

// Make sure to link against the (-lrt) (real-time) library when compiling your program,
// as the clock_gettime function is part of this library:
f64 platform_time() {
    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);
    return (double)curr_time.tv_sec + (double)curr_time.tv_nsec / 1e9;
}

u32 platform_processor_count() {
    // Load processor info.
    i32 processor_count = get_nprocs_conf();
    i32 processors_available = get_nprocs();
    LOGI("%i processor cores detected, %i cores available.", processor_count, processors_available);
    return processors_available;
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
    ASSERT(thread && start_func && pthread_create((pthread_t*)thread, 0, start_func, params) == 0);
}

void zthread_destroy(zthread* thread) {
    ASSERT(thread);
}

void zthread_wait(zthread* thread) {
    ASSERT(thread && pthread_join((pthread_t)thread->internal_data, 0) == 0);
}

void zthread_wait_on_all(zthread* threads, u32 count) {
    ASSERT(threads && count);
    for (u32 i = 0; i < count; ++i) {
        ASSERT(pthread_join((pthread_t)threads[i].internal_data, 0) == 0);
    }
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
    mutex->internal_data = malloc(sizeof(pthread_mutex_t));
    ASSERT(pthread_mutex_init((pthread_mutex_t*)mutex->internal_data, 0) == 0);
}

void zmutex_destroy(zmutex* mutex) {
    ASSERT(mutex && pthread_mutex_destroy((pthread_mutex_t*)mutex->internal_data) == 0);
    free(mutex->internal_data);
}

void zmutex_lock(zmutex* mutex) {
    // if mutex is signaled then the mutex is unsignaled and the thread will enter
    // else thread will wait until the mutex is signaled
    ASSERT(mutex && pthread_mutex_lock((pthread_mutex_t*)mutex->internal_data) == 0);
}

void zmutex_unlock(zmutex* mutex) {
    // mutex is signaled;
    ASSERT(mutex && pthread_mutex_unlock((pthread_mutex_t*)mutex->internal_data) == 0);
}

#endif