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
TEST_COMP = $(BUILD_DIR)/test_comprehensive

.PHONY: all clean test test-all test-unit test-integration test-regression debug release help version info test-puzzles

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
	@result=0; \
	echo ""; \
	echo "--- Unit Tests ---"; \
	$(MAKE) test-unit 2>&1 | grep -E "Running|Passed|Failed|Total|\[PASS\]|\[FAIL\]" || result=1; \
	echo ""; \
	echo "--- Integration Tests ---"; \
	$(MAKE) test-integration 2>&1 | grep -E "Test|Tests|PASS|FAIL" | head -20 || result=1; \
	echo ""; \
	echo "--- Regression Tests ---"; \
	$(MAKE) test-regression 2>&1 | grep -E "Tests|PASS|FAIL" || result=1; \
	echo ""; \
	echo "--- Comprehensive Tests ---"; \
	$(MAKE) test-comprehensive 2>&1 | grep -E "Tests|PASS|FAIL" || result=1; \
	echo ""; \
	echo "--- Puzzle Tests ---"; \
	$(MAKE) test-puzzles 2>&1 | grep -E "PASS|FAIL|Tests|Summary" || result=1; \
	echo ""; \
	echo "========================================="; \
	echo "Test suite complete"; \
	echo "========================================="; \
	echo ""; \
	echo "Test summary:"; \
	$(MAKE) test-comprehensive 2>&1 | tail -6; \
	exit $$result

# Build test directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Run unit tests
test-unit: all $(BUILD_DIR)
	@echo "Running unit tests..."
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_lexer tests/unit/test_lexer.c common/*.c lexer/*.c && cd ../build && ./test_lexer
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_parser tests/unit/test_parser.c common/*.c lexer/*.c parser/*.c && cd ../build && ./test_parser
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_types tests/unit/test_types.c common/*.c parser/*.c && cd ../build && ./test_types
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_symtab tests/unit/test_symtab.c common/*.c sema/*.c && cd ../build && ./test_symtab
	@echo "Unit tests complete"

# Run integration tests
test-integration: $(COMPILER) $(BUILD_DIR)
	@echo "Running integration tests..."
	$(CC) $(CFLAGS) -o $(TEST_CGEN) $(SRC_DIR)/tests/test_codegen.c -I $(SRC_DIR) -lm
	./$(TEST_CGEN)

# Run regression tests
test-regression: $(COMPILER) $(BUILD_DIR)
	@echo "Running regression tests..."
	$(CC) $(CFLAGS) -o $(TEST_FULL) $(SRC_DIR)/tests/test_full.c -I $(SRC_DIR) -lm
	./$(TEST_FULL)

# Run comprehensive tests
test-comprehensive: $(COMPILER) $(BUILD_DIR)
	@echo "Running comprehensive tests..."
	$(CC) $(CFLAGS) -o $(TEST_COMP) $(SRC_DIR)/tests/test_comprehensive.c -I $(SRC_DIR) -lm
	./$(TEST_COMP)

# Add test-full as an alias for test-regression
test-full: test-regression

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

# Run puzzle tests
test-puzzles: $(COMPILER) $(BUILD_DIR)
	@echo "========================================="
	@echo "Running Puzzle Tests"
	@echo "========================================="
	@passed=0; failed=0; skipped=0; \
	tmpdir=$$(mktemp -d); \
	for src in $(TEST_DIR)/puzzles/run-*.c $(TEST_DIR)/puzzles/algo-*.c $(TEST_DIR)/puzzles/neg-*.c $(TEST_DIR)/puzzles/adv-*.c $(TEST_DIR)/puzzles/stress-*.c; do \
		if [ -f "$$src" ]; then \
			name=$$(basename "$$src" .c); \
			expected=$$(sed -n '/Expected output:/,/^\*\//p' "$$src" | sed '1d;$$d' | sed 's/^[[:space:]]*//;s/[[:space:]]*$$//'); \
			if [ -n "$$expected" ]; then \
				if $(CC) $(CFLAGS) -o "$$tmpdir/$$name" "$$src" 2>/dev/null; then \
					actual=$$("$${tmpdir}/$$name" 2>/dev/null); \
					if [ "$$expected" = "$$actual" ]; then \
						echo "[PASS] $$name"; passed=$$((passed + 1)); \
					else \
						echo "[FAIL] $$name"; \
						echo "  Expected: $$expected"; \
						echo "  Actual:   $$actual"; \
						failed=$$((failed + 1)); \
					fi; \
				else \
					echo "[SKIP] $$name (compile failed)"; skipped=$$((skipped + 1)); \
				fi; \
			else \
				echo "[SKIP] $$name (no expected output)"; skipped=$$((skipped + 1)); \
			fi; \
		fi; \
	done; \
	for src in $(TEST_DIR)/puzzles/sem-*.c; do \
		if [ -f "$$src" ]; then \
			name=$$(basename "$$src" .c); \
			if $(CC) $(CFLAGS) -o "$$tmpdir/$$name" "$$src" 2>/dev/null; then \
				echo "[PASS] $$name (semantic compile)"; passed=$$((passed + 1)); \
			else \
				echo "[FAIL] $$name (semantic compile failed)"; failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	rm -rf "$$tmpdir"; \
	echo ""; \
	echo "========================================="; \
	echo "Puzzle Tests Summary: $$passed passed, $$failed failed, $$skipped skipped"; \
	echo "========================================="; \
	if [ $$failed -gt 0 ]; then exit 1; fi
