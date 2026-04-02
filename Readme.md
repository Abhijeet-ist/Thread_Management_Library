# Scalable Thread Management Library

A high-performance, cross-platform thread management library written in C that provides thread pooling, synchronization primitives, and efficient task scheduling.

## Overview

**sthread** is a user-space library that wraps POSIX threads and builds a higher-level, intelligent threading system. It provides:

- **Thread Pool**: Fixed pool of reusable worker threads
- **Task Queue**: Thread-safe FIFO queue for work distribution
- **Synchronization Primitives**: Mutex, semaphore, and condition variable
- **Cross-Platform**: Works on both macOS and Linux

## Features

| Feature | Description |
|---------|-------------|
| Thread Pool | Efficient worker thread pooling with configurable size |
| Task Queue | Bounded, thread-safe queue with backpressure support |
| Mutex | Mutual exclusion lock for critical sections |
| Semaphore | Counting semaphore (works on macOS and Linux) |
| Condition Variable | Wait/signal mechanism for thread coordination |
| Auto-Detection | Automatic CPU core detection for optimal thread count |

## Quick Start

### Building

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run tests
ctest --output-on-failure
```

### Basic Usage

```c
#include <sthread.h>

// Task function
void my_task(void* arg) {
    int* value = (int*)arg;
    printf("Processing: %d\n", *value);
}

int main(void) {
    // Create a thread pool with 4 workers
    sthread_pool_t* pool = sthread_pool_create(4);
    
    // Submit tasks
    int data[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        sthread_pool_submit(pool, my_task, &data[i]);
    }
    
    // Wait for all tasks to complete
    sthread_pool_wait(pool);
    
    // Clean up
    sthread_pool_destroy(pool);
    
    return 0;
}
```

## API Reference

### Thread Pool

| Function | Description |
|----------|-------------|
| `sthread_pool_create(n)` | Create pool with n workers (0 = auto-detect) |
| `sthread_pool_destroy(pool)` | Destroy pool (waits for current tasks) |
| `sthread_pool_submit(pool, func, arg)` | Submit a task |
| `sthread_pool_wait(pool)` | Wait for all tasks to complete |
| `sthread_pool_pending(pool)` | Get number of queued tasks |
| `sthread_pool_active(pool)` | Get number of running tasks |
| `sthread_pool_size(pool)` | Get number of worker threads |

### Mutex

```c
sthread_mutex_t mutex;
sthread_mutex_init(&mutex);
sthread_mutex_lock(&mutex);
// critical section
sthread_mutex_unlock(&mutex);
sthread_mutex_destroy(&mutex);
```

### Semaphore

```c
sthread_sem_t sem;
sthread_sem_init(&sem, 3);  // Initial count = 3
sthread_sem_wait(&sem);      // Decrement (block if 0)
sthread_sem_post(&sem);      // Increment
sthread_sem_destroy(&sem);
```

### Condition Variable

```c
sthread_mutex_t mutex;
sthread_cond_t cond;
int ready = 0;

// Waiter thread
sthread_mutex_lock(&mutex);
while (!ready) {
    sthread_cond_wait(&cond, &mutex);
}
sthread_mutex_unlock(&mutex);

// Signaler thread
sthread_mutex_lock(&mutex);
ready = 1;
sthread_cond_signal(&cond);
sthread_mutex_unlock(&mutex);
```

## Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `STHREAD_SUCCESS` | 0 | Operation succeeded |
| `STHREAD_ERROR_NOMEM` | -1 | Out of memory |
| `STHREAD_ERROR_INVALID` | -2 | Invalid argument |
| `STHREAD_ERROR_BUSY` | -3 | Resource busy |
| `STHREAD_ERROR_TIMEOUT` | -4 | Operation timed out |
| `STHREAD_ERROR_DEADLOCK` | -5 | Deadlock detected |
| `STHREAD_ERROR_SHUTDOWN` | -6 | Pool shutting down |
| `STHREAD_ERROR_SYSTEM` | -7 | System call failed |

## Performance

The thread pool provides significant performance benefits over creating a new thread for each task:

| Approach | 1000 Tasks | Memory Usage |
|----------|------------|--------------|
| Thread Pool (8 threads) | ~50ms | ~2 MB (constant) |
| Thread-per-task | ~500ms | ~8 GB (grows) |

Run the benchmark:
```bash
./build/bench_throughput
```

## Examples

### Hello Threads
Basic demonstration of all library features:
```bash
./build/hello_threads
```

### Matrix Multiplication
Parallel matrix multiplication showing speedup:
```bash
./build/matrix_multiply
```

## Testing

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run individual tests
./test_create    # Basic thread tests
./test_sync      # Synchronization tests
./test_pool      # Thread pool tests
./test_scale     # Scalability tests
```

### With Sanitizers

```bash
# Address Sanitizer (memory bugs)
cmake -DENABLE_ASAN=ON ..
make && ctest

# Thread Sanitizer (race conditions)
cmake -DENABLE_TSAN=ON ..
make && ctest
```

## Project Structure

```
sthread_lib/
├── include/
│   └── sthread.h          # Public API header
├── src/
│   ├── platform.h/c       # OS abstraction layer
│   ├── sync.h/c           # Synchronization primitives
│   ├── scheduler.h/c      # Task queue implementation
│   ├── thread_pool.h/c    # Thread pool implementation
│   ├── memory.h/c         # Memory management
│   └── sthread.c          # Public API implementation
├── tests/
│   ├── test_create.c      # Basic thread tests
│   ├── test_sync.c        # Sync primitive tests
│   ├── test_pool.c        # Thread pool tests
│   └── test_scale.c       # Stress tests
├── benchmarks/
│   └── bench_throughput.c # Performance benchmark
├── examples/
│   ├── hello_threads.c    # Simple demo
│   └── matrix_multiply.c  # Parallel computation demo
├── CMakeLists.txt         # Build configuration
└── README.md              # This file
```

## Platform Support

| Platform | Compiler | Status |
|----------|----------|--------|
| macOS | Clang | ✅ Fully supported |
| Linux | GCC | ✅ Fully supported |
| Linux | Clang | ✅ Fully supported |

### macOS Notes

- Uses `mach_absolute_time()` for high-resolution timing
- Semaphore implemented using mutex + condition variable (named semaphores not used)
- Tested on Apple Silicon and Intel

### Linux Notes

- Uses `clock_gettime()` for timing
- Links with `-lpthread`
- Tested on Ubuntu 20.04+, Debian 11+

## Design Decisions

### Why Thread Pool?

Creating threads is expensive (~10,000 CPU cycles). A thread pool amortizes this cost by reusing workers.

### Bounded Queue

The task queue has a configurable maximum capacity (default: 1024). This prevents memory exhaustion when tasks arrive faster than processing.

### Portable Semaphore

macOS doesn't support `sem_init()` for unnamed semaphores. We implement semaphores using mutex + condition variable, which works identically on both platforms.

### Clean Shutdown

`sthread_pool_destroy()` performs an orderly shutdown:
1. Sets shutdown flag
2. Wakes all waiting workers
3. Waits for workers to finish current task
4. Joins all threads
5. Frees resources

No tasks are interrupted mid-execution.

## Known Limitations

1. **No task cancellation**: Once submitted, tasks cannot be cancelled
2. **No priority scheduling**: Tasks execute in FIFO order (priority support is optional)
3. **No work stealing**:Each worker pulls from a shared queue

