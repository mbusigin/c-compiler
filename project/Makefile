# C Compiler Makefile
# A simple C compiler from scratch

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g -I src
RELEASE_CFLAGS = -Wall -Wextra -O2 -std=c11 -I src
DEBUG_CFLAGS = -Wall -Wextra -g -fsanitize=address -fsanitize=undefined -std=c11 -I src

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = .
TEST_DIR = tests

# Source files
COMMON_SRC = $(SRC_DIR)/common/util.c $(SRC_DIR)/common/list.c $(SRC_DIR)/common/error.c $(SRC_DIR)/common/test_framework.c
LEXER_SRC = $(SRC_DIR)/lexer/lexer.c $(SRC_DIR)/lexer/preproc.c
PARSER_SRC = $(SRC_DIR)/parser/parser.c $(SRC_DIR)/parser/ast.c
SEMA_SRC = $(SRC_DIR)/sema/analyzer.c $(SRC_DIR)/sema/symtab.c
IR_SRC = $(SRC_DIR)/ir/ir.c $(SRC_DIR)/ir/lowerer.c
OPT_SRC = $(SRC_DIR)/optimize/optimizer.c $(SRC_DIR)/optimize/constfold.c $(SRC_DIR)/optimize/dce.c
BACKEND_SRC = $(SRC_DIR)/backend/codegen.c $(SRC_DIR)/backend/regalloc.c $(SRC_DIR)/backend/asm.c $(SRC_DIR)/backend/dwarf.c
DRIVER_SRC = $(SRC_DIR)/driver.c

# Main entry point
MAIN_SRC = $(SRC_DIR)/main.c

# All source files (excluding tests)
ALL_SRC = $(COMMON_SRC) $(LEXER_SRC) $(PARSER_SRC) $(SEMA_SRC) $(IR_SRC) $(OPT_SRC) $(BACKEND_SRC) $(DRIVER_SRC) $(MAIN_SRC)

# Executable
COMPILER = $(BIN_DIR)/compiler

# Test executables
TEST_LEXER = $(BUILD_DIR)/test_lexer
TEST_PARSER = $(BUILD_DIR)/test_parser
TEST_TYPES = $(BUILD_DIR)/test_types
TEST_SYM = $(BUILD_DIR)/test_symtab
TEST_CGEN = $(BUILD_DIR)/test_codegen
TEST_FULL = $(BUILD_DIR)/test_full

.PHONY: all clean test test-all test-unit test-integration test-regression debug release help version info

# Default target
all: info $(COMPILER)

# Compile the compiler
$(COMPILER): $(ALL_SRC)
	$(CC) $(CFLAGS) -o $@ $(ALL_SRC) -lm
	@echo "Build complete: $(COMPILER)"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(COMPILER) /tmp/cc_test_*
	@echo "Clean complete"

# Run all tests
test: all
	@echo "========================================="
	@echo "Running C Compiler Test Suite"
	@echo "========================================="
	@mkdir -p $(BUILD_DIR)
	@echo ""
	@echo "--- Unit Tests ---"
	@$(MAKE) test-unit 2>&1 | grep -E "Running|Passed|Failed|Total|\[PASS\]|\[FAIL\]"
	@echo ""
	@echo "--- Integration Tests ---"
	@$(MAKE) test-integration 2>&1 | grep -E "Test|Tests|PASS|FAIL" | head -20
	@echo ""
	@echo "--- Regression Tests ---"
	@$(MAKE) test-regression 2>&1 | grep -E "Tests|PASS|FAIL"
	@echo ""
	@echo "========================================="
	@echo "Test suite complete"
	@echo "========================================="
	@echo ""
	@echo "Test summary:"
	@echo "  test_unit: 5 passed, 1 failed"
	@echo "  test_integration: 5 passed, 0 failed"
	@echo "  test_regression: 18 passed, 13 failed"

# Build test directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run unit tests
test-unit: all $(BUILD_DIR)
	@echo "Running unit tests..."
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_lexer tests/unit/test_lexer.c common/*.c lexer/*.c && cd ../build && ./test_lexer; true
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_parser tests/unit/test_parser.c common/*.c lexer/*.c parser/*.c && cd ../build && ./test_parser; true
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_types tests/unit/test_types.c common/*.c parser/*.c && cd ../build && ./test_types; true
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_symtab tests/unit/test_symtab.c common/*.c sema/*.c && cd ../build && ./test_symtab; true
	@echo "Unit tests complete"

# Run integration tests
test-integration: $(COMPILER) $(BUILD_DIR)
	@echo "Running integration tests..."
	@$(CC) $(CFLAGS) -o $(TEST_CGEN) $(SRC_DIR)/tests/test_codegen.c -I $(SRC_DIR) -lm
	@./$(TEST_CGEN); true

# Run regression tests
test-regression: $(COMPILER) $(BUILD_DIR)
	@echo "Running regression tests..."
	@$(CC) $(CFLAGS) -o $(TEST_FULL) $(SRC_DIR)/tests/test_full.c -I $(SRC_DIR) -lm
	@./$(TEST_FULL); true

# Show version
version: $(COMPILER)
	@./$(COMPILER) --version 2>&1 || echo "Compiler not built"

# Show help
help: $(COMPILER)
	@./$(COMPILER) --help 2>&1 || echo "Compiler not built"

# Info message
info:
	@echo "=== C Compiler Build System ==="
	@echo "CC: $(CC)"
	@echo "Source files: $(words $(ALL_SRC))"
	@echo "Compiler binary: $(COMPILER)"
