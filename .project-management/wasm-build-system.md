# WASM Build System Specification

## Overview

This document describes the Makefile modifications and build system changes needed to support WASM backend compilation and testing.

## Makefile Modifications

### Source File Additions

```makefile
# Add WASM backend sources to BACKEND_SRC
BACKEND_SRC = $(SRC_DIR)/backend/codegen.c \
              $(SRC_DIR)/backend/regalloc.c \
              $(SRC_DIR)/backend/asm.c \
              $(SRC_DIR)/backend/dwarf.c \
              $(SRC_DIR)/backend/wasm_codegen.c \
              $(SRC_DIR)/backend/wasm_func.c \
              $(SRC_DIR)/backend/wasm_instr.c \
              $(SRC_DIR)/backend/wasm_emit.c
```

### New Build Variables

```makefile
# WASM tools
WAT2WASM = wat2wasm
WASM2WAT = wasm2wat
WASMTIME = wasmtime
WASM_VALIDATE = wasm-validate

# WASM build directory
WASM_BUILD_DIR = $(BUILD_DIR)/wasm
```

### New Targets

```makefile
.PHONY: wasm wasm-clean test-wasm test-wasm-unit test-wasm-integration

# Build WASM examples
wasm: $(COMPILER) $(WASM_BUILD_DIR)
	@echo "Building WASM examples..."
	@for src in tests/wasm/*.c; do \
		name=$$(basename $$src .c); \
		echo "  Compiling $$src..."; \
		./$(COMPILER) --target=wasm $$src -o $(WASM_BUILD_DIR)/$$name.wat || true; \
	done
	@echo "WASM build complete"

# Create WASM build directory
$(WASM_BUILD_DIR):
	mkdir -p $(WASM_BUILD_DIR)

# Clean WASM artifacts
wasm-clean:
	rm -rf $(WASM_BUILD_DIR)
	@echo "WASM artifacts cleaned"
```

## Test Targets

### Main WASM Test Target

```makefile
# Run all WASM tests
test-wasm: $(COMPILER) $(WASM_BUILD_DIR)
	@echo "========================================="
	@echo "Running WASM Test Suite"
	@echo "========================================="
	@result=0; \
	echo ""; \
	echo "--- Unit Tests ---"; \
	$(MAKE) test-wasm-unit || result=1; \
	echo ""; \
	echo "--- Integration Tests ---"; \
	$(MAKE) test-wasm-integration || result=1; \
	echo ""; \
	echo "========================================="; \
	if [ $$result -eq 0 ]; then \
		echo "WASM Test Suite: PASS"; \
	else \
		echo "WASM Test Suite: FAIL"; \
	fi; \
	echo "========================================="; \
	exit $$result
```

### Unit Test Target

```makefile
# Run WASM unit tests
test-wasm-unit: $(COMPILER) $(BUILD_DIR)
	@echo "Running WASM unit tests..."
	@cd $(SRC_DIR) && $(CC) $(CFLAGS) -I . -o ../build/test_wasm \
		tests/test_wasm.c \
		common/*.c \
		lexer/*.c \
		parser/*.c \
		sema/*.c \
		ir/*.c \
		optimize/*.c \
		backend/wasm_*.c \
		-lm && cd ../build && ./test_wasm
	@echo "WASM unit tests complete"
```

### Integration Test Target

```makefile
# Run WASM integration tests
test-wasm-integration: $(COMPILER) $(WASM_BUILD_DIR)
	@echo "Running WASM integration tests..."
	@passed=0; failed=0; skipped=0; \
	for src in tests/wasm/test_*.c; do \
		if [ -f "$$src" ]; then \
			name=$$(basename $$src .c); \
			expected_file="tests/wasm/$$name.expected"; \
			echo -n "  Testing $$name... "; \
			\
			if ./$(COMPILER) --target=wasm $$src -o $(WASM_BUILD_DIR)/$$name.wat 2>/dev/null; then \
				if $(WAT2WASM) $(WASM_BUILD_DIR)/$$name.wat -o $(WASM_BUILD_DIR)/$$name.wasm 2>/dev/null; then \
					if [ -f "$$expected_file" ]; then \
						expected=$$(cat "$$expected_file"); \
						actual=$$($(WASMTIME) $(WASM_BUILD_DIR)/$$name.wasm --invoke main 2>&1); \
						if [ "$$expected" = "$$actual" ]; then \
							echo "[PASS]"; passed=$$((passed + 1)); \
						else \
							echo "[FAIL] (expected '$$expected', got '$$actual')"; failed=$$((failed + 1)); \
						fi; \
					else \
						echo "[SKIP] (no expected file)"; skipped=$$((skipped + 1)); \
					fi; \
				else \
					echo "[FAIL] (wat2wasm failed)"; failed=$$((failed + 1)); \
				fi; \
			else \
				echo "[FAIL] (compilation failed)"; failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "Integration Tests: $$passed passed, $$failed failed, $$skipped skipped"; \
	if [ $$failed -gt 0 ]; then exit 1; fi
```

### Validation Test Target

```makefile
# Validate WASM output
test-wasm-validate: $(WASM_BUILD_DIR)
	@echo "Validating WASM binaries..."
	@failed=0; \
	for wasm in $(WASM_BUILD_DIR)/*.wasm; do \
		if [ -f "$$wasm" ]; then \
			echo -n "  Validating $$(basename $$wasm)... "; \
			if $(WASM_VALIDATE) "$$wasm" 2>/dev/null; then \
				echo "[OK]"; \
			else \
				echo "[FAIL]"; failed=$$((failed + 1)); \
			fi; \
		fi; \
	done; \
	if [ $$failed -gt 0 ]; then exit 1; fi
```

