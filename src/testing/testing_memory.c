#include "test_manager.h"
#include "memory.h"
#include "zthread.h"
#include "logger.h"

// ============================================================================
// BASIC ALLOCATION TESTS
// ============================================================================

u32 test_memory_single_allocation() {
    void* ptr = memory_allocate(128);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    memory_free(ptr);
    return TRUE;
}

u32 test_memory_multiple_different_sizes() {
    void* ptr1 = memory_allocate(1);
    void* ptr2 = memory_allocate(16);
    void* ptr3 = memory_allocate(256);
    void* ptr4 = memory_allocate(4096);
    void* ptr5 = memory_allocate(65536);

    EXPECTED_NOT_TO_BE(0, (u64)ptr1);
    EXPECTED_NOT_TO_BE(0, (u64)ptr2);
    EXPECTED_NOT_TO_BE(0, (u64)ptr3);
    EXPECTED_NOT_TO_BE(0, (u64)ptr4);
    EXPECTED_NOT_TO_BE(0, (u64)ptr5);

    memory_free(ptr1);
    memory_free(ptr2);
    memory_free(ptr3);
    memory_free(ptr4);
    memory_free(ptr5);
    return TRUE;
}

u32 test_memory_allocation_uniqueness() {
    void* ptrs[100];

    for (u32 i = 0; i < 100; i++) {
        ptrs[i] = memory_allocate(64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);

        // Verify all previous allocations are different
        for (u32 j = 0; j < i; j++) {
            EXPECTED_NOT_TO_BE((u64)ptrs[i], (u64)ptrs[j]);
        }
    }

    for (u32 i = 0; i < 100; i++) {
        memory_free(ptrs[i]);
    }
    return TRUE;
}

u32 test_memory_power_of_two_sizes() {
    for (u32 power = 0; power < 20; power++) {
        u32 size = 1 << power;
        void* ptr = memory_allocate(size);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
        memory_free(ptr);
    }
    return TRUE;
}

u32 test_memory_odd_sizes() {
    u32 sizes[] = {1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095};

    for (u32 i = 0; i < 12; i++) {
        void* ptr = memory_allocate(sizes[i]);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
        memory_free(ptr);
    }
    return TRUE;
}

u32 test_memory_large_allocation() {
    void* ptr = memory_allocate(1024 * 1024 * 16); // 16 MB
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    memory_free(ptr);
    return TRUE;
}

u32 test_memory_very_large_allocation() {
    void* ptr = memory_allocate(1024 * 1024 * 128); // 128 MB
    EXPECTED_NOT_TO_BE(0, (u64)ptr);
    memory_free(ptr);
    return TRUE;
}

// ============================================================================
// DATA INTEGRITY TESTS
// ============================================================================

