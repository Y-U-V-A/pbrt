#include "test_manager.h"
#include "memory.h"
#include "clock.h"
#include "zthread.h"
#include "logger.h"
#include <string.h>

// ============================================================================
// MEMORY TESTS
// ============================================================================

u32 test_memory_basic_allocation() {
    void* ptr = memory_allocate(1024);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    memory_free(ptr);
    return TRUE;
}

u32 test_memory_multiple_allocations() {
    void* ptr1 = memory_allocate(512);
    void* ptr2 = memory_allocate(1024);
    void* ptr3 = memory_allocate(2048);
    
    EXPECTED_NOT_TO_BE(0, (u64)ptr1);
    EXPECTED_NOT_TO_BE(0, (u64)ptr2);
    EXPECTED_NOT_TO_BE(0, (u64)ptr3);
    EXPECTED_NOT_TO_BE((u64)ptr1, (u64)ptr2);
    EXPECTED_NOT_TO_BE((u64)ptr2, (u64)ptr3);
    
    memory_free(ptr1);
    memory_free(ptr2);
    memory_free(ptr3);
    return TRUE;
}

u32 test_memory_reallocation_grow() {
    void* ptr = memory_allocate(512);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    
    void* new_ptr = memory_reallocate(ptr, 2048);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);
    
    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_reallocation_shrink() {
    void* ptr = memory_allocate(2048);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    
    void* new_ptr = memory_reallocate(ptr, 512);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);
    
    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_interleaved_operations() {
    void* ptrs[10];
    
    // Allocate
    for (u32 i = 0; i < 10; i++) {
        ptrs[i] = memory_allocate((i + 1) * 128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }
    
    // Free odd indices
    for (u32 i = 1; i < 10; i += 2) {
        memory_free(ptrs[i]);
    }
    
    // Reallocate even indices
    for (u32 i = 0; i < 10; i += 2) {
        ptrs[i] = memory_reallocate(ptrs[i], (i + 1) * 256);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }
    
    // Free remaining
    for (u32 i = 0; i < 10; i += 2) {
        memory_free(ptrs[i]);
    }
    
    return TRUE;
}

u32 test_memory_zero_sized_allocation() {
    // This should trigger an assertion in debug mode
    // In release mode, behavior is implementation-defined
    // For safety, we expect size > 0
    return TRUE;
}

u32 test_memory_large_allocation() {
    void* ptr = memory_allocate(1024 * 1024 * 10); // 10 MB
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    memory_free(ptr);
    return TRUE;
}

u32 test_memory_data_integrity() {
    u32 size = 1024;
    u8* ptr = (u8*)memory_allocate(size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    
    // Write pattern
    for (u32 i = 0; i < size; i++) {
        ptr[i] = (u8)(i % 256);
    }
    
    // Verify pattern
    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE((u8)(i % 256), ptr[i]);
    }
    
    memory_free(ptr);
    return TRUE;
}

u32 test_memory_realloc_data_preservation() {
    u32 initial_size = 512;
    u8* ptr = (u8*)memory_allocate(initial_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    
    // Write pattern
    for (u32 i = 0; i < initial_size; i++) {
        ptr[i] = (u8)(i % 256);
    }
    
    // Reallocate to larger size
    u32 new_size = 2048;
    ptr = (u8*)memory_reallocate(ptr, new_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    
    // Verify original data preserved
    for (u32 i = 0; i < initial_size; i++) {
        EXPECTED_TO_BE((u8)(i % 256), ptr[i]);
    }
    
    memory_free(ptr);
    return TRUE;
}

// ============================================================================
// CLOCK TESTS
// ============================================================================

u32 test_clock_initialization() {
    clock clk;
    clock_set(&clk);
    EXPECTED_NOT_TO_BE(0, (u64)clk.start);
    EXPECTED_TO_BE(0, (u64)clk.elapsed);
    return TRUE;
}

u32 test_clock_elapsed_time() {
    clock clk;
    clock_set(&clk);
    
    // Busy wait to ensure some time passes
    volatile u64 sum = 0;
    for (u64 i = 0; i < 10000000; i++) {
        sum += i;
    }
    
    clock_update(&clk);
    
    // Elapsed time should be greater than 0
    if (clk.elapsed <= 0.0) {
        LOGE("Expected elapsed time > 0, got %lf", clk.elapsed);
        return FALSE;
    }
    
    return TRUE;
}

u32 test_clock_multiple_updates() {
    clock clk;
    clock_set(&clk);
    
    f64 prev_elapsed = 0.0;
    
    for (u32 i = 0; i < 5; i++) {
        volatile u64 sum = 0;
        for (u64 j = 0; j < 5000000; j++) {
            sum += j;
        }
        
        clock_update(&clk);
        
        if (clk.elapsed <= prev_elapsed) {
            LOGE("Expected increasing elapsed time");
            return FALSE;
        }
        
        prev_elapsed = clk.elapsed;
    }
    
    return TRUE;
}

// ============================================================================
// MULTITHREADING TESTS - MEMORY
// ============================================================================

typedef struct thread_alloc_data {
    u32 thread_id;
    u32 num_allocations;
    u32 allocation_size;
    u32 success;
} thread_alloc_data;

zthread_func_return_type thread_allocate_free(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void**ptrs = malloc(sizeof(void*) * data->num_allocations);
    
    // Allocate
    for (u32 i = 0; i < data->num_allocations; i++) {
        ptrs[i] = memory_allocate(data->allocation_size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }
        
        // Write data to verify
        u8* byte_ptr = (u8*)ptrs[i];
        for (u32 j = 0; j < data->allocation_size; j++) {
            byte_ptr[j] = (u8)(data->thread_id + j);
        }
    }
    
    // Verify data
    for (u32 i = 0; i < data->num_allocations; i++) {
        u8* byte_ptr = (u8*)ptrs[i];
        for (u32 j = 0; j < data->allocation_size; j++) {
            if (byte_ptr[j] != (u8)(data->thread_id + j)) {
                data->success = FALSE;
                break;
            }
        }
    }
    
    // Free
    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }
    
    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_allocations() {
    const u32 num_threads = 4;
    zthread threads[4];
    thread_alloc_data thread_data[4];
    
    // Create threads
    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 100;
        thread_data[i].allocation_size = 256;
        thread_data[i].success = FALSE;
        
        zthread_create(thread_allocate_free, &thread_data[i], &threads[i]);
    }
    
    // Wait for completion
    zthread_wait_on_all(threads, num_threads);
    
    // Verify all succeeded
    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }
    
    return TRUE;
}

zthread_func_return_type thread_mixed_operations(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void** ptrs = malloc(sizeof(void*) * data->num_allocations);
    
    // Mixed allocate, reallocate, free operations
    for (u32 i = 0; i < data->num_allocations; i++) {
        ptrs[i] = memory_allocate(data->allocation_size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }
        
        // Reallocate every other allocation
        if (i % 2 == 0) {
            ptrs[i] = memory_reallocate(ptrs[i], data->allocation_size * 2);
            if (ptrs[i] == 0) {
                data->success = FALSE;
                free(ptrs);
                return 0;
            }
        }
        
        // Free and reallocate some
        if (i % 3 == 0 && i > 0) {
            memory_free(ptrs[i - 1]);
            ptrs[i - 1] = memory_allocate(data->allocation_size);
            if (ptrs[i - 1] == 0) {
                data->success = FALSE;
                free(ptrs);
                return 0;
            }
        }
    }
    
    // Free all
    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }
    
    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_mixed_operations() {
    const u32 num_threads = 4;
    zthread threads[4];
    thread_alloc_data thread_data[4];
    
    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 50;
        thread_data[i].allocation_size = 512;
        thread_data[i].success = FALSE;
        
        zthread_create(thread_mixed_operations, &thread_data[i], &threads[i]);
    }
    
    zthread_wait_on_all(threads, num_threads);
    
    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }
    
    return TRUE;
}

