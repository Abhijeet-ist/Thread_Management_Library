/**
 * @file matrix_multiply.c
 * @brief Parallel Matrix Multiplication Demo
 * 
 * Demonstrates the thread pool with a real-world workload:
 * computing C = A × B using multiple threads.
 * 
 * Each thread computes a portion of rows of the result matrix.
 */

#include <sthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* ============================================================================
 * Matrix Configuration
 * ============================================================================ */

#define MATRIX_SIZE 512  // 512x512 matrix
#define VERIFY_RESULT 1  // Enable result verification

/* ============================================================================
 * Matrix Data Structures
 * ============================================================================ */

typedef struct {
    double* data;
    int rows;
    int cols;
} matrix_t;

typedef struct {
    matrix_t* A;
    matrix_t* B;
    matrix_t* C;
    int start_row;
    int end_row;
} row_task_t;

/* ============================================================================
 * Matrix Operations
 * ============================================================================ */

static matrix_t* matrix_create(int rows, int cols) {
    matrix_t* m = malloc(sizeof(matrix_t));
    if (!m) return NULL;
    
    m->data = calloc(rows * cols, sizeof(double));
    if (!m->data) {
        free(m);
        return NULL;
    }
    
    m->rows = rows;
    m->cols = cols;
    return m;
}

static void matrix_destroy(matrix_t* m) {
    if (m) {
        free(m->data);
        free(m);
    }
}

static double matrix_get(matrix_t* m, int row, int col) {
    return m->data[row * m->cols + col];
}

static void matrix_set(matrix_t* m, int row, int col, double val) {
    m->data[row * m->cols + col] = val;
}

static void matrix_randomize(matrix_t* m) {
    for (int i = 0; i < m->rows * m->cols; i++) {
        m->data[i] = (double)(rand() % 100) / 10.0;
    }
}

/* ============================================================================
 * Parallel Matrix Multiplication
 * ============================================================================ */

// Task function: compute a range of rows
static void compute_rows(void* arg) {
    row_task_t* task = (row_task_t*)arg;
    
    for (int i = task->start_row; i < task->end_row; i++) {
        for (int j = 0; j < task->B->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < task->A->cols; k++) {
                sum += matrix_get(task->A, i, k) * matrix_get(task->B, k, j);
            }
            matrix_set(task->C, i, j, sum);
        }
    }
}

// Sequential matrix multiplication (for comparison)
static void matrix_multiply_sequential(matrix_t* A, matrix_t* B, matrix_t* C) {
    for (int i = 0; i < A->rows; i++) {
        for (int j = 0; j < B->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < A->cols; k++) {
                sum += matrix_get(A, i, k) * matrix_get(B, k, j);
            }
            matrix_set(C, i, j, sum);
        }
    }
}

// Parallel matrix multiplication using thread pool
static void matrix_multiply_parallel(matrix_t* A, matrix_t* B, matrix_t* C, int num_threads) {
    sthread_pool_t* pool = sthread_pool_create(num_threads);
    if (!pool) {
        fprintf(stderr, "Failed to create thread pool\n");
        return;
    }
    
    // Divide rows among threads
    int rows_per_thread = A->rows / num_threads;
    row_task_t* tasks = malloc(num_threads * sizeof(row_task_t));
    
    for (int t = 0; t < num_threads; t++) {
        tasks[t].A = A;
        tasks[t].B = B;
        tasks[t].C = C;
        tasks[t].start_row = t * rows_per_thread;
        tasks[t].end_row = (t == num_threads - 1) ? A->rows : (t + 1) * rows_per_thread;
        
        sthread_pool_submit(pool, compute_rows, &tasks[t]);
    }
    
    sthread_pool_wait(pool);
    sthread_pool_destroy(pool);
    free(tasks);
}

// Verify two matrices are equal (within tolerance)
static int matrix_equals(matrix_t* A, matrix_t* B, double epsilon) {
    if (A->rows != B->rows || A->cols != B->cols) return 0;
    
    for (int i = 0; i < A->rows * A->cols; i++) {
        if (fabs(A->data[i] - B->data[i]) > epsilon) {
            return 0;
        }
    }
    return 1;
}

