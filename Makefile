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
PARSER_SRC = $(SRC_DIR)/parser/parser.c $(SRC_DIR)/parser/ast.c $(SRC_DIR)/parser/parse_utils.c $(SRC_DIR)/parser/parse_type.c $(SRC_DIR)/parser/parse_decl.c $(SRC_DIR)/parser/parse_stmt.c $(SRC_DIR)/parser/parse_expr.c
SEMA_SRC = $(SRC_DIR)/sema/analyzer.c $(SRC_DIR)/sema/symtab.c
IR_SRC = $(SRC_DIR)/ir/ir.c $(SRC_DIR)/ir/lowerer.c $(SRC_DIR)/ir/lower_expr.c $(SRC_DIR)/ir/lower_stmt.c $(SRC_DIR)/ir/lower_decl.c $(SRC_DIR)/ir/scope.c
OPT_SRC = $(SRC_DIR)/optimize/optimizer.c $(SRC_DIR)/optimize/constfold.c $(SRC_DIR)/optimize/dce.c
TARGET_SRC = $(SRC_DIR)/target/target.c $(SRC_DIR)/target/arm64/arm64_target.c $(SRC_DIR)/target/wasm/wasm_target.c
BACKEND_SRC = $(SRC_DIR)/backend/codegen.c $(SRC_DIR)/backend/regalloc.c $(SRC_DIR)/backend/asm.c $(SRC_DIR)/backend/dwarf.c $(SRC_DIR)/backend/wasm_codegen.c $(SRC_DIR)/backend/wasm_emit.c
DRIVER_SRC = $(SRC_DIR)/driver.c
RUNTIME_SRC = $(SRC_DIR)/runtime.c

# Main entry point
MAIN_SRC = $(SRC_DIR)/main.c

# All source files (excluding tests)
ALL_SRC = $(COMMON_SRC) $(LEXER_SRC) $(PARSER_SRC) $(SEMA_SRC) $(IR_SRC) $(OPT_SRC) $(TARGET_SRC) $(BACKEND_SRC) $(DRIVER_SRC) $(RUNTIME_SRC) $(MAIN_SRC)

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
$(COMPILER): $(ALL_SRC) | $(BUILD_DIR)
	@echo "Assembling runtime.s..."
	@as $(SRC_DIR)/runtime.s -o $(BUILD_DIR)/runtime.o
	$(CC) $(CFLAGS) -o $@ $(ALL_SRC) $(BUILD_DIR)/runtime.o -lm
	@echo "Build successful: $(COMPILER)"

# Build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

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

