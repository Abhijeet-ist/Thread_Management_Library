/**
 * @file sync.c
 * @brief Synchronization primitives implementation
 */

#include "sync.h"
#include <errno.h>
#include <string.h>

/* ============================================================================
 * Error code mapping
 * ============================================================================ */

#define STHREAD_SUCCESS        0
#define STHREAD_ERROR_NOMEM   -1
#define STHREAD_ERROR_INVALID -2
#define STHREAD_ERROR_BUSY    -3
#define STHREAD_ERROR_TIMEOUT -4
#define STHREAD_ERROR_SYSTEM  -7

static int map_pthread_error(int err) {
    switch (err) {
        case 0:        return STHREAD_SUCCESS;
        case ENOMEM:   return STHREAD_ERROR_NOMEM;
        case EINVAL:   return STHREAD_ERROR_INVALID;
        case EBUSY:    return STHREAD_ERROR_BUSY;
        case ETIMEDOUT: return STHREAD_ERROR_TIMEOUT;
        default:       return STHREAD_ERROR_SYSTEM;
    }
}

/* ============================================================================
 * Mutex Implementation
 * ============================================================================ */

int sync_mutex_init(sync_mutex_t* mutex) {
    if (!mutex) {
        return STHREAD_ERROR_INVALID;
    }
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // Use error-checking mutex for debugging
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    
    int ret = pthread_mutex_init(&mutex->lock, &attr);
    pthread_mutexattr_destroy(&attr);
    
    if (ret == 0) {
        mutex->initialized = true;
    }
    
    return map_pthread_error(ret);
}

int sync_mutex_destroy(sync_mutex_t* mutex) {
    if (!mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = pthread_mutex_destroy(&mutex->lock);
    if (ret == 0) {
        mutex->initialized = false;
    }
    
    return map_pthread_error(ret);
}

int sync_mutex_lock(sync_mutex_t* mutex) {
    if (!mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_mutex_lock(&mutex->lock));
}

int sync_mutex_trylock(sync_mutex_t* mutex) {
    if (!mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_mutex_trylock(&mutex->lock));
}

int sync_mutex_unlock(sync_mutex_t* mutex) {
    if (!mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_mutex_unlock(&mutex->lock));
}

/* ============================================================================
 * Condition Variable Implementation
 * ============================================================================ */

int sync_cond_init(sync_cond_t* cond) {
    if (!cond) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = pthread_cond_init(&cond->cond, NULL);
    if (ret == 0) {
        cond->initialized = true;
    }
    
    return map_pthread_error(ret);
}

int sync_cond_destroy(sync_cond_t* cond) {
    if (!cond || !cond->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = pthread_cond_destroy(&cond->cond);
    if (ret == 0) {
        cond->initialized = false;
    }
    
    return map_pthread_error(ret);
}

int sync_cond_wait(sync_cond_t* cond, sync_mutex_t* mutex) {
    if (!cond || !cond->initialized || !mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_cond_wait(&cond->cond, &mutex->lock));
}

int sync_cond_timedwait(sync_cond_t* cond, sync_mutex_t* mutex, unsigned int timeout_ms) {
    if (!cond || !cond->initialized || !mutex || !mutex->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    struct timespec ts;
    platform_get_abstime(&ts, timeout_ms);
    
    return map_pthread_error(pthread_cond_timedwait(&cond->cond, &mutex->lock, &ts));
}

int sync_cond_signal(sync_cond_t* cond) {
    if (!cond || !cond->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_cond_signal(&cond->cond));
}

int sync_cond_broadcast(sync_cond_t* cond) {
    if (!cond || !cond->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    return map_pthread_error(pthread_cond_broadcast(&cond->cond));
}

/* ============================================================================
 * Semaphore Implementation
 * 
 * Uses mutex + condition variable for portability (works on macOS and Linux).
 * ============================================================================ */

int sync_sem_init(sync_sem_t* sem, unsigned int value) {
    if (!sem) {
        return STHREAD_ERROR_INVALID;
    }
    
    int ret = pthread_mutex_init(&sem->lock, NULL);
    if (ret != 0) {
        return map_pthread_error(ret);
    }
    
    ret = pthread_cond_init(&sem->cond, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&sem->lock);
        return map_pthread_error(ret);
    }
    
    sem->count = value;
    sem->max_count = value;  // Track initial value for potential debugging
    sem->initialized = true;
    
    return STHREAD_SUCCESS;
}

int sync_sem_destroy(sync_sem_t* sem) {
    if (!sem || !sem->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    // Destroy condition variable first
    int ret = pthread_cond_destroy(&sem->cond);
    if (ret != 0) {
        return map_pthread_error(ret);
    }
    
    ret = pthread_mutex_destroy(&sem->lock);
    if (ret == 0) {
        sem->initialized = false;
    }
    
    return map_pthread_error(ret);
}

int sync_sem_wait(sync_sem_t* sem) {
    if (!sem || !sem->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    pthread_mutex_lock(&sem->lock);
    
    // Wait while count is zero
    while (sem->count == 0) {
        pthread_cond_wait(&sem->cond, &sem->lock);
    }
    
    // Decrement count
    sem->count--;
    
    pthread_mutex_unlock(&sem->lock);
    
    return STHREAD_SUCCESS;
}

int sync_sem_trywait(sync_sem_t* sem) {
    if (!sem || !sem->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    pthread_mutex_lock(&sem->lock);
    
    if (sem->count == 0) {
        pthread_mutex_unlock(&sem->lock);
        return STHREAD_ERROR_BUSY;
    }
    
    sem->count--;
    
    pthread_mutex_unlock(&sem->lock);
    
    return STHREAD_SUCCESS;
}

int sync_sem_post(sync_sem_t* sem) {
    if (!sem || !sem->initialized) {
        return STHREAD_ERROR_INVALID;
    }
    
    pthread_mutex_lock(&sem->lock);
    
    sem->count++;
    
    // Signal one waiting thread
    pthread_cond_signal(&sem->cond);
    
    pthread_mutex_unlock(&sem->lock);
    
    return STHREAD_SUCCESS;
}

int sync_sem_getvalue(sync_sem_t* sem, int* value) {
    if (!sem || !sem->initialized || !value) {
        return STHREAD_ERROR_INVALID;
    }
    
    pthread_mutex_lock(&sem->lock);
    *value = (int)sem->count;
    pthread_mutex_unlock(&sem->lock);
    
    return STHREAD_SUCCESS;
}
