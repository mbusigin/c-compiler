# Project Management: C Compiler Refactoring

## Overview

This document outlines the epics and sprints for implementing the recommendations from the Distinguished Engineer review.

## Project Vision

A stable, well-architected C compiler with:
- Clean target-independent IR
- Proper type system
- Comprehensive test coverage
- Successful self-hosting bootstrap

---

## Epic 1: Stabilize Compiler Core (4 sprints)

**Goal**: Fix fundamental issues preventing reliable compilation

### Sprint 1.1: Fix Stack Frame Management
**Duration**: 1 week
**Stories**:
- As a compiler, I need to handle stack frames > 504 bytes so large functions compile correctly
- As a compiler, I need dynamic offset calculation so I don't exceed ARM64 limits
- As a developer, I need a test for large local variable functions

**Acceptance Criteria**:
- Functions with > 60 local variables compile correctly
- All existing tests pass
- New regression test added

### Sprint 1.2: Remove Duplicate Epilogues
**Duration**: 3 days
**Stories**:
- As a compiler, I need to emit exactly one return sequence so generated code is clean
- As a developer, I need to understand when ret_void is emitted

**Acceptance Criteria**:
- No duplicate return sequences in assembly
- Control flow analysis shows single exit point per function

### Sprint 1.3: Fix Self-Hosting Bootstrap
**Duration**: 1 week
**Stories**:
- As a compiler, I need working stage1 so I can verify self-compilation
- As a developer, I need convergence verification so I know the compiler is stable

**Acceptance Criteria**:
- stage1 compiles all source files
- stage2 converges with stage1
- Self-hosting test passes

### Sprint 1.4: Add Comprehensive Tests
**Duration**: 1 week
**Stories**:
- As a developer, I need test coverage for edge cases so bugs are caught early
- As a developer, I need integration tests so I verify the full pipeline

**Acceptance Criteria**:
- 90% code coverage
- All regression tests pass
- Integration tests for lexer → parser → codegen

---

## Epic 2: Architecture Refactoring (3 sprints)

**Goal**: Establish clean architecture with target-independent IR

### Sprint 2.1: Define IR Semantics
**Duration**: 1 week
**Stories**:
- As an architect, I need formal IR semantics so implementation is clear
- As a developer, I need IR documentation so I understand the representation

**Acceptance Criteria**:
- IR specification document created
- All opcodes documented with semantics
- Type system formally defined

### Sprint 2.2: Remove Target-Specific IR Opcodes
**Duration**: 1 week
**Stories**:
- As an architect, I need target-independent IR so backends are swappable
- As a developer, I need to refactor IR_SAVE_X8 so it works for all targets

**Acceptance Criteria**:
- No ARM64-specific opcodes in IR
- All target-specific logic in backend
- WASM and ARM64 backends work identically

### Sprint 2.3: Remove Global State
**Duration**: 1 week
**Stories**:
- As a developer, I need context passing so the compiler is reentrant
- As a developer, I need to refactor lowerer.c so it's testable

**Acceptance Criteria**:
- All global state converted to context structs
- lowerer.c passes without static variables
- Unit tests for IR generation

---

## Epic 3: Feature Completion (4 sprints)

**Goal**: Complete C99/C11 feature support

### Sprint 3.1: Complete Type System
**Duration**: 1 week
**Stories**:
- As a compiler, I need proper type checking so type errors are caught
- As a developer, I need to remove permissive mode so types are enforced

**Acceptance Criteria**:
- Type checking passes standard compliance tests
- No permissive type compatibility bypasses
- Error messages are clear and actionable

### Sprint 3.2: Complete Expression Support
**Duration**: 1 week
**Stories**:
- As a compiler, I need complex expressions so I can compile real code
- As a developer, I need to complete lower_expr.c so it's not a placeholder

**Acceptance Criteria**:
- All C11 expressions parse and compile
- Complex expression test suite passes
- lower_expr.c has actual implementation

### Sprint 3.3: Complete Statement Support
**Duration**: 1 week
**Stories**:
- As a compiler, I need all statement types so control flow works
- As a developer, I need switch/case support so I can compile switch statements

