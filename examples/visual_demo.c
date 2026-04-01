/**
 * @file visual_demo.c
 * @brief Visual demonstration of the sthread library with colored output and ASCII art
 * 
 * Shows thread pool activity in real-time with animations and colors.
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <unistd.h>
#include <time.h>

/* ============================================================================
 * ANSI Color Codes
 * ============================================================================ */

#define RESET       "\033[0m"
#define BOLD        "\033[1m"
#define DIM         "\033[2m"

/* Foreground Colors */
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"

/* Bright Colors */
#define BRIGHT_RED     "\033[91m"
#define BRIGHT_GREEN   "\033[92m"
#define BRIGHT_YELLOW  "\033[93m"
#define BRIGHT_BLUE    "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN    "\033[96m"
#define BRIGHT_WHITE   "\033[97m"

/* Background Colors */
#define BG_BLACK    "\033[40m"
#define BG_RED      "\033[41m"
#define BG_GREEN    "\033[42m"
#define BG_YELLOW   "\033[43m"
#define BG_BLUE     "\033[44m"

/* ============================================================================
 * Unicode Symbols (works in most terminals)
 * ============================================================================ */

#define SYMBOL_CHECK    "✓"
#define SYMBOL_CROSS    "✗"
#define SYMBOL_ARROW    "➜"
#define SYMBOL_BULLET   "●"
#define SYMBOL_CIRCLE   "○"
#define SYMBOL_SQUARE   "■"
#define SYMBOL_DIAMOND  "◆"
#define SYMBOL_STAR     "★"
#define SYMBOL_GEAR     "⚙"
#define SYMBOL_THREAD   "┃"
#define SYMBOL_WORKER   "👷"
#define SYMBOL_TASK     "📋"
#define SYMBOL_ROCKET   "🚀"
#define SYMBOL_FIRE     "🔥"
#define SYMBOL_CLOCK    "⏱"

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

static void hide_cursor(void) {
    printf("\033[?25l");
}

static void show_cursor(void) {
    printf("\033[?25h");
}

static void sleep_ms(int ms) {
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000 };
    nanosleep(&ts, NULL);
}

/* ============================================================================
 * ASCII Art Banner
 * ============================================================================ */

static void print_banner(void) {
    printf("\n");
    printf(BRIGHT_CYAN);
    printf("  ╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                                       ║\n");
    printf("  ║   " BRIGHT_YELLOW "███████╗████████╗██╗  ██╗██████╗ ███████╗ █████╗ ██████╗ " BRIGHT_CYAN "        ║\n");
    printf("  ║   " BRIGHT_YELLOW "██╔════╝╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗██╔══██╗" BRIGHT_CYAN "        ║\n");
    printf("  ║   " BRIGHT_YELLOW "███████╗   ██║   ███████║██████╔╝█████╗  ███████║██║  ██║" BRIGHT_CYAN "        ║\n");
    printf("  ║   " BRIGHT_YELLOW "╚════██║   ██║   ██╔══██║██╔══██╗██╔══╝  ██╔══██║██║  ██║" BRIGHT_CYAN "        ║\n");
    printf("  ║   " BRIGHT_YELLOW "███████║   ██║   ██║  ██║██║  ██║███████╗██║  ██║██████╔╝" BRIGHT_CYAN "        ║\n");
    printf("  ║   " BRIGHT_YELLOW "╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ " BRIGHT_CYAN "        ║\n");
    printf("  ║                                                                       ║\n");
    printf("  ║          " WHITE "🧵  Scalable Thread Management Library  🧵" BRIGHT_CYAN "                ║\n");
    printf("  ║                                                                       ║\n");
    printf("  ╚═══════════════════════════════════════════════════════════════════════╝\n");
    printf(RESET "\n");
}

static void print_section(const char* title) {
    printf("\n");
    printf("%s%s  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓\n", BOLD, BRIGHT_MAGENTA);
    printf("  ▓  %s%s%s%s\n", RESET, BRIGHT_WHITE, title, RESET);
    printf("%s  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓%s\n", BRIGHT_MAGENTA, RESET);
    printf("\n");
}

