/**
 * @file thread_pool.c
 * @brief Thread pool implementation
 */

#include "thread_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Worker Thread Function
 * ============================================================================ */

/**
 * @brief Worker thread main loop
 * 
 * Each worker:
 * 1. Waits for a task from the queue
 * 2. Executes the task
 * 3. Updates statistics
 * 4. Goes back to waiting
 * 
 * Loop continues until shutdown is signaled.
 */
static void* worker_thread(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    
    while (1) {
        task_func_t func;
        void* task_arg;
        
        // Wait for a task (blocking)
        int ret = task_queue_pop(&pool->queue, &func, &task_arg);
        
        // Check if we should exit
        if (ret != 0) {
            // Queue returned error (shutdown with empty queue)
            break;
        }
        
        // Check pool state
        if (atomic_load(&pool->state) == POOL_STATE_TERMINATED) {
            break;
        }
        
        // Increment active count BEFORE executing (we now own this task)
        atomic_fetch_add(&pool->active_tasks, 1);
        
        // Execute the task
        func(task_arg);
        
        // Decrement active count and increment completed
        atomic_fetch_sub(&pool->active_tasks, 1);
        atomic_fetch_add(&pool->completed_tasks, 1);
        
        // Signal waiters that a task completed
        pthread_mutex_lock(&pool->wait_lock);
        pthread_cond_broadcast(&pool->wait_cond);
        pthread_mutex_unlock(&pool->wait_lock);
    }
    
    return NULL;
}

/* ============================================================================
 * Thread Pool Creation
 * ============================================================================ */

thread_pool_t* thread_pool_create(size_t num_threads, size_t queue_capacity) {
    // Allocate pool structure
    thread_pool_t* pool = calloc(1, sizeof(thread_pool_t));
    if (!pool) {
        return NULL;
    }
    
    // Determine number of threads
    if (num_threads == 0) {
        num_threads = platform_get_num_cores();
        if (num_threads == 0) {
            num_threads = 4;  // Fallback default
        }
    }
    
    // Set queue capacity
    if (queue_capacity == 0) {
        queue_capacity = POOL_DEFAULT_QUEUE_CAPACITY;
    }
    
    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    
    // Initialize task queue
    if (task_queue_init(&pool->queue, queue_capacity) != 0) {
        free(pool);
        return NULL;
    }
    
    // Initialize wait synchronization
    if (pthread_mutex_init(&pool->wait_lock, NULL) != 0) {
        task_queue_destroy(&pool->queue);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->wait_cond, NULL) != 0) {
        pthread_mutex_destroy(&pool->wait_lock);
        task_queue_destroy(&pool->queue);
        free(pool);
        return NULL;
    }
    
    // Initialize state
    atomic_init(&pool->state, POOL_STATE_RUNNING);
    atomic_init(&pool->active_tasks, 0);
    atomic_init(&pool->completed_tasks, 0);
    atomic_init(&pool->submitted_tasks, 0);
    
    // Allocate worker thread handles
    pool->workers = calloc(num_threads, sizeof(pthread_t));
    if (!pool->workers) {
        pthread_cond_destroy(&pool->wait_cond);
        pthread_mutex_destroy(&pool->wait_lock);
        task_queue_destroy(&pool->queue);
        free(pool);
        return NULL;
    }
    
    // Create worker threads
    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->workers[i], NULL, worker_thread, pool) != 0) {
            // Failed to create thread - initiate shutdown
            atomic_store(&pool->state, POOL_STATE_SHUTDOWN);
            task_queue_shutdown(&pool->queue);
            
            // Join already created threads
            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->workers[j], NULL);
            }
            
            free(pool->workers);
            pthread_cond_destroy(&pool->wait_cond);
            pthread_mutex_destroy(&pool->wait_lock);
            task_queue_destroy(&pool->queue);
            free(pool);
            return NULL;
        }
    }
    
    return pool;
}

/* ============================================================================
 * Thread Pool Destruction
 * ============================================================================ */

void thread_pool_destroy(thread_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Set shutdown state
    int expected = POOL_STATE_RUNNING;
    if (!atomic_compare_exchange_strong(&pool->state, &expected, POOL_STATE_SHUTDOWN)) {
        // Already shutting down
        return;
    }
    
    // Signal shutdown to queue (wakes all waiting workers)
    task_queue_shutdown(&pool->queue);
    
    // Wait for all workers to finish
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->workers[i], NULL);
    }
    
    // Mark as terminated
    atomic_store(&pool->state, POOL_STATE_TERMINATED);
    
    // Clean up resources
    free(pool->workers);
    pthread_cond_destroy(&pool->wait_cond);
    pthread_mutex_destroy(&pool->wait_lock);
    task_queue_destroy(&pool->queue);
    free(pool);
}

/* ============================================================================
 * Task Submission
 * ============================================================================ */

int thread_pool_submit(thread_pool_t* pool, task_func_t func, void* arg,
                       task_priority_t priority) {
    if (!pool || !func) {
        return -1;
    }
    
    // Check if pool is accepting tasks
    if (atomic_load(&pool->state) != POOL_STATE_RUNNING) {
        return -2;  // Pool is shutting down
    }
    
    int ret = task_queue_push(&pool->queue, func, arg, priority);
    if (ret == 0) {
        atomic_fetch_add(&pool->submitted_tasks, 1);
    }
    return ret;
}

/* ============================================================================
 * Wait for Completion
 * ============================================================================ */

int thread_pool_wait(thread_pool_t* pool) {
    if (!pool) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->wait_lock);
    
    // Wait until all submitted tasks are completed
    while (atomic_load(&pool->completed_tasks) < atomic_load(&pool->submitted_tasks)) {
        pthread_cond_wait(&pool->wait_cond, &pool->wait_lock);
    }
    
    pthread_mutex_unlock(&pool->wait_lock);
    
    return 0;
}

/* ============================================================================
 * Statistics
 * ============================================================================ */

size_t thread_pool_pending(thread_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    return task_queue_size(&pool->queue);
}

size_t thread_pool_active(thread_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    return atomic_load(&pool->active_tasks);
}

size_t thread_pool_completed(thread_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    return atomic_load(&pool->completed_tasks);
}

size_t thread_pool_size(thread_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    return pool->num_threads;
}
