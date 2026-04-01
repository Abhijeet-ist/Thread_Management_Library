# Scalable Thread Management Library - Makefile
# Compatible with macOS and Linux

# ============================================================================
# Platform Detection
# ============================================================================

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
    CC = clang
    PLATFORM = macos
    LDFLAGS = 
else
    CC = gcc
    PLATFORM = linux
    LDFLAGS = -lpthread
endif

# ============================================================================
# Compiler Flags
# ============================================================================

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CFLAGS += -I./include -I./src

# Debug build
ifdef DEBUG
    CFLAGS += -O0 -DDEBUG
endif

# Address Sanitizer
ifdef ASAN
    CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address
endif

# Thread Sanitizer
ifdef TSAN
    CFLAGS += -fsanitize=thread
    LDFLAGS += -fsanitize=thread
endif

# ============================================================================
# Directories
# ============================================================================

SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BENCH_DIR = benchmarks
EXAMPLE_DIR = examples
BUILD_DIR = build

# ============================================================================
# Source Files
# ============================================================================

LIB_SOURCES = \
    $(SRC_DIR)/platform.c \
    $(SRC_DIR)/sync.c \
    $(SRC_DIR)/scheduler.c \
    $(SRC_DIR)/thread_pool.c \
    $(SRC_DIR)/memory.c \
    $(SRC_DIR)/sthread.c

LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SOURCES))

# ============================================================================
# Targets
# ============================================================================

LIBRARY = $(BUILD_DIR)/libsthread.a

TESTS = \
    $(BUILD_DIR)/test_create \
    $(BUILD_DIR)/test_sync \
    $(BUILD_DIR)/test_pool \
    $(BUILD_DIR)/test_scale

BENCHMARKS = \
    $(BUILD_DIR)/bench_throughput

EXAMPLES = \
    $(BUILD_DIR)/hello_threads \
    $(BUILD_DIR)/matrix_multiply \
    $(BUILD_DIR)/visual_demo

# ============================================================================
# Default Target
# ============================================================================

.PHONY: all clean test benchmark examples help

all: $(BUILD_DIR) $(LIBRARY) $(TESTS) $(BENCHMARKS) $(EXAMPLES)
	@echo ""
	@echo "=== Build Complete ==="
	@echo "Platform: $(PLATFORM)"
	@echo "Library:  $(LIBRARY)"
	@echo ""
	@echo "Run tests:     make test"
	@echo "Run benchmark: make benchmark"
	@echo "Run examples:  make examples"
	@echo ""

# ============================================================================
# Build Directory
# ============================================================================

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# ============================================================================
# Library
# ============================================================================

$(LIBRARY): $(LIB_OBJECTS)
	@echo "Creating static library: $@"
	@ar rcs $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# Tests
# ============================================================================

$(BUILD_DIR)/test_create: $(TEST_DIR)/test_create.c $(LIBRARY)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

$(BUILD_DIR)/test_sync: $(TEST_DIR)/test_sync.c $(LIBRARY)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

$(BUILD_DIR)/test_pool: $(TEST_DIR)/test_pool.c $(LIBRARY)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

$(BUILD_DIR)/test_scale: $(TEST_DIR)/test_scale.c $(LIBRARY)
	@echo "Building test: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

# ============================================================================
# Benchmarks
# ============================================================================

$(BUILD_DIR)/bench_throughput: $(BENCH_DIR)/bench_throughput.c $(LIBRARY)
	@echo "Building benchmark: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

# ============================================================================
# Examples
# ============================================================================

$(BUILD_DIR)/hello_threads: $(EXAMPLE_DIR)/hello_threads.c $(LIBRARY)
	@echo "Building example: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

$(BUILD_DIR)/matrix_multiply: $(EXAMPLE_DIR)/matrix_multiply.c $(LIBRARY)
	@echo "Building example: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -lm -o $@

$(BUILD_DIR)/visual_demo: $(EXAMPLE_DIR)/visual_demo.c $(LIBRARY)
	@echo "Building example: $@"
	@$(CC) $(CFLAGS) $< -L$(BUILD_DIR) -lsthread $(LDFLAGS) -o $@

# ============================================================================
# Run Targets
# ============================================================================

test: $(TESTS)
	@echo ""
	@echo "=== Running Tests ==="
	@echo ""
	@for test in $(TESTS); do \
		echo "Running: $$test"; \
		$$test || exit 1; \
	done
	@echo ""
	@echo "=== All Tests Passed ==="
	@echo ""

benchmark: $(BENCHMARKS)
	@echo ""
	@echo "=== Running Benchmarks ==="
	@echo ""
	@$(BUILD_DIR)/bench_throughput

examples: $(EXAMPLES)
	@echo ""
	@echo "=== Running Examples ==="
	@echo ""
	@echo "--- Hello Threads ---"
	@$(BUILD_DIR)/hello_threads
	@echo ""
	@echo "--- Matrix Multiply ---"
	@$(BUILD_DIR)/matrix_multiply

# ============================================================================
# Clean
# ============================================================================

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Done."

# ============================================================================
# Help
# ============================================================================

help:
	@echo ""
	@echo "Scalable Thread Management Library (sthread)"
	@echo "============================================"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build library, tests, benchmarks, and examples (default)"
	@echo "  test       - Build and run all tests"
	@echo "  benchmark  - Build and run benchmarks"
	@echo "  examples   - Build and run examples"
	@echo "  clean      - Remove build directory"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build Options:"
	@echo "  make DEBUG=1  - Build with debug flags"
	@echo "  make ASAN=1   - Build with Address Sanitizer"
	@echo "  make TSAN=1   - Build with Thread Sanitizer"
	@echo ""
	@echo "Examples:"
	@echo "  make                  # Build everything"
	@echo "  make test             # Run tests"
	@echo "  make DEBUG=1 test     # Debug build and test"
	@echo "  make ASAN=1 test      # Test with Address Sanitizer"
	@echo ""