/* ============================================================================
 * Progress Bar
 * ============================================================================ */

static void print_progress_bar(const char* label, int current, int total, const char* color) {
    int width = 40;
    int filled = (current * width) / total;
    int percent = (current * 100) / total;
    
    printf("  %s%-15s%s [", BOLD, label, RESET);
    printf("%s", color);
    
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            printf("█");
        } else if (i == filled) {
            printf("▓");
        } else {
            printf(DIM "░" RESET "%s", color);
        }
    }
    
    printf(RESET "] %s%3d%%%s\n", BOLD, percent, RESET);
}

/* ============================================================================
 * Thread Pool Visualization
 * ============================================================================ */

#define MAX_WORKERS 8
#define VISUAL_QUEUE_SIZE 20

typedef struct {
    atomic_int worker_states[MAX_WORKERS];  // 0=idle, 1=working
    atomic_int tasks_completed;
    atomic_int tasks_total;
    atomic_int queue_size;
    int num_workers;
} visual_state_t;

static visual_state_t g_state;

static void print_worker_status(void) {
    printf("  " BOLD "Workers:" RESET " ");
    
    for (int i = 0; i < g_state.num_workers; i++) {
        int state = atomic_load(&g_state.worker_states[i]);
        
        if (state == 1) {
            // Working - show animated spinner
            printf(BRIGHT_GREEN "⚡Worker %d " RESET, i);
        } else {
            // Idle
            printf(DIM "○ Worker %d " RESET, i);
        }
    }
    printf("\n");
}

static void print_queue_visual(void) {
    int queue = atomic_load(&g_state.queue_size);
    
    printf("  " BOLD "Queue:   " RESET "[");
    
    for (int i = 0; i < VISUAL_QUEUE_SIZE; i++) {
        if (i < queue && i < VISUAL_QUEUE_SIZE) {
            printf(BRIGHT_YELLOW "■" RESET);
        } else {
            printf(DIM "·" RESET);
        }
    }
    
    printf("] %d tasks waiting\n", queue);
}

static void print_stats(void) {
    int completed = atomic_load(&g_state.tasks_completed);
    int total = atomic_load(&g_state.tasks_total);
    
    printf("\n");
    print_progress_bar("Progress:", completed, total, BRIGHT_GREEN);
    printf("\n");
    printf("  " BRIGHT_CYAN "📊 Statistics:" RESET "\n");
    printf("     • Tasks Completed: " BRIGHT_GREEN "%d" RESET " / %d\n", completed, total);
    printf("     • Workers Active:  ");
    
    int active = 0;
    for (int i = 0; i < g_state.num_workers; i++) {
        if (atomic_load(&g_state.worker_states[i]) == 1) active++;
    }
    printf(BRIGHT_YELLOW "%d" RESET " / %d\n", active, g_state.num_workers);
}

/* ============================================================================
 * Visual Task Functions
 * ============================================================================ */

typedef struct {
    int task_id;
    int worker_id;
    int work_ms;
} visual_task_t;

static atomic_int worker_id_counter = 0;

static void visual_task(void* arg) {
    visual_task_t* task = (visual_task_t*)arg;
    
    // Assign a worker ID (simple round-robin)
    int worker = atomic_fetch_add(&worker_id_counter, 1) % g_state.num_workers;
    
    // Mark worker as busy
    atomic_store(&g_state.worker_states[worker], 1);
    
    // Decrease queue size
    atomic_fetch_sub(&g_state.queue_size, 1);
    
    // Simulate work
    sleep_ms(task->work_ms);
    
    // Mark worker as idle
    atomic_store(&g_state.worker_states[worker], 0);
    
    // Increment completed
    atomic_fetch_add(&g_state.tasks_completed, 1);
    
    free(task);
}

/* ============================================================================
 * Demo 1: Basic Thread Pool Visualization
 * ============================================================================ */

