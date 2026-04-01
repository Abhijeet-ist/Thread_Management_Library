/**
 * @file hello_threads.c
 * @brief Simple demonstration of the sthread library
 * 
 * This example shows basic usage of:
 * - Thread pool creation and destruction
 * - Task submission
 * - Waiting for completion
 * - Synchronization primitives
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

/* ============================================================================
 * Example 1: Basic Thread Pool Usage
 * ============================================================================ */

static atomic_int greeting_count = 0;

static void greet_task(void* arg) {
    int id = *(int*)arg;
    printf("  Hello from task %d!\n", id);
    atomic_fetch_add(&greeting_count, 1);
}

void example_basic_pool(void) {
    printf("\n=== Example 1: Basic Thread Pool ===\n\n");
    
    // Create a thread pool with 4 workers
    sthread_pool_t* pool = sthread_pool_create(4);
    if (!pool) {
        fprintf(stderr, "Failed to create pool\n");
        return;
    }
    
    printf("Created thread pool with %zu workers\n\n", sthread_pool_size(pool));
    
    // Submit some tasks
    int task_ids[10];
    for (int i = 0; i < 10; i++) {
        task_ids[i] = i;
        sthread_pool_submit(pool, greet_task, &task_ids[i]);
    }
    
    // Wait for all tasks to complete
    sthread_pool_wait(pool);
    
    printf("\nAll %d greetings completed!\n", atomic_load(&greeting_count));
    
    // Clean up
    sthread_pool_destroy(pool);
}

/* ============================================================================
 * Example 2: Using Mutex for Thread Safety
 * ============================================================================ */

static sthread_mutex_t counter_mutex;
static int shared_counter = 0;

static void increment_with_mutex(void* arg) {
    int increments = *(int*)arg;
    
    for (int i = 0; i < increments; i++) {
        sthread_mutex_lock(&counter_mutex);
        shared_counter++;
        sthread_mutex_unlock(&counter_mutex);
    }
}

void example_mutex(void) {
    printf("\n=== Example 2: Mutex Synchronization ===\n\n");
    
    sthread_mutex_init(&counter_mutex);
    shared_counter = 0;
    
    sthread_pool_t* pool = sthread_pool_create(4);
    
    int increments = 10000;
    
    // Submit 10 tasks, each incrementing 10000 times
    for (int i = 0; i < 10; i++) {
        sthread_pool_submit(pool, increment_with_mutex, &increments);
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    sthread_mutex_destroy(&counter_mutex);
    
    printf("Expected counter: %d\n", 10 * increments);
    printf("Actual counter:   %d\n", shared_counter);
    printf("Result: %s\n", (shared_counter == 10 * increments) ? "CORRECT" : "RACE CONDITION!");
}

/* ============================================================================
 * Example 3: Producer-Consumer with Semaphore
 * ============================================================================ */

static sthread_sem_t empty_slots;   // Tracks empty buffer slots
static sthread_sem_t full_slots;    // Tracks filled buffer slots
static sthread_mutex_t buffer_lock;
static int buffer[5];
static int buffer_in = 0;
static int buffer_out = 0;

static void producer_task(void* arg) {
    int items = *(int*)arg;
    
    for (int i = 0; i < items; i++) {
        sthread_sem_wait(&empty_slots);     // Wait for empty slot
        
        sthread_mutex_lock(&buffer_lock);
        buffer[buffer_in] = i + 1;          // Produce item
        buffer_in = (buffer_in + 1) % 5;
        sthread_mutex_unlock(&buffer_lock);
        
        sthread_sem_post(&full_slots);      // Signal item available
    }
}

static void consumer_task(void* arg) {
    int items = *(int*)arg;
    int sum = 0;
    
    for (int i = 0; i < items; i++) {
        sthread_sem_wait(&full_slots);      // Wait for item
        
        sthread_mutex_lock(&buffer_lock);
        int item = buffer[buffer_out];      // Consume item
        buffer_out = (buffer_out + 1) % 5;
        sthread_mutex_unlock(&buffer_lock);
        
        sthread_sem_post(&empty_slots);     // Signal slot empty
        
        sum += item;
    }
    
    printf("Consumer consumed %d items, sum = %d\n", items, sum);
}

void example_semaphore(void) {
    printf("\n=== Example 3: Producer-Consumer with Semaphore ===\n\n");
    
    sthread_sem_init(&empty_slots, 5);  // 5 empty slots initially
    sthread_sem_init(&full_slots, 0);   // 0 items initially
    sthread_mutex_init(&buffer_lock);
    buffer_in = 0;
    buffer_out = 0;
    
    sthread_pool_t* pool = sthread_pool_create(2);
    
    int num_items = 20;
    
    // Start producer and consumer
    sthread_pool_submit(pool, producer_task, &num_items);
    sthread_pool_submit(pool, consumer_task, &num_items);
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    sthread_sem_destroy(&empty_slots);
    sthread_sem_destroy(&full_slots);
    sthread_mutex_destroy(&buffer_lock);
    
    // Sum of 1 to 20 = 210
    printf("Expected sum: 210\n");
}

/* ============================================================================
 * Example 4: Parallel Computation
 * ============================================================================ */

typedef struct {
    int start;
    int end;
    long* result;
} range_task_t;

static void sum_range(void* arg) {
    range_task_t* task = (range_task_t*)arg;
    long sum = 0;
    
    for (int i = task->start; i < task->end; i++) {
        sum += i;
    }
    
    atomic_fetch_add((atomic_long*)task->result, sum);
}

void example_parallel_sum(void) {
    printf("\n=== Example 4: Parallel Sum Computation ===\n\n");
    
    sthread_pool_t* pool = sthread_pool_create(4);
    
    int n = 1000000;
    int num_chunks = 4;
    int chunk_size = n / num_chunks;
    
    atomic_long result = 0;
    range_task_t tasks[4];
    
    // Submit parallel tasks
    for (int i = 0; i < num_chunks; i++) {
        tasks[i].start = i * chunk_size;
        tasks[i].end = (i == num_chunks - 1) ? n : (i + 1) * chunk_size;
        tasks[i].result = (long*)&result;
        
        sthread_pool_submit(pool, sum_range, &tasks[i]);
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    
    // Sum of 0 to n-1 = n*(n-1)/2
    long expected = (long)n * (n - 1) / 2;
    long actual = atomic_load(&result);
    
    printf("Computing sum of 0 to %d:\n", n - 1);
    printf("Expected: %ld\n", expected);
    printf("Computed: %ld\n", actual);
    printf("Result: %s\n", (actual == expected) ? "CORRECT" : "ERROR");
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              STHREAD LIBRARY DEMONSTRATION                       ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    
    printf("\nLibrary Version: %s\n", sthread_version());
    printf("CPU Cores: %zu\n", sthread_get_num_cores());
    
    example_basic_pool();
    example_mutex();
    example_semaphore();
    example_parallel_sum();
    
    printf("\n=== All Examples Completed ===\n\n");
    
    return 0;
}
