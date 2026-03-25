# WASM Integration Test Framework Design

## Overview

This document describes the integration test framework, fixtures, and test runners for comprehensive WASM backend testing.

## Test Framework Architecture

### Components

```
Integration Test Framework
├── Test Runner (Makefile/bash)
├── Test Fixtures
│   ├── WASM Runtime Setup
│   ├── Import Mocks
│   └── Memory Helpers
├── Test Cases
│   ├── Unit Tests
│   ├── Integration Tests
│   └── End-to-End Tests
└── Reporting
    ├── Pass/Fail Status
    ├── Output Comparison
    └── Performance Metrics
```

## Test Fixtures

### Runtime Fixture

```javascript
// tests/wasm/fixtures/runtime.js

const fs = require('fs');

class WasmTestRuntime {
    constructor() {
        this.output = [];
        this.memory = new WebAssembly.Memory({ initial: 2 });
        this.heapPtr = 65536;  // Start after first page
        
        this.imports = {
            env: {
                memory: this.memory,
                
                printf: (fmt, ...args) => {
                    const str = this.readString(fmt);
                    // Simple printf implementation
                    let formatted = str;
                    for (const arg of args) {
                        formatted = formatted.replace('%d', arg.toString());
                    }
                    this.output.push(formatted.replace('\\n', '\n'));
                    return formatted.length;
                },
                
                putchar: (c) => {
                    this.output.push(String.fromCharCode(c));
                    return c;
                },
                
                malloc: (size) => {
                    const ptr = this.heapPtr;
                    this.heapPtr += size;
                    return ptr;
                },
                
                free: (ptr) => {
                    // No-op for simple allocator
                },
                
                memcpy: (dst, src, len) => {
                    const mem = new Uint8Array(this.memory.buffer);
                    for (let i = 0; i < len; i++) {
                        mem[dst + i] = mem[src + i];
                    }
                    return dst;
                },
                
                memset: (ptr, val, len) => {
                    const mem = new Uint8Array(this.memory.buffer);
                    for (let i = 0; i < len; i++) {
                        mem[ptr + i] = val;
                    }
                    return ptr;
                }
            }
        };
    }
    
    readString(addr) {
        const mem = new Uint8Array(this.memory.buffer);
        let str = '';
        for (let i = addr; mem[i] !== 0; i++) {
            str += String.fromCharCode(mem[i]);
        }
        return str;
    }
    
    async loadModule(wasmPath) {
        const wasm = fs.readFileSync(wasmPath);
        const module = await WebAssembly.compile(wasm);
        const instance = await WebAssembly.instantiate(module, this.imports);
        return instance;
    }
    
    invoke(instance, funcName, ...args) {
        if (instance.exports[funcName]) {
            return instance.exports[funcName](...args);
        }
        throw new Error(`Function ${funcName} not found`);
    }
    
    getOutput() {
        return this.output.join('');
    }
    
    reset() {
        this.output = [];
        this.heapPtr = 65536;
    }
}

module.exports = { WasmTestRuntime };
```

### Makefile Fixture

```makefile
# tests/wasm/Makefile

COMPILER = ../../compiler
WAT2WASM = wat2wasm
WASMTIME = wasmtime

BUILD_DIR = ../../build/wasm

.PHONY: all clean test

all: test

test: $(BUILD_DIR)
	@echo "Running WASM Integration Tests"
	@./run_tests.sh

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
```

### Shell Test Runner

```bash
#!/bin/bash
# tests/wasm/run_tests.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../../build/wasm"
COMPILER="$SCRIPT_DIR/../../compiler"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0
SKIPPED=0

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

# Test function
run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .c)
    local expected_file="$SCRIPT_DIR/$(dirname ${test_file#tests/})/${test_name}.expected"
    
    echo -n "  Testing $test_name... "
    
    # Compile to WAT
    if ! "$COMPILER" --target=wasm "$test_file" -o "$BUILD_DIR/$test_name.wat" 2>/dev/null; then
        echo -e "${RED}[FAIL]${NC} (compilation failed)"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Convert to WASM
    if ! wat2wasm "$BUILD_DIR/$test_name.wat" -o "$BUILD_DIR/$test_name.wasm" 2>/dev/null; then
        echo -e "${RED}[FAIL]${NC} (wat2wasm failed)"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Validate
    if ! wasm-validate "$BUILD_DIR/$test_name.wasm" 2>/dev/null; then
        echo -e "${RED}[FAIL]${NC} (validation failed)"
        FAILED=$((FAILED + 1))
        return
    fi
    
    # Check for expected output file
    if [ ! -f "$expected_file" ]; then
        echo -e "${YELLOW}[SKIP]${NC} (no expected file)"
        SKIPPED=$((SKIPPED + 1))
        return
    fi
    
    # Run and compare
    local expected=$(cat "$expected_file")
    local actual=$(wasmtime "$BUILD_DIR/$test_name.wasm" --invoke main 2>&1 || true)
    
    if [ "$expected" = "$actual" ]; then
        echo -e "${GREEN}[PASS]${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}[FAIL]${NC}"
        echo "    Expected: $expected"
        echo "    Actual:   $actual"
        FAILED=$((FAILED + 1))
    fi
}

# Run all tests
echo "========================================="
echo "WASM Integration Test Suite"
echo "========================================="
echo ""

# Unit tests
echo "--- Unit Tests ---"
for test in "$SCRIPT_DIR"/unit/*.c; do
    [ -f "$test" ] && run_test "$test"
done

# Control flow tests
echo ""
echo "--- Control Flow Tests ---"
for test in "$SCRIPT_DIR"/control_flow/*.c; do
    [ -f "$test" ] && run_test "$test"
done

# Memory tests
echo ""
echo "--- Memory Tests ---"
for test in "$SCRIPT_DIR"/memory/*.c; do
    [ -f "$test" ] && run_test "$test"
done

# Function tests
echo ""
echo "--- Function Tests ---"
for test in "$SCRIPT_DIR"/functions/*.c; do
    [ -f "$test" ] && run_test "$test"
done

# I/O tests
echo ""
echo "--- I/O Tests ---"
for test in "$SCRIPT_DIR"/io/*.c; do
    [ -f "$test" ] && run_test "$test"
done

# Summary
echo ""
echo "========================================="
echo "Test Summary: $PASSED passed, $FAILED failed, $SKIPPED skipped"
echo "========================================="

if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
```

