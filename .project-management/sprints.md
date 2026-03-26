# Compiler Self-Hosting Sprint Plan

## Project Overview

**Goal**: Fix all compiler bugs until the self-hosting test works completely.

**Current Status**:
- All 23 source files compile individually without errors
- WASM backend has critical bugs in control flow code generation
- Self-hosting test script has a minor bug with `set -e`
- WASM tests: 3/6 passing (arith, bitwise, branch), 3 failing (cmp, loop, memory)

## Identified Issues

### 1. WASM Code Generation Bugs (HIGH PRIORITY)
- **Block/Loop Structure**: Missing type annotations, improper nesting
- **Label Resolution**: Labels not tracked correctly for `br`/`br_if`
- **Comparison Values**: Wrong operands being compared
- **Function Arguments**: Arguments passed in wrong order

### 2. Self-Hosting Test Script Bug (LOW PRIORITY)
- `set -e` causes premature exit during error output capture

---

## Sprint 1: Fix WASM Control Flow (Week 1)

### Epic: WASM Control Flow Correctness

**Objective**: Fix all failing WASM tests by correcting control flow code generation.

#### Task 1.1: Fix Block Structure and Type Annotations
- **Description**: Add proper WASM type annotations to blocks (e.g., `block (result i32)`)
- **Files**: `src/backend/wasm_codegen.c`, `src/backend/wasm_emit.c`
- **Test**: `wasm_test_cmp` produces valid WAT with `wat2wasm`

#### Task 1.2: Fix Label Resolution for Br/Br_if
- **Description**: Implement stack-based label tracking for correct depth calculation
- **Files**: `src/backend/wasm_emit.c`
- **Test**: `wasm_test_loop` produces valid WAT with nested loops

#### Task 1.3: Fix Comparison Value Generation
- **Description**: Ensure IR_CMP_* uses actual operands, not result registers
- **Files**: `src/backend/wasm_codegen.c`
- **Test**: `wasm_test_cmp` produces correct comparison results

#### Task 1.4: Fix Function Argument Passing
- **Description**: Ensure function arguments are in correct order without duplication
- **Files**: `src/backend/wasm_codegen.c`
- **Test**: `wasm_test_memory` correctly passes array/length to functions

#### Task 1.5: Remove Invalid Comments from WAT Output
- **Description**: Debug comments like "label not found" shouldn't appear in output
- **Files**: `src/backend/wasm_emit.c`
- **Test**: No "label not found" comments in generated WAT

---

## Sprint 2: WASM Memory Operations (Week 2)

### Epic: WASM Memory Model Support

**Objective**: Implement proper memory operations for arrays and pointers.

#### Task 2.1: Implement IR_LOAD/IR_STORE for WASM
- **Description**: Emit `memory.load` and `memory.store` with correct alignment
- **Files**: `src/backend/wasm_codegen.c`
- **Test**: `wasm_test_memory` correctly accesses array elements

#### Task 2.2: Implement Linear Memory Declaration
- **Description**: Generate valid memory section in WAT output
- **Files**: `src/backend/wasm_emit.c`
- **Test**: WAT output includes `(memory 1)` declaration

#### Task 2.3: Fix Array Index Calculation
- **Description**: Correct pointer arithmetic for array access
- **Files**: `src/backend/wasm_codegen.c`
- **Test**: `wasm_test_memory` produces correct sums

---

## Sprint 3: Self-Hosting Test Infrastructure (Week 2-3)

### Epic: Self-Hosting Validation Framework

**Objective**: Create robust self-hosting test pipeline.

#### Task 3.1: Fix Self-Hosting Test Script
- **Description**: Remove `set -e` or handle error capture properly
- **Files**: `scripts/selfhost-test.sh`
- **Test**: Script runs to completion and reports accurate results

#### Task 3.2: Create Link Test for Compiler
- **Description**: Verify all object files can link into executable
- **Files**: `scripts/selfhost-test.sh`
- **Test**: `./compiler` built from self-compiled object files runs

#### Task 3.3: Create Bootstrap Verification Script
- **Description**: Verify stage1 and stage2 produce identical output
- **Files**: `scripts/bootstrap.sh`
- **Test**: Two-stage compilation produces identical binaries

