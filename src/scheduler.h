/**
 * @file scheduler.h
 * @brief Task scheduler interface
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "platform.h"
#include <stddef.h>
#include <stdbool.h>

typedef void (*task_func_t)(void* arg);

typedef enum {
    TASK_PRIORITY_LOW = 0,
    TASK_PRIORITY_NORMAL = 1,
    TASK_PRIORITY_HIGH = 2,
    TASK_PRIORITY_CRITICAL = 3
} task_priority_t;

typedef struct task_node {
    task_func_t func;
    void* arg;
    task_priority_t priority;
    uint64_t submit_time;
    struct task_node* next;
} task_node_t;

typedef struct {
    task_node_t* head;
    task_node_t* tail;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    bool shutdown;
} task_queue_t;

int task_queue_init(task_queue_t* queue, size_t capacity);
void task_queue_destroy(task_queue_t* queue);
int task_queue_push(task_queue_t* queue, task_func_t func, void* arg, 
                    task_priority_t priority);
int task_queue_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg);
int task_queue_try_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg);
size_t task_queue_size(task_queue_t* queue);
void task_queue_shutdown(task_queue_t* queue);

#endif /* SCHEDULER_H */
