// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

#include "test_manager.h"
#include "memory.h"

void register_memory_testcases();

int main() {
    memory_init(TRUE);
    test_manager_init(100); // Initialize with max 100 tests
    register_memory_testcases();
    test_manager_run();
    test_manager_shutdown();
    memory_shutdown();
    return 0;
}
