#ifndef DEFINES__H
#define DEFINES__H

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef _Bool bool;

#define TRUE 1
#define FALSE 0

#if defined(_WIN64) || defined(_WIN32)
#    define PLATFORM_WINDOWS 1
#elif defined(__linux__) || defined(linux)
#    define PLATFORM_LINUX 1
#else
#    error platform not suported
#endif

#define DEBUG_BREAK __builtin_trap()

#endif