#ifndef TEST_MANAGER__H
#define TEST_MANAGER__H

#include "defines.h"

#define EXPECTED_TO_BE(expected, obtained)                                                  \
    if (expected != obtained) {                                                             \
        LOGE("expected %lld but got %lld , %s:%d", expected, obtained, __FILE__, __LINE__); \
        return FALSE;                                                                       \
    }

#define EXPECTED_NOT_TO_BE(expected, obtained)                                                  \
    if (expected == obtained) {                                                                 \
        LOGE("not expected %lld but got %lld , %s:%d", expected, obtained, __FILE__, __LINE__); \
        return FALSE;                                                                           \
    }

#define EXPECTED_FLOAT_TO_BE(expected, obtained, tolerance)                              \
    if (absf(expected - obtained) > tolerance) {                                          \
        LOGE("expected %lf but got %lf, %s:%d", expected, obtained, __FILE__, __LINE__); \
        return FALSE;                                                                    \
    }

void test_manager_init(u64 no_of_tests);

void test_manager_add(u32 (*function)(), char* name);

void test_manager_shutdown();

void test_manager_run();

#endif