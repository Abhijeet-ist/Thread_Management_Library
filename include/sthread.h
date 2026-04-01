/**
 * @file sthread.h
 * @brief Scalable Thread Management Library - Public API
 * 
 * A high-performance thread management library that provides:
 * - Thread pool management with efficient worker thread pooling
 * - Thread-safe task queue for work distribution
 * - Synchronization primitives (mutex, semaphore, condition variable)
 * - Clean lifecycle management with graceful shutdown
 * 
 * @author CSE 316 Project
 * @version 1.0.0
 * 
 * Usage Example:
 * @code
 *     sthread_pool_t *pool = sthread_pool_create(4);  // 4 worker threads
 *     sthread_pool_submit(pool, my_task_function, task_data);
 *     sthread_pool_wait(pool);
 *     sthread_pool_destroy(pool);
 * @endcode
 */

#ifndef STHREAD_H
#define STHREAD_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Error Codes
 * ============================================================================ */

/**
 * @brief Error codes returned by sthread functions
 */
typedef enum {
    STHREAD_SUCCESS = 0,          /**< Operation completed successfully */
    STHREAD_ERROR_NOMEM = -1,     /**< Memory allocation failed */
    STHREAD_ERROR_INVALID = -2,   /**< Invalid argument provided */
    STHREAD_ERROR_BUSY = -3,      /**< Resource is busy */
    STHREAD_ERROR_TIMEOUT = -4,   /**< Operation timed out */
    STHREAD_ERROR_DEADLOCK = -5,  /**< Potential deadlock detected */
    STHREAD_ERROR_SHUTDOWN = -6,  /**< Pool is shutting down */
    STHREAD_ERROR_SYSTEM = -7     /**< System call failed */
} sthread_error_t;

/* ============================================================================
 * Basic Thread Types
 * ============================================================================ */

/**
 * @brief Thread handle type (opaque)
 */
typedef struct sthread_handle* sthread_t;

/**
 * @brief Thread function signature
 * @param arg User-provided argument
 * @return Pointer to return value
 */
typedef void* (*sthread_func_t)(void* arg);

/* ============================================================================
 * Synchronization Primitives
 * ============================================================================ */

/**
 * @brief Mutex lock type
 * 
 * Provides mutual exclusion - only one thread can hold the lock at a time.
 * Implemented as a wrapper around pthread_mutex_t for portability.
 */
typedef struct sthread_mutex {
    void* internal;  /**< Internal platform-specific data */
} sthread_mutex_t;

/**
 * @brief Condition variable type
 * 
 * Allows threads to wait for a condition to become true.
 * Must be used in conjunction with a mutex.
 */
typedef struct sthread_cond {
    void* internal;  /**< Internal platform-specific data */
} sthread_cond_t;

/**
 * @brief Semaphore type
 * 
 * Counting semaphore implementation that works on both macOS and Linux.
 * Uses mutex + condition variable internally for portability.
 */
typedef struct sthread_sem {
    void* internal;  /**< Internal platform-specific data */
} sthread_sem_t;

/* ============================================================================
 * Thread Pool Types
 * ============================================================================ */

/**
 * @brief Task function signature for thread pool
 * @param arg User-provided argument for the task
 */
typedef void (*sthread_task_func_t)(void* arg);

/**
 * @brief Thread pool handle (opaque pointer)
 */
typedef struct sthread_pool sthread_pool_t;

/**
 * @brief Thread pool configuration options
 */
typedef struct {
    size_t num_threads;      /**< Number of worker threads (0 = auto-detect cores) */
    size_t queue_capacity;   /**< Maximum tasks in queue (0 = unbounded) */
    size_t stack_size;       /**< Stack size per thread (0 = default 64KB) */
    bool enable_priorities;  /**< Enable task priorities (default: false) */
} sthread_pool_config_t;

/**
 * @brief Task priority levels
 */
