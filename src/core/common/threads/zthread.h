#ifndef ZTHREAD__H
#define ZTHREAD__H

#include "defines.h"

typedef struct zthread {
    void* internal_data;
} zthread;

#ifdef PLATFORM_WINDOWS
typedef unsigned long zthread_func_return_type;
#endif
#ifdef PLATFORM_LINUX
typedef void* zthread_func_return_type;
#endif

void zthread_create(zthread_func_return_type (*start_func)(void*), void* params, zthread* thread);

void zthread_destroy(zthread* thread);

void zthread_wait(zthread* thread);

void zthread_wait_on_all(zthread* threads, u32 count);

#endif