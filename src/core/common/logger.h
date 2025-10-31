#ifndef LOGGER__H
#define LOGGER__H

#include "defines.h"

//    ██       ██████   ██████   ██████  ███████ ██████
//    ██      ██    ██ ██       ██       ██      ██   ██
//    ██      ██    ██ ██   ███ ██   ███ █████   ██████
//    ██      ██    ██ ██    ██ ██    ██ ██      ██   ██
//    ███████  ██████   ██████   ██████  ███████ ██   ██
//
//

#ifdef NDEBUG
#    define LOGW(msg_fmt, ...)
#    define LOGI(msg_fmt, ...)
#    define LOGD(msg_fmt, ...)
#    define LOGT(msg_fmt, ...)
#else
#    define LOGW(msg_fmt, ...) log_stdout("\033[33m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#    define LOGI(msg_fmt, ...) log_stdout("\033[37m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#    define LOGD(msg_fmt, ...) log_stdout("\033[34m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#    define LOGT(msg_fmt, ...) log_stdout("\033[32m" msg_fmt "\033[0m\n", ##__VA_ARGS__)
#endif

#define LOGE(msg_fmt, ...) log_stdout("\033[31m" msg_fmt "\033[0m\n", ##__VA_ARGS__)

void log_stdout(const char* msg_fmt, ...);

void log_stderr(const char* msg_fmt, ...);

u32 log_buffer(char* buffer, u32 size, const char* msg_fmt, ...);

//     █████  ███████ ███████ ███████ ██████  ████████ ███████
//    ██   ██ ██      ██      ██      ██   ██    ██    ██
//    ███████ ███████ ███████ █████   ██████     ██    ███████
//    ██   ██      ██      ██ ██      ██   ██    ██         ██
//    ██   ██ ███████ ███████ ███████ ██   ██    ██    ███████
//
//

#define _GLUE(a, b) a##b
#define GLUE(a, b) _GLUE(a, b)
#define STATIC_ASSERT(exp) enum {                \
    GLUE(assert_, __LINE__) = (1 / (int)(!!(exp))) \
}

#ifdef NDEBUG
#    define ASSERT(exp)
#else
#    define ASSERT(exp)                                               \
        do {                                                          \
            if (exp) {                                                \
            } else {                                                  \
                LOGE("assert:%s -> %s:%i", #exp, __FILE__, __LINE__); \
                DEBUG_BREAK;                                          \
            }                                                         \
        } while (0)
#endif
#endif