/**
 * @file sthread.c
 * @brief Public API implementation - glue layer for the sthread library
 * 
 * This file implements all public API functions declared in sthread.h.
 * It connects user-facing functions to internal implementations.
 */

#include "../include/sthread.h"
#include "platform.h"
#include "sync.h"
#include "thread_pool.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* ============================================================================
 * Version Information
 * ============================================================================ */

#define STHREAD_VERSION_STRING "1.0.0"

/* ============================================================================
 * Thread Handle Structure
 * ============================================================================ */

struct sthread_handle {
    pthread_t thread;
    bool detached;
};

/* ============================================================================
 * Basic Thread Operations
 * ============================================================================ */

int sthread_create(sthread_t* thread, sthread_func_t func, void* arg) {
    if (!thread || !func) {
        return STHREAD_ERROR_INVALID;
    }
    
    struct sthread_handle* handle = malloc(sizeof(struct sthread_handle));
    if (!handle) {
        return STHREAD_ERROR_NOMEM;
    }
    
    handle->detached = false;
    
    int ret = pthread_create(&handle->thread, NULL, func, arg);
    if (ret != 0) {
        free(handle);
        return STHREAD_ERROR_SYSTEM;
    }
    
    *thread = handle;
    return STHREAD_SUCCESS;
}

int sthread_join(sthread_t thread, void** retval) {
    if (!thread) {
        return STHREAD_ERROR_INVALID;
    }
    
    struct sthread_handle* handle = thread;
    
    if (handle->detached) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = pthread_join(handle->thread, retval);
    
    free(handle);
    
    if (ret != 0) {
        return STHREAD_ERROR_SYSTEM;
    }
    
    return STHREAD_SUCCESS;
}

int sthread_detach(sthread_t thread) {
    if (!thread) {
        return STHREAD_ERROR_INVALID;
    }
    
    struct sthread_handle* handle = thread;
    
    int ret = pthread_detach(handle->thread);
    if (ret == 0) {
        handle->detached = true;
        free(handle);
    }
    
    return ret == 0 ? STHREAD_SUCCESS : STHREAD_ERROR_SYSTEM;
}

sthread_t sthread_self(void) {
    // Note: This returns a new handle each time - not ideal but functional
    struct sthread_handle* handle = malloc(sizeof(struct sthread_handle));
    if (!handle) {
        return NULL;
    }
    handle->thread = pthread_self();
    handle->detached = false;
    return handle;
}

void sthread_yield(void) {
    sched_yield();
}

/* ============================================================================
 * Mutex Operations
 * ============================================================================ */

int sthread_mutex_init(sthread_mutex_t* mutex) {
    if (!mutex) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_mutex_t* internal = malloc(sizeof(sync_mutex_t));
    if (!internal) {
        return STHREAD_ERROR_NOMEM;
    }
    
    int ret = sync_mutex_init(internal);
    if (ret != 0) {
        free(internal);
        return ret;
    }
    
    mutex->internal = internal;
    return STHREAD_SUCCESS;
}

