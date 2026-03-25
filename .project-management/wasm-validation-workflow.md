# WASM Output Validation and Verification Workflow

## Overview

This document defines the validation and verification workflow for WASM output, including CI/CD integration and quality gates.

## Validation Pipeline

### Stages

```
C Source
    │
    ▼
┌─────────────────┐
│ 1. Compilation  │ ──► Check: Exit code, error messages
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ 2. WAT Syntax   │ ──► Check: wat2wasm validation
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ 3. WASM Binary  │ ──► Check: wasm-validate
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ 4. Execution    │ ──► Check: Output matches expected
└─────────────────┘
    │
    ▼
┌─────────────────┐
│ 5. Round-trip   │ ──► Check: wasm2wat produces equivalent WAT
└─────────────────┘
```

## Stage 1: Compilation Validation

### Checks
- Compiler exits with code 0
- No error messages to stderr
- Output file created

### Script
```bash
#!/bin/bash
# validate_compilation.sh

SOURCE="$1"
OUTPUT="$2"

if ./compiler --target=wasm "$SOURCE" -o "$OUTPUT" 2>compile.err; then
    if [ -f "$OUTPUT" ]; then
        echo "[PASS] Compilation successful"
        exit 0
    else
        echo "[FAIL] No output file generated"
        exit 1
    fi
else
    echo "[FAIL] Compilation failed"
    cat compile.err
    exit 1
fi
```

---

## Stage 2: WAT Syntax Validation

### Checks
- WAT file is well-formed
- All required sections present
- No syntax errors

### Script
```bash
#!/bin/bash
# validate_wat_syntax.sh

WAT_FILE="$1"

if wat2wasm "$WAT_FILE" -o /dev/null 2>wat.err; then
    echo "[PASS] WAT syntax valid"
    exit 0
else
    echo "[FAIL] WAT syntax error"
    cat wat.err
    exit 1
fi
```

### Expected WAT Structure
```wat
(module
  ;; Types section (optional but recommended)
  (type $t0 (func ...))
  
  ;; Imports (if needed)
  (import "env" "func" (func ...))
  
  ;; Memory
  (memory ...)
  
  ;; Functions
  (func $name ...
    ...)
  
  ;; Exports
  (export "name" (func $name))
  
  ;; Data (if needed)
  (data ...)
)
```

---

## Stage 3: WASM Binary Validation

### Checks
- Binary format is valid
- All sections well-formed
- Type constraints satisfied

### Script
```bash
#!/bin/bash
# validate_wasm_binary.sh

WASM_FILE="$1"

if wasm-validate "$WASM_FILE" 2>validate.err; then
    echo "[PASS] WASM binary valid"
    exit 0
else
    echo "[FAIL] WASM validation error"
    cat validate.err
    exit 1
fi
```

### Common Validation Errors

```
# Type mismatch
error: type mismatch in i32.add, expected [i32, i32] but got [i32]

# Undefined label
error: undefined label

# Invalid export
error: invalid export name
```

---

## Stage 4: Execution Validation

### Checks
- Module loads successfully
- Functions execute without trap
- Output matches expected

### Script
```bash
#!/bin/bash
# validate_execution.sh

WASM_FILE="$1"
EXPECTED="$2"
FUNC="${3:-main}"

ACTUAL=$(wasmtime "$WASM_FILE" --invoke "$FUNC" 2>&1)
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo "[FAIL] Execution failed with exit code $EXIT_CODE"
    echo "$ACTUAL"
    exit 1
fi

if [ "$EXPECTED" = "$ACTUAL" ]; then
    echo "[PASS] Output matches expected"
    exit 0
else
    echo "[FAIL] Output mismatch"
    echo "  Expected: $EXPECTED"
    echo "  Actual:   $ACTUAL"
    exit 1
fi
```

---

## Stage 5: Round-trip Validation

### Checks
- wasm2wat produces valid WAT
- Re-compiled WASM is equivalent