typedef enum {
    STHREAD_PRIORITY_LOW = 0,
    STHREAD_PRIORITY_NORMAL = 1,
    STHREAD_PRIORITY_HIGH = 2,
    STHREAD_PRIORITY_CRITICAL = 3
} sthread_priority_t;

/* ============================================================================
 * Basic Thread Operations
 * ============================================================================ */

/**
 * @brief Create a new thread
 * 
 * @param thread Pointer to store the thread handle
 * @param func Thread function to execute
 * @param arg Argument to pass to the thread function
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_create(sthread_t* thread, sthread_func_t func, void* arg);

/**
 * @brief Wait for a thread to complete
 * 
 * @param thread Thread handle to wait for
 * @param retval Pointer to store the thread's return value (can be NULL)
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_join(sthread_t thread, void** retval);

/**
 * @brief Detach a thread (no longer joinable)
 * 
 * @param thread Thread handle to detach
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_detach(sthread_t thread);

/**
 * @brief Get the current thread's handle
 * 
 * @return Current thread handle
 */
sthread_t sthread_self(void);

/**
 * @brief Yield execution to another thread
 */
void sthread_yield(void);

/* ============================================================================
 * Mutex Operations
 * ============================================================================ */

/**
 * @brief Initialize a mutex
 * 
 * @param mutex Pointer to mutex to initialize
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_mutex_init(sthread_mutex_t* mutex);

/**
 * @brief Destroy a mutex
 * 
 * @param mutex Pointer to mutex to destroy
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_mutex_destroy(sthread_mutex_t* mutex);

/**
 * @brief Lock a mutex (blocking)
 * 
 * @param mutex Pointer to mutex to lock
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_mutex_lock(sthread_mutex_t* mutex);

/**
 * @brief Try to lock a mutex (non-blocking)
 * 
 * @param mutex Pointer to mutex to lock
 * @return STHREAD_SUCCESS if lock acquired, STHREAD_ERROR_BUSY otherwise
 */
int sthread_mutex_trylock(sthread_mutex_t* mutex);

/**
 * @brief Unlock a mutex
 * 
 * @param mutex Pointer to mutex to unlock
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_mutex_unlock(sthread_mutex_t* mutex);

/* ============================================================================
 * Condition Variable Operations
 * ============================================================================ */

/**
 * @brief Initialize a condition variable
 * 
 * @param cond Pointer to condition variable to initialize
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_cond_init(sthread_cond_t* cond);

/**
 * @brief Destroy a condition variable
 * 
 * @param cond Pointer to condition variable to destroy
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_cond_destroy(sthread_cond_t* cond);

/**
 * @brief Wait on a condition variable
 * 
 * Atomically releases the mutex and waits on the condition variable.
 * 
 * @param cond Pointer to condition variable
 * @param mutex Pointer to associated mutex (must be locked)
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_cond_wait(sthread_cond_t* cond, sthread_mutex_t* mutex);

/**
 * @brief Wait on a condition variable with timeout
 * 
 * @param cond Pointer to condition variable
 * @param mutex Pointer to associated mutex (must be locked)
 * @param timeout_ms Timeout in milliseconds
 * @return STHREAD_SUCCESS on success, STHREAD_ERROR_TIMEOUT on timeout
 */
int sthread_cond_timedwait(sthread_cond_t* cond, sthread_mutex_t* mutex, 
                           unsigned int timeout_ms);

/**
 * @brief Signal one waiting thread
 * 
 * @param cond Pointer to condition variable
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_cond_signal(sthread_cond_t* cond);

/**
 * @brief Signal all waiting threads
 * 
 * @param cond Pointer to condition variable
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_cond_broadcast(sthread_cond_t* cond);

/* ============================================================================
 * Semaphore Operations
 * ============================================================================ */

