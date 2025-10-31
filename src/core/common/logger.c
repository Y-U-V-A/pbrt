#include "logger.h"
#include <stdio.h>
#include <stdarg.h>

STATIC_ASSERT(sizeof(i8) == 1);
STATIC_ASSERT(sizeof(i16) == 2);
STATIC_ASSERT(sizeof(i32) == 4);
STATIC_ASSERT(sizeof(i64) == 8);

STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);

STATIC_ASSERT(sizeof(f32) == 4);
STATIC_ASSERT(sizeof(f64) == 8);

void log_stdout(const char* msg_fmt, ...) {
    va_list args;
    va_start(args, msg_fmt);
    vfprintf(stdout, msg_fmt, args);
    va_end(args);
}

void log_stderr(const char* msg_fmt, ...) {
    va_list args;
    va_start(args, msg_fmt);
    vfprintf(stderr, msg_fmt, args);
    va_end(args);
}

u32 log_buffer(char* buffer, u32 size, const char* msg_fmt, ...) {
    va_list args;
    va_start(args, msg_fmt);
    i32 written = vsnprintf(buffer, size, msg_fmt, args);
    va_end(args);
    return (u32)written;
}