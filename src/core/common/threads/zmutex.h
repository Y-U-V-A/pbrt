#ifndef ZMUTEX__H
#define ZMUTEX__H

#include "defines.h"

typedef struct zmutex {
    void* internal_data;
} zmutex;

void zmutex_create(zmutex* mutex);

void zmutex_destroy(zmutex* mutex);

void zmutex_lock(zmutex* mutex);

void zmutex_unlock(zmutex* mutex);

#endif