**Acceptance Criteria**:
- Switch/case implemented
- Do-while loops work
- goto support (minimal)

### Sprint 3.4: Error Handling
**Duration**: 1 week
**Stories**:
- As a compiler, I need clear error messages so developers fix code easily
- As a developer, I need error recovery so multiple errors are reported

**Acceptance Criteria**:
- Error messages point to correct line/column
- Multiple errors reported in single run
- Warning count displayed

---

## Epic 4: Self-Hosting (6 sprints)

**Goal**: Achieve working self-hosting bootstrap

### Sprint 4.1: Bootstrap Infrastructure
**Duration**: 1 week
**Stories**:
- As a developer, I need build system support so I can run bootstrap
- As a developer, I need convergence testing so I verify correctness

**Acceptance Criteria**:
- make self runs without errors
- Convergence.txt shows PASS
- Stage0, stage1, stage2 all build

### Sprint 4.2: Missing Features for Self-Hosting
**Duration**: 2 weeks
**Stories**:
- As a compiler, I need typedef support so I can compile type definitions
- As a compiler, I need static function support so I can compile internal functions

**Acceptance Criteria**:
- typedef works correctly
- Static functions have internal linkage
- #include finds standard headers

### Sprint 4.3: Preprocessor Completion
**Duration**: 1 week
**Stories**:
- As a compiler, I need full #define support so macros work
- As a developer, I need #ifdef so conditional compilation works

**Acceptance Criteria**:
- Object-like macros work
- Function-like macros expand correctly
- #ifdef/#ifndef/#endif work

### Sprint 4.4: Library Functions
**Duration**: 1 week
**Stories**:
- As a compiler, I need standard library headers so code compiles
- As a developer, I need malloc/free so dynamic memory works

**Acceptance Criteria**:
- stdlib.h provides malloc, free, etc.
- stdio.h provides printf, etc.
- string.h provides memcpy, etc.

### Sprint 4.5: Performance Benchmarking
**Duration**: 1 week
**Stories**:
- As a developer, I need benchmark suite so I measure compiler speed
- As a developer, I need to compare with GCC so I know my compiler is competitive

**Acceptance Criteria**:
- Benchmark suite runs
- Compiler performance measured
- Performance gap vs GCC quantified

### Sprint 4.6: Documentation Update
**Duration**: 1 week
**Stories**:
- As a developer, I need self-hosting documentation so others understand the process
- As a project, I need README updates so newcomers can contribute

**Acceptance Criteria**:
- Self-hosting documented
- Architecture diagram updated
- Contributing guide written

---

## Timeline Summary

| Epic | Duration | Total Sprints |
|------|----------|---------------|
| Epic 1: Stabilize | 4 sprints | 4 |
| Epic 2: Architecture | 3 sprints | 7 |
| Epic 3: Features | 4 sprints | 11 |
| Epic 4: Self-Hosting | 6 sprints | 17 |

**Estimated Total**: 17 sprints (~4 months at 4 sprints/month)

---

## Definition of Done

For each sprint, "done" means:

1. All acceptance criteria met
2. All tests pass (including new regression tests)
3. Code reviewed and approved
4. Documentation updated
5. No known regressions

---

## Risks and Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| IR redesign breaks existing code | High | Medium | Comprehensive test coverage |
| Self-hosting requires many missing features | High | High | Incremental approach, validate each sprint |
| Performance gap vs GCC is large | Low | Medium | Acceptable for initial version |
| Developer burnout from refactoring | Medium | Medium | Regular retrospectives, sustainable pace |

---

## Success Metrics

| Metric | Target | Current |
|--------|--------|---------|
| Test Coverage | 90% | ~70% |
| Self-Hosting | PASS | FAIL |
| Regression Failures | 0 | 0 |
| Code Coverage | 90% | ~75% |
| Architecture Documentation | Complete | Partial |

---

*Document Version: 1.0*
*Created: 2026-03-27*
*Author: Distinguished Engineer Review*
