/**
 * @file bench_throughput.c
 * @brief Benchmark: Tasks per second throughput measurement
 * 
 * Compares thread pool performance against naive thread-per-task approach.
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <time.h>
#include <pthread.h>

/* ============================================================================
 * Timing Utilities
 * ============================================================================ */

static double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ============================================================================
 * Task Definition
 * ============================================================================ */

static atomic_long total_work = 0;

// Simple task that does a small amount of work
static void simple_task(void* arg) {
    int iterations = *(int*)arg;
    volatile long sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;
    }
    atomic_fetch_add(&total_work, sum);
}

// Thread wrapper for naive approach
static void* thread_wrapper(void* arg) {
    simple_task(arg);
    return NULL;
}

/* ============================================================================
 * Benchmark: Thread Pool
 * ============================================================================ */

static double benchmark_thread_pool(int num_tasks, int task_iterations, int num_threads) {
    sthread_pool_t* pool = sthread_pool_create(num_threads);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return -1;
    }
    
    atomic_store(&total_work, 0);
    
    double start = get_time_seconds();
    
    for (int i = 0; i < num_tasks; i++) {
        sthread_pool_submit(pool, simple_task, &task_iterations);
    }
    
    sthread_pool_wait(pool);
    
    double end = get_time_seconds();
    
    sthread_pool_destroy(pool);
    
    return end - start;
}

/* ============================================================================
 * Benchmark: Naive Thread-Per-Task
 * ============================================================================ */

static double benchmark_naive(int num_tasks, int task_iterations) {
    pthread_t* threads = malloc(num_tasks * sizeof(pthread_t));
    if (!threads) {
        fprintf(stderr, "Failed to allocate thread array\n");
        return -1;
    }
    
    atomic_store(&total_work, 0);
    
    double start = get_time_seconds();
    
    // Create all threads
    for (int i = 0; i < num_tasks; i++) {
        if (pthread_create(&threads[i], NULL, thread_wrapper, &task_iterations) != 0) {
            // Failed to create thread - join what we have
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            return -1;
        }
    }
    
    // Join all threads
    for (int i = 0; i < num_tasks; i++) {
        pthread_join(threads[i], NULL);
    }
    
    double end = get_time_seconds();
    
    free(threads);
    
    return end - start;
}

/* ============================================================================
 * Main Benchmark Driver
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║           STHREAD LIBRARY THROUGHPUT BENCHMARK                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("System: %zu CPU cores detected\n", sthread_get_num_cores());
    printf("Library version: %s\n\n", sthread_version());
    
    int task_iterations = 1000;  // Work per task
    int num_threads = 8;
    
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Benchmark 1: Thread Pool Throughput (varying task count)\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    printf("Configuration: %d worker threads, %d iterations per task\n\n", 
           num_threads, task_iterations);
    
    int task_counts[] = {1000, 5000, 10000, 50000, 100000};
    int num_counts = sizeof(task_counts) / sizeof(task_counts[0]);
    
    printf("┌──────────────┬──────────────┬────────────────┐\n");
    printf("│    Tasks     │     Time     │   Throughput   │\n");
    printf("├──────────────┼──────────────┼────────────────┤\n");
    
    for (int i = 0; i < num_counts; i++) {
        int num_tasks = task_counts[i];
        double elapsed = benchmark_thread_pool(num_tasks, task_iterations, num_threads);
        
        if (elapsed > 0) {
            double throughput = num_tasks / elapsed;
            printf("│ %10d   │ %8.3f s   │ %10.0f/s   │\n", 
                   num_tasks, elapsed, throughput);
        }
    }
    
    printf("└──────────────┴──────────────┴────────────────┘\n\n");
    
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Benchmark 2: Thread Pool vs Naive (Thread-Per-Task)\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    // Only test with smaller task counts for naive approach (it's slow!)
    int comparison_tasks[] = {100, 500, 1000, 2000};
    int num_comparison = sizeof(comparison_tasks) / sizeof(comparison_tasks[0]);
    
    printf("┌──────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("│    Tasks     │ Thread Pool  │    Naive     │   Speedup    │\n");
    printf("├──────────────┼──────────────┼──────────────┼──────────────┤\n");
    
    for (int i = 0; i < num_comparison; i++) {
        int num_tasks = comparison_tasks[i];
        
        double pool_time = benchmark_thread_pool(num_tasks, task_iterations, num_threads);
        double naive_time = benchmark_naive(num_tasks, task_iterations);
        
        if (pool_time > 0 && naive_time > 0) {
            double speedup = naive_time / pool_time;
            printf("│ %10d   │ %8.4f s   │ %8.4f s   │ %8.2fx    │\n", 
                   num_tasks, pool_time, naive_time, speedup);
        } else if (pool_time > 0) {
            printf("│ %10d   │ %8.4f s   │    FAILED    │      -       │\n", 
                   num_tasks, pool_time);
        }
    }
    
    printf("└──────────────┴──────────────┴──────────────┴──────────────┘\n\n");
    
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Benchmark 3: Thread Scaling (constant tasks, varying threads)\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    int scaling_tasks = 50000;
    int thread_counts[] = {1, 2, 4, 8, 16};
    int num_thread_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    printf("Configuration: %d tasks, %d iterations per task\n\n", 
           scaling_tasks, task_iterations);
    
    printf("┌──────────────┬──────────────┬────────────────┬──────────────┐\n");
    printf("│   Threads    │     Time     │   Throughput   │   Scaling    │\n");
    printf("├──────────────┼──────────────┼────────────────┼──────────────┤\n");
    
    double baseline_time = 0;
    
    for (int i = 0; i < num_thread_configs; i++) {
        int threads = thread_counts[i];
        double elapsed = benchmark_thread_pool(scaling_tasks, task_iterations, threads);
        
        if (elapsed > 0) {
            double throughput = scaling_tasks / elapsed;
            
            if (i == 0) {
                baseline_time = elapsed;
                printf("│ %10d   │ %8.3f s   │ %10.0f/s   │   baseline   │\n", 
                       threads, elapsed, throughput);
            } else {
                double scaling = baseline_time / elapsed;
                printf("│ %10d   │ %8.3f s   │ %10.0f/s   │ %8.2fx    │\n", 
                       threads, elapsed, throughput, scaling);
            }
        }
    }
    
    printf("└──────────────┴──────────────┴────────────────┴──────────────┘\n\n");
    
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Benchmark Complete!\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}
