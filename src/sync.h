/**
 * @file sync.h
 * @brief Internal synchronization primitives interface
 */

#ifndef SYNC_H
#define SYNC_H

#include "platform.h"
#include <pthread.h>
#include <stdbool.h>

/* ============================================================================
 * Internal Mutex Implementation
 * ============================================================================ */

typedef struct {
    pthread_mutex_t lock;
    bool initialized;
} sync_mutex_t;

int sync_mutex_init(sync_mutex_t* mutex);
int sync_mutex_destroy(sync_mutex_t* mutex);
int sync_mutex_lock(sync_mutex_t* mutex);
int sync_mutex_trylock(sync_mutex_t* mutex);
int sync_mutex_unlock(sync_mutex_t* mutex);

/* ============================================================================
 * Internal Condition Variable Implementation
 * ============================================================================ */

typedef struct {
    pthread_cond_t cond;
    bool initialized;
} sync_cond_t;

int sync_cond_init(sync_cond_t* cond);
int sync_cond_destroy(sync_cond_t* cond);
int sync_cond_wait(sync_cond_t* cond, sync_mutex_t* mutex);
int sync_cond_timedwait(sync_cond_t* cond, sync_mutex_t* mutex, unsigned int timeout_ms);
int sync_cond_signal(sync_cond_t* cond);
int sync_cond_broadcast(sync_cond_t* cond);

/* ============================================================================
 * Internal Semaphore Implementation
 * 
 * Implemented using mutex + condition variable for macOS compatibility.
 * ============================================================================ */

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    unsigned int count;
    unsigned int max_count;
    bool initialized;
} sync_sem_t;

int sync_sem_init(sync_sem_t* sem, unsigned int value);
int sync_sem_destroy(sync_sem_t* sem);
int sync_sem_wait(sync_sem_t* sem);
int sync_sem_trywait(sync_sem_t* sem);
int sync_sem_post(sync_sem_t* sem);
int sync_sem_getvalue(sync_sem_t* sem, int* value);

#endif /* SYNC_H */
