# C Compiler Self-Hosting Epic - Detailed Sprint Plan

**Project**: `/Users/mbusigin/c-compiler/project/`
**Goal**: Achieve self-hosting capability - compiler compiling itself
**Epic Owner**: Development Team
**Created**: 2024
**Status**: Planning Complete

---

## Executive Summary

The C compiler project has a well-documented architecture but cannot achieve self-hosting due to **6 critical bugs** preventing the compiler from compiling its own source code. This epic details the sprint plan to diagnose, fix, and verify self-hosting capability.

### Current State
- **Working Components**: Lexer, basic Parser, AST, type definitions, list utilities
- **Blocking Issues**: Function pointer parsing, infinite loops in preprocessor, union member access, type compatibility, output file handling
- **Source Files**: 24+ C files, 16+ header files
- **Test Infrastructure**: Unit tests, integration tests, puzzle tests (70+ test cases)

### Target State
- Stage0 (GCC-built): Compiles all source files
- Stage1 (stage0-built): Successfully compiles compiler
- Stage2 (stage1-built): Produces identical output to stage1 (convergence)
- All puzzle tests pass

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| More bugs discovered during implementation | HIGH | Medium | Incremental testing, sprint boundaries |
| Infinite loop has multiple causes | MEDIUM | HIGH | Add debug instrumentation early |
| Type system changes break existing code | LOW | HIGH | Comprehensive regression tests |
| Self-hosting takes longer than estimated | MEDIUM | LOW | Split into smaller sprints |

### STUCK_COUNTER Strategy
If any sprint encounters >3 stuck counter:
1. Refactor approach - try alternative solution
2. Add 424% entropy - parallelize investigation
3. Break down complex tasks into smaller spikes
4. Document findings and pivot if necessary

---

## Sprint 0: Diagnosis & Investigation

**Duration**: 1-2 days
**Goal**: Validate known issues, discover unknown issues, create test harness

### Task 0.1: Reproduce Known Bugs
**Description**: Verify all documented bugs exist with current codebase
**Test**: 
```bash
cd project && make clean && make
./compiler -I src -I include src/common/util.c -o /tmp/util.s 2>&1 | head -20
./compiler -I src -I include src/ir/ir.c -o /tmp/ir.s 2>&1 | head -20
```

### Task 0.2: Add Debug Instrumentation
**Description**: Add debug logging to identify infinite loop location in preprocessor/IR processing
**Test**:
```bash
cd project && make DEBUG=1
./compiler -I src -I include src/ir/ir.c -o /tmp/ir.s 2>&1 | grep -E "DEBUG|Processing|Infinite" | head -50
```

### Task 0.3: Create Minimal Reproduction Cases
**Description**: Extract minimal C code that triggers each bug for faster testing
**Test**:
```bash
# Function pointer test
echo 'void qsort(void *base, int (*compar)(const void*, const void*));' > /tmp/fp_test.c
./compiler /tmp/fp_test.c -o /tmp/fp_test.s 2>&1
# Should fail with parse error

# Struct member access test
echo 'struct S { int x; }; struct S s; int main() { return s.x; }' > /tmp/struct_test.c
./compiler /tmp/struct_test.c -o /tmp/struct_test.s 2>&1
# Should compile successfully
```

### Task 0.4: Establish Baseline Metrics
**Description**: Document which source files compile, which hang, which error
**Test**:
```bash
cd project && make stage0 2>/dev/null
count=0; failed=0; hung=0
for f in src/**/*.c src/*.c; do
    timeout 5 ./compiler -I src -I include "$f" -o /tmp/out.s 2>/dev/null && ((count++)) || \
    (timeout 5 ./compiler -I src -I include "$f" -o /tmp/out.s 2>&1 | grep -q error && ((failed++))) || \
    ((hung++))
done
echo "Compiled: $count, Failed: $failed, Hung: $hung"
test "$count" -gt 5 && echo "Baseline established"
```

### Sprint 0 Acceptance Criteria
- [ ] All 6 documented bugs reproduced
- [ ] Debug logging identifies infinite loop location
- [ ] Minimal test cases created for each bug
- [ ] Baseline: 0/24 files successfully compile (current state)

---

## Sprint 1: Function Pointer Support (CRITICAL PATH)

**Duration**: 3-4 days
**Goal**: Parse function pointer parameters, variables, and typedefs
**Status**: BLOCKER - Prevents all stdlib includes