static void demo_basic_pool(void) {
    print_section("DEMO 1: Thread Pool in Action");
    
    printf("  " BRIGHT_CYAN "Creating thread pool with 4 workers...\n\n" RESET);
    sleep_ms(500);
    
    // Initialize state
    g_state.num_workers = 4;
    atomic_store(&g_state.tasks_completed, 0);
    atomic_store(&g_state.tasks_total, 20);
    atomic_store(&g_state.queue_size, 0);
    atomic_store(&worker_id_counter, 0);
    
    for (int i = 0; i < MAX_WORKERS; i++) {
        atomic_store(&g_state.worker_states[i], 0);
    }
    
    // Create pool
    sthread_pool_t* pool = sthread_pool_create(4);
    
    printf("  " BRIGHT_GREEN SYMBOL_CHECK " Pool created successfully!\n\n" RESET);
    
    // Show initial state
    printf(BOLD "  Initial State:\n" RESET);
    print_worker_status();
    print_queue_visual();
    printf("\n");
    sleep_ms(1000);
    
    // Submit tasks
    printf("  " BRIGHT_YELLOW "Submitting 20 tasks...\n\n" RESET);
    
    for (int i = 0; i < 20; i++) {
        visual_task_t* task = malloc(sizeof(visual_task_t));
        task->task_id = i;
        task->work_ms = 100 + (rand() % 200);  // 100-300ms
        
        atomic_fetch_add(&g_state.queue_size, 1);
        sthread_pool_submit(pool, visual_task, task);
        
        // Visual feedback
        printf("\r  Submitted task %2d... ", i + 1);
        fflush(stdout);
        sleep_ms(50);
    }
    printf(BRIGHT_GREEN SYMBOL_CHECK " Done!\n\n" RESET);
    
    // Monitor progress
    printf(BOLD "  Processing:\n" RESET);
    
    while (atomic_load(&g_state.tasks_completed) < 20) {
        printf("\033[4A");  // Move up 4 lines
        print_worker_status();
        print_queue_visual();
        print_stats();
        sleep_ms(100);
    }
    
    printf("\n  " BRIGHT_GREEN SYMBOL_CHECK " All tasks completed!\n" RESET);
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
}

/* ============================================================================
 * Demo 2: Mutex Visualization
 * ============================================================================ */

static sthread_mutex_t viz_mutex;
static atomic_int critical_section_holder = -1;
static atomic_int mutex_counter = 0;

static void* mutex_visual_thread(void* arg) {
    int id = *(int*)arg;
    
    for (int i = 0; i < 3; i++) {
        // Trying to acquire
        printf("  Thread %d: " YELLOW "Waiting for lock..." RESET "\n", id);
        
        sthread_mutex_lock(&viz_mutex);
        
        // In critical section
        atomic_store(&critical_section_holder, id);
        printf("  Thread %d: " BRIGHT_GREEN "🔒 LOCKED - In critical section" RESET "\n", id);
        
        atomic_fetch_add(&mutex_counter, 1);
        sleep_ms(200);
        
        atomic_store(&critical_section_holder, -1);
        sthread_mutex_unlock(&viz_mutex);
        
        printf("  Thread %d: " CYAN "🔓 Released lock" RESET "\n", id);
        sleep_ms(50);
    }
    
    return NULL;
}

static void demo_mutex(void) {
    print_section("DEMO 2: Mutex Synchronization");
    
    printf("  " BRIGHT_CYAN "Demonstrating mutex protecting a critical section...\n\n" RESET);
    sleep_ms(500);
    
    sthread_mutex_init(&viz_mutex);
    atomic_store(&mutex_counter, 0);
    
    printf("  " BOLD "Legend:\n" RESET);
    printf("     " YELLOW "● Waiting" RESET " = Thread waiting for lock\n");
    printf("     " BRIGHT_GREEN "● Locked" RESET "  = Thread in critical section\n");
    printf("     " CYAN "● Released" RESET " = Thread released lock\n\n");
    
    sthread_t threads[3];
    int ids[3] = {1, 2, 3};
    
    printf("  " BOLD "Activity:\n" RESET);
    
    for (int i = 0; i < 3; i++) {
        sthread_create(&threads[i], mutex_visual_thread, &ids[i]);
    }
    
    for (int i = 0; i < 3; i++) {
        sthread_join(threads[i], NULL);
    }
    
    sthread_mutex_destroy(&viz_mutex);
    
    printf("\n  " BRIGHT_GREEN SYMBOL_CHECK " Mutex demo complete! Counter = %d (expected 9)\n" RESET, 
           atomic_load(&mutex_counter));
}

