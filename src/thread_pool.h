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

#define POOL_DEFAULT_THREADS       0
#define POOL_DEFAULT_QUEUE_CAPACITY 1024
#define POOL_DEFAULT_STACK_SIZE    65536

typedef enum {
    POOL_STATE_RUNNING,
    POOL_STATE_SHUTDOWN,
    POOL_STATE_TERMINATED
} pool_state_t;

typedef struct thread_pool {
    pthread_t* workers;
    size_t num_threads;
    task_queue_t queue;
    _Atomic int state;
    atomic_size_t active_tasks;
    atomic_size_t completed_tasks;
    atomic_size_t submitted_tasks;
    pthread_mutex_t wait_lock;
    pthread_cond_t wait_cond;
    size_t queue_capacity;
    bool enable_priorities;
} thread_pool_t;

thread_pool_t* thread_pool_create(size_t num_threads, size_t queue_capacity);
void thread_pool_destroy(thread_pool_t* pool);
int thread_pool_submit(thread_pool_t* pool, task_func_t func, void* arg,
                       task_priority_t priority);
int thread_pool_wait(thread_pool_t* pool);
size_t thread_pool_pending(thread_pool_t* pool);
size_t thread_pool_active(thread_pool_t* pool);
size_t thread_pool_completed(thread_pool_t* pool);
size_t thread_pool_size(thread_pool_t* pool);

#endif /* THREAD_POOL_H */
