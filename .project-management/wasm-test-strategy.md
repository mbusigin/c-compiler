# WASM Testing Strategy and Validation Approach

## Overview

This document describes the comprehensive testing strategy for the WASM backend, including test categories, validation tools, and expected behaviors.

## Test Categories (test cases)

### Unit Tests

Test individual codegen components in isolation.

#### Codegen Unit Tests
- **Location**: `src/tests/test_wasm_codegen.c`
- **Purpose**: Test WAT emission functions
- **Coverage**:
  - Module header/footer emission
  - Function signature generation
  - Instruction emission (each IR opcode)
  - Type conversions
  - Constant handling

#### Example Test Structure
```c
void test_wasm_emit_module_header(void) {
    IRModule *mod = ir_module_create();
    char *wat = wasm_emit_module(mod);
    assert(strstr(wat, "(module") != NULL);
    free(wat);
    ir_module_destroy(mod);
}

void test_wasm_emit_i32_const(void) {
    IRValue *val = ir_value_int(42);
    char *wat = wasm_emit_value(val);
    assert(strcmp(wat, "i32.const 42") == 0);
    free(wat);
}
```

### Integration Tests

Test complete compilation pipeline from C source to WASM execution.

#### Location
- `tests/wasm/` directory

#### Test Format
Each test consists of:
- `testname.c` - C source file
- `testname.expected` - Expected output
- `testname.wat` - Generated WAT (after compilation)
- `testname.wasm` - Compiled binary (after wat2wasm)

#### Example Test
```c
// tests/wasm/test_add.c
int add(int a, int b) {
    return a + b;
}

// Expected: add(3, 5) = 8
```

### Regression Tests

Ensure existing functionality continues to work.

#### Self-Compilation Tests
- Attempt to compile compiler source with WASM target
- Track which files compile successfully
- Monitor progress toward self-hosting

#### Compatibility Tests
- Verify ARM64 backend still works
- Ensure driver changes don't break existing targets

## Validation Tools

### wat2wasm

**Purpose**: Validate WAT syntax and produce binary

**Usage**:
```bash
wat2wasm output.wat -o output.wasm
```

**Test Integration**:
```bash
# In Makefile test-wasm target
wat2wasm $(BUILD_DIR)/test.wat -o $(BUILD_DIR)/test.wasm 2>&1 | grep -q "error" && exit 1
```

### wasm2wat

**Purpose**: Decompile binary, verify round-trip

**Usage**:
```bash
wasm2wat output.wasm -o roundtrip.wat
diff output.wat roundtrip.wat
```

**Test Integration**:
```bash
# Verify round-trip produces equivalent output
wasm2wat $(BUILD_DIR)/test.wasm | diff - $(BUILD_DIR)/test.wat || echo "Round-trip differs (may be formatting)"
```

### wasmtime

**Purpose**: Execute WASM binaries and verify output

**Usage**:
```bash
wasmtime output.wasm --invoke main
wasmtime output.wasm --invoke add -- 3 5
```

**Test Integration**:
```bash
# Run and capture output
output=$(wasmtime $(BUILD_DIR)/test.wasm --invoke main 2>&1)
[ "$output" = "8" ] || exit 1
```

### wasm-validate

**Purpose**: Validate binary format

**Usage**:
```bash
wasm-validate output.wasm
```

**Test Integration**:
```bash
wasm-validate $(BUILD_DIR)/test.wasm || exit 1
```

### Node.js

**Purpose**: Browser-like execution environment

**Usage**:
```javascript
// test-runner.js
const fs = require('fs');
const wasm = fs.readFileSync('output.wasm');
const instance = new WebAssembly.Instance(new WebAssembly.Module(wasm));
console.log(instance.exports.main());
```

## Test Cases

### Category 1: Arithmetic Operations

| Test | Description | Expected |
|------|-------------|----------|
| test_add | Integer addition | Correct sum |
| test_sub | Integer subtraction | Correct difference |
| test_mul | Integer multiplication | Correct product |
| test_div | Integer division | Correct quotient |
| test_mod | Modulo operation | Correct remainder |
| test_neg | Negation | Correct negative |

### Category 2: Control Flow

| Test | Description | Expected |
|------|-------------|----------|
| test_if | Simple if statement | Correct branch |
| test_if_else | If-else statement | Correct branch |
| test_while | While loop | Correct iteration |
| test_for | For loop | Correct iteration |
| test_nested_if | Nested conditionals | Correct result |

### Category 3: Memory and Variables

