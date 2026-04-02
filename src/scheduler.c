/**
 * @file scheduler.c
 * @brief Task scheduler implementation
 */

#include "scheduler.h"
#include <stdlib.h>
#include <string.h>

int task_queue_init(task_queue_t* queue, size_t capacity) {
    if (!queue) {
        return -1;
    }
    
    memset(queue, 0, sizeof(task_queue_t));
    queue->capacity = capacity;
    queue->shutdown = false;
    
    if (pthread_mutex_init(&queue->lock, NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->lock);
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->lock);
        return -1;
    }
    
    return 0;
}

void task_queue_destroy(task_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->lock);
    
    task_node_t* node = queue->head;
    while (node) {
        task_node_t* next = node->next;
        free(node);
        node = next;
    }
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    
    pthread_mutex_unlock(&queue->lock);
    
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->lock);
}

int task_queue_push(task_queue_t* queue, task_func_t func, void* arg,
                    task_priority_t priority) {
    if (!queue || !func) {
        return -1;
    }
    
    task_node_t* node = malloc(sizeof(task_node_t));
    if (!node) {
        return -1;
    }
    
    node->func = func;
    node->arg = arg;
    node->priority = priority;
    node->submit_time = platform_get_time_ns();
    node->next = NULL;
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        free(node);
        return -1;
    }
    
    while (queue->capacity > 0 && queue->count >= queue->capacity && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->lock);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        free(node);
        return -1;
    }
    
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->count++;
    
    pthread_cond_signal(&queue->not_empty);
    
    pthread_mutex_unlock(&queue->lock);
    
    return 0;
}

int task_queue_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg) {
    if (!queue || !out_func || !out_arg) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->lock);
    
    while (queue->count == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }
    
    if (queue->count == 0 && queue->shutdown) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    
    task_node_t* node = queue->head;
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    queue->count--;
    
    if (queue->capacity > 0) {
        pthread_cond_signal(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->lock);
    
    *out_func = node->func;
    *out_arg = node->arg;
    free(node);
    
    return 0;
}

int task_queue_try_pop(task_queue_t* queue, task_func_t* out_func, void** out_arg) {
    if (!queue || !out_func || !out_arg) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->lock);
    
    if (queue->count == 0) {
        pthread_mutex_unlock(&queue->lock);
        return -1;
    }
    
    task_node_t* node = queue->head;
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    queue->count--;
    
    if (queue->capacity > 0) {
        pthread_cond_signal(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->lock);
    
    *out_func = node->func;
    *out_arg = node->arg;
    free(node);
    
    return 0;
}

size_t task_queue_size(task_queue_t* queue) {
    if (!queue) {
        return 0;
    }
    
    pthread_mutex_lock(&queue->lock);
    size_t count = queue->count;
    pthread_mutex_unlock(&queue->lock);
    
    return count;
}

void task_queue_shutdown(task_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->lock);
    queue->shutdown = true;
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);
}