int sthread_mutex_destroy(sthread_mutex_t* mutex) {
    if (!mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_mutex_t* internal = (sync_mutex_t*)mutex->internal;
    int ret = sync_mutex_destroy(internal);
    
    free(internal);
    mutex->internal = NULL;
    
    return ret;
}

int sthread_mutex_lock(sthread_mutex_t* mutex) {
    if (!mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_mutex_lock((sync_mutex_t*)mutex->internal);
}

int sthread_mutex_trylock(sthread_mutex_t* mutex) {
    if (!mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_mutex_trylock((sync_mutex_t*)mutex->internal);
}

int sthread_mutex_unlock(sthread_mutex_t* mutex) {
    if (!mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_mutex_unlock((sync_mutex_t*)mutex->internal);
}

/* ============================================================================
 * Condition Variable Operations
 * ============================================================================ */

int sthread_cond_init(sthread_cond_t* cond) {
    if (!cond) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_cond_t* internal = malloc(sizeof(sync_cond_t));
    if (!internal) {
        return STHREAD_ERROR_NOMEM;
    }
    
    int ret = sync_cond_init(internal);
    if (ret != 0) {
        free(internal);
        return ret;
    }
    
    cond->internal = internal;
    return STHREAD_SUCCESS;
}

int sthread_cond_destroy(sthread_cond_t* cond) {
    if (!cond || !cond->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_cond_t* internal = (sync_cond_t*)cond->internal;
    int ret = sync_cond_destroy(internal);
    
    free(internal);
    cond->internal = NULL;
    
    return ret;
}

int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex) {
    if (!cond || !cond->internal || !mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_cond_wait(
        (sync_cond_t*)cond->internal,
        (sync_mutex_t*)mutex->internal
    );
}

int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex,
                           unsigned int timeout_ms) {
    if (!cond || !cond->internal || !mutex || !mutex->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_cond_timedwait(
        (sync_cond_t*)cond->internal,
        (sync_mutex_t*)mutex->internal,
        timeout_ms
    );
}

int sthread_cond_signal(sthread_cond_t* cond) {
    if (!cond || !cond->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_cond_signal((sync_cond_t*)cond->internal);
}

int sthread_cond_broadcast(sthread_cond_t* cond) {
    if (!cond || !cond->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_cond_broadcast((sync_cond_t*)cond->internal);
}

/* ============================================================================
 * Semaphore Operations
 * ============================================================================ */

int sthread_sem_init(sthread_sem_t* sem, unsigned int value) {
    if (!sem) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_sem_t* internal = malloc(sizeof(sync_sem_t));
    if (!internal) {
        return STHREAD_ERROR_NOMEM;
    }
    
    int ret = sync_sem_init(internal, value);
    if (ret != 0) {
        free(internal);
        return ret;
    }
    
    sem->internal = internal;
    return STHREAD_SUCCESS;
}

int sthread_sem_destroy(sthread_sem_t* sem) {
    if (!sem || !sem->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    sync_sem_t* internal = (sync_sem_t*)sem->internal;
    int ret = sync_sem_destroy(internal);
    
    free(internal);
    sem->internal = NULL;
    
    return ret;
}

int sthread_sem_wait(sthread_sem_t* sem) {
    if (!sem || !sem->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_sem_wait((sync_sem_t*)sem->internal);
}

int sthread_sem_trywait(sthread_sem_t* sem) {
    if (!sem || !sem->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_sem_trywait((sync_sem_t*)sem->internal);
}

int sthread_sem_post(sthread_sem_t* sem) {
    if (!sem || !sem->internal) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_sem_post((sync_sem_t*)sem->internal);
}

int sthread_sem_getvalue(sthread_sem_t* sem, int* value) {
    if (!sem || !sem->internal || !value) {
        return STHREAD_ERROR_INVALID;
    }
    
    return sync_sem_getvalue((sync_sem_t*)sem->internal, value);
}

/* ============================================================================
 * Thread Pool Operations
 * ============================================================================ */

sthread_pool_t* sthread_pool_create(size_t num_threads) {
    return (sthread_pool_t*)thread_pool_create(num_threads, 0);
}

sthread_pool_t* sthread_pool_create_with_config(const sthread_pool_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    return (sthread_pool_t*)thread_pool_create(
        config->num_threads,
        config->queue_capacity
    );
}

int sthread_pool_destroy(sthread_pool_t* pool) {
    if (!pool) {
        return STHREAD_ERROR_INVALID;
    }
    
    thread_pool_destroy((thread_pool_t*)pool);
    return STHREAD_SUCCESS;
}

int sthread_pool_submit(sthread_pool_t* pool, sthread_task_func_t func, void* arg) {
    if (!pool || !func) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = thread_pool_submit(
        (thread_pool_t*)pool,
        func,
        arg,
        TASK_PRIORITY_NORMAL
    );
    
    if (ret == -2) {
        return STHREAD_ERROR_SHUTDOWN;
    }
    
    return ret == 0 ? STHREAD_SUCCESS : STHREAD_ERROR_SYSTEM;
}

int sthread_pool_submit_priority(sthread_pool_t* pool, sthread_task_func_t func,
                                  void* arg, sthread_priority_t priority) {
    if (!pool || !func) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = thread_pool_submit(
        (thread_pool_t*)pool,
        func,
        arg,
        (task_priority_t)priority
    );
    
    if (ret == -2) {
        return STHREAD_ERROR_SHUTDOWN;
    }
    
    return ret == 0 ? STHREAD_SUCCESS : STHREAD_ERROR_SYSTEM;
}

int sthread_pool_wait(sthread_pool_t* pool) {
    if (!pool) {
        return STHREAD_ERROR_INVALID;
    }
    
    return thread_pool_wait((thread_pool_t*)pool) == 0 ? 
           STHREAD_SUCCESS : STHREAD_ERROR_SYSTEM;
}

size_t sthread_pool_pending(sthread_pool_t* pool) {
    return thread_pool_pending((thread_pool_t*)pool);
}

size_t sthread_pool_active(sthread_pool_t* pool) {
    return thread_pool_active((thread_pool_t*)pool);
}

size_t sthread_pool_size(sthread_pool_t* pool) {
    return thread_pool_size((thread_pool_t*)pool);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

size_t sthread_get_num_cores(void) {
    return platform_get_num_cores();
}

const char* sthread_strerror(int error) {
    switch (error) {
        case STHREAD_SUCCESS:        return "Success";
        case STHREAD_ERROR_NOMEM:    return "Out of memory";
        case STHREAD_ERROR_INVALID:  return "Invalid argument";
        case STHREAD_ERROR_BUSY:     return "Resource busy";
        case STHREAD_ERROR_TIMEOUT:  return "Operation timed out";
        case STHREAD_ERROR_DEADLOCK: return "Potential deadlock detected";
        case STHREAD_ERROR_SHUTDOWN: return "Pool is shutting down";
        case STHREAD_ERROR_SYSTEM:   return "System call failed";
        default:                     return "Unknown error";
    }
}

const char* sthread_version(void) {
    return STHREAD_VERSION_STRING;
}