u32 test_memory_write_read_bytes() {
    u32 size = 1024;
    u8* ptr = (u8*)memory_allocate(size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < size; i++) {
        ptr[i] = (u8)(i & 0xFF);
    }

    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE((u8)(i & 0xFF), ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_write_read_words() {
    u32 size = 1024;
    u32* ptr = (u32*)memory_allocate(size * sizeof(u32));
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < size; i++) {
        ptr[i] = i * 0xDEADBEEF;
    }

    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE(i * 0xDEADBEEF, ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_pattern_verification() {
    u32 size = 4096;
    u8* ptr = (u8*)memory_allocate(size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    // Write pattern: 0xAA, 0x55, 0xAA, 0x55...
    for (u32 i = 0; i < size; i++) {
        ptr[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }

    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE(((i % 2 == 0) ? 0xAA : 0x55), ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_sequential_pattern() {
    u32 size = 8192;
    u8* ptr = (u8*)memory_allocate(size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < size; i++) {
        ptr[i] = (u8)((i * 7 + 13) & 0xFF);
    }

    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE((u8)((i * 7 + 13) & 0xFF), ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_isolation() {
    u32 size = 512;
    u8* ptr1 = (u8*)memory_allocate(size);
    u8* ptr2 = (u8*)memory_allocate(size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr1);
    EXPECTED_NOT_TO_BE(0, (u64)ptr2);

    // Write different patterns
    for (u32 i = 0; i < size; i++) {
        ptr1[i] = 0xAA;
        ptr2[i] = 0x55;
    }

    // Verify patterns didn't interfere
    for (u32 i = 0; i < size; i++) {
        EXPECTED_TO_BE(0xAA, ptr1[i]);
        EXPECTED_TO_BE(0x55, ptr2[i]);
    }

    memory_free(ptr1);
    memory_free(ptr2);
    return TRUE;
}

// ============================================================================
// REALLOCATION TESTS
// ============================================================================

u32 test_memory_realloc_grow_small() {
    void* ptr = memory_allocate(64);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    void* new_ptr = memory_reallocate(ptr, 128);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);

    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_realloc_grow_large() {
    void* ptr = memory_allocate(1024);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    void* new_ptr = memory_reallocate(ptr, 1024 * 1024);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);

    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_realloc_shrink() {
    void* ptr = memory_allocate(4096);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    void* new_ptr = memory_reallocate(ptr, 512);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);

    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_realloc_same_size() {
    void* ptr = memory_allocate(1024);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    void* new_ptr = memory_reallocate(ptr, 1024);
    EXPECTED_NOT_TO_BE(0, (u64)new_ptr);

    memory_free(new_ptr);
    return TRUE;
}

u32 test_memory_realloc_data_preservation_grow() {
    u32 initial_size = 256;
    u8* ptr = (u8*)memory_allocate(initial_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < initial_size; i++) {
        ptr[i] = (u8)(i & 0xFF);
    }

    u32 new_size = 2048;
    ptr = (u8*)memory_reallocate(ptr, new_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < initial_size; i++) {
        EXPECTED_TO_BE((u8)(i & 0xFF), ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_realloc_data_preservation_shrink() {
    u32 initial_size = 2048;
    u8* ptr = (u8*)memory_allocate(initial_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < initial_size; i++) {
        ptr[i] = (u8)(i & 0xFF);
    }

    u32 new_size = 256;
    ptr = (u8*)memory_reallocate(ptr, new_size);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < new_size; i++) {
        EXPECTED_TO_BE((u8)(i & 0xFF), ptr[i]);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_realloc_multiple_times() {
    u32 sizes[] = {64, 128, 512, 2048, 8192, 4096, 1024, 256};
    void* ptr = memory_allocate(sizes[0]);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 1; i < 8; i++) {
        ptr = memory_reallocate(ptr, sizes[i]);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_realloc_with_data_multiple_times() {
    u32 sizes[] = {64, 128, 256, 512, 1024, 512, 256, 128};
    u8* ptr = (u8*)memory_allocate(sizes[0]);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 0; i < sizes[0]; i++) {
        ptr[i] = (u8)(i & 0xFF);
    }

    for (u32 i = 1; i < 8; i++) {
        ptr = (u8*)memory_reallocate(ptr, sizes[i]);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
        u32 small = (sizes[i-1] < sizes[i]) ? sizes[i-1] : sizes[i];
        u32 big = (sizes[i-1] > sizes[i]) ? sizes[i-1] : sizes[i];
        u32 x=big;
        for (u32 j = 0; j < small; j++) {
            EXPECTED_TO_BE(((u8)(j & 0xFF)), ptr[j]);
        }
        if(sizes[i-1]<sizes[i]){
            for (u32 j = small; j < x; j++) {
            ptr[j] = (u8)(j & 0xFF);
            }
        }
    }

    memory_free(ptr);
    return TRUE;
}

// ============================================================================
// ORDERING AND INTERLEAVING TESTS
// ============================================================================

u32 test_memory_fifo_order() {
    void* ptrs[50];

    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 50; i++) {
        memory_free(ptrs[i]);
    }
    return TRUE;
}

u32 test_memory_lifo_order() {
    void* ptrs[50];

    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (i32 i = 49; i >= 0; i--) {
        memory_free(ptrs[i]);
    }
    return TRUE;
}

u32 test_memory_random_order() {
    void* ptrs[50];

    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free in pseudo-random order
    u32 indices[] = {7, 23, 5, 41, 12, 38, 3, 29, 15, 47, 1, 34, 19, 45, 8, 26,
                     11, 39, 4, 32, 18, 44, 2, 28, 14, 40, 6, 33, 20, 46, 0, 27,
                     13, 37, 9, 35, 21, 43, 10, 36, 22, 48, 16, 42, 17, 49, 24,
                     25, 30, 31};

    for (u32 i = 0; i < 50; i++) {
        memory_free(ptrs[indices[i]]);
    }
    return TRUE;
}

u32 test_memory_alternating_alloc_free() {
    for (u32 i = 0; i < 100; i++) {
        void* ptr = memory_allocate(256);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
        memory_free(ptr);
    }
    return TRUE;
}

u32 test_memory_interleaved_operations() {
    void* ptrs[20];

    // Allocate first 10
    for (u32 i = 0; i < 10; i++) {
        ptrs[i] = memory_allocate((i + 1) * 64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free odd indices
    for (u32 i = 1; i < 10; i += 2) {
        memory_free(ptrs[i]);
    }

    // Allocate next 10
    for (u32 i = 10; i < 20; i++) {
        ptrs[i] = memory_allocate((i + 1) * 64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free even indices from first 10
    for (u32 i = 0; i < 10; i += 2) {
        memory_free(ptrs[i]);
    }

    // Free all remaining
    for (u32 i = 10; i < 20; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_complex_interleaving() {
    void* ptrs[30];

    for (u32 i = 0; i < 30; i++) {
        ptrs[i] = memory_allocate((i + 1) * 32);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);

        if (i > 5 && i % 3 == 0 && ptrs[i - 3]) {
            memory_free(ptrs[i - 3]);
            ptrs[i - 3] = 0;
        }

        if (i > 8 && i % 5 == 0 && ptrs[i - 5]) {
            ptrs[i - 5] = memory_reallocate(ptrs[i - 5], (i + 1) * 64);
            EXPECTED_NOT_TO_BE(0, (u64)ptrs[i - 5]);
        }
    }

    for (u32 i = 0; i < 30; i++) {
        if (ptrs[i]) {
            memory_free(ptrs[i]);
        }
    }

    return TRUE;
}

// ============================================================================
// FRAGMENTATION TESTS
// ============================================================================

u32 test_memory_fragmentation_basic() {
    void* ptrs[100];

    // Allocate 100 blocks
    for (u32 i = 0; i < 100; i++) {
        ptrs[i] = memory_allocate(256);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free every other block
    for (u32 i = 1; i < 100; i += 2) {
        memory_free(ptrs[i]);
    }

    // Try to allocate in gaps
    for (u32 i = 1; i < 100; i += 2) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free all
    for (u32 i = 0; i < 100; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_fragmentation_varying_sizes() {
    void* ptrs[50];

    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate((i % 10 + 1) * 128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 50; i += 3) {
        memory_free(ptrs[i]);
    }

    for (u32 i = 0; i < 50; i += 3) {
        ptrs[i] = memory_allocate((i % 5 + 1) * 64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 50; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_fragmentation_worst_case() {
    void* ptrs[200];

    for (u32 i = 0; i < 200; i++) {
        ptrs[i] = memory_allocate(64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free pattern: keep first, free second, repeat
    for (u32 i = 1; i < 200; i += 2) {
        memory_free(ptrs[i]);
    }

    // Allocate larger blocks
    for (u32 i = 1; i < 200; i += 2) {
        ptrs[i] = memory_allocate(32);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 200; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

// ============================================================================
// STRESS TESTS
// ============================================================================

u32 test_memory_stress_many_allocations() {
    void* ptrs[1000];

    for (u32 i = 0; i < 1000; i++) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 1000; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_stress_varying_sizes() {
    void* ptrs[500];

    for (u32 i = 0; i < 500; i++) {
        u32 size = ((i * 97) % 1024) + 1;
        ptrs[i] = memory_allocate(size);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 500; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_stress_repeated_cycles() {
    for (u32 cycle = 0; cycle < 10; cycle++) {
        void* ptrs[100];

        for (u32 i = 0; i < 100; i++) {
            ptrs[i] = memory_allocate((i + 1) * 16);
            EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
        }

        for (u32 i = 0; i < 100; i++) {
            memory_free(ptrs[i]);
        }
    }

    return TRUE;
}

u32 test_memory_stress_with_realloc() {
    void* ptrs[100];

    for (u32 i = 0; i < 100; i++) {
        ptrs[i] = memory_allocate(256);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 100; i++) {
        u32 new_size = ((i * 73) % 2048) + 128;
        ptrs[i] = memory_reallocate(ptrs[i], new_size);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 100; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

// ============================================================================
// RED-BLACK TREE SPECIFIC TESTS
// ============================================================================

u32 test_memory_tree_left_heavy() {
    void* ptrs[50];

    // Allocate in descending address pattern (forces left-heavy tree)
    for (i32 i = 49; i >= 0; i--) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 50; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_tree_right_heavy() {
    void* ptrs[50];

    // Normal allocation pattern (likely right-heavy)
    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 0; i < 50; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_tree_balanced() {
    void* ptrs[31];

    // Allocate in binary tree order (balanced)
    u32 order[] = {15, 7, 23, 3, 11, 19, 27, 1, 5, 9, 13, 17, 21, 25, 29,
                   0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};

    for (u32 i = 0; i < 31; i++) {
        ptrs[order[i]] = memory_allocate(128);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[order[i]]);
    }

    for (u32 i = 0; i < 31; i++) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_tree_rotations() {
    void* ptrs[20];

    for (u32 i = 0; i < 20; i++) {
        ptrs[i] = memory_allocate((i + 1) * 64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    // Free in pattern that causes rotations
    memory_free(ptrs[10]);
    memory_free(ptrs[5]);
    memory_free(ptrs[15]);
    memory_free(ptrs[3]);
    memory_free(ptrs[7]);
    memory_free(ptrs[13]);
    memory_free(ptrs[17]);

    // Free remaining
    for (u32 i = 0; i < 20; i++) {
        if (i != 10 && i != 5 && i != 15 && i != 3 &&
            i != 7 && i != 13 && i != 17) {
            memory_free(ptrs[i]);
        }
    }

    return TRUE;
}

// ============================================================================
// MULTITHREADING TESTS
// ============================================================================

typedef struct thread_alloc_data {
    u32 thread_id;
    u32 num_allocations;
    u32 allocation_size;
    u32 success;
} thread_alloc_data;

zthread_func_return_type thread_allocate_free(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void** ptrs = malloc(sizeof(void*) * data->num_allocations);

    for (u32 i = 0; i < data->num_allocations; i++) {
        ptrs[i] = memory_allocate(data->allocation_size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }

        u8* byte_ptr = (u8*)ptrs[i];
        for (u32 j = 0; j < data->allocation_size; j++) {
            byte_ptr[j] = (u8)((data->thread_id + j) & 0xFF);
        }
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        u8* byte_ptr = (u8*)ptrs[i];
        for (u32 j = 0; j < data->allocation_size; j++) {
            if (byte_ptr[j] != (u8)((data->thread_id + j) & 0xFF)) {
                data->success = FALSE;
                free(ptrs);
                return 0;
            }
        }
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }

    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_allocations_4_threads() {
    const u32 num_threads = 4;
    zthread threads[4];
    thread_alloc_data thread_data[4];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 100;
        thread_data[i].allocation_size = 256;
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

u32 test_memory_concurrent_allocations_8_threads() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 200;
        thread_data[i].allocation_size = 512;
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

u32 test_memory_concurrent_allocations_16_threads() {
    const u32 num_threads = 16;
    zthread threads[16];
    thread_alloc_data thread_data[16];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 50;
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

zthread_func_return_type thread_mixed_operations(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void** ptrs = malloc(sizeof(void*) * data->num_allocations);

    for (u32 i = 0; i < data->num_allocations; i++) {
        ptrs[i] = memory_allocate(data->allocation_size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }

        if (i % 2 == 0 && i > 0) {
            ptrs[i - 1] = memory_reallocate(ptrs[i - 1], data->allocation_size * 2);
            if (ptrs[i - 1] == 0) {
                data->success = FALSE;
                free(ptrs);
                return 0;
            }
        }

        if (i % 5 == 0 && i > 0) {
            memory_free(ptrs[i - 1]);
            ptrs[i - 1] = memory_allocate(data->allocation_size);
            if (ptrs[i - 1] == 0) {
                data->success = FALSE;
                free(ptrs);
                return 0;
            }
        }
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }

    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_mixed_operations() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 100;
        thread_data[i].allocation_size = 256;
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

zthread_func_return_type thread_realloc_stress(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void* ptr = memory_allocate(64);

    if (ptr == 0) {
        data->success = FALSE;
        return 0;
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        u32 new_size = ((i * 37 + data->thread_id) % 4096) + 64;
        ptr = memory_reallocate(ptr, new_size);
        if (ptr == 0) {
            data->success = FALSE;
            return 0;
        }

        u8* byte_ptr = (u8*)ptr;
        byte_ptr[0] = (u8)(data->thread_id & 0xFF);
        byte_ptr[new_size - 1] = (u8)((data->thread_id + 1) & 0xFF);
    }

    memory_free(ptr);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_realloc_stress() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 200;
        thread_data[i].allocation_size = 0;
        thread_data[i].success = FALSE;

        zthread_create(thread_realloc_stress, &thread_data[i], &threads[i]);
    }

    zthread_wait_on_all(threads, num_threads);

    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }

    return TRUE;
}

zthread_func_return_type thread_varying_sizes(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void** ptrs = malloc(sizeof(void*) * data->num_allocations);

    for (u32 i = 0; i < data->num_allocations; i++) {
        u32 size = ((i * 97 + data->thread_id * 13) % 8192) + 1;
        ptrs[i] = memory_allocate(size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }

    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_varying_sizes() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 150;
        thread_data[i].allocation_size = 0;
        thread_data[i].success = FALSE;

        zthread_create(thread_varying_sizes, &thread_data[i], &threads[i]);
    }

    zthread_wait_on_all(threads, num_threads);

    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }

    return TRUE;
}

u32 test_memory_concurrent_stress_heavy() {
    const u32 num_threads = 16;
    zthread threads[16];
    thread_alloc_data thread_data[16];

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

zthread_func_return_type thread_interleaved_ops(void* params) {
    thread_alloc_data* data = (thread_alloc_data*)params;
    void** ptrs = malloc(sizeof(void*) * data->num_allocations);

    for (u32 i = 0; i < data->num_allocations; i++) {
        ptrs[i] = memory_allocate(data->allocation_size);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }
    }

    for (u32 i = 1; i < data->num_allocations; i += 2) {
        memory_free(ptrs[i]);
    }

    for (u32 i = 1; i < data->num_allocations; i += 2) {
        ptrs[i] = memory_allocate(data->allocation_size / 2);
        if (ptrs[i] == 0) {
            data->success = FALSE;
            free(ptrs);
            return 0;
        }
    }

    for (u32 i = 0; i < data->num_allocations; i++) {
        memory_free(ptrs[i]);
    }

    free(ptrs);
    data->success = TRUE;
    return 0;
}

u32 test_memory_concurrent_interleaved_operations() {
    const u32 num_threads = 8;
    zthread threads[8];
    thread_alloc_data thread_data[8];

    for (u32 i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_allocations = 100;
        thread_data[i].allocation_size = 512;
        thread_data[i].success = FALSE;

        zthread_create(thread_interleaved_ops, &thread_data[i], &threads[i]);
    }

    zthread_wait_on_all(threads, num_threads);

    for (u32 i = 0; i < num_threads; i++) {
        EXPECTED_TO_BE(TRUE, thread_data[i].success);
        zthread_destroy(&threads[i]);
    }

    return TRUE;
}

// ============================================================================
// BOUNDARY AND EDGE CASE TESTS
// ============================================================================

u32 test_memory_single_byte_allocation() {
    void* ptr = memory_allocate(1);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    u8* byte_ptr = (u8*)ptr;
    byte_ptr[0] = 0xFF;
    EXPECTED_TO_BE(0xFF, byte_ptr[0]);

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_alignment_check() {
    for (u32 i = 1; i <= 128; i++) {
        void* ptr = memory_allocate(i);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);

        u8* byte_ptr = (u8*)ptr;
        for (u32 j = 0; j < i; j++) {
            byte_ptr[j] = (u8)(j & 0xFF);
        }

        for (u32 j = 0; j < i; j++) {
            EXPECTED_TO_BE((u8)(j & 0xFF), byte_ptr[j]);
        }

        memory_free(ptr);
    }
    return TRUE;
}

u32 test_memory_repeated_same_size() {
    for (u32 cycle = 0; cycle < 20; cycle++) {
        void* ptr = memory_allocate(1024);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
        memory_free(ptr);
    }
    return TRUE;
}

u32 test_memory_progressive_growth() {
    void* ptr = memory_allocate(1);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 1; i <= 4096; i *= 2) {
        ptr = memory_reallocate(ptr, i);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_progressive_shrink() {
    void* ptr = memory_allocate(4096);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    for (u32 i = 4096; i >= 1; i /= 2) {
        ptr = memory_reallocate(ptr, i);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
    }

    memory_free(ptr);
    return TRUE;
}

u32 test_memory_zigzag_realloc() {
    void* ptr = memory_allocate(256);
    EXPECTED_NOT_TO_BE(0, (u64)ptr);

    u32 sizes[] = {512, 128, 1024, 64, 2048, 32, 4096, 16, 8192};
    for (u32 i = 0; i < 9; i++) {
        ptr = memory_reallocate(ptr, sizes[i]);
        EXPECTED_NOT_TO_BE(0, (u64)ptr);
    }

    memory_free(ptr);
    return TRUE;
}

// ============================================================================
// COMPREHENSIVE INTEGRATION TESTS
// ============================================================================

u32 test_memory_lifecycle_complete() {
    void* ptrs[50];

    for (u32 i = 0; i < 50; i++) {
        ptrs[i] = memory_allocate((i + 1) * 32);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);

        u8* data = (u8*)ptrs[i];
        for (u32 j = 0; j < (i + 1) * 32; j++) {
            data[j] = (u8)((i + j) & 0xFF);
        }
    }

    for (u32 i = 0; i < 50; i += 2) {
        u8* data = (u8*)ptrs[i];
        for (u32 j = 0; j < (i + 1) * 32; j++) {
            EXPECTED_TO_BE((u8)((i + j) & 0xFF), data[j]);
        }
        memory_free(ptrs[i]);
    }

    for (u32 i = 1; i < 50; i += 2) {
        ptrs[i] = memory_reallocate(ptrs[i], (i + 1) * 64);
        EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
    }

    for (u32 i = 1; i < 50; i += 2) {
        memory_free(ptrs[i]);
    }

    return TRUE;
}

u32 test_memory_torture_test() {
    for (u32 iteration = 0; iteration < 5; iteration++) {
        void* ptrs[100];

        for (u32 i = 0; i < 100; i++) {
            u32 size = ((i * 73 + iteration * 17) % 2048) + 1;
            ptrs[i] = memory_allocate(size);
            EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);

            u8* data = (u8*)ptrs[i];
            for (u32 j = 0; j < size; j++) {
                data[j] = (u8)((i + j + iteration) & 0xFF);
            }
        }

        for (u32 i = 0; i < 100; i += 3) {
            u32 old_size = ((i * 73 + iteration * 17) % 2048) + 1;
            u32 new_size = ((i * 97 + iteration * 23) % 4096) + 1;
            u32 check_size = (new_size < old_size) ? new_size : old_size;

            ptrs[i] = memory_reallocate(ptrs[i], new_size);
            EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);

            u8* data = (u8*)ptrs[i];
            for (u32 j = 0; j < check_size; j++) {
                EXPECTED_TO_BE((u8)((i + j + iteration) & 0xFF), data[j]);
            }
        }

        for (u32 i = 1; i < 100; i += 5) {
            memory_free(ptrs[i]);
        }

        for (u32 i = 1; i < 100; i += 5) {
            u32 size = ((i * 53) % 1024) + 1;
            ptrs[i] = memory_allocate(size);
            EXPECTED_NOT_TO_BE(0, (u64)ptrs[i]);
        }

        for (u32 i = 0; i < 100; i++) {
            memory_free(ptrs[i]);
        }
    }

    return TRUE;
}

// ============================================================================
// MAIN TEST REGISTRATION
// ============================================================================

void register_memory_testcases() {

    // Basic allocation tests
    test_manager_add(test_memory_single_allocation, "single_allocation");
    test_manager_add(test_memory_multiple_different_sizes, "multiple_different_sizes");
    test_manager_add(test_memory_allocation_uniqueness, "allocation_uniqueness");
    test_manager_add(test_memory_power_of_two_sizes, "power_of_two_sizes");
    test_manager_add(test_memory_odd_sizes, "odd_sizes");
    test_manager_add(test_memory_large_allocation, "large_allocation");
    test_manager_add(test_memory_very_large_allocation, "very_large_allocation");

    // Data integrity tests
    test_manager_add(test_memory_write_read_bytes, "write_read_bytes");
    test_manager_add(test_memory_write_read_words, "write_read_words");
    test_manager_add(test_memory_pattern_verification, "pattern_verification");
    test_manager_add(test_memory_sequential_pattern, "sequential_pattern");
    test_manager_add(test_memory_isolation, "isolation");

    // Reallocation tests
    test_manager_add(test_memory_realloc_grow_small, "realloc_grow_small");
    test_manager_add(test_memory_realloc_grow_large, "realloc_grow_large");
    test_manager_add(test_memory_realloc_shrink, "realloc_shrink");
    test_manager_add(test_memory_realloc_same_size, "realloc_same_size");
    test_manager_add(test_memory_realloc_data_preservation_grow, "realloc_data_preservation_grow");
    test_manager_add(test_memory_realloc_data_preservation_shrink, "realloc_data_preservation_shrink");
    test_manager_add(test_memory_realloc_multiple_times, "realloc_multiple_times");
    test_manager_add(test_memory_realloc_with_data_multiple_times, "realloc_with_data_multiple_times");

    // Ordering tests
    test_manager_add(test_memory_fifo_order, "fifo_order");
    test_manager_add(test_memory_lifo_order, "lifo_order");
    test_manager_add(test_memory_random_order, "random_order");
    test_manager_add(test_memory_alternating_alloc_free, "alternating_alloc_free");
    test_manager_add(test_memory_interleaved_operations, "interleaved_operations");
    test_manager_add(test_memory_complex_interleaving, "complex_interleaving");

    // Fragmentation tests
    test_manager_add(test_memory_fragmentation_basic, "fragmentation_basic");
    test_manager_add(test_memory_fragmentation_varying_sizes, "fragmentation_varying_sizes");
    test_manager_add(test_memory_fragmentation_worst_case, "fragmentation_worst_case");

    // Stress tests
    test_manager_add(test_memory_stress_many_allocations, "stress_many_allocations");
    test_manager_add(test_memory_stress_varying_sizes, "stress_varying_sizes");
    test_manager_add(test_memory_stress_repeated_cycles, "stress_repeated_cycles");
    test_manager_add(test_memory_stress_with_realloc, "stress_with_realloc");

    // Red-black tree specific tests
    test_manager_add(test_memory_tree_left_heavy, "tree_left_heavy");
    test_manager_add(test_memory_tree_right_heavy, "tree_right_heavy");
    test_manager_add(test_memory_tree_balanced, "tree_balanced");
    test_manager_add(test_memory_tree_rotations, "tree_rotations");

    // Multithreading tests
    test_manager_add(test_memory_concurrent_allocations_4_threads, "concurrent_allocations_4_threads");
    test_manager_add(test_memory_concurrent_allocations_8_threads, "concurrent_allocations_8_threads");
    test_manager_add(test_memory_concurrent_allocations_16_threads, "concurrent_allocations_16_threads");
    test_manager_add(test_memory_concurrent_mixed_operations, "concurrent_mixed_operations");
    test_manager_add(test_memory_concurrent_realloc_stress, "concurrent_realloc_stress");
    test_manager_add(test_memory_concurrent_varying_sizes, "concurrent_varying_sizes");
    test_manager_add(test_memory_concurrent_stress_heavy, "concurrent_stress_heavy");
    test_manager_add(test_memory_concurrent_interleaved_operations, "concurrent_interleaved_operations");

    // Boundary and edge cases
    test_manager_add(test_memory_single_byte_allocation, "single_byte_allocation");
    test_manager_add(test_memory_alignment_check, "alignment_check");
    test_manager_add(test_memory_repeated_same_size, "repeated_same_size");
    test_manager_add(test_memory_progressive_growth, "progressive_growth");
    test_manager_add(test_memory_progressive_shrink, "progressive_shrink");
    test_manager_add(test_memory_zigzag_realloc, "zigzag_realloc");

    // Comprehensive integration tests
    test_manager_add(test_memory_lifecycle_complete, "lifecycle_complete");
    test_manager_add(test_memory_torture_test, "torture_test");
}