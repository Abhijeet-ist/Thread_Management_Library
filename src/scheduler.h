/**
 * @file scheduler.h
 * @brief Task scheduler interface
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Task Definition
 * ============================================================================ */

typedef void (*task_func_t)(void* arg);

typedef enum {
    TASK_PRIORITY_LOW = 0,
    TASK_PRIORITY_NORMAL = 1,
    TASK_PRIORITY_HIGH = 2,
    TASK_PRIORITY_CRITICAL = 3
} task_priority_t;

typedef struct task_node {
    task_func_t func;           /* Task function to execute */
    void* arg;                  /* Argument for the task */
    task_priority_t priority;   /* Task priority */
    uint64_t submit_time;       /* Time when task was submitted */
    struct task_node* next;     /* Next task in queue */
} task_node_t;

/* ============================================================================
 * Task Queue (Thread-Safe FIFO Queue)
 * ============================================================================ */

typedef struct {
    task_node_t* head;          /* Front of queue */
    task_node_t* tail;          /* Back of queue */
    size_t count;               /* Number of tasks in queue */
    size_t capacity;            /* Maximum capacity (0 = unbounded) */
    pthread_mutex_t lock;       /* Queue lock */
    pthread_cond_t not_empty;   /* Signaled when queue becomes non-empty */
    pthread_cond_t not_full;    /* Signaled when queue has space */
    bool shutdown;              /* Shutdown flag */
} task_queue_t;

/**
 * @brief Initialize task queue
 * 
 * @param queue Queue to initialize
 * @param capacity Maximum capacity (0 for unbounded)
 * @return 0 on success, -1 on failure
 */
int task_queue_init(task_queue_t* queue, size_t capacity);

/**
 * @brief Destroy task queue and free all pending tasks
 * 
 * @param queue Queue to destroy
 */
void task_queue_destroy(task_queue_t* queue);

/**
 * @brief Push a task to the queue
 * 
 * @param queue Queue to push to
 * @param func Task function
 * @param arg Task argument
 * @param priority Task priority
 * @return 0 on success, -1 on failure, -2 if queue is full (bounded)
 */
int task_queue_push(task_queue_t* queue, task_func_t func, void* arg, 
                    task_priority_t priority);

/**
 * @brief Pop a task from the queue (blocking)
 * 
 * Blocks until a task is available or shutdown is signaled.
 * 
 * @param queue Queue to pop from
 * @param out_func Output: task function
 * @param out_arg Output: task argument
 * @return 0 on success, -1 on shutdown
 */
int task_queue_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg);

/**
 * @brief Try to pop a task (non-blocking)
 * 
 * @param queue Queue to pop from
 * @param out_func Output: task function
 * @param out_arg Output: task argument
 * @return 0 on success, -1 if empty
 */
int task_queue_try_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg);

/**
 * @brief Get number of tasks in queue
 */
size_t task_queue_size(task_queue_t* queue);

/**
 * @brief Signal shutdown to all waiting threads
 */
void task_queue_shutdown(task_queue_t* queue);

#endif /* SCHEDULER_H */