| Test | Description | Expected |
|------|-------------|----------|
| test_locals | Local variables | Correct values |
| test_assign | Variable assignment | Correct assignment |
| test_array | Array access | Correct element |
| test_pointer | Pointer dereference | Correct value |

### Category 4: Functions

| Test | Description | Expected |
|------|-------------|----------|
| test_call | Function call | Correct return |
| test_recursive | Recursive function | Correct result |
| test_multiple_args | Multiple parameters | Correct handling |
| test_void | Void function | No return value |

### Category 5: I/O and Strings

| Test | Description | Expected |
|------|-------------|----------|
| test_printf | Printf output | Correct string |
| test_string | String literal | Correct address |
| test_hello | Hello World | "Hello, World!" |

### Category 6: Type Conversions

| Test | Description | Expected |
|------|-------------|----------|
| test_cast | Type casting | Correct conversion |
| test_float | Floating point | Correct result |
| test_widen | Integer widening | Correct value |

## Test Execution Flow

### Makefile Target

```makefile
test-wasm: $(COMPILER)
	@echo "Running WASM tests..."
	@mkdir -p $(BUILD_DIR)/wasm
	@passed=0; failed=0; \
	for src in tests/wasm/test_*.c; do \
		name=$$(basename $$src .c); \
		echo "Testing $$name..."; \
		./$(COMPILER) --target=wasm $$src -o $(BUILD_DIR)/wasm/$$name.wat || { failed=$$((failed+1)); continue; }; \
		wat2wasm $(BUILD_DIR)/wasm/$$name.wat -o $(BUILD_DIR)/wasm/$$name.wasm 2>/dev/null || { failed=$$((failed+1)); continue; }; \
		expected=$$(cat tests/wasm/$$name.expected 2>/dev/null); \
		actual=$$(wasmtime $(BUILD_DIR)/wasm/$$name.wasm --invoke main 2>&1); \
		if [ "$$expected" = "$$actual" ]; then \
			echo "  [PASS] $$name"; passed=$$((passed+1)); \
		else \
			echo "  [FAIL] $$name: expected '$$expected', got '$$actual'"; failed=$$((failed+1)); \
		fi; \
	done; \
	echo ""; \
	echo "WASM Tests: $$passed passed, $$failed failed"; \
	if [ $$failed -gt 0 ]; then exit 1; fi
```

### CI/CD Integration

```yaml
# .github/workflows/wasm-tests.yml
name: WASM Tests

on: [push, pull_request]

jobs:
  wasm-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install WABT
        run: sudo apt-get install wabt
      
      - name: Install Wasmtime
        run: curl https://wasmtime.dev/install.sh -sSf | bash
      
      - name: Build Compiler
        run: make
      
      - name: Run WASM Tests
        run: make test-wasm
```

## Validation Workflow

### Pre-commit Validation
```bash
# Run before committing WASM changes
make test-wasm
```

### Pull Request Validation
- All WASM tests must pass
- wat2wasm validation must succeed
- No regression in ARM64 tests

### Release Validation
- Full test suite pass
- Self-compilation assessment
- Performance benchmarks

## Expected Behaviors

### Valid WAT Output
- Proper module structure
- Correct type annotations
- Valid instruction sequences
- Proper block nesting

### Correct Execution
- Arithmetic produces correct results
- Control flow follows expected paths
- Memory operations preserve values
- Function calls return correct values

### Error Handling
- Invalid C code produces compilation errors
- Unsupported features produce clear messages
- Runtime errors are caught by wasmtime

## Test Coverage Goals

| Component | Coverage Goal |
|-----------|---------------|
| Instruction emission | 100% of IR opcodes |
| Control flow | All branch patterns |
| Memory ops | Load, store, alloca |
| Functions | Calls, returns, params |
| Types | All supported C types |

## Known Limitations

### Initial Release
- No floating point support (Sprint 5)
- No function pointers (Sprint 5)
- Limited standard library
- No exception handling

### Test Exclusions
- Multi-threading tests (not supported)
- SIMD tests (not supported)
- GC tests (not supported)

## Metrics and Reporting

### Test Metrics
- Total tests run
- Pass/fail ratio
- Coverage percentage
- Execution time

### Quality Metrics
- Bug detection rate
- Regression frequency
- Time to fix failures

## References

- [WABT Tools](https://github.com/WebAssembly/wabt)
- [Wasmtime](https://wasmtime.dev/)
- [WebAssembly Test Suite](https://github.com/WebAssembly/testsuite)