### Task 1.1: Spike - Minimal Function Pointer Parameter Parsing
**Description**: Quick hack to validate approach for function pointer parameters
**Time**: 4 hours
**Test**:
```bash
echo 'void qsort(void *base, int (*compar)(const void*, const void*));' > /tmp/test.c
./compiler /tmp/test.c -o /tmp/test.s 2>&1
grep -q "error" /tmp/test.s && echo "Still failing (expected)" || echo "Works!"
```

### Task 1.2: Implement Declarator Parser
**Description**: Create dedicated `parse_declarator()` function handling complex declarators
**Time**: 8 hours
**Test**:
```bash
# Named function pointer
echo 'int (*callback)(int x, int y);' > /tmp/fp1.c
./compiler /tmp/fp1.c -o /tmp/fp1.s 2>&1 && echo "Named FP: PASS" || echo "Named FP: FAIL"

# Function pointer typedef
echo 'typedef int (*CompFn)(const void*, const void*);' > /tmp/fp2.c
./compiler /tmp/fp2.c -o /tmp/fp2.s 2>&1 && echo "Typedef FP: PASS" || echo "Typedef FP: FAIL"

# Function pointer parameter
echo 'void sort(int (*cmp)(int, int));' > /tmp/fp3.c
./compiler /tmp/fp3.c -o /tmp/fp3.s 2>&1 && echo "FP Param: PASS" || echo "FP Param: FAIL"
```

### Task 1.3: Add Semantic Analysis for Function Pointers
**Description**: Type inference and validation for function pointer types
**Time**: 4 hours
**Test**:
```bash
echo 'int add(int a, int b) { return a + b; }
int (*fp)(int, int) = add;
int main() { return fp(1, 2); }' > /tmp/fp_test.c
./compiler /tmp/fp_test.c -o /tmp/fp_test.s && \
clang /tmp/fp_test.s -o /tmp/fp_test && \
/tmp/fp_test; test $? -eq 3 && echo "Function pointer call: PASS" || echo "Function pointer call: FAIL"
```

### Task 1.4: Add Code Generation for Function Pointer Calls
**Description**: Generate indirect call instructions (CALL RAX pattern)
**Time**: 4 hours
**Test**:
```bash
echo 'int triple(int x) { return x * 3; }
int apply(int (*f)(int), int x) { return f(x); }
int main() { return apply(triple, 5); }' > /tmp/fp_call.c
./compiler /tmp/fp_call.c -o /tmp/fp_call.s && \
clang /tmp/fp_call.s -o /tmp/fp_call && \
/tmp/fp_call; test $? -eq 15 && echo "Indirect call: PASS" || echo "Indirect call: FAIL"
```

### Task 1.5: Verify Stdlib Header Compatibility
**Description**: Compile files that include standard library headers
**Time**: 4 hours
**Test**:
```bash
# Test util.c (uses qsort prototype)
./compiler -I src -I include src/common/util.c -o /tmp/util.s 2>&1 | \
grep -E "error|warning" | head -5
test ! -f /tmp/util.s && echo "util.c: Still has errors" || echo "util.c: Compiles!"

# Test error.c
./compiler -I src -I include src/common/error.c -o /tmp/error.s 2>&1 | \
grep -E "error|warning" | head -5
```

### Sprint 1 Acceptance Criteria
- [ ] Function pointer parameters parse correctly
- [ ] Function pointer variables parse correctly  
- [ ] Function pointer typedefs parse correctly
- [ ] Semantic analysis validates function pointer types
- [ ] Code generation produces working indirect calls
- [ ] `src/common/util.c` compiles without errors

---

## Sprint 2: Infinite Loop & Hang Fixes

**Duration**: 2-3 days
**Goal**: Fix preprocessor/IR hangs preventing file compilation
**Status**: CRITICAL - Blocks 28.6% of source files

### Task 2.1: Isolate Infinite Loop Trigger
**Description**: Identify exact content pattern causing hang
**Time**: 4 hours
**Test**:
```bash
# Test with progressively complex files
timeout 5 ./compiler -I src -I include src/common/util.c -o /tmp/out.s 2>&1
echo "util.c: $?"

timeout 5 ./compiler -I src -I include src/common/list.c -o /tmp/out.s 2>&1
echo "list.c: $?"

timeout 5 ./compiler -I src -I include src/ir/ir.c -o /tmp/out.s 2>&1
echo "ir.c: $?"
```