## Tool Detection

### Check for Required Tools

```makefile
# Check for WASM tools
check-wasm-tools:
	@echo "Checking for WASM tools..."
	@which $(WAT2WASM) >/dev/null 2>&1 || (echo "Error: wat2wasm not found. Install WABT."; exit 1)
	@which $(WASM2WAT) >/dev/null 2>&1 || (echo "Error: wasm2wat not found. Install WABT."; exit 1)
	@which $(WASMTIME) >/dev/null 2>&1 || (echo "Warning: wasmtime not found. Runtime tests will be skipped.")
	@which $(WASM_VALIDATE) >/dev/null 2>&1 || (echo "Warning: wasm-validate not found. Validation will be skipped.")
	@echo "WASM tools check complete"
```

### Conditional Test Execution

```makefile
# Run tests only if tools are available
test-wasm-if-available: check-wasm-tools test-wasm
```

## CI/CD Integration

### GitHub Actions Workflow

```yaml
# .github/workflows/wasm-tests.yml
name: WASM Tests

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  wasm-test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y wabt
    
    - name: Install Wasmtime
      run: curl https://wasmtime.dev/install.sh -sSf | bash
    
    - name: Add Wasmtime to PATH
      run: echo "$HOME/.wasmtime/bin" >> $GITHUB_PATH
    
    - name: Build compiler
      run: make
    
    - name: Check WASM tools
      run: make check-wasm-tools
    
    - name: Run WASM tests
      run: make test-wasm
    
    - name: Validate WASM binaries
      run: make test-wasm-validate
```

### Makefile Help Target Update

```makefile
# Update help target
help: $(COMPILER)
	@echo "C Compiler Build System"
	@echo ""
	@echo "Build Targets:"
	@echo "  all              Build the compiler (default)"
	@echo "  debug            Build debug version"
	@echo "  release          Build optimized version"
	@echo "  wasm             Build WASM examples"
	@echo ""
	@echo "Test Targets:"
	@echo "  test             Run all tests"
	@echo "  test-wasm        Run WASM test suite"
	@echo "  test-wasm-unit   Run WASM unit tests"
	@echo "  test-wasm-integration  Run WASM integration tests"
	@echo "  test-wasm-validate     Validate WASM binaries"
	@echo ""
	@echo "Clean Targets:"
	@echo "  clean            Clean all build artifacts"
	@echo "  wasm-clean       Clean WASM artifacts"
	@echo ""
	@echo "Other:"
	@echo "  check-wasm-tools     Check for required WASM tools"
	@echo "  help             Show this help"
	@echo "  version          Show version"
```

## Build Dependencies

### Tool Requirements

| Tool | Purpose | Required For |
|------|---------|--------------|
| wat2wasm | WAT to WASM conversion | Integration tests |
| wasm2wat | WASM to WAT decompilation | Debugging |
| wasmtime | WASM runtime | Integration tests |
| wasm-validate | Binary validation | Validation tests |

### Installation Instructions

```bash
# Install WABT (wat2wasm, wasm2wat, wasm-validate)
# macOS
brew install wabt

# Ubuntu/Debian
sudo apt-get install wabt

# Install Wasmtime
curl https://wasmtime.dev/install.sh -sSf | bash
```

## Directory Structure

```
build/
  wasm/
    test_add.wat       # Generated WAT
    test_add.wasm      # Compiled binary
    test_loop.wat
    test_loop.wasm
    ...

tests/wasm/
  test_add.c           # Test source
  test_add.expected    # Expected output
  test_loop.c
  test_loop.expected
  ...

.project-management/
  wasm-build-system.md # This document
```

## Performance Considerations

### Parallel Test Execution

```makefile
# Run WASM tests in parallel (GNU Make)
test-wasm-parallel: $(COMPILER) $(WASM_BUILD_DIR)
	@echo "Running WASM tests in parallel..."
	@jobs=4; \
	for src in tests/wasm/test_*.c; do \
		( \
			name=$$(basename $$src .c); \
			./$(COMPILER) --target=wasm $$src -o $(WASM_BUILD_DIR)/$$name.wat && \
			$(WAT2WASM) $(WASM_BUILD_DIR)/$$name.wat -o $(WASM_BUILD_DIR)/$$name.wasm \
		) & \
		if [ $$(jobs -r | wc -l) -ge $$jobs ]; then wait; fi; \
	done; \
	wait
	@echo "Parallel WASM build complete"
```

### Incremental Builds

```makefile
# Only rebuild if source changed
$(WASM_BUILD_DIR)/%.wat: tests/wasm/%.c $(COMPILER)
	@echo "  WASM    $<"
	@./$(COMPILER) --target=wasm $< -o $@

$(WASM_BUILD_DIR)/%.wasm: $(WASM_BUILD_DIR)/%.wat
	@echo "  WAT2WASM $<"
	@$(WAT2WASM) $< -o $@
```

## Debugging

### Verbose Build

```makefile
# Verbose WASM build
wasm-verbose: WASM_VERBOSE = 1
wasm-verbose: wasm
	@echo "Verbose WASM build complete"

# In test targets, use $(WASM_VERBOSE) to control output
```

### Build Artifacts

Keep intermediate files for debugging:
```makefile
# Don't delete .wat files after conversion
# (Default behavior keeps them)
```