/* ============================================================================
 * Demo 3: Producer-Consumer Visualization
 * ============================================================================ */

#define BUFFER_SIZE 5
static int buffer[BUFFER_SIZE];
static int buffer_count = 0;
static sthread_mutex_t pc_mutex;
static sthread_sem_t empty_slots;
static sthread_sem_t full_slots;

static void print_buffer(void) {
    printf("  Buffer: [");
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (i < buffer_count) {
            printf(BRIGHT_GREEN " %2d " RESET, buffer[i]);
        } else {
            printf(DIM " __ " RESET);
        }
    }
    printf("]\n");
}

static void* producer_visual(void* arg) {
    (void)arg;
    
    for (int i = 1; i <= 10; i++) {
        sthread_sem_wait(&empty_slots);
        sthread_mutex_lock(&pc_mutex);
        
        buffer[buffer_count] = i;
        buffer_count++;
        
        printf("  " BRIGHT_YELLOW "📦 Producer" RESET " added item %2d   ", i);
        print_buffer();
        
        sthread_mutex_unlock(&pc_mutex);
        sthread_sem_post(&full_slots);
        
        sleep_ms(150);
    }
    
    return NULL;
}

static void* consumer_visual(void* arg) {
    (void)arg;
    int sum = 0;
    
    for (int i = 0; i < 10; i++) {
        sthread_sem_wait(&full_slots);
        sthread_mutex_lock(&pc_mutex);
        
        buffer_count--;
        int item = buffer[buffer_count];
        sum += item;
        
        printf("  " BRIGHT_CYAN "🛒 Consumer" RESET " took item  %2d   ", item);
        print_buffer();
        
        sthread_mutex_unlock(&pc_mutex);
        sthread_sem_post(&empty_slots);
        
        sleep_ms(200);
    }
    
    printf("\n  " BRIGHT_GREEN SYMBOL_CHECK " Consumer finished! Sum = %d\n" RESET, sum);
    return NULL;
}

static void demo_producer_consumer(void) {
    print_section("DEMO 3: Producer-Consumer Pattern");
    
    printf("  " BRIGHT_CYAN "Demonstrating semaphore-based producer-consumer...\n\n" RESET);
    sleep_ms(500);
    
    sthread_mutex_init(&pc_mutex);
    sthread_sem_init(&empty_slots, BUFFER_SIZE);
    sthread_sem_init(&full_slots, 0);
    buffer_count = 0;
    
    printf("  " BOLD "Bounded Buffer (size %d):\n\n" RESET, BUFFER_SIZE);
    
    sthread_t producer, consumer;
    
    sthread_create(&producer, producer_visual, NULL);
    sthread_create(&consumer, consumer_visual, NULL);
    
    sthread_join(producer, NULL);
    sthread_join(consumer, NULL);
    
    sthread_sem_destroy(&empty_slots);
    sthread_sem_destroy(&full_slots);
    sthread_mutex_destroy(&pc_mutex);
}

/* ============================================================================
 * Demo 4: Performance Comparison
 * ============================================================================ */

static atomic_long perf_counter = 0;

static void perf_task(void* arg) {
    int iterations = *(int*)arg;
    volatile long sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += i;
    }
    atomic_fetch_add(&perf_counter, sum);
}

