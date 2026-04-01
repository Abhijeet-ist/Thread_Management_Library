/**
 * @file thread_pool.h
 * @brief Thread pool interface
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "platform.h"
#include "scheduler.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ============================================================================
 * Thread Pool Configuration
 * ============================================================================ */

#define POOL_DEFAULT_THREADS       0       /* 0 = auto-detect CPU cores */
#define POOL_DEFAULT_QUEUE_CAPACITY 1024   /* Default bounded queue size */
#define POOL_DEFAULT_STACK_SIZE    65536   /* 64KB per thread stack */

/* ============================================================================
 * Thread Pool State
 * ============================================================================ */

typedef enum {
    POOL_STATE_RUNNING,     /* Pool is active and accepting tasks */
    POOL_STATE_SHUTDOWN,    /* Pool is shutting down */
    POOL_STATE_TERMINATED   /* Pool is fully terminated */
} pool_state_t;

/* ============================================================================
 * Thread Pool Structure
 * ============================================================================ */

typedef struct thread_pool {
    /* Worker threads */
    pthread_t* workers;             /* Array of worker thread handles */
    size_t num_threads;             /* Number of worker threads */
    
    /* Task queue */
    task_queue_t queue;             /* Thread-safe task queue */
    
    /* Pool state */
    _Atomic int state;               /* Current pool state */
    
    /* Statistics */
    atomic_size_t active_tasks;     /* Number of tasks currently executing */
    atomic_size_t completed_tasks;  /* Total tasks completed */
    atomic_size_t submitted_tasks;  /* Total tasks submitted */
    
    /* Wait synchronization */
    pthread_mutex_t wait_lock;      /* Lock for wait condition */
    pthread_cond_t wait_cond;       /* Condition for pool_wait() */
    
    /* Configuration */
    size_t queue_capacity;          /* Queue capacity */
    bool enable_priorities;         /* Priority scheduling enabled */
} thread_pool_t;

/* ============================================================================
 * Thread Pool API
 * ============================================================================ */

/**
 * @brief Create a thread pool
 * 
 * @param num_threads Number of worker threads (0 = auto-detect)
 * @param queue_capacity Maximum queue size (0 = default 1024)
 * @return Thread pool handle, NULL on failure
 */
thread_pool_t* thread_pool_create(size_t num_threads, size_t queue_capacity);

/**
 * @brief Destroy a thread pool
 * 
 * Initiates shutdown, waits for workers to finish current tasks, then cleans up.
 * 
 * @param pool Thread pool to destroy
 */
void thread_pool_destroy(thread_pool_t* pool);

/**
 * @brief Submit a task to the pool
 * 
 * @param pool Thread pool
 * @param func Task function
 * @param arg Task argument
 * @param priority Task priority
 * @return 0 on success, error code on failure
 */
int thread_pool_submit(thread_pool_t* pool, task_func_t func, void* arg,
                       task_priority_t priority);

/**
 * @brief Wait for all submitted tasks to complete
 * 
 * @param pool Thread pool
 * @return 0 on success
 */
int thread_pool_wait(thread_pool_t* pool);

/**
 * @brief Get number of pending tasks
 */
size_t thread_pool_pending(thread_pool_t* pool);

/**
 * @brief Get number of active tasks
 */
size_t thread_pool_active(thread_pool_t* pool);

/**
 * @brief Get number of completed tasks
 */
size_t thread_pool_completed(thread_pool_t* pool);

/**
 * @brief Get pool size (number of workers)
 */
size_t thread_pool_size(thread_pool_t* pool);

#endif /* THREAD_POOL_H */
