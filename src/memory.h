/**
 * @file memory.h
 * @brief Memory management interface
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "platform.h"
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * Stack Allocator
 * 
 * Provides custom stack allocation with guard pages for overflow detection.
 * ============================================================================ */

#define STACK_DEFAULT_SIZE     65536   /* 64KB default stack */
#define STACK_MIN_SIZE         4096    /* 4KB minimum */
#define STACK_MAX_SIZE         8388608 /* 8MB maximum */

/**
 * @brief Stack allocation handle
 */
typedef struct {
    void* base;             /* Base of allocated memory (including guard) */
    void* stack_top;        /* Top of stack (highest address for stack growth) */
    size_t stack_size;      /* Usable stack size */
    size_t total_size;      /* Total allocated size (including guard page) */
    bool has_guard;         /* Whether guard page is present */
} stack_alloc_t;

/**
 * @brief Allocate a stack with guard page
 * 
 * @param alloc Output allocation handle
 * @param size Stack size in bytes (0 = default)
 * @return 0 on success, -1 on failure
 */
int stack_alloc_create(stack_alloc_t* alloc, size_t size);

/**
 * @brief Free a stack allocation
 * 
 * @param alloc Allocation handle
 */
void stack_alloc_destroy(stack_alloc_t* alloc);

/* ============================================================================
 * Memory Pool (Optional - for task node allocation)
 * 
 * Simple slab allocator for fixed-size objects.
 * ============================================================================ */

typedef struct memory_pool memory_pool_t;

/**
 * @brief Create a memory pool for fixed-size objects
 * 
 * @param object_size Size of each object
 * @param initial_count Initial number of objects to preallocate
 * @return Memory pool handle, NULL on failure
 */
memory_pool_t* memory_pool_create(size_t object_size, size_t initial_count);

/**
 * @brief Destroy a memory pool
 */
void memory_pool_destroy(memory_pool_t* pool);

/**
 * @brief Allocate an object from the pool
 * 
 * @param pool Memory pool
 * @return Pointer to object, NULL if pool is exhausted
 */
void* memory_pool_alloc(memory_pool_t* pool);

/**
 * @brief Return an object to the pool
 * 
 * @param pool Memory pool
 * @param ptr Pointer to object
 */
void memory_pool_free(memory_pool_t* pool, void* ptr);

/**
 * @brief Get statistics
 */
size_t memory_pool_allocated(memory_pool_t* pool);
size_t memory_pool_available(memory_pool_t* pool);

#endif /* MEMORY_H */