### Script
```bash
#!/bin/bash
# validate_roundtrip.sh

WASM_FILE="$1"
ORIGINAL_WAT="$2"

# Decompile
wasm2wat "$WASM_FILE" -o roundtrip.wat 2>roundtrip.err
if [ $? -ne 0 ]; then
    echo "[FAIL] wasm2wat failed"
    cat roundtrip.err
    exit 1
fi

# Note: Exact text match may fail due to formatting differences
# Instead, validate that roundtrip.wat compiles back to valid WASM
wat2wasm roundtrip.wat -o roundtrip.wasm 2>roundtrip2.err
if [ $? -ne 0 ]; then
    echo "[FAIL] Roundtrip compilation failed"
    cat roundtrip2.err
    exit 1
fi

# Validate the roundtrip binary
if wasm-validate roundtrip.wasm; then
    echo "[PASS] Roundtrip validation successful"
    rm -f roundtrip.wat roundtrip.wasm
    exit 0
else
    echo "[FAIL] Roundtrip binary invalid"
    exit 1
fi
```

---

## Complete Validation Script

```bash
#!/bin/bash
# validate_wasm_complete.sh

set -e

SOURCE="$1"
EXPECTED="$2"
BUILD_DIR="/tmp/wasm_validate_$$"

mkdir -p "$BUILD_DIR"
trap "rm -rf $BUILD_DIR" EXIT

WAT="$BUILD_DIR/output.wat"
WASM="$BUILD_DIR/output.wasm"

echo "========================================="
echo "WASM Validation Pipeline"
echo "========================================="
echo "Source: $SOURCE"
echo ""

# Stage 1: Compilation
echo "Stage 1: Compilation..."
if ! ./compiler --target=wasm "$SOURCE" -o "$WAT" 2>"$BUILD_DIR/compile.err"; then
    echo "[FAIL] Compilation failed"
    cat "$BUILD_DIR/compile.err"
    exit 1
fi
echo "[PASS] Compilation successful"

# Stage 2: WAT Syntax
echo "Stage 2: WAT Syntax Validation..."
if ! wat2wasm "$WAT" -o "$WASM" 2>"$BUILD_DIR/wat.err"; then
    echo "[FAIL] WAT syntax error"
    cat "$BUILD_DIR/wat.err"
    exit 1
fi
echo "[PASS] WAT syntax valid"

# Stage 3: WASM Binary
echo "Stage 3: WASM Binary Validation..."
if ! wasm-validate "$WASM" 2>"$BUILD_DIR/validate.err"; then
    echo "[FAIL] WASM validation error"
    cat "$BUILD_DIR/validate.err"
    exit 1
fi
echo "[PASS] WASM binary valid"

# Stage 4: Execution
echo "Stage 4: Execution Validation..."
ACTUAL=$(wasmtime "$WASM" --invoke main 2>&1)
if [ -f "$EXPECTED" ]; then
    EXP=$(cat "$EXPECTED")
    if [ "$EXP" = "$ACTUAL" ]; then
        echo "[PASS] Output matches expected"
    else
        echo "[FAIL] Output mismatch"
        echo "  Expected: $EXP"
        echo "  Actual:   $ACTUAL"
        exit 1
    fi
else
    echo "[SKIP] No expected file, output: $ACTUAL"
fi

# Stage 5: Round-trip
echo "Stage 5: Round-trip Validation..."
wasm2wat "$WASM" -o "$BUILD_DIR/roundtrip.wat"
if wat2wasm "$BUILD_DIR/roundtrip.wat" -o "$BUILD_DIR/roundtrip.wasm" 2>/dev/null; then
    if wasm-validate "$BUILD_DIR/roundtrip.wasm" 2>/dev/null; then
        echo "[PASS] Round-trip validation successful"
    else
        echo "[FAIL] Round-trip binary invalid"
        exit 1
    fi
else
    echo "[FAIL] Round-trip compilation failed"
    exit 1
fi

echo ""
echo "========================================="
echo "All validation stages passed!"
echo "========================================="
```

---

## CI/CD Integration

### GitHub Actions Workflow

