/**
 * @file memory.h
 * @brief Memory management interface
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "platform.h"
#include <stddef.h>
#include <stdbool.h>

#define STACK_DEFAULT_SIZE     65536
#define STACK_MIN_SIZE         4096
#define STACK_MAX_SIZE         8388608

typedef struct {
    void* base;
    void* stack_top;
    size_t stack_size;
    size_t total_size;
    bool has_guard;
} stack_alloc_t;

int stack_alloc_create(stack_alloc_t* alloc, size_t size);
void stack_alloc_destroy(stack_alloc_t* alloc);

typedef struct memory_pool memory_pool_t;

memory_pool_t* memory_pool_create(size_t object_size, size_t initial_count);
void memory_pool_destroy(memory_pool_t* pool);
void* memory_pool_alloc(memory_pool_t* pool);
void memory_pool_free(memory_pool_t* pool, void* ptr);
size_t memory_pool_allocated(memory_pool_t* pool);
size_t memory_pool_available(memory_pool_t* pool);

#endif /* MEMORY_H */