static void* perf_thread_wrapper(void* arg) {
    perf_task(arg);
    return NULL;
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void demo_performance(void) {
    print_section("DEMO 4: Performance Comparison");
    
    printf("  " BRIGHT_CYAN "Comparing Thread Pool vs Thread-per-Task approach...\n\n" RESET);
    sleep_ms(500);
    
    int num_tasks = 500;
    int iterations = 5000;
    
    printf("  " BOLD "Test Configuration:\n" RESET);
    printf("     • Number of tasks: %d\n", num_tasks);
    printf("     • Work per task:   %d iterations\n\n", iterations);
    
    // Thread Pool approach
    printf("  " BRIGHT_YELLOW "Running Thread Pool (8 workers)...\n" RESET);
    
    atomic_store(&perf_counter, 0);
    sthread_pool_t* pool = sthread_pool_create(8);
    
    double pool_start = get_time_ms();
    
    for (int i = 0; i < num_tasks; i++) {
        sthread_pool_submit(pool, perf_task, &iterations);
        
        if (i % 50 == 0) {
            print_progress_bar("Thread Pool:", i, num_tasks, BRIGHT_GREEN);
            printf("\033[1A");  // Move up
        }
    }
    sthread_pool_wait(pool);
    
    double pool_end = get_time_ms();
    double pool_time = pool_end - pool_start;
    
    print_progress_bar("Thread Pool:", num_tasks, num_tasks, BRIGHT_GREEN);
    sthread_pool_destroy(pool);
    
    printf("     " BRIGHT_GREEN SYMBOL_CHECK " Completed in %.2f ms\n\n" RESET, pool_time);
    
    // Naive approach (limited to fewer tasks)
    int naive_tasks = 100;  // Limit to avoid system overload
    printf("  " BRIGHT_YELLOW "Running Naive (1 thread per task, %d tasks)...\n" RESET, naive_tasks);
    
    atomic_store(&perf_counter, 0);
    sthread_t* threads = malloc(naive_tasks * sizeof(sthread_t));
    
    double naive_start = get_time_ms();
    
    for (int i = 0; i < naive_tasks; i++) {
        sthread_create(&threads[i], perf_thread_wrapper, &iterations);
        
        if (i % 10 == 0) {
            print_progress_bar("Naive:", i, naive_tasks, YELLOW);
            printf("\033[1A");
        }
    }
    
    for (int i = 0; i < naive_tasks; i++) {
        sthread_join(threads[i], NULL);
    }
    
    double naive_end = get_time_ms();
    double naive_time = naive_end - naive_start;
    
    print_progress_bar("Naive:", naive_tasks, naive_tasks, YELLOW);
    free(threads);
    
    printf("     " YELLOW SYMBOL_CHECK " Completed in %.2f ms\n\n" RESET, naive_time);
    
    // Results
    double pool_rate = num_tasks / (pool_time / 1000.0);
    double naive_rate = naive_tasks / (naive_time / 1000.0);
    double speedup = pool_rate / naive_rate;
    
    printf("  " BOLD "Results:\n" RESET);
    printf("  ┌────────────────────┬─────────────────┬─────────────────┐\n");
    printf("  │ " BOLD "Approach" RESET "           │ " BOLD "Tasks/Second" RESET "    │ " BOLD "Relative Speed" RESET "  │\n");
    printf("  ├────────────────────┼─────────────────┼─────────────────┤\n");
    printf("  │ Thread Pool        │ " BRIGHT_GREEN "%10.0f" RESET "      │ " BRIGHT_GREEN "%10.1fx" RESET "      │\n", pool_rate, speedup);
    printf("  │ Naive (per-task)   │ " YELLOW "%10.0f" RESET "      │ " YELLOW "%10.1fx" RESET "      │\n", naive_rate, 1.0);
    printf("  └────────────────────┴─────────────────┴─────────────────┘\n\n");
    
    printf("  " BRIGHT_GREEN "🚀 Thread Pool is %.1fx faster!\n" RESET, speedup);
}

/* ============================================================================
 * Demo 5: Thread Scaling Visualization
 * ============================================================================ */

static void demo_scaling(void) {
    print_section("DEMO 5: Thread Scaling Analysis");
    
    printf("  " BRIGHT_CYAN "Testing performance with different thread counts...\n\n" RESET);
    sleep_ms(500);
    
    int num_tasks = 2000;
    int iterations = 10000;
    int thread_counts[] = {1, 2, 4, 8};
    double times[4];
    
    printf("  " BOLD "Running tests (2000 tasks each):\n\n" RESET);
    
    for (int t = 0; t < 4; t++) {
        int threads = thread_counts[t];
        
        printf("  Testing %d thread(s)... ", threads);
        fflush(stdout);
        
        atomic_store(&perf_counter, 0);
        sthread_pool_t* pool = sthread_pool_create(threads);
        
        double start = get_time_ms();
        
        for (int i = 0; i < num_tasks; i++) {
            sthread_pool_submit(pool, perf_task, &iterations);
        }
        sthread_pool_wait(pool);
        
        double end = get_time_ms();
        times[t] = end - start;
        
        sthread_pool_destroy(pool);
        
        printf(BRIGHT_GREEN "%.1f ms\n" RESET, times[t]);
    }
    
    printf("\n  " BOLD "Scaling Chart:\n" RESET);
    printf("  ┌────────────┬────────────┬───────────────────────────────────────┐\n");
    printf("  │  Threads   │    Time    │  Performance Bar                      │\n");
    printf("  ├────────────┼────────────┼───────────────────────────────────────┤\n");
    
    double max_time = times[0];
    const char* colors[] = {RED, YELLOW, BRIGHT_GREEN, BRIGHT_CYAN};
    
    for (int t = 0; t < 4; t++) {
        int bar_len = (int)(30 * (max_time / times[t]));
        if (bar_len > 30) bar_len = 30;
        
        printf("  │     %d      │ %7.1f ms │ %s", thread_counts[t], times[t], colors[t]);
        for (int i = 0; i < bar_len; i++) printf("█");
        printf(RESET);
        for (int i = bar_len; i < 30; i++) printf(" ");
        printf(" │\n");
    }
    
    printf("  └────────────┴────────────┴───────────────────────────────────────┘\n\n");
    
    printf("  " BOLD "Speedup relative to 1 thread:\n" RESET);
    for (int t = 1; t < 4; t++) {
        double speedup = times[0] / times[t];
        printf("     %d threads: " BRIGHT_GREEN "%.2fx" RESET " speedup\n", thread_counts[t], speedup);
    }
}

/* ============================================================================
 * Main Menu
 * ============================================================================ */

static void print_menu(void) {
    printf("\n");
    printf(BOLD "  Select a demo to run:\n\n" RESET);
    printf("     " BRIGHT_CYAN "1" RESET " │ Thread Pool Visualization\n");
    printf("     " BRIGHT_CYAN "2" RESET " │ Mutex Synchronization\n");
    printf("     " BRIGHT_CYAN "3" RESET " │ Producer-Consumer Pattern\n");
    printf("     " BRIGHT_CYAN "4" RESET " │ Performance Comparison\n");
    printf("     " BRIGHT_CYAN "5" RESET " │ Thread Scaling Analysis\n");
    printf("     " BRIGHT_CYAN "A" RESET " │ Run All Demos\n");
    printf("     " BRIGHT_CYAN "Q" RESET " │ Quit\n");
    printf("\n  Enter choice: ");
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    srand(time(NULL));
    
    clear_screen();
    print_banner();
    
    printf("  " BOLD "System Information:\n" RESET);
    printf("     • Library Version: " BRIGHT_GREEN "%s" RESET "\n", sthread_version());
    printf("     • CPU Cores:       " BRIGHT_GREEN "%zu" RESET "\n", sthread_get_num_cores());
    printf("     • Platform:        " BRIGHT_GREEN 
#ifdef __APPLE__
           "macOS"
#else
           "Linux"
#endif
           RESET "\n");
    
    while (1) {
        print_menu();
        
        char choice;
        if (scanf(" %c", &choice) != 1) break;
        
        switch (choice) {
            case '1':
                demo_basic_pool();
                break;
            case '2':
                demo_mutex();
                break;
            case '3':
                demo_producer_consumer();
                break;
            case '4':
                demo_performance();
                break;
            case '5':
                demo_scaling();
                break;
            case 'A':
            case 'a':
                demo_basic_pool();
                sleep_ms(1000);
                demo_mutex();
                sleep_ms(1000);
                demo_producer_consumer();
                sleep_ms(1000);
                demo_performance();
                sleep_ms(1000);
                demo_scaling();
                break;
            case 'Q':
            case 'q':
                printf("\n  " BRIGHT_GREEN "Goodbye! 👋\n\n" RESET);
                return 0;
            default:
                printf("  " RED "Invalid choice. Try again.\n" RESET);
        }
    }
    
    return 0;
}