### Task 2.2: Fix Circular Dependency in Includes
**Description**: Prevent infinite recursion in header file processing
**Time**: 6 hours
**Test**:
```bash
timeout 10 ./compiler -I src -I include src/ir/ir.c -o /tmp/ir.s 2>&1
grep -q "Hang\|infinite\|loop" /tmp/ir.s && echo "Still hanging" || \
(test -f /tmp/ir.s && echo "ir.c: Compiles!" || echo "ir.c: Other error")
```

### Task 2.3: Fix IR Type Resolution
**Description**: Handle IRModule/List type interactions without infinite recursion
**Time**: 4 hours
**Test**:
```bash
# The minimal test file from BUGS_IDENTIFIED.md
echo '#include "backend/codegen.h"
IRModule *m;' > /tmp/minimal.c
timeout 5 ./compiler /tmp/minimal.c -o /tmp/minimal.s 2>&1
echo "Minimal test: $?"
```

### Task 2.4: Regression Testing
**Description**: Verify existing working files still compile
**Time**: 2 hours
**Test**:
```bash
cd project
for f in src/common/util.c src/common/list.c src/common/error.c src/lexer/lexer.c src/main.c; do
    timeout 5 ./compiler -I src -I include "$f" -o /tmp/out.s 2>/dev/null && echo "$f: OK" || echo "$f: FAIL"
done
```

### Sprint 2 Acceptance Criteria
- [ ] `test_framework.c` (minimal IR test) compiles without hanging
- [ ] `ir.c` compiles without hanging
- [ ] `codegen.c` compiles without hanging
- [ ] Existing working files still compile
- [ ] No new hangs introduced

---

## Sprint 3: Self-Hosting Bootstrap

**Duration**: 2-3 days
**Goal**: Successfully build stage1 compiler (compiler compiled with itself)

### Task 3.1: Compile All Source Files
**Description**: Ensure every source file compiles individually
**Time**: 4 hours
**Test**:
```bash
cd project
mkdir -p build/stage1_asm
count=0; total=0
for f in $(find src -name "*.c" | grep -v test); do
    total=$((total + 1))
    base=$(basename "$f" .c)
    timeout 10 ./compiler -I src -I include "$f" -o "build/stage1_asm/${base}.s" 2>/dev/null && \
    ((count++)) || echo "FAILED: $f"
done
echo "Compiled $count/$total files"
test $count -eq $total && echo "Sprint 3.1: PASS" || echo "Sprint 3.1: FAIL"
```

### Task 3.2: Assemble and Link Stage1
**Description**: Convert compiled assembly to object files and link
**Time**: 2 hours
**Test**:
```bash
cd project/build/stage1_asm
for f in *.s; do
    clang -c "$f" -o "${f%.s}.o" 2>/dev/null || echo "ASM FAIL: $f"
done
objects=$(ls *.o 2>/dev/null | wc -l)
echo "Assembled $objects object files"
test $objects -gt 15 && echo "Assembly: PASS" || echo "Assembly: FAIL"
```

### Task 3.3: Build and Verify Stage1 Compiler
**Description**: Link object files into stage1 executable
**Time**: 2 hours
**Test**:
```bash
cd project
clang build/stage1_asm/*.o -o build/compiler_stage1 -lm 2>/dev/null && \
test -x build/compiler_stage1 && echo "Stage1 binary exists" || echo "Stage1 link failed"
./build/compiler_stage1 --version 2>&1 | head -3
```

### Task 3.4: Stage1 Smoke Test
**Description**: Verify stage1 can compile basic programs
**Time**: 2 hours
**Test**:
```bash
echo 'int main() { return 42; }' > /tmp/hello.c
./build/compiler_stage1 /tmp/hello.c -o /tmp/hello.s && \
clang /tmp/hello.s -o /tmp/hello && \
/tmp/hello; test $? -eq 42 && echo "Stage1 smoke test: PASS" || echo "Stage1 smoke test: FAIL"

# Test hello.c program
./build/compiler_stage1 tests/hello.c -o /tmp/hello2.s 2>&1 | head -5
```

### Sprint 3 Acceptance Criteria
- [ ] All source files compile with stage0
- [ ] All assembly files assemble to object files
- [ ] stage1 executable exists and runs
- [ ] stage1 can compile hello world
- [ ] stage1 produces working executables

---

## Sprint 4: Bootstrap Convergence

**Duration**: 2-3 days
**Goal**: Verify stage1 and stage2 produce identical output (proven correctness)