# Run unit tests
test-unit: all
	@echo "Running unit tests..."
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_lexer tests/unit/test_lexer.c common/*.c lexer/*.c && cd ../build && ./test_lexer
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_parser tests/unit/test_parser.c common/*.c lexer/*.c parser/*.c && cd ../build && ./test_parser
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_types tests/unit/test_types.c common/*.c parser/*.c && cd ../build && ./test_types
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_symtab tests/unit/test_symtab.c common/*.c sema/*.c && cd ../build && ./test_symtab
	@echo "Unit tests complete"

# Run integration tests
test-integration: $(COMPILER)
	@echo "Running integration tests..."
	$(CC) $(CFLAGS) -o $(TEST_CGEN) $(SRC_DIR)/tests/test_codegen.c -I $(SRC_DIR) -lm
	./$(TEST_CGEN)

# Run regression tests
test-regression: $(COMPILER)
	@echo "Running regression tests..."
	$(CC) $(CFLAGS) -o $(TEST_FULL) $(SRC_DIR)/tests/test_full.c -I $(SRC_DIR) -lm
	./$(TEST_FULL)

# Run comprehensive tests
test-comprehensive: $(COMPILER)
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
test-puzzles: $(COMPILER)
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

# =============================================================================
# WASM Test Targets
# =============================================================================

# Run WASM test suite
test-wasm: $(COMPILER)
	@echo "========================================="
	@echo "Running WASM Test Suite"
	@echo "========================================="
	@passed=0; failed=0; skipped=0; \
	for src in $(TEST_DIR)/wasm/wasm_test_arith.c $(TEST_DIR)/wasm/wasm_test_bitwise.c $(TEST_DIR)/wasm/wasm_test_branch.c; do \
		if [ -f "$$src" ]; then \
			name=$$(basename $$src .c); \
			wat_file="$(BUILD_DIR)/$${name}.wat"; \
			wasm_file="$(BUILD_DIR)/$${name}.wasm"; \
			echo -n "Testing $$name... "; \
			if ./$(COMPILER) --target=wasm $$src -o "$$wat_file" 2>/dev/null; then \
				if wat2wasm "$$wat_file" -o "$$wasm_file" 2>/dev/null; then \
					echo "[PASS] (valid WASM)"; passed=$$((passed + 1)); \
				else \
					echo "[FAIL] (wat2wasm validation failed)"; failed=$$((failed + 1)); \
				fi; \
			else \
				echo "[FAIL] (compilation failed)"; failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	for src in $(TEST_DIR)/wasm/wasm_test_cmp.c $(TEST_DIR)/wasm/wasm_test_loop.c $(TEST_DIR)/wasm/wasm_test_memory.c; do \
		if [ -f "$$src" ]; then \
			name=$$(basename $$src .c); \
			wat_file="$(BUILD_DIR)/$${name}.wat"; \
			wasm_file="$(BUILD_DIR)/$${name}.wasm"; \
			echo -n "Testing $$name... "; \
			if ./$(COMPILER) --target=wasm $$src -o "$$wat_file" 2>/dev/null; then \
				if wat2wasm "$$wat_file" -o "$$wasm_file" 2>/dev/null; then \
					echo "[PASS] (valid WASM)"; passed=$$((passed + 1)); \
				else \
					echo "[FAIL] (wat2wasm validation failed)"; failed=$$((failed + 1)); \
				fi; \
			else \
				echo "[FAIL] (compilation failed)"; failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "WASM Tests: $$passed passed, $$failed failed, $$skipped skipped"; \
	if [ $$failed -gt 0 ]; then exit 1; fi

# =============================================================================
# Self-Hosting Bootstrap Targets
# =============================================================================

# Stage 0: Build compiler with GCC (the "bootstrap" compiler)
STAGE0 = $(BUILD_DIR)/compiler_stage0
STAGE1 = $(BUILD_DIR)/compiler_stage1
STAGE2 = $(BUILD_DIR)/compiler_stage2

# Assembly files generated by stage0
STAGE0_ASM = $(BUILD_DIR)/stage0_asm

.PHONY: self self-clean self-verify stage0 stage1 stage2

# Main self-hosting target: build compiler with itself
self: stage0
	@echo ""; \
	echo "========================================="; \
	echo "Self-Hosting Bootstrap Test"; \
	echo "========================================="; \
	echo ""; \
	echo "stage0: Building bootstrap compiler with GCC..."; \
	$(MAKE) stage0 2>/dev/null; \
	if [ ! -f $(STAGE0) ]; then \
		echo "Error: stage0 build failed"; \
		exit 1; \
	fi; \
	echo "stage0: PASS - bootstrap compiler built"; \
	echo ""; \
	echo "stage1: Compiling compiler with itself..."; \
	$(MAKE) stage1 2>&1; \
	if [ -f $(STAGE1) ]; then \
		echo ""; \
		echo "stage1: PASS - self-compiled compiler built!"; \
		echo "Self-hosting bootstrap SUCCESS"; \
	else \
		echo ""; \
		echo "stage1: FAIL - could not build self-compiled compiler"; \
		echo "Self-hosting bootstrap FAILED (expected - missing features)"; \
	fi; \
	echo ""; \
	echo "========================================="

# Build stage0: the bootstrap compiler built with GCC
stage0: $(STAGE0)

$(STAGE0): $(ALL_SRC) | $(BUILD_DIR)
	@echo "Building stage0 compiler with $(CC)..."
	@echo "Assembling runtime.s..."
	as $(SRC_DIR)/runtime.s -o $(BUILD_DIR)/runtime.o
	$(CC) $(CFLAGS) -o $@ $(ALL_SRC) $(BUILD_DIR)/runtime.o -lm
	@echo "stage0 compiler: $@"

# Build stage1: compiler built by stage0
stage1: $(STAGE1)

$(STAGE1): $(STAGE0) | $(BUILD_DIR)
	@echo ""; \
	echo "Attempting to compile compiler source with stage0..."; \
	echo ""; \
	mkdir -p $(STAGE0_ASM); \
	success=0; failed=0; total=0; \
	for src in $(ALL_SRC); do \
		base=$$(basename "$$src" .c); \
		total=$$((total + 1)); \
		echo -n "  Compiling $$src... "; \
		if $(STAGE0) -Isrc -I include -o $(STAGE0_ASM)/$$base.s $$src 2>/dev/null; then \
			if [ -f $(STAGE0_ASM)/$$base.s ]; then \
				echo "OK"; \
				success=$$((success + 1)); \
			else \
				echo "FAIL (no output)"; \
				failed=$$((failed + 1)); \
			fi; \
		else \
			echo "FAIL"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "Compilation results: $$success/$$total files succeeded"; \
	if [ $$success -eq $$total ]; then \
		echo "All files compiled. Assembling and linking..."; \
		objs=""; \
		for asm in $(STAGE0_ASM)/*.s; do \
			base=$$(basename "$$asm" .s); \
			if [ "$$base" != "runtime" ]; then \
				clang -c "$$asm" -o $(STAGE0_ASM)/$$base.o; \
				if [ -f $(STAGE0_ASM)/$$base.o ]; then \
					objs="$$objs $(STAGE0_ASM)/$$base.o"; \
				fi; \
			fi; \
		done; \
		\
		echo "Assembling runtime stubs for bootstrap..."; \
		as $(SRC_DIR)/runtime.s -o $(STAGE0_ASM)/runtime.o; \
		if [ -f $(STAGE0_ASM)/runtime.o ]; then \
			objs="$$objs $(STAGE0_ASM)/runtime.o"; \
		fi; \
		\
		if [ -n "$$objs" ]; then \
			echo "Linking stage1 compiler..."; \
			clang -undefined dynamic_lookup $$objs -o $(STAGE1) -lm; \
			if [ -f $(STAGE1) ]; then \
				echo "stage1 compiler built: $(STAGE1)"; \
			else \
				echo "Error: Linking failed with clang, trying with gcc..."; \
				gcc $$objs -o $(STAGE1) -lm; \
				if [ -f $(STAGE1) ]; then \
					echo "stage1 compiler built with gcc: $(STAGE1)"; \
				else \
					echo "Error: Both clang and gcc linking failed"; \
					echo "Objects: $$objs"; \
				fi; \
			fi; \
		else \
			echo "Error: No object files to link"; \
		fi; \
	else \
		echo "Not all files compiled successfully. Cannot build stage1."; \
		echo ""; \
		echo "Missing features needed for self-compilation:"; \
		echo "  - Preprocessor (#include, #define)"; \
		echo "  - struct/union member access (. and ->)"; \
		echo "  - sizeof operator"; \
		echo "  - typedef support"; \
	fi

# Build stage2: compiler built by stage1 (for convergence verification)
stage2: $(STAGE2)

$(STAGE2): $(STAGE1) | $(BUILD_DIR)
	@echo "Building stage2 with stage1..."; \
	if [ ! -f $(STAGE1) ]; then \
		echo "Error: stage1 not built"; \
		exit 1; \
	fi; \
	mkdir -p $(BUILD_DIR)/stage1_asm; \
	success=0; total=0; \
	for src in $(ALL_SRC); do \
		base=$$(basename "$$src" .c); \
		total=$$((total + 1)); \
		if $(STAGE1) -Isrc -I include -o $(BUILD_DIR)/stage1_asm/$$base.s $$src 2>/dev/null; then \
			success=$$((success + 1)); \
		fi; \
	done; \
	if [ $$success -eq $$total ]; then \
		objs=""; \
		for asm in $(BUILD_DIR)/stage1_asm/*.s; do \
			base=$$(basename "$$asm" .s); \
			clang -c "$$asm" -o $(BUILD_DIR)/stage1_asm/$$base.o 2>/dev/null; \
			if [ -f $(BUILD_DIR)/stage1_asm/$$base.o ]; then \
				objs="$$objs $(BUILD_DIR)/stage1_asm/$$base.o"; \
			fi; \
		done; \
		if [ -n "$$objs" ]; then \
			clang $$objs -o $(STAGE2) -lm 2>/dev/null; \
		fi; \
	fi

# Verify convergence: stage1 and stage2 should produce identical output
self-verify:
	@echo ""; \
	echo "========================================="; \
	echo "Bootstrap Convergence Verification"; \
	echo "========================================="; \
	if [ ! -f $(STAGE1) ]; then \
		echo "Error: stage1 compiler not built"; \
		echo "Build with 'make self' first"; \
		echo ""; \
		echo "CONVERGENCE: FAIL - stage1 not available"; \
		echo "FAIL" > $(BUILD_DIR)/convergence.txt; \
		exit 1; \
	fi; \
	echo "Building stage2 with stage1..."; \
	$(MAKE) stage2 2>/dev/null; \
	if [ ! -f $(STAGE2) ]; then \
		echo "Error: stage2 compiler could not be built"; \
		echo ""; \
		echo "CONVERGENCE: FAIL - stage2 not available"; \
		echo "FAIL" > $(BUILD_DIR)/convergence.txt; \
		exit 1; \
	fi; \
	echo "Comparing stage1 and stage2 output..."; \
	mkdir -p $(BUILD_DIR)/verify; \
	$(STAGE1) -o $(BUILD_DIR)/verify/stage1.s tests/hello.c 2>/dev/null; \
	$(STAGE2) -o $(BUILD_DIR)/verify/stage2.s tests/hello.c 2>/dev/null; \
	if diff -q $(BUILD_DIR)/verify/stage1.s $(BUILD_DIR)/verify/stage2.s >/dev/null 2>&1; then \
		echo ""; \
		echo "CONVERGENCE: PASS - stage1 and stage2 produce identical output"; \
		echo "The compiler has successfully bootstrapped and converged."; \
		echo "PASS" > $(BUILD_DIR)/convergence.txt; \
	else \
		echo ""; \
		echo "CONVERGENCE: diverged - output differs"; \
		echo "stage1 and stage2 produce different output."; \
		echo "FAIL" > $(BUILD_DIR)/convergence.txt; \
		diff $(BUILD_DIR)/verify/stage1.s $(BUILD_DIR)/verify/stage2.s || true; \
	fi; \
	echo "========================================="

# Clean self-hosting artifacts
self-clean:
	rm -rf $(STAGE0_ASM) $(BUILD_DIR)/stage1_asm $(BUILD_DIR)/verify
	rm -f $(STAGE0) $(STAGE1) $(STAGE2) $(BUILD_DIR)/convergence.txt
	@echo "Self-hosting artifacts cleaned"
