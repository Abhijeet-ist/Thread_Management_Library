cmake_minimum_required(VERSION 3.10)
project(sthread C)

# ============================================================================
# Compiler Settings
# ============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Warning flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# Debug and release configurations
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Default to Release build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================================
# Platform Detection
# ============================================================================

if(APPLE)
    message(STATUS "Building for macOS")
    add_definitions(-DSTHREAD_PLATFORM_MACOS)
elseif(UNIX)
    message(STATUS "Building for Linux")
    add_definitions(-DSTHREAD_PLATFORM_LINUX)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# ============================================================================
# Find Dependencies
# ============================================================================

find_package(Threads REQUIRED)

# ============================================================================
# Library Sources
# ============================================================================

set(STHREAD_SOURCES
    src/platform.c
    src/sync.c
    src/scheduler.c
    src/thread_pool.c
    src/memory.c
    src/sthread.c
)

set(STHREAD_HEADERS
    include/sthread.h
    src/platform.h
    src/sync.h
    src/scheduler.h
    src/thread_pool.h
    src/memory.h
)

# ============================================================================
# Library Target
# ============================================================================

# Static library
add_library(sthread STATIC ${STHREAD_SOURCES})

target_include_directories(sthread
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(sthread PUBLIC Threads::Threads)

# ============================================================================
# Tests
# ============================================================================

enable_testing()

# Test executables
add_executable(test_create tests/test_create.c)
target_link_libraries(test_create sthread)
add_test(NAME test_create COMMAND test_create)

add_executable(test_sync tests/test_sync.c)
target_link_libraries(test_sync sthread)
add_test(NAME test_sync COMMAND test_sync)

add_executable(test_pool tests/test_pool.c)
target_link_libraries(test_pool sthread)
add_test(NAME test_pool COMMAND test_pool)

add_executable(test_scale tests/test_scale.c)
target_link_libraries(test_scale sthread)
add_test(NAME test_scale COMMAND test_scale)

# ============================================================================
# Benchmarks
# ============================================================================

add_executable(bench_throughput benchmarks/bench_throughput.c)
target_link_libraries(bench_throughput sthread)

# ============================================================================
# Examples
# ============================================================================

add_executable(hello_threads examples/hello_threads.c)
target_link_libraries(hello_threads sthread)

add_executable(matrix_multiply examples/matrix_multiply.c)
target_link_libraries(matrix_multiply sthread)

# ============================================================================
# Sanitizer Options (for debugging)
# ============================================================================

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)

if(ENABLE_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENABLE_TSAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

# ============================================================================
# Installation
# ============================================================================

install(TARGETS sthread
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES include/sthread.h
    DESTINATION include
)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "=== sthread Library Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "Thread Sanitizer: ${ENABLE_TSAN}")
message(STATUS "")