```yaml
# .github/workflows/wasm-validation.yml
name: WASM Validation

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  validate:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install Tools
      run: |
        sudo apt-get update
        sudo apt-get install -y wabt
        curl https://wasmtime.dev/install.sh -sSf | bash
        echo "$HOME/.wasmtime/bin" >> $GITHUB_PATH
    
    - name: Build Compiler
      run: make
    
    - name: Run Validation Pipeline
      run: |
        for test in tests/wasm/test_*.c; do
          name=$(basename $test .c)
          expected="tests/wasm/${name}.expected"
          ./validate_wasm_complete.sh "$test" "$expected"
        done
    
    - name: Validate All Binaries
      run: make test-wasm-validate
```

### Quality Gates

| Gate | Criteria | Action on Failure |
|------|----------|-------------------|
| Compilation | Exit code 0 | Block merge |
| WAT Syntax | wat2wasm success | Block merge |
| WASM Binary | wasm-validate success | Block merge |
| Execution | Output match | Block merge |
| Coverage | >80% tests pass | Warning |

---

## Validation Reports

### Summary Report

```
WASM Validation Report
======================
Date: 2024-01-15
Commit: abc123

Tests Run: 25
Passed: 24
Failed: 1
Skipped: 0

Failures:
  - test_while_loop: Output mismatch (expected 55, got 0)

Binary Sizes:
  - Average: 512 bytes
  - Min: 128 bytes
  - Max: 1024 bytes

Validation Time: 12.5s
```

### Detailed Report

```xml
<?xml version="1.0" encoding="UTF-8"?>
<validation-report>
    <summary>
        <total>25</total>
        <passed>24</passed>
        <failed>1</failed>
    </summary>
    <tests>
        <test name="test_add" status="pass">
            <stage name="compilation" status="pass"/>
            <stage name="wat-syntax" status="pass"/>
            <stage name="wasm-binary" status="pass"/>
            <stage name="execution" status="pass"/>
            <stage name="roundtrip" status="pass"/>
        </test>
        <test name="test_while_loop" status="fail">
            <stage name="compilation" status="pass"/>
            <stage name="wat-syntax" status="pass"/>
            <stage name="wasm-binary" status="pass"/>
            <stage name="execution" status="fail">
                <error>Output mismatch</error>
                <expected>55</expected>
                <actual>0</actual>
            </stage>
        </test>
    </tests>
</validation-report>
```

---

## Automated Validation

### Pre-commit Hook

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Validate WASM tests before commit
echo "Running WASM validation..."
make test-wasm-validate || exit 1

echo "WASM validation passed"
exit 0
```

### Makefile Target

```makefile
# Full validation
validate-wasm: 
	@echo "Running full WASM validation..."
	@./validate_wasm_complete.sh tests/wasm/test_add.c tests/wasm/test_add.expected
	@./validate_wasm_complete.sh tests/wasm/test_loop.c tests/wasm/test_loop.expected
	@echo "All validations passed"

# Quick validation (compilation + syntax only)
validate-wasm-quick:
	@for wat in build/wasm/*.wat; do \
		wat2wasm $$wat -o /dev/null || exit 1; \
	done
	@echo "Quick validation passed"
```

---

## Troubleshooting

### Common Issues

**Issue**: wat2wasm fails with "unexpected token"
- **Cause**: Invalid WAT syntax
- **Fix**: Check generated WAT for syntax errors

**Issue**: wasm-validate fails with "type mismatch"
- **Cause**: Stack type error in generated code
- **Fix**: Review instruction sequence in codegen

**Issue**: Execution produces wrong output
- **Cause**: Logic error in generated code
- **Fix**: Debug with wasm-interp or wasmtime --trace

**Issue**: Round-trip validation fails
- **Cause**: Non-canonical WAT formatting
- **Fix**: Accept formatting differences, validate binary equivalence

---

## References

- [WABT Documentation](https://github.com/WebAssembly/wabt)
- [Wasmtime CLI](https://docs.wasmtime.dev/cli.html)
- [WebAssembly Validation](https://webassembly.github.io/spec/core/valid/index.html)