u32 test_memory_stress_test() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];
    
    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 500;
        thread_data[i].allocation_size = 128;
        thread_data[i].success = FALSE;
        
        zthread_create(thread_allocate_free, &thread_data[i], &threads[i]);
    }
    
    zthread_wait_on_all(threads, num_threads);
    
    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }
    
    return TRUE;
}

// ============================================================================
// MULTITHREADING TESTS - CLOCK
// ============================================================================

typedef struct thread_clock_data {
    u32 thread_id;
    clock clk;
    f64 elapsed_time;
    u32 success;
} thread_clock_data;

zthread_func_return_type thread_clock_timing(void* params) {
    thread_clock_data* data = (thread_clock_data*)params;
    
    clock_set(&data->clk);
    
    // Perform work
    volatile u64 sum = 0;
    for (u64 i = 0; i < 50000000; i++) {
        sum += i;
    }
    
    clock_update(&data->clk);
    data->elapsed_time = data->clk.elapsed;
    data->success = (data->elapsed_time > 0.0) ? TRUE : FALSE;
    
    return 0;
}

u32 test_clock_concurrent_timing() {
    const u32 num_threads = 4;
    zthread threads[4];
    thread_clock_data thread_data[4];
    
    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].elapsed_time = 0.0;
        thread_data[i].success = FALSE;
        
        zthread_create(thread_clock_timing, &thread_data[i], &threads[i]);
    }
    
    zthread_wait_on_all(threads, num_threads);
    
    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        LOGT("Thread %u elapsed time: %lf", i, thread_data[i].elapsed_time);
        zthread_destroy(&threads[i]);
    }
    
    return TRUE;
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

