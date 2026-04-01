/**
 * @file test_pool.c
 * @brief Thread pool tests
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

#define TEST_PASS() printf("  [PASS]\n")
#define TEST_FAIL(msg) do { printf("  [FAIL] %s\n", msg); tests_failed++; return 1; } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Task Functions
 * ============================================================================ */

static atomic_int task_counter = 0;

static void increment_task(void* arg) {
    (void)arg;
    atomic_fetch_add(&task_counter, 1);
}

static void increment_array_task(void* arg) {
    atomic_int* counter = (atomic_int*)arg;
    atomic_fetch_add(counter, 1);
}

static void sleep_task(void* arg) {
    int ms = *(int*)arg;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ms * 1000000L };
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * Tests
 * ============================================================================ */

int test_pool_create_destroy(void) {
    printf("Test: pool create and destroy...");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    size_t size = sthread_pool_size(pool);
    if (size != 4) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Pool size incorrect");
    }
    
    int ret = sthread_pool_destroy(pool);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to destroy pool");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_auto_detect_cores(void) {
    printf("Test: pool auto-detect cores...");
    
    sthread_pool_t* pool = sthread_pool_create(0);  // 0 = auto-detect
    if (!pool) {
        TEST_FAIL("Failed to create pool with auto-detect");
    }
    
    size_t size = sthread_pool_size(pool);
    size_t cores = sthread_get_num_cores();
    
    if (size != cores) {
        printf(" (pool size %zu, expected %zu cores)", size, cores);
        sthread_pool_destroy(pool);
        TEST_FAIL("Pool size should match core count");
    }
    
    sthread_pool_destroy(pool);
    
    printf(" (%zu threads)", size);
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_single_task(void) {
    printf("Test: pool single task...");
    
    sthread_pool_t* pool = sthread_pool_create(2);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    atomic_store(&task_counter, 0);
    
    int ret = sthread_pool_submit(pool, increment_task, NULL);
    if (ret != STHREAD_SUCCESS) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Failed to submit task");
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    if (atomic_load(&task_counter) != 1) {
        TEST_FAIL("Task did not execute");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_many_tasks(void) {
    printf("Test: pool 1000 tasks...");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    atomic_store(&task_counter, 0);
    
    #define NUM_TASKS 1000
    
    for (int i = 0; i < NUM_TASKS; i++) {
        int ret = sthread_pool_submit(pool, increment_task, NULL);
        if (ret != STHREAD_SUCCESS) {
            sthread_pool_destroy(pool);
            TEST_FAIL("Failed to submit task");
        }
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    int count = atomic_load(&task_counter);
    if (count != NUM_TASKS) {
        printf(" (executed %d, expected %d)", count, NUM_TASKS);
        TEST_FAIL("Not all tasks executed");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_TASKS
}

int test_pool_task_with_data(void) {
    printf("Test: pool tasks with data...");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    #define NUM_COUNTERS 100
    atomic_int counters[NUM_COUNTERS];
    
    for (int i = 0; i < NUM_COUNTERS; i++) {
        atomic_init(&counters[i], 0);
    }
    
    // Submit tasks that increment each counter 10 times
    for (int j = 0; j < 10; j++) {
        for (int i = 0; i < NUM_COUNTERS; i++) {
            sthread_pool_submit(pool, increment_array_task, &counters[i]);
        }
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    // Verify all counters are 10
    for (int i = 0; i < NUM_COUNTERS; i++) {
        if (atomic_load(&counters[i]) != 10) {
            printf(" (counter[%d] = %d, expected 10)", i, atomic_load(&counters[i]));
            TEST_FAIL("Counter value incorrect");
        }
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_COUNTERS
}

int test_pool_pending_active(void) {
    printf("Test: pool pending/active tracking...");
    
    sthread_pool_t* pool = sthread_pool_create(2);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    // Initially nothing pending or active
    if (sthread_pool_pending(pool) != 0 || sthread_pool_active(pool) != 0) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Initial counts should be 0");
    }
    
    // Submit some long-running tasks
    int sleep_ms = 100;
    for (int i = 0; i < 10; i++) {
        sthread_pool_submit(pool, sleep_task, &sleep_ms);
    }
    
    // Should have some pending (2 workers, 10 tasks)
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000 };  // 10ms
    nanosleep(&ts, NULL);
    
    size_t pending = sthread_pool_pending(pool);
    size_t active = sthread_pool_active(pool);
    
    // With 2 workers and 10 tasks, should have ~8 pending and 2 active
    // (timing dependent, so be flexible)
    if (active > 2) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Active should be <= 2 (worker count)");
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    printf(" (pending=%zu, active=%zu observed)", pending, active);
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_wait_idempotent(void) {
    printf("Test: pool wait is idempotent...");
    
    sthread_pool_t* pool = sthread_pool_create(2);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    atomic_store(&task_counter, 0);
    
    for (int i = 0; i < 100; i++) {
        sthread_pool_submit(pool, increment_task, NULL);
    }
    
    // Multiple waits should be safe
    sthread_pool_wait(pool);
    sthread_pool_wait(pool);
    sthread_pool_wait(pool);
    
    if (atomic_load(&task_counter) != 100) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Tasks not completed correctly");
    }
    
    sthread_pool_destroy(pool);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_shutdown_with_pending(void) {
    printf("Test: pool shutdown with pending tasks...");
    
    sthread_pool_t* pool = sthread_pool_create(2);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    // Submit many long tasks
    int sleep_ms = 50;
    for (int i = 0; i < 20; i++) {
        sthread_pool_submit(pool, sleep_task, &sleep_ms);
    }
    
    // Destroy should wait for current tasks but discard pending
    // This should not hang or crash
    sthread_pool_destroy(pool);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_null_params(void) {
    printf("Test: pool NULL parameter handling...");
    
    // NULL pool
    int ret = sthread_pool_submit(NULL, increment_task, NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        TEST_FAIL("Should reject NULL pool");
    }
    
    // NULL function
    sthread_pool_t* pool = sthread_pool_create(2);
    ret = sthread_pool_submit(pool, NULL, NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        sthread_pool_destroy(pool);
        TEST_FAIL("Should reject NULL function");
    }
    sthread_pool_destroy(pool);
    
    // NULL destroy
    ret = sthread_pool_destroy(NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        TEST_FAIL("Should reject NULL destroy");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_pool_stress(void) {
    printf("Test: pool stress (10000 tasks)...");
    
    sthread_pool_t* pool = sthread_pool_create(8);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    atomic_store(&task_counter, 0);
    
    #define STRESS_TASKS 10000
    
    for (int i = 0; i < STRESS_TASKS; i++) {
        sthread_pool_submit(pool, increment_task, NULL);
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    int count = atomic_load(&task_counter);
    if (count != STRESS_TASKS) {
        printf(" (executed %d, expected %d)", count, STRESS_TASKS);
        TEST_FAIL("Not all tasks executed");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef STRESS_TASKS
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n=== Thread Pool Tests ===\n\n");
    
    test_pool_create_destroy();
    test_pool_auto_detect_cores();
    test_pool_single_task();
    test_pool_many_tasks();
    test_pool_task_with_data();
    test_pool_pending_active();
    test_pool_wait_idempotent();
    test_pool_shutdown_with_pending();
    test_pool_null_params();
    test_pool_stress();
    
    printf("\n=== Results: %d passed, %d failed ===\n\n", 
           tests_passed, tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
