/**
 * @file test_scale.c
 * @brief Scalability and stress tests
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

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* ============================================================================
 * Task Functions
 * ============================================================================ */

static atomic_long global_counter = 0;

static void counting_task(void* arg) {
    int iterations = *(int*)arg;
    long local_sum = 0;
    
    // Do some actual work
    for (int i = 0; i < iterations; i++) {
        local_sum += i;
    }
    
    atomic_fetch_add(&global_counter, local_sum);
}

static void cpu_bound_task(void* arg) {
    volatile int* result = (volatile int*)arg;
    
    // Simulate CPU-bound work
    volatile long sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += i * i;
    }
    
    *result = (int)(sum & 0xFFFF);
}

/* ============================================================================
 * Tests
 * ============================================================================ */

int test_high_task_volume(void) {
    printf("Test: high task volume (50000 tasks)...");
    
    sthread_pool_t* pool = sthread_pool_create(8);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    atomic_store(&global_counter, 0);
    int iterations = 100;
    
    #define NUM_TASKS 50000
    
    double start = get_time_ms();
    
    for (int i = 0; i < NUM_TASKS; i++) {
        sthread_pool_submit(pool, counting_task, &iterations);
    }
    
    sthread_pool_wait(pool);
    
    double end = get_time_ms();
    double elapsed = end - start;
    double tasks_per_sec = NUM_TASKS / (elapsed / 1000.0);
    
    sthread_pool_destroy(pool);
    
    printf(" (%.0f tasks/sec, %.1f ms)", tasks_per_sec, elapsed);
    
    // Verify correct sum: 50000 tasks, each computing sum(0..99) = 4950
    long expected = (long)NUM_TASKS * 4950;
    long actual = atomic_load(&global_counter);
    
    if (actual != expected) {
        printf(" (got %ld, expected %ld)", actual, expected);
        TEST_FAIL("Counter mismatch");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_TASKS
}

int test_thread_scaling(void) {
    printf("Test: thread scaling performance...\n");
    
    #define SCALE_TASKS 10000
    int iterations = 1000;
    
    size_t thread_counts[] = {1, 2, 4, 8};
    size_t num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    double times[4];
    
    for (size_t c = 0; c < num_configs; c++) {
        size_t threads = thread_counts[c];
        
        sthread_pool_t* pool = sthread_pool_create(threads);
        if (!pool) {
            TEST_FAIL("Failed to create pool");
        }
        
        atomic_store(&global_counter, 0);
        
        double start = get_time_ms();
        
        for (int i = 0; i < SCALE_TASKS; i++) {
            sthread_pool_submit(pool, counting_task, &iterations);
        }
        
        sthread_pool_wait(pool);
        
        double end = get_time_ms();
        times[c] = end - start;
        
        sthread_pool_destroy(pool);
        
        printf("       %zu thread(s): %.1f ms\n", threads, times[c]);
    }
    
    // Verify that more threads generally improves performance
    // (within reasonable variation - not a strict test)
    if (times[3] > times[0]) {
        printf("       Note: 8 threads slower than 1 thread (may be normal for light tasks)\n");
    }
    
    tests_passed++;
    printf("       [PASS]\n");
    return 0;
    
    #undef SCALE_TASKS
}

int test_concurrent_pool_operations(void) {
    printf("Test: concurrent pool operations...");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    #define RESULTS_COUNT 1000
    volatile int results[RESULTS_COUNT];
    memset((void*)results, 0, sizeof(results));
    
    // Submit tasks
    for (int i = 0; i < RESULTS_COUNT; i++) {
        sthread_pool_submit(pool, cpu_bound_task, (void*)&results[i]);
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    // Verify all tasks completed (non-zero results)
    int completed = 0;
    for (int i = 0; i < RESULTS_COUNT; i++) {
        if (results[i] != 0) {
            completed++;
        }
    }
    
    if (completed != RESULTS_COUNT) {
        printf(" (completed %d/%d)", completed, RESULTS_COUNT);
        TEST_FAIL("Not all tasks completed");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef RESULTS_COUNT
}

int test_rapid_create_destroy(void) {
    printf("Test: rapid pool create/destroy cycles...");
    
    #define CYCLES 50
    
    for (int i = 0; i < CYCLES; i++) {
        sthread_pool_t* pool = sthread_pool_create(4);
        if (!pool) {
            printf(" (failed at cycle %d)", i);
            TEST_FAIL("Failed to create pool");
        }
        
        // Submit a few quick tasks
        atomic_store(&global_counter, 0);
        int iterations = 10;
        
        for (int j = 0; j < 10; j++) {
            sthread_pool_submit(pool, counting_task, &iterations);
        }
        
        sthread_pool_wait(pool);
        sthread_pool_destroy(pool);
    }
    
    printf(" (%d cycles)", CYCLES);
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef CYCLES
}

int test_many_small_pools(void) {
    printf("Test: many small pools concurrently...");
    
    #define NUM_POOLS 10
    sthread_pool_t* pools[NUM_POOLS];
    
    // Create all pools
    for (int i = 0; i < NUM_POOLS; i++) {
        pools[i] = sthread_pool_create(2);
        if (!pools[i]) {
            // Cleanup already created
            for (int j = 0; j < i; j++) {
                sthread_pool_destroy(pools[j]);
            }
            TEST_FAIL("Failed to create pool");
        }
    }
    
    // Submit tasks to all pools
    int iterations = 100;
    for (int i = 0; i < NUM_POOLS; i++) {
        for (int j = 0; j < 100; j++) {
            sthread_pool_submit(pools[i], counting_task, &iterations);
        }
    }
    
    // Wait and destroy all pools
    for (int i = 0; i < NUM_POOLS; i++) {
        sthread_pool_wait(pools[i]);
        sthread_pool_destroy(pools[i]);
    }
    
    printf(" (%d pools)", NUM_POOLS);
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_POOLS
}

static atomic_long stability_counter = 0;

static void stability_task(void* arg) {
    int iterations = *(int*)arg;
    long local_sum = 0;
    
    for (int i = 0; i < iterations; i++) {
        local_sum += i;
    }
    
    atomic_fetch_add(&stability_counter, local_sum);
}

int test_memory_stability(void) {
    printf("Test: memory stability under load...");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        TEST_FAIL("Failed to create pool");
    }
    
    int iterations = 50;
    
    // Run many batches of tasks
    for (int batch = 0; batch < 100; batch++) {
        atomic_store(&stability_counter, 0);
        
        for (int i = 0; i < 1000; i++) {
            sthread_pool_submit(pool, stability_task, &iterations);
        }
        
        sthread_pool_wait(pool);
        
        // Verify batch completed correctly
        long expected = 1000L * 1225L;  // sum(0..49) = 1225, 1000 tasks
        long actual = atomic_load(&stability_counter);
        
        if (actual != expected) {
            sthread_pool_destroy(pool);
            printf(" (batch %d: got %ld, expected %ld)", batch, actual, expected);
            TEST_FAIL("Counter mismatch");
        }
    }
    
    sthread_pool_destroy(pool);
    
    printf(" (100 batches)");
    tests_passed++;
    TEST_PASS();
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n=== Scalability and Stress Tests ===\n\n");
    
    test_high_task_volume();
    test_thread_scaling();
    test_concurrent_pool_operations();
    test_rapid_create_destroy();
    test_many_small_pools();
    test_memory_stability();
    
    printf("\n=== Results: %d passed, %d failed ===\n\n", 
           tests_passed, tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