## Test Categories

### Unit Tests

Test individual compiler components:
- Instruction emission
- Type handling
- Constant folding

### Integration Tests

Test complete compilation flow:
- C source → WAT → WASM → Execution
- Multi-function programs
- Library linking

### End-to-End Tests

Test real-world scenarios:
- Complete programs
- Standard library usage
- Complex algorithms

## Test Fixtures by Category

### Arithmetic Fixture

```c
// tests/wasm/fixtures/arithmetic.h
#ifndef ARITHMETIC_FIXTURE_H
#define ARITHMETIC_FIXTURE_H

// Test data for arithmetic operations
typedef struct {
    int a;
    int b;
    int expected_add;
    int expected_sub;
    int expected_mul;
    int expected_div;
} ArithmeticTest;

static Arithmetic arithmetic_tests[] = {
    {10, 5, 15, 5, 50, 2},
    {100, 3, 103, 97, 300, 33},
    {0, 0, 0, 0, 0, 0},  // Edge case
    {-10, 5, -5, -15, -50, -2},  // Negative
};

#endif
```

### Control Flow Fixture

```c
// tests/wasm/fixtures/control_flow.h
#ifndef CONTROL_FLOW_FIXTURE_H
#define CONTROL_FLOW_FIXTURE_H

// Test cases for control flow
typedef struct {
    const char *name;
    const char *source;
    int expected;
} ControlFlowTest;

static ControlFlowTest control_flow_tests[] = {
    {"if_positive", 
     "int f(int x) { if (x > 0) return 1; return 0; }",
     1},
    {"while_count",
     "int f(int n) { int c = 0; while (n > 0) { c++; n--; } return c; }",
     10},
};

#endif
```

## Reporting

### JUnit XML Output

```bash
# Generate JUnit XML for CI/CD
./run_tests.sh --junit-output=test-results.xml
```

### Sample XML Output

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites name="WASM Integration Tests" tests="25" failures="2" errors="0">
    <testsuite name="Unit Tests" tests="7" failures="0">
        <testcase name="test_arithmetic_add" classname="arithmetic" time="0.05"/>
        <testcase name="test_arithmetic_sub" classname="arithmetic" time="0.04"/>
    </testsuite>
    <testsuite name="Control Flow Tests" tests="6" failures="1">
        <testcase name="test_if_simple" classname="control_flow" time="0.06"/>
        <testcase name="test_while_loop" classname="control_flow" time="0.08">
            <failure message="Expected 55, got 0">Assertion failed</failure>
        </testcase>
    </testsuite>
</testsuites>
```

### Coverage Report

```bash
# Generate coverage report
./run_tests.sh --coverage
```

## Continuous Integration

### GitHub Actions Integration

```yaml
# .github/workflows/wasm-integration.yml
name: WASM Integration Tests

on: [push, pull_request]

jobs:
  integration:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install WABT
      run: sudo apt-get install wabt
    
    - name: Install Wasmtime
      run: curl https://wasmtime.dev/install.sh -sSf | bash
    
    - name: Build Compiler
      run: make
    
    - name: Run Integration Tests
      run: cd tests/wasm && ./run_tests.sh
    
    - name: Upload Test Results
      uses: actions/upload-artifact@v3
      if: always()
      with:
        name: test-results
        path: tests/wasm/test-results.xml
```

## Debugging Support

### Verbose Mode

```bash
# Run with verbose output
./run_tests.sh --verbose

# Output includes:
# - Compilation commands
# - Generated WAT
# - WASM binary size
# - Execution trace
```

### Debug Artifacts

```bash
# Keep intermediate files for debugging
./run_tests.sh --keep-artifacts

# Creates:
# build/wasm/test_name.wat
# build/wasm/test_name.wasm
# build/wasm/test_name.trace
```

## Performance Testing

### Benchmark Tests

```c
// tests/wasm/benchmarks/bench_fibonacci.c
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    return fibonacci(20);
}
// Expected: 6765
// Time limit: 1 second
```

### Performance Metrics

```bash
# Run benchmarks
./run_tests.sh --benchmark

# Output:
# test_fibonacci: 0.45s (limit: 1.0s) [PASS]
# test_matrix: 0.82s (limit: 2.0s) [PASS]
```

## References

- [WebAssembly Test Suite](https://github.com/WebAssembly/testsuite)
- [WABT Tools](https://github.com/WebAssembly/wabt)
- [Wasmtime](https://github.com/bytecodealliance/wasmtime)
