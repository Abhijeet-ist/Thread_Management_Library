/**
 * @file memory.c
 * @brief Memory management implementation
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

int stack_alloc_create(stack_alloc_t* alloc, size_t size) {
    if (!alloc) {
        return -1;
    }
    
    memset(alloc, 0, sizeof(stack_alloc_t));
    
    if (size == 0) {
        size = STACK_DEFAULT_SIZE;
    } else if (size < STACK_MIN_SIZE) {
        size = STACK_MIN_SIZE;
    } else if (size > STACK_MAX_SIZE) {
        size = STACK_MAX_SIZE;
    }
    
    size_t page_size = platform_get_page_size();
    size = (size + page_size - 1) & ~(page_size - 1);
    
    size_t total_size;
    void* stack = platform_alloc_with_guard(size, &total_size);
    if (!stack) {
        return -1;
    }
    
    alloc->base = (char*)stack - page_size;
    alloc->stack_top = (char*)stack + size;
    alloc->stack_size = size;
    alloc->total_size = total_size;
    alloc->has_guard = true;
    
    return 0;
}

void stack_alloc_destroy(stack_alloc_t* alloc) {
    if (!alloc || !alloc->base) {
        return;
    }
    
    platform_free_with_guard(
        (char*)alloc->base + platform_get_page_size(),
        alloc->total_size
    );
    
    memset(alloc, 0, sizeof(stack_alloc_t));
}

typedef struct pool_node {
    struct pool_node* next;
} pool_node_t;

struct memory_pool {
    void* memory;
    size_t memory_size;
    size_t object_size;
    size_t total_count;
    pool_node_t* free_list;
    size_t allocated_count;
    pthread_mutex_t lock;
};

memory_pool_t* memory_pool_create(size_t object_size, size_t initial_count) {
    if (object_size == 0 || initial_count == 0) {
        return NULL;
    }
    
    if (object_size < sizeof(pool_node_t)) {
        object_size = sizeof(pool_node_t);
    }
    
    object_size = (object_size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    
    memory_pool_t* pool = malloc(sizeof(memory_pool_t));
    if (!pool) {
        return NULL;
    }
    
    size_t memory_size = object_size * initial_count;
    pool->memory = malloc(memory_size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }
    
    if (pthread_mutex_init(&pool->lock, NULL) != 0) {
        free(pool->memory);
        free(pool);
        return NULL;
    }
    
    pool->memory_size = memory_size;
    pool->object_size = object_size;
    pool->total_count = initial_count;
    pool->allocated_count = 0;
    
    pool->free_list = NULL;
    char* ptr = (char*)pool->memory;
    for (size_t i = 0; i < initial_count; i++) {
        pool_node_t* node = (pool_node_t*)ptr;
        node->next = pool->free_list;
        pool->free_list = node;
        ptr += object_size;
    }
    
    return pool;
}

void memory_pool_destroy(memory_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pthread_mutex_destroy(&pool->lock);
    free(pool->memory);
    free(pool);
}

void* memory_pool_alloc(memory_pool_t* pool) {
    if (!pool) {
        return NULL;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    if (!pool->free_list) {
        pthread_mutex_unlock(&pool->lock);
        return NULL;
    }
    
    pool_node_t* node = pool->free_list;
    pool->free_list = node->next;
    pool->allocated_count++;
    
    pthread_mutex_unlock(&pool->lock);
    
    return node;
}

void memory_pool_free(memory_pool_t* pool, void* ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    pthread_mutex_lock(&pool->lock);
    
    pool_node_t* node = (pool_node_t*)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
    pool->allocated_count--;
    
    pthread_mutex_unlock(&pool->lock);
}

size_t memory_pool_allocated(memory_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    
    pthread_mutex_lock(&pool->lock);
    size_t count = pool->allocated_count;
    pthread_mutex_unlock(&pool->lock);
    
    return count;
}

size_t memory_pool_available(memory_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    
    pthread_mutex_lock(&pool->lock);
    size_t count = pool->total_count - pool->allocated_count;
    pthread_mutex_unlock(&pool->lock);
    
    return count;
}