### Task 4.1: Build Stage2 Compiler
**Description**: Compile compiler with stage1 to produce stage2
**Time**: 4 hours
**Test**:
```bash
cd project
mkdir -p build/stage2_asm
count=0; total=$(find src -name "*.c" | grep -v test | wc -l)
for f in $(find src -name "*.c" | grep -v test); do
    base=$(basename "$f" .c)
    timeout 10 ./build/compiler_stage1 -I src -I include "$f" -o "build/stage2_asm/${base}.s" 2>/dev/null && \
    ((count++))
done
echo "Stage2 compiled $count/$total files"
test $count -eq $total && echo "Stage2 compilation: PASS" || echo "Stage2 compilation: FAIL"
```

### Task 4.2: Link Stage2 and Verify
**Description**: Assemble stage2 objects and link stage2 executable
**Time**: 2 hours
**Test**:
```bash
cd project/build/stage2_asm
for f in *.s; do
    clang -c "$f" -o "${f%.s}.o" 2>/dev/null
done
cd ../..
clang build/stage2_asm/*.o -o build/compiler_stage2 -lm 2>/dev/null && \
test -x build/compiler_stage2 && echo "Stage2 binary: PASS" || echo "Stage2 binary: FAIL"
```

### Task 4.3: Convergence Verification
**Description**: Compare stage1 and stage2 output for identical compilation
**Time**: 4 hours
**Test**:
```bash
cd project
mkdir -p build/verify
./build/compiler_stage1 tests/hello.c -o build/verify/stage1.s 2>/dev/null
./build/compiler_stage2 tests/hello.c -o build/verify/stage2.s 2>/dev/null
diff build/verify/stage1.s build/verify/stage2.s >/dev/null 2>&1 && \
echo "CONVERGENCE: PASS - stage1 and stage2 produce identical output" || \
echo "CONVERGENCE: FAIL - output differs"
```

### Task 4.4: Multi-File Convergence Test
**Description**: Verify convergence across multiple source files
**Time**: 2 hours
**Test**:
```bash
cd project
passed=0; failed=0
for src in tests/hello.c tests/unit/test_lexer.c; do
    ./build/compiler_stage1 "$src" -o /tmp/s1.s 2>/dev/null
    ./build/compiler_stage2 "$src" -o /tmp/s2.s 2>/dev/null
    if diff -q /tmp/s1.s /tmp/s2.s >/dev/null 2>&1; then
        ((passed++))
    else
        ((failed++))
        echo "Convergence FAIL: $src"
    fi
done
echo "Convergence: $passed passed, $failed failed"
test $failed -eq 0 && echo "Sprint 4.4: PASS" || echo "Sprint 4.4: FAIL"
```

### Sprint 4 Acceptance Criteria
- [ ] stage2 compiler exists
- [ ] stage2 can compile programs
- [ ] stage1 and stage2 produce identical output for hello.c
- [ ] Convergence verified across multiple test files

---

## Sprint 5: Comprehensive Testing & Polish

**Duration**: 2-3 days
**Goal**: Verify all functionality, fix edge cases, document

### Task 5.1: Run Full Test Suite
**Description**: Execute all existing tests
**Time**: 2 hours
**Test**:
```bash
cd project && make test 2>&1 | tee /tmp/test_output.txt
passed=$(grep -c "PASS" /tmp/test_output.txt)
failed=$(grep -c "FAIL" /tmp/test_output.txt)
echo "Tests: $passed passed, $failed failed"
```

### Task 5.2: Run Puzzle Tests
**Description**: Execute all 70+ puzzle test cases
**Time**: 2 hours
**Test**:
```bash
cd project && make test-puzzles 2>&1 | tee /tmp/puzzle_output.txt
passed=$(grep -c "\[PASS\]" /tmp/puzzle_output.txt)
failed=$(grep -c "\[FAIL\]" /tmp/puzzle_output.txt)
skipped=$(grep -c "\[SKIP\]" /tmp/puzzle_output.txt)
echo "Puzzles: $passed passed, $failed failed, $skipped skipped"
test $failed -eq 0 && echo "Puzzle tests: PASS" || echo "Puzzle tests: FAIL"
```

### Task 5.3: Stress Testing
**Description**: Compile large/complex programs
**Time**: 2 hours
**Test**:
```bash
cd project
# Test with stress tests
for f in tests/puzzles/stress-*.c; do
    name=$(basename "$f" .c)
    ./compiler "$f" -o /tmp/"$name".s 2>/dev/null && \
    clang /tmp/"$name".s -o /tmp/"$name" 2>/dev/null && \
    timeout 5 /tmp/"$name" >/dev/null 2>&1 && \
    echo "[PASS] $name" || echo "[FAIL] $name"
done
```

