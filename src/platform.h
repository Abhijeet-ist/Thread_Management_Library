/**
 * @file platform.h
 * @brief Platform abstraction layer for macOS/Linux compatibility
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * Platform Detection
 * ============================================================================ */

#if defined(__APPLE__) && defined(__MACH__)
    #define STHREAD_PLATFORM_MACOS 1
    #define STHREAD_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define STHREAD_PLATFORM_LINUX 1
    #define STHREAD_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

/* ============================================================================
 * Platform-Specific Includes
 * ============================================================================ */

#ifdef STHREAD_PLATFORM_MACOS
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <mach/mach_time.h>
#endif

#ifdef STHREAD_PLATFORM_LINUX
    #include <sys/sysinfo.h>
#endif

#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

/* ============================================================================
 * Platform Functions
 * ============================================================================ */

/**
 * @brief Get the number of CPU cores
 */
static inline size_t platform_get_num_cores(void) {
#ifdef STHREAD_PLATFORM_MACOS
    int count;
    size_t size = sizeof(count);
    if (sysctlbyname("hw.logicalcpu", &count, &size, NULL, 0) == 0) {
        return (size_t)count;
    }
    return 1;
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    return cores > 0 ? (size_t)cores : 1;
#endif
}

/**
 * @brief Get monotonic time in nanoseconds
 */
static inline uint64_t platform_get_time_ns(void) {
#ifdef STHREAD_PLATFORM_MACOS
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    return (mach_absolute_time() * timebase.numer) / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

/**
 * @brief Get current time for pthread_cond_timedwait
 */
static inline void platform_get_abstime(struct timespec* ts, unsigned int timeout_ms) {
#ifdef STHREAD_PLATFORM_MACOS
    // macOS uses CLOCK_REALTIME for pthread_cond_timedwait
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec + timeout_ms / 1000;
    ts->tv_nsec = tv.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
#else
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += timeout_ms / 1000;
    ts->tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
#endif
}

/**
 * @brief Get the system page size
 */
static inline size_t platform_get_page_size(void) {
    return (size_t)sysconf(_SC_PAGESIZE);
}

/**
 * @brief Allocate memory with guard page
 * 
 * Allocates size bytes plus a guard page at the bottom for stack overflow detection.
 * 
 * @param size Size of memory to allocate
 * @param out_total_size Output: total size including guard page
 * @return Pointer to usable memory (after guard page), NULL on failure
 */
static inline void* platform_alloc_with_guard(size_t size, size_t* out_total_size) {
    size_t page_size = platform_get_page_size();
    size_t total_size = size + page_size;
    
    // Allocate memory
    void* mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    
    // Set guard page as inaccessible
    if (mprotect(mem, page_size, PROT_NONE) != 0) {
        munmap(mem, total_size);
        return NULL;
    }
    
    if (out_total_size) {
        *out_total_size = total_size;
    }
    
    // Return pointer after guard page
    return (char*)mem + page_size;
}

/**
 * @brief Free memory allocated with guard page
 */
static inline void platform_free_with_guard(void* ptr, size_t total_size) {
    if (ptr) {
        size_t page_size = platform_get_page_size();
        void* real_ptr = (char*)ptr - page_size;
        munmap(real_ptr, total_size);
    }
}

/**
 * @brief Thread-safe memory barrier
 */
static inline void platform_memory_barrier(void) {
    __sync_synchronize();
}

#endif /* PLATFORM_H */
