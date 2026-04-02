/**
 * @file platform.h
 * @brief Platform abstraction layer
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__APPLE__) && defined(__MACH__)
    #define STHREAD_PLATFORM_MACOS 1
    #define STHREAD_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define STHREAD_PLATFORM_LINUX 1
    #define STHREAD_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif

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

static inline void platform_get_abstime(struct timespec* ts, unsigned int timeout_ms) {
#ifdef STHREAD_PLATFORM_MACOS
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

static inline size_t platform_get_page_size(void) {
    return (size_t)sysconf(_SC_PAGESIZE);
}

static inline void* platform_alloc_with_guard(size_t size, size_t* out_total_size) {
    size_t page_size = platform_get_page_size();
    size_t total_size = size + page_size;
    
    void* mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        return NULL;
    }
    
    if (mprotect(mem, page_size, PROT_NONE) != 0) {
        munmap(mem, total_size);
        return NULL;
    }
    
    if (out_total_size) {
        *out_total_size = total_size;
    }
    
    return (char*)mem + page_size;
}

static inline void platform_free_with_guard(void* ptr, size_t total_size) {
    if (ptr) {
        size_t page_size = platform_get_page_size();
        void* real_ptr = (char*)ptr - page_size;
        munmap(real_ptr, total_size);
    }
}

static inline void platform_memory_barrier(void) {
    __sync_synchronize();
}

#endif /* PLATFORM_H */
