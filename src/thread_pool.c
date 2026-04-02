/**
 * @file thread_pool.c
 * @brief Thread pool implementation
 */

#include "thread_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void* worker_thread(void* arg) {
    thread_pool_t* pool = (thread_pool_t*)arg;
    
    while (1) {
        task_func_t func;
        void* task_arg;
        
        int ret = task_queue_pop(&pool->queue, &func, &task_arg);
        
        if (ret != 0) {
            break;
        }
        
        if (atomic_load(&pool->state) == POOL_STATE_TERMINATED) {
            break;
        }
        
        atomic_fetch_add(&pool->active_tasks, 1);
        func(task_arg);
        atomic_fetch_sub(&pool->active_tasks, 1);
        atomic_fetch_add(&pool->completed_tasks, 1);
        
        pthread_mutex_lock(&pool->wait_lock);
        pthread_cond_broadcast(&pool->wait_cond);
        pthread_mutex_unlock(&pool->wait_lock);
    }
    
    return NULL;
}

thread_pool_t* thread_pool_create(size_t num_threads, size_t queue_capacity) {
    thread_pool_t* pool = calloc(1, sizeof(thread_pool_t));
    if (!pool) {
        return NULL;
    }
    
    if (num_threads == 0) {
        num_threads = platform_get_num_cores();
        if (num_threads == 0) {
            num_threads = 4;
        }
    }
    
    if (queue_capacity == 0) {
        queue_capacity = POOL_DEFAULT_QUEUE_CAPACITY;
    }
    
    pool->num_threads = num_threads;
    pool->queue_capacity = queue_capacity;
    
    if (task_queue_init(&pool->queue, queue_capacity) != 0) {
        free(pool);
        return NULL;
    }
    
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
    
    atomic_init(&pool->state, POOL_STATE_RUNNING);
    atomic_init(&pool->active_tasks, 0);
    atomic_init(&pool->completed_tasks, 0);
    atomic_init(&pool->submitted_tasks, 0);
    
    pool->workers = calloc(num_threads, sizeof(pthread_t));
    if (!pool->workers) {
        pthread_cond_destroy(&pool->wait_cond);
        pthread_mutex_destroy(&pool->wait_lock);
        task_queue_destroy(&pool->queue);
        free(pool);
        return NULL;
    }
    
    for (size_t i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->workers[i], NULL, worker_thread, pool) != 0) {
            atomic_store(&pool->state, POOL_STATE_SHUTDOWN);
            task_queue_shutdown(&pool->queue);
            
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

void thread_pool_destroy(thread_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    int expected = POOL_STATE_RUNNING;
    if (!atomic_compare_exchange_strong(&pool->state, &expected, POOL_STATE_SHUTDOWN)) {
        return;
    }
    
    task_queue_shutdown(&pool->queue);
    
    for (size_t i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->workers[i], NULL);
    }
    
    atomic_store(&pool->state, POOL_STATE_TERMINATED);
    
    free(pool->workers);
    pthread_cond_destroy(&pool->wait_cond);
    pthread_mutex_destroy(&pool->wait_lock);
    task_queue_destroy(&pool->queue);
    free(pool);
}

int thread_pool_submit(thread_pool_t* pool, task_func_t func, void* arg,
                       task_priority_t priority) {
    if (!pool || !func) {
        return -1;
    }
    
    if (atomic_load(&pool->state) != POOL_STATE_RUNNING) {
        return -2;
    }
    
    int ret = task_queue_push(&pool->queue, func, arg, priority);
    if (ret == 0) {
        atomic_fetch_add(&pool->submitted_tasks, 1);
    }
    return ret;
}

int thread_pool_wait(thread_pool_t* pool) {
    if (!pool) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->wait_lock);
    
    while (atomic_load(&pool->completed_tasks) < atomic_load(&pool->submitted_tasks)) {
        pthread_cond_wait(&pool->wait_cond, &pool->wait_lock);
    }
    
    pthread_mutex_unlock(&pool->wait_lock);
    
    return 0;
}

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
