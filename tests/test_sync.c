/**
 * @file test_sync.c
 * @brief Synchronization primitives tests (mutex, semaphore, condition variable)
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>
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
 * Mutex Tests
 * ============================================================================ */

static sthread_mutex_t counter_mutex;
static int shared_counter = 0;

static void* mutex_increment_thread(void* arg) {
    int iterations = *(int*)arg;
    
    for (int i = 0; i < iterations; i++) {
        sthread_mutex_lock(&counter_mutex);
        shared_counter++;
        sthread_mutex_unlock(&counter_mutex);
    }
    
    return NULL;
}

int test_mutex_basic(void) {
    printf("Test: mutex basic operations...");
    
    sthread_mutex_t mutex;
    
    int ret = sthread_mutex_init(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to init mutex");
    }
    
    ret = sthread_mutex_lock(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to lock mutex");
    }
    
    ret = sthread_mutex_unlock(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to unlock mutex");
    }
    
    ret = sthread_mutex_destroy(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to destroy mutex");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_mutex_trylock(void) {
    printf("Test: mutex trylock...");
    
    sthread_mutex_t mutex;
    sthread_mutex_init(&mutex);
    
    // First trylock should succeed
    int ret = sthread_mutex_trylock(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("First trylock should succeed");
    }
    
    // Second trylock should fail (already locked)
    ret = sthread_mutex_trylock(&mutex);
    if (ret != STHREAD_ERROR_BUSY) {
        sthread_mutex_unlock(&mutex);
        sthread_mutex_destroy(&mutex);
        TEST_FAIL("Second trylock should return BUSY");
    }
    
    sthread_mutex_unlock(&mutex);
    sthread_mutex_destroy(&mutex);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_mutex_contention(void) {
    printf("Test: mutex under contention...");
    
    sthread_mutex_init(&counter_mutex);
    shared_counter = 0;
    
    #define NUM_THREADS 4
    #define ITERATIONS 10000
    
    sthread_t threads[NUM_THREADS];
    int iter = ITERATIONS;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        sthread_create(&threads[i], mutex_increment_thread, &iter);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        sthread_join(threads[i], NULL);
    }
    
    sthread_mutex_destroy(&counter_mutex);
    
    int expected = NUM_THREADS * ITERATIONS;
    if (shared_counter != expected) {
        printf(" (got %d, expected %d)", shared_counter, expected);
        TEST_FAIL("Counter mismatch - possible race condition");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_THREADS
    #undef ITERATIONS
}

/* ============================================================================
 * Semaphore Tests
 * ============================================================================ */

int test_semaphore_basic(void) {
    printf("Test: semaphore basic operations...");
    
    sthread_sem_t sem;
    
    int ret = sthread_sem_init(&sem, 2);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to init semaphore");
    }
    
    int value;
    sthread_sem_getvalue(&sem, &value);
    if (value != 2) {
        TEST_FAIL("Initial value should be 2");
    }
    
    // Wait should decrement
    sthread_sem_wait(&sem);
    sthread_sem_getvalue(&sem, &value);
    if (value != 1) {
        TEST_FAIL("Value should be 1 after wait");
    }
    
    // Post should increment
    sthread_sem_post(&sem);
    sthread_sem_getvalue(&sem, &value);
    if (value != 2) {
        TEST_FAIL("Value should be 2 after post");
    }
    
    sthread_sem_destroy(&sem);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_semaphore_trywait(void) {
    printf("Test: semaphore trywait...");
    
    sthread_sem_t sem;
    sthread_sem_init(&sem, 1);
    
    // First trywait should succeed
    int ret = sthread_sem_trywait(&sem);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("First trywait should succeed");
    }
    
    // Second trywait should fail (count is 0)
    ret = sthread_sem_trywait(&sem);
    if (ret != STHREAD_ERROR_BUSY) {
        TEST_FAIL("Second trywait should return BUSY");
    }
    
    sthread_sem_destroy(&sem);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

static sthread_sem_t producer_sem;
static sthread_sem_t consumer_sem;
static int buffer_value = 0;

static void* producer_thread(void* arg) {
    int count = *(int*)arg;
    
    for (int i = 0; i < count; i++) {
        sthread_sem_wait(&producer_sem);  // Wait for space
        buffer_value = i + 1;
        sthread_sem_post(&consumer_sem);  // Signal data available
    }
    
    return NULL;
}

static void* consumer_thread(void* arg) {
    int count = *(int*)arg;
    int sum = 0;
    
    for (int i = 0; i < count; i++) {
        sthread_sem_wait(&consumer_sem);  // Wait for data
        sum += buffer_value;
        sthread_sem_post(&producer_sem);  // Signal space available
    }
    
    return (void*)(long)sum;
}

int test_semaphore_producer_consumer(void) {
    printf("Test: semaphore producer/consumer...");
    
    sthread_sem_init(&producer_sem, 1);  // One slot available
    sthread_sem_init(&consumer_sem, 0);  // No data initially
    
    int count = 100;
    sthread_t producer, consumer;
    
    sthread_create(&producer, producer_thread, &count);
    sthread_create(&consumer, consumer_thread, &count);
    
    sthread_join(producer, NULL);
    
    void* result;
    sthread_join(consumer, &result);
    
    sthread_sem_destroy(&producer_sem);
    sthread_sem_destroy(&consumer_sem);
    
    // Sum of 1 to 100 = 5050
    long sum = (long)result;
    if (sum != 5050) {
        printf(" (got %ld, expected 5050)", sum);
        TEST_FAIL("Sum mismatch");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

/* ============================================================================
 * Condition Variable Tests
 * ============================================================================ */

static sthread_mutex_t cond_mutex;
static sthread_cond_t cond_var;
static int cond_ready = 0;

static void* cond_waiter_thread(void* arg) {
    (void)arg;
    
    sthread_mutex_lock(&cond_mutex);
    while (!cond_ready) {
        sthread_cond_wait(&cond_var, &cond_mutex);
    }
    sthread_mutex_unlock(&cond_mutex);
    
    return (void*)42L;
}

int test_cond_basic(void) {
    printf("Test: condition variable basic...");
    
    sthread_cond_t cond;
    sthread_mutex_t mutex;
    
    int ret = sthread_cond_init(&cond);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to init cond");
    }
    
    ret = sthread_mutex_init(&mutex);
    if (ret != STHREAD_SUCCESS) {
        TEST_FAIL("Failed to init mutex");
    }
    
    sthread_cond_destroy(&cond);
    sthread_mutex_destroy(&mutex);
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

int test_cond_signal(void) {
    printf("Test: condition variable signal...");
    
    sthread_mutex_init(&cond_mutex);
    sthread_cond_init(&cond_var);
    cond_ready = 0;
    
    sthread_t waiter;
    sthread_create(&waiter, cond_waiter_thread, NULL);
    
    // Give waiter time to start waiting
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 10000000 };  // 10ms
    nanosleep(&ts, NULL);
    
    // Signal the waiter
    sthread_mutex_lock(&cond_mutex);
    cond_ready = 1;
    sthread_cond_signal(&cond_var);
    sthread_mutex_unlock(&cond_mutex);
    
    void* result;
    sthread_join(waiter, &result);
    
    sthread_cond_destroy(&cond_var);
    sthread_mutex_destroy(&cond_mutex);
    
    if ((long)result != 42) {
        TEST_FAIL("Waiter thread did not complete correctly");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

static atomic_int broadcast_count = 0;

static void* broadcast_waiter_thread(void* arg) {
    (void)arg;
    
    sthread_mutex_lock(&cond_mutex);
    while (!cond_ready) {
        sthread_cond_wait(&cond_var, &cond_mutex);
    }
    sthread_mutex_unlock(&cond_mutex);
    
    atomic_fetch_add(&broadcast_count, 1);
    return NULL;
}

int test_cond_broadcast(void) {
    printf("Test: condition variable broadcast...");
    
    sthread_mutex_init(&cond_mutex);
    sthread_cond_init(&cond_var);
    cond_ready = 0;
    atomic_store(&broadcast_count, 0);
    
    #define NUM_WAITERS 5
    sthread_t waiters[NUM_WAITERS];
    
    for (int i = 0; i < NUM_WAITERS; i++) {
        sthread_create(&waiters[i], broadcast_waiter_thread, NULL);
    }
    
    // Give waiters time to start waiting
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 20000000 };  // 20ms
    nanosleep(&ts, NULL);
    
    // Broadcast to all waiters
    sthread_mutex_lock(&cond_mutex);
    cond_ready = 1;
    sthread_cond_broadcast(&cond_var);
    sthread_mutex_unlock(&cond_mutex);
    
    for (int i = 0; i < NUM_WAITERS; i++) {
        sthread_join(waiters[i], NULL);
    }
    
    sthread_cond_destroy(&cond_var);
    sthread_mutex_destroy(&cond_mutex);
    
    int count = atomic_load(&broadcast_count);
    if (count != NUM_WAITERS) {
        printf(" (woke %d, expected %d)", count, NUM_WAITERS);
        TEST_FAIL("Not all threads woke up");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
    
    #undef NUM_WAITERS
}

/* ============================================================================
 * NULL Parameter Tests
 * ============================================================================ */

int test_sync_null_params(void) {
    printf("Test: sync NULL parameter handling...");
    
    // Mutex
    if (sthread_mutex_init(NULL) != STHREAD_ERROR_INVALID) {
        TEST_FAIL("mutex_init should reject NULL");
    }
    if (sthread_mutex_lock(NULL) != STHREAD_ERROR_INVALID) {
        TEST_FAIL("mutex_lock should reject NULL");
    }
    
    // Semaphore
    if (sthread_sem_init(NULL, 1) != STHREAD_ERROR_INVALID) {
        TEST_FAIL("sem_init should reject NULL");
    }
    
    // Condition variable
    if (sthread_cond_init(NULL) != STHREAD_ERROR_INVALID) {
        TEST_FAIL("cond_init should reject NULL");
    }
    
    tests_passed++;
    TEST_PASS();
    return 0;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n=== Synchronization Primitives Tests ===\n\n");
    
    // Mutex tests
    test_mutex_basic();
    test_mutex_trylock();
    test_mutex_contention();
    
    // Semaphore tests
    test_semaphore_basic();
    test_semaphore_trywait();
    test_semaphore_producer_consumer();
    
    // Condition variable tests
    test_cond_basic();
    test_cond_signal();
    test_cond_broadcast();
    
    // Error handling
    test_sync_null_params();
    
    printf("\n=== Results: %d passed, %d failed ===\n\n", 
           tests_passed, tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
