/**
 * @file sthread.h
 * @brief Scalable thread management library public API
 */

#ifndef STHREAD_H
#define STHREAD_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STHREAD_SUCCESS = 0,
    STHREAD_ERROR_NOMEM = -1,
    STHREAD_ERROR_INVALID = -2,
    STHREAD_ERROR_BUSY = -3,
    STHREAD_ERROR_TIMEOUT = -4,
    STHREAD_ERROR_DEADLOCK = -5,
    STHREAD_ERROR_SHUTDOWN = -6,
    STHREAD_ERROR_SYSTEM = -7
} sthread_error_t;

typedef struct sthread_handle* sthread_t;
typedef void* (*sthread_func_t)(void* arg);

typedef struct sthread_mutex {
    void* internal;
} sthread_mutex_t;

typedef struct sthread_cond {
    void* internal;
} sthread_cond_t;

typedef struct sthread_sem {
    void* internal;
} sthread_sem_t;

typedef void (*sthread_task_func_t)(void* arg);
typedef struct sthread_pool sthread_pool_t;

typedef struct {
    size_t num_threads;
    size_t queue_capacity;
    size_t stack_size;
    bool enable_priorities;
} sthread_pool_config_t;

typedef enum {
    STHREAD_PRIORITY_LOW = 0,
    STHREAD_PRIORITY_NORMAL = 1,
    STHREAD_PRIORITY_HIGH = 2,
    STHREAD_PRIORITY_CRITICAL = 3
} sthread_priority_t;

int sthread_create(sthread_t* thread, sthread_func_t func, void* arg);
int sthread_join(sthread_t thread, void** retval);
int sthread_detach(sthread_t thread);
sthread_t sthread_self(void);
void sthread_yield(void);

int sthread_mutex_init(sthread_mutex_t* mutex);
int sthread_mutex_destroy(sthread_mutex_t* mutex);
int sthread_mutex_lock(sthread_mutex_t* mutex);
int sthread_mutex_trylock(sthread_mutex_t* mutex);
int sthread_mutex_unlock(sthread_mutex_t* mutex);

int sthread_cond_init(sthread_cond_t* cond);
int sthread_cond_destroy(sthread_cond_t* cond);
int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex);
int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex, 
                           unsigned int timeout_ms);
int sthread_cond_signal(sthread_cond_t* cond);
int sthread_cond_broadcast(sthread_cond_t* cond);

int sthread_sem_init(sthread_sem_t* sem, unsigned int value);
int sthread_sem_destroy(sthread_sem_t* sem);
int sthread_sem_wait(sthread_sem_t* sem);
int sthread_sem_trywait(sthread_sem_t* sem);
int sthread_sem_post(sthread_sem_t* sem);
int sthread_sem_getvalue(sthread_sem_t* sem, int* value);

sthread_pool_t* sthread_pool_create(size_t num_threads);
sthread_pool_t* sthread_pool_create_with_config(const sthread_pool_config_t* config);
int sthread_pool_destroy(sthread_pool_t* pool);
int sthread_pool_submit(sthread_pool_t* pool, sthread_task_func_t func, void* arg);
int sthread_pool_submit_priority(sthread_pool_t* pool, sthread_task_func_t func,
                                  void* arg, sthread_priority_t priority);
int sthread_pool_wait(sthread_pool_t* pool);
size_t sthread_pool_pending(sthread_pool_t* pool);
size_t sthread_pool_active(sthread_pool_t* pool);
size_t sthread_pool_size(sthread_pool_t* pool);

size_t sthread_get_num_cores(void);
const char* sthread_strerror(int error);
const char* sthread_version(void);

#ifdef __cplusplus
}
#endif

#endif /* STHREAD_H */