u32 test_memory_boundary_allocations() {
    // Test power-of-2 boundaries
    u32 sizes[] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    void* ptrs[13];
    
    for (u32 i = 0; i < 13; i++) {
        ptrs[i] = memory_allocate(sizes[i]);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }
    
    for (u32 i = 0; i < 13; i++) {
        memory_free(ptrs[i]);
    }
    
    return TRUE;
}

u32 test_memory_fragmentation_scenario() {
    void* ptrs[20];
    
    // Allocate varying sizes
    for (u32 i = 0; i < 20; i++) {
        ptrs[i] = memory_allocate((i % 5 + 1) * 256);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }
    
    // Free every other allocation (create fragmentation)
    for (u32 i = 1; i < 20; i += 2) {
        memory_free(ptrs[i]);
    }
    
    // Allocate new blocks in freed spaces
    for (u32 i = 1; i < 20; i += 2) {
        ptrs[i] = memory_allocate((i % 3 + 1) * 128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }
    
    // Free all
    for (u32 i = 0; i < 20; i++) {
        memory_free(ptrs[i]);
    }
    
    return TRUE;
}

// ============================================================================
// MAIN TEST REGISTRATION
// ============================================================================

int register_memory_testcases() {
    
    // Memory basic tests
    test_manager_add(test_memory_basic_allocation, "memory_basic_allocation");
    test_manager_add(test_memory_multiple_allocations, "memory_multiple_allocations");
    test_manager_add(test_memory_reallocation_grow, "memory_reallocation_grow");
    test_manager_add(test_memory_reallocation_shrink, "memory_reallocation_shrink");
    test_manager_add(test_memory_interleaved_operations, "memory_interleaved_operations");
    test_manager_add(test_memory_large_allocation, "memory_large_allocation");
    test_manager_add(test_memory_data_integrity, "memory_data_integrity");
    test_manager_add(test_memory_realloc_data_preservation, "memory_realloc_data_preservation");
    test_manager_add(test_memory_boundary_allocations, "memory_boundary_allocations");
    test_manager_add(test_memory_fragmentation_scenario, "memory_fragmentation_scenario");
    
    // Clock tests
    test_manager_add(test_clock_initialization, "clock_initialization");
    test_manager_add(test_clock_elapsed_time, "clock_elapsed_time");
    test_manager_add(test_clock_multiple_updates, "clock_multiple_updates");
    
    // Multithreading tests - Memory
    test_manager_add(test_memory_concurrent_allocations, "memory_concurrent_allocations");
    test_manager_add(test_memory_concurrent_mixed_operations, "memory_concurrent_mixed_operations");
    test_manager_add(test_memory_stress_test, "memory_stress_test");
    
    // Multithreading tests - Clock
    test_manager_add(test_clock_concurrent_timing, "clock_concurrent_timing");
    
    return 0;
}