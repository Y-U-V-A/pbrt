#include "test_manager.h"
#include <stdlib.h>
#include "clock.h"
#include "logger.h"

typedef struct test {
    u32 (*function)();
    char* name;
} test;

static test* tests;
static u64 tests_size;
static u64 idx;

#define PRINT_TIME(msg, seconds)                  \
    do {                                          \
        if (seconds < 60) {                       \
            LOGD(msg ":%lf secs", seconds);       \
        } else if (seconds < 3600) {              \
            LOGD(msg ":%lf mins", seconds / 60);  \
        } else {                                  \
            LOGD(msg ":%lf hrs", seconds / 3600); \
        }                                         \
    } while (0)

void test_manager_init(u64 max_tests) {
    ASSERT(tests == 0);
    tests = (test*)malloc(sizeof(test) * max_tests);
    tests_size = max_tests;
    LOGD("test_manager_init");
}

void test_manager_shutdown() {
    ASSERT(tests);
    free(tests);
    LOGD("test_manager_shutdown");
}

void test_manager_add(u32 (*function)(), char* name) {
    ASSERT(tests_size > idx);
    tests[idx].function = function;
    tests[idx].name = name;
    idx += 1;
}

void test_manager_run() {
    u32 passed = 0;
    u32 failed = 0;
    clock total;
    clock clk;

    clock_set(&total);
    for (u64 i = 0; i < idx; ++i) {
        clock_set(&clk);
        u32 result = tests[i].function();
        clock_update(&clk);
        if (result == TRUE) {
            passed += 1;
            LOGT("passed : name = %s ,time_taken: %lf", tests[i].name, clk.elapsed);
        } else {
            failed += 1;
            LOGE("failed : name = %s", tests[i].name);
        }
    }
    clock_update(&total);
    PRINT_TIME("test_manager_run_time_taken ", total.elapsed);
    LOGD("total_tests = %u", idx);
    LOGD("passed = %u", passed);
    LOGD("failed = %u", failed);
}
