#ifndef MEMORY__H
#define MEMORY__H

#include "defines.h"

#define memory_allocate(size) _memory_allocate(size, __FILE__, __LINE__)
#define malloc(size) _memory_allocate(size, __FILE__, __LINE__)
#define free(block) memory_free(block)
#define realloc(block, size) memory_reallocate(block, size)

void memory_init(bool auto_free_memory);

void memory_shutdown();

void* _memory_allocate(u32 size, const char* file, i32 line);

void memory_free(const void* addr);

void* memory_reallocate(const void* addr, u64 size);

#endif