---

## Sprint 4: WASM Full Test Suite (Week 3)

### Epic: WASM Test Suite Completion

**Objective**: Ensure all WASM tests pass and document remaining issues.

#### Task 4.1: Verify All 6 WASM Tests Pass
- **Description**: Validate wasm_test_arith, wasm_test_bitwise, wasm_test_branch
- **Test**: `make test-wasm` shows 6/6 passing

#### Task 4.2: Run Full Integration Test Suite
- **Description**: Ensure no regressions in existing functionality
- **Test**: `make test` passes all existing tests

#### Task 4.3: Document Known Limitations
- **Description**: Document any unimplemented WASM features
- **Files**: `.project-management/wasm-status.md`
- **Test**: No blocking bugs, only documented limitations

---

## Sprint 5: Self-Hosting Completion (Week 4)

### Epic: Full Self-Hosting Achievement

**Objective**: Achieve complete self-hosting capability.

#### Task 5.1: Compile All Source Files
- **Description**: Verify all 23 source files compile without errors
- **Test**: `find src -name "*.c" | xargs ./compiler -c` succeeds

#### Task 5.2: Link Complete Compiler
- **Description**: Produce working compiler from self-compiled objects
- **Test**: `./compiler --version` works

#### Task 5.3: Run Self-Compiled Compiler Tests
- **Description**: Self-compiled compiler passes test suite
- **Test**: `./compiler tests/*.c -o /tmp/test && /tmp/test` passes

---

## Task Dependencies

```
Sprint 1 (WASM Control Flow)
├── Task 1.1: Block Structure (Foundation)
├── Task 1.2: Label Resolution (depends on 1.1)
├── Task 1.3: Comparison Values (depends on 1.2)
├── Task 1.4: Argument Passing (depends on 1.3)
└── Task 1.5: Remove Comments (depends on 1.2)

Sprint 2 (WASM Memory)
├── Task 2.1: LOAD/STORE (depends on Sprint 1)
├── Task 2.2: Memory Declaration (parallel with 2.1)
└── Task 2.3: Array Index (depends on 2.1)

Sprint 3 (Self-Hosting Infrastructure)
├── Task 3.1: Fix Test Script (can parallel)
├── Task 3.2: Link Test (depends on all source files compiling)
└── Task 3.3: Bootstrap Script (depends on 3.2)

Sprint 4 (Test Suite)
└── All tasks depend on Sprint 1 & 2 completion

Sprint 5 (Self-Hosting)
└── All tasks depend on previous sprints
```

---

## Success Criteria

### Sprint 1 Complete When:
- [ ] `wasm_test_cmp` passes
- [ ] `wasm_test_loop` passes
- [ ] `wasm_test_memory` passes
- [ ] All generated WAT is valid (passes `wat2wasm`)

### Sprint 2 Complete When:
- [ ] Memory operations work correctly
- [ ] Array access produces correct results
- [ ] No memory-related errors in tests

### Sprint 3 Complete When:
- [ ] Self-hosting test script runs correctly
- [ ] All source files compile
- [ ] Objects can link into executable

### Sprint 4 Complete When:
- [ ] 6/6 WASM tests pass
- [ ] No regressions in other tests
- [ ] Documentation updated

### Sprint 5 Complete When:
- [ ] Full self-hosting achieved
- [ ] Self-compiled compiler functional
- [ ] All tests pass with self-compiled compiler

---

## Risk Mitigation

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| WASM spec complexity | Medium | Medium | Use MVP features first |
| Label resolution bugs | High | Medium | Incremental testing |
| Test infrastructure issues | Low | Low | Fix script early |
| Regression in working features | High | Low | Comprehensive test suite |

---

## Timeline

| Week | Sprint | Focus | Key Deliverable |
|------|--------|-------|-----------------|
| 1 | Sprint 1 | WASM Control Flow | All WASM tests pass |
| 2 | Sprint 2 | WASM Memory | Memory ops work |
| 2-3 | Sprint 3 | Self-Hosting Infra | Test framework ready |
| 3 | Sprint 4 | Test Suite | Full validation |
| 4 | Sprint 5 | Completion | Self-hosting works |