/**
 * @brief Initialize a semaphore
 * 
 * @param sem Pointer to semaphore to initialize
 * @param value Initial value (count)
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_sem_init(sthread_sem_t* sem, unsigned int value);

/**
 * @brief Destroy a semaphore
 * 
 * @param sem Pointer to semaphore to destroy
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_sem_destroy(sthread_sem_t* sem);

/**
 * @brief Wait on a semaphore (decrement, blocking if zero)
 * 
 * @param sem Pointer to semaphore
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_sem_wait(sthread_sem_t* sem);

/**
 * @brief Try to wait on a semaphore (non-blocking)
 * 
 * @param sem Pointer to semaphore
 * @return STHREAD_SUCCESS if decremented, STHREAD_ERROR_BUSY if zero
 */
int sthread_sem_trywait(sthread_sem_t* sem);

/**
 * @brief Post to a semaphore (increment)
 * 
 * @param sem Pointer to semaphore
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_sem_post(sthread_sem_t* sem);

/**
 * @brief Get current semaphore value
 * 
 * @param sem Pointer to semaphore
 * @param value Pointer to store the value
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_sem_getvalue(sthread_sem_t* sem, int* value);

/* ============================================================================
 * Thread Pool Operations
 * ============================================================================ */

/**
 * @brief Create a thread pool with default configuration
 * 
 * Creates a thread pool with the specified number of worker threads.
 * Uses default queue capacity (1024) and stack size (64KB).
 * 
 * @param num_threads Number of worker threads (0 = auto-detect CPU cores)
 * @return Thread pool handle, or NULL on failure
 */
sthread_pool_t* sthread_pool_create(size_t num_threads);

/**
 * @brief Create a thread pool with custom configuration
 * 
 * @param config Configuration options
 * @return Thread pool handle, or NULL on failure
 */
sthread_pool_t* sthread_pool_create_with_config(const sthread_pool_config_t* config);

/**
 * @brief Destroy a thread pool
 * 
 * Waits for all currently executing tasks to complete, then destroys the pool.
 * Tasks still in the queue will be discarded.
 * 
 * @param pool Thread pool handle
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_pool_destroy(sthread_pool_t* pool);

/**
 * @brief Submit a task to the thread pool
 * 
 * @param pool Thread pool handle
 * @param func Task function to execute
 * @param arg Argument to pass to the task function
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_pool_submit(sthread_pool_t* pool, sthread_task_func_t func, void* arg);

/**
 * @brief Submit a task with priority
 * 
 * @param pool Thread pool handle
 * @param func Task function to execute
 * @param arg Argument to pass to the task function
 * @param priority Task priority level
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_pool_submit_priority(sthread_pool_t* pool, sthread_task_func_t func,
                                  void* arg, sthread_priority_t priority);

/**
 * @brief Wait for all submitted tasks to complete
 * 
 * Blocks until all tasks that have been submitted are finished.
 * New tasks can still be submitted during this wait.
 * 
 * @param pool Thread pool handle
 * @return STHREAD_SUCCESS on success, error code otherwise
 */
int sthread_pool_wait(sthread_pool_t* pool);

/**
 * @brief Get the number of pending tasks
 * 
 * @param pool Thread pool handle
 * @return Number of tasks waiting in the queue
 */
size_t sthread_pool_pending(sthread_pool_t* pool);

/**
 * @brief Get the number of active (running) tasks
 * 
 * @param pool Thread pool handle
 * @return Number of tasks currently being executed
 */
size_t sthread_pool_active(sthread_pool_t* pool);

/**
 * @brief Get the number of worker threads
 * 
 * @param pool Thread pool handle
 * @return Number of worker threads in the pool
 */
size_t sthread_pool_size(sthread_pool_t* pool);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Get number of CPU cores available
 * 
 * @return Number of CPU cores
 */
size_t sthread_get_num_cores(void);

/**
 * @brief Get error message for error code
 * 
 * @param error Error code
 * @return Human-readable error message
 */
const char* sthread_strerror(int error);

/**
 * @brief Get library version string
 * 
 * @return Version string (e.g., "1.0.0")
 */
const char* sthread_version(void);

#ifdef __cplusplus
}
#endif

#endif /* STHREAD_H */