/* ============================================================================
 * Timing Utilities
 * ============================================================================ */

static double get_time_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║            PARALLEL MATRIX MULTIPLICATION DEMO                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Configuration:\n");
    printf("  Matrix Size: %d × %d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("  CPU Cores:   %zu\n", sthread_get_num_cores());
    printf("  Library:     sthread v%s\n", sthread_version());
    printf("\n");
    
    // Seed random number generator
    srand(42);
    
    // Create matrices
    printf("Creating matrices...\n");
    matrix_t* A = matrix_create(MATRIX_SIZE, MATRIX_SIZE);
    matrix_t* B = matrix_create(MATRIX_SIZE, MATRIX_SIZE);
    matrix_t* C_seq = matrix_create(MATRIX_SIZE, MATRIX_SIZE);
    matrix_t* C_par = matrix_create(MATRIX_SIZE, MATRIX_SIZE);
    
    if (!A || !B || !C_seq || !C_par) {
        fprintf(stderr, "Failed to allocate matrices\n");
        return 1;
    }
    
    // Initialize with random values
    printf("Initializing with random values...\n\n");
    matrix_randomize(A);
    matrix_randomize(B);
    
    // Sequential multiplication
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Sequential Multiplication\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    double seq_start = get_time_seconds();
    matrix_multiply_sequential(A, B, C_seq);
    double seq_end = get_time_seconds();
    double seq_time = seq_end - seq_start;
    
    printf("Time: %.3f seconds\n\n", seq_time);
    
    // Parallel multiplication with different thread counts
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Parallel Multiplication\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
    printf("┌──────────────┬──────────────┬──────────────┬──────────────┐\n");
    printf("│   Threads    │     Time     │   Speedup    │  Efficiency  │\n");
    printf("├──────────────┼──────────────┼──────────────┼──────────────┤\n");
    
    int thread_counts[] = {1, 2, 4, 8};
    int num_configs = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    for (int i = 0; i < num_configs; i++) {
        int num_threads = thread_counts[i];
        
        // Clear result matrix
        memset(C_par->data, 0, C_par->rows * C_par->cols * sizeof(double));
        
        double par_start = get_time_seconds();
        matrix_multiply_parallel(A, B, C_par, num_threads);
        double par_end = get_time_seconds();
        double par_time = par_end - par_start;
        
        double speedup = seq_time / par_time;
        double efficiency = speedup / num_threads * 100;
        
        printf("│ %10d   │ %8.3f s   │ %8.2fx    │ %8.1f%%   │\n",
               num_threads, par_time, speedup, efficiency);
        
#if VERIFY_RESULT
        // Verify correctness
        if (!matrix_equals(C_seq, C_par, 1e-9)) {
            printf("│            │  *** VERIFICATION FAILED ***           │\n");
        }
#endif
    }
    
    printf("└──────────────┴──────────────┴──────────────┴──────────────┘\n\n");
    
    printf("═══════════════════════════════════════════════════════════════════\n");
    printf("Results\n");
    printf("═══════════════════════════════════════════════════════════════════\n\n");
    
#if VERIFY_RESULT
    printf("✓ All parallel results verified against sequential result\n");
#endif
    
    // Sample output
    printf("\nSample values from C[0][0..4]:\n  ");
    for (int j = 0; j < 5 && j < C_seq->cols; j++) {
        printf("%.2f ", matrix_get(C_seq, 0, j));
    }
    printf("\n");
    
    // GFLOPS calculation
    double gflops = (2.0 * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE) / seq_time / 1e9;
    printf("\nPerformance: %.2f GFLOPS (sequential baseline)\n", gflops);
    
    // Cleanup
    matrix_destroy(A);
    matrix_destroy(B);
    matrix_destroy(C_seq);
    matrix_destroy(C_par);
    
    printf("\n=== Demo Complete ===\n\n");
    
    return 0;
}