### Task 5.4: Error Message Improvements
**Description**: Improve error messages for better developer experience
**Time**: 2 hours
**Test**:
```bash
echo 'int x = ;' | ./compiler - 2>&1 | grep -E "error|:[0-9]+:[0-9]+" | head -3
echo "Should show line:column and clear error message"
```

### Task 5.5: Documentation Update
**Description**: Update project documentation with self-hosting status
**Time**: 1 hour
**Test**:
```bash
grep -q "SELF-HOSTING" project/README.md && echo "README updated" || echo "README needs update"
grep -q "convergence" project/ARCHITECTURE.md && echo "Architecture updated" || echo "Architecture needs update"
```

### Sprint 5 Acceptance Criteria
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] All puzzle tests pass (or known failures documented)
- [ ] Stress tests complete successfully
- [ ] Error messages are helpful
- [ ] Documentation is current

---

## Epic Completion Criteria

### Must Have
- [ ] stage1 compiler exists and runs
- [ ] stage1 can compile all source files
- [ ] stage1 produces working executables
- [ ] All puzzle tests pass (except known skips)
- [ ] Bootstrap convergence verified

### Nice to Have
- [ ] Error messages are excellent
- [ ] Full documentation
- [ ] Performance benchmarks
- [ ] Additional optimization passes

---

## Dependency Graph

```
Sprint 0 (Diagnosis)
    │
    ▼
Sprint 1 (Function Pointers) ──────┐
    │                              │
    ▼                              │
Sprint 2 (Infinite Loops)          │
    │                              │
    ▼                              │
Sprint 3 (Self-Hosting) ◄──────────┘
    │
    ▼
Sprint 4 (Convergence)
    │
    ▼
Sprint 5 (Testing & Polish)
```

---

## Appendix A: Test Commands Reference

### Quick Test Commands
```bash
# Build compiler
cd project && make

# Run all tests
cd project && make test

# Run puzzle tests
cd project && make test-puzzles

# Try self-hosting
cd project && make self

# Verify convergence
cd project && make self-verify

# Clean and rebuild
cd project && make clean && make
```

### Individual File Test
```bash
./compiler -I src -I include <file.c> -o /tmp/out.s
```

### Regression Test
```bash
for f in src/**/*.c; do
    ./compiler -I src -I include "$f" -o /tmp/out.s 2>&1 | head -3
done
```

---

## Appendix B: File Inventory

### Files to Compile (17 source files)
```
src/main.c                    - Entry point
src/driver.c                  - Compiler driver
src/common/util.c             - Utilities (BLOCKER: function pointers)
src/common/list.c             - Dynamic arrays (BLOCKER: function pointers)
src/common/error.c            - Error reporting (BLOCKER: function pointers)
src/lexer/lexer.c             - Lexer
src/lexer/preproc.c           - Preprocessor
src/parser/parser.c           - Parser
src/parser/ast.c              - AST nodes
src/sema/analyzer.c           - Semantic analysis
src/sema/symtab.c             - Symbol table
src/ir/ir.c                   - IR generation (BLOCKER: infinite loop)
src/ir/lowerer.c              - AST to IR (BLOCKER: infinite loop)
src/optimize/optimizer.c      - Optimizations (BLOCKER: infinite loop)
src/backend/codegen.c         - Code generation (BLOCKER: infinite loop)
src/backend/regalloc.c        - Register allocation
src/backend/asm.c             - Assembly output
```

### Header Files (16 files)
```
include/stddef.h, stdbool.h, stdarg.h, stdio.h, stdlib.h, string.h
src/common/*.h, lexer/*.h, parser/*.h, sema/*.h, ir/*.h, backend/*.h
```

---

## Appendix C: Known Bugs Summary

| Bug # | Description | Severity | Status |
|-------|-------------|----------|--------|
| 1 | Function pointer parameters | CRITICAL | TODO |
| 2 | Function pointer declarations | CRITICAL | TODO |
| 3 | Struct member access | HIGH | TODO |
| 4 | Infinite loop in IR processing | CRITICAL | TODO |
| 5 | Type compatibility checking | HIGH | TODO |
| 6 | Output file handling | LOW | TODO |

---

**Epic Status**: Ready for Sprint 0
**Last Updated**: 2024
**Plan Version**: 1.0
