/**
 * @file test_create.c
 * @brief Basic thread creation and joining tests
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

/* ============================================================================
 * Test Utilities
 * ============================================================================ */

#define TEST_PASS() printf("  [PASS]\n")
#define TEST_FAIL(msg) do { printf("  [FAIL] %s\n", msg); return 1; } while(0)

static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================================
 * Test Thread Functions
 * ============================================================================ */

static void* simple_thread(void* arg) {
    int* value = (int*)arg;
    (*value)++;
    return (void*)(long)(*value);
}

static void* sleep_thread(void* arg) {
    int ms = *(int*)arg;
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ms * 1000000 };
    nanosleep(&ts, NULL);
    return NULL;
}

/* ============================================================================
 * Tests
 * ============================================================================ */

int test_thread_create_join(void) {
    printf("Test: thread create and join...");
    
    int value = 42;
    sthread_t thread;
    
    int ret = sthread_create(&thread, simple_thread, &value);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to create thread");
    }
    
    void* result;
    ret = sthread_join(thread, &result);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to join thread");
    }
    
    if (value != 43) {
        TEST_FAIL("Thread did not modify value");
    }
    
    if ((long)result != 43) {
        TEST_FAIL("Thread return value incorrect");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_multiple_threads(void) {
    printf("Test: multiple threads...");
    
    #define NUM_THREADS 10
    sthread_t threads[NUM_THREADS];
    int values[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        values[i] = i;
        int ret = sthread_create(&threads[i], simple_thread, &values[i]);
        if (ret != STHREAD_SUCCESS) {
            TEST_FAIL("Failed to create thread");
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        int ret = sthread_join(threads[i], NULL);
        if (ret != STHREAD_SUCCESS) {
            TEST_FAIL("Failed to join thread");
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (values[i] != i + 1) {
            TEST_FAIL("Thread did not modify value correctly");
        }
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    #undef NUM_THREADS
}

int test_thread_yield(void) {
    printf("Test: thread yield...");
    
    // Just verify yield doesn't crash
    sthread_yield();
    sthread_yield();
    sthread_yield();
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_null_parameters(void) {
    printf("Test: NULL parameter handling...");
    
    // NULL thread pointer
    int ret = sthread_create(NULL, simple_thread, NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        TEST_FAIL("Should return INVALID for NULL thread");
    }
    
    // NULL function pointer
    sthread_t thread;
    ret = sthread_create(&thread, NULL, NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        TEST_FAIL("Should return INVALID for NULL function");
    }
    
    // NULL join
    ret = sthread_join(NULL, NULL);
    if (ret != STHREAD_ERROR_INVALID) {
        TEST_FAIL("Should return INVALID for NULL join");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_get_num_cores(void) {
    printf("Test: get number of cores...");
    
    size_t cores = sthread_get_num_cores();
    if (cores == 0) {
        TEST_FAIL("Number of cores should be > 0");
    }
    
    printf(" (detected %zu cores)", cores);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_version_and_errors(void) {
    printf("Test: version and error strings...");
    
    const char* version = sthread_version();
    if (!version || strlen(version) == 0) {
        TEST_FAIL("Version string is empty");
    }
    
    const char* err = sthread_strerror(STHREAD_SUCCESS);
    if (!err || strlen(err) == 0) {
        TEST_FAIL("Error string is empty");
    }
    
    printf(" (version %s)", version);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n=== Basic Thread Tests ===\n\n");
    
    test_thread_create_join();
    test_multiple_threads();
    test_thread_yield();
    test_null_parameters();
    test_get_num_cores();
    test_version_and_errors();
    
    printf("\n=== Results: %d passed, %d failed ===\n\n", 
           tests_passed, tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
