# Refactoring Plan
## C Compiler Technical Debt Remediation

**Version**: 1.0  
**Date**: March 2026  
**Status**: Planning Complete

---

## Overview

This document outlines a prioritized refactoring plan to address technical debt identified in the Distinguished Engineer review. The plan is organized by priority and includes effort estimates, risk assessments, and verification criteria.

---

## Priority Classification

| Priority | Label | Description | Timeline |
|----------|-------|-------------|----------|
| P0 | Critical | Blocks self-hosting or causes incorrect behavior | 1-3 days |
| P1 | High | Impacts maintainability significantly | 1-2 weeks |
| P2 | Medium | Improves code quality | 2-4 weeks |
| P3 | Low | Nice to have | 1-2 months |

---

## P0: Critical Issues

### P0-1: Remove DEBUG Statements

**Problem**: 20+ DEBUG fprintf statements in production code  
**Impact**: Noisy output, performance overhead, unprofessional appearance  
**Files**:
- `src/sema/analyzer.c` (8 instances)
- `src/backend/codegen.c` (1 instance)
- `src/parser/parser.c` (5 instances)

**Solution**:
```c
// Remove all lines matching:
fprintf(stderr, "DEBUG: ...");
```

**Effort**: 2 hours  
**Risk**: None  
**Verification**:
```bash
grep -rn "fprintf(stderr, \"DEBUG" src/ | wc -l
# Should return 0
```

---

### P0-2: Delete Backup Files

**Problem**: Backup files in source tree  
**Impact**: Version control pollution, developer confusion  
**Files**:
- `src/driver.c.bak`
- `src/main.c.bak2`
- `src/main.c.bak3`

**Solution**:
```bash
rm src/*.bak*
echo "*.bak*" >> .gitignore
echo "*.bak" >> .gitignore
```

**Effort**: 15 minutes  
**Risk**: None  
**Verification**:
```bash
find src -name "*.bak*" | wc -l
# Should return 0
```

---

### P0-3: Fix BUG Comment in codegen.c

**Problem**: Known bug documented but not fixed  
**Location**: `src/backend/codegen.c:621`  
**Comment**: `// BUG: This incorrectly sets type even if x9 was a constant`

**Investigation Needed**: Analyze the context and fix the bug or remove if no longer relevant.

**Effort**: 4 hours (investigation + fix)  
**Risk**: Medium (could affect codegen correctness)  
**Verification**: Run full test suite after fix

---

## P1: High Priority Refactoring

### P1-1: Complete WASM Test Suite

**Problem**: 3 of 6 WASM tests failing  
**Impact**: Self-hosting blocked, incomplete feature  
**Current Status**:
- ✅ `wasm_test_arith`
- ✅ `wasm_test_bitwise`
- ✅ `wasm_test_branch`
- ❌ `wasm_test_cmp`
- ❌ `wasm_test_loop`
- ❌ `wasm_test_memory`

**Root Causes Identified**:
1. Comparison value generation incorrect
2. Loop label tracking issues
3. Memory operations not fully implemented

**Solution**: Follow existing sprint plan in `sprints.md`

**Effort**: 1-2 weeks  
**Risk**: Medium  
**Verification**:
```bash
make test-wasm
# All 6 tests should pass
```

---

### P1-2: Implement Global Variables

**Problem**: `// TODO: Handle global variables` in lowerer.c  
**Impact**: Self-hosting blocked (compiler uses globals)  
**Location**: `src/ir/lowerer.c:1922`

**Solution**:
1. Add IR_GLOBAL_LOAD and IR_GLOBAL_STORE opcodes
2. Add global variable tracking in IR module
3. Implement in both backends

**Effort**: 1 week  
**Risk**: Medium  
**Verification**: Compile test program with globals

---

### P1-3: Fix Struct Field Access

**Problem**: Struct member access incomplete  
**Impact**: Self-hosting blocked (AST uses structs extensively)  
**Status**: Partial - field access `.` and `->` need work

**Solution**:
1. Complete struct field access in parser
2. Add IR_STRUCT_MEMBER opcode
3. Implement in both backends

**Effort**: 1-2 weeks  
**Risk**: Medium  
**Verification**: Compile and run:
```c
struct Point { int x; int y; };
int main() {
    struct Point p = {1, 2};
    return p.x + p.y;
}
```

---

### P1-4: Complete sizeof for Structs

**Problem**: `sizeof(struct)` not fully implemented  
**Impact**: Self-hosting blocked  
**Location**: Semantic analyzer

**Solution**:
1. Calculate struct size during semantic analysis
2. Handle padding and alignment
3. Support nested structs

**Effort**: 3-5 days  
**Risk**: Low  
**Verification**: Test `sizeof` on various struct types

---

## P2: Medium Priority Refactoring

### P2-1: Split parser.c into Modules

**Problem**: 2,118 lines in single file  
**Impact**: Hard to navigate, merge conflicts  
**Current Structure**: Single monolithic file

**Target Structure**:
```
src/parser/
├── parser.h          # Public interface
├── parser.c          # Main parser driver (200 lines)
├── parse_expr.c      # Expression parsing (500 lines)
├── parse_stmt.c      # Statement parsing (400 lines)
├── parse_decl.c      # Declaration parsing (500 lines)
├── parse_type.c      # Type parsing (300 lines)
└── parse_utils.c     # Utilities (200 lines)
```

**Effort**: 1 week  
**Risk**: Medium (large refactor)  
**Verification**: All tests pass, no behavior change

---

### P2-2: Split lowerer.c into Modules

**Problem**: 1,953 lines in single file  
**Impact**: Hard to maintain, test, and understand  
**Current Structure**: Single monolithic file

**Target Structure**:
```
src/ir/
├── ir.h              # IR types
├── ir.c              # IR construction
├── lowerer.h         # Public interface
├── lowerer.c         # Main driver (300 lines)
├── lower_expr.c      # Expression lowering (600 lines)
├── lower_stmt.c      # Statement lowering (400 lines)
├── lower_decl.c      # Declaration lowering (300 lines)
└── scope.c           # Scope management (200 lines)
```

**Effort**: 1-2 weeks  
**Risk**: Medium  
**Verification**: All tests pass, no behavior change

---

### P2-3: Add Target Abstraction Layer

**Problem**: IR contains ARM64-specific opcodes  
**Impact**: WASM backend must "handle" ARM64 instructions  
**Current**: `IR_SAVE_X8`, `IR_ADD_X21`, `IR_RESTORE_X8_RESULT`

**Solution**:
1. Define target interface
2. Move register management to ARM64 backend
3. Replace ARM64 IR opcodes with target-independent alternatives

**Target Structure**:
```
src/target/
├── target.h          # Target interface
├── target.c          # Target factory
├── arm64/
│   ├── arm64.h
│   ├── arm64.c
│   └── regalloc.c
└── wasm/
    ├── wasm.h
    ├── wasm.c
    └── wasm_codegen.c
```

**Effort**: 2-3 weeks  
**Risk**: High (architectural change)  
**Verification**: All tests pass, clean IR dump

---

### P2-4: Standardize Memory Management

**Problem**: Mixed use of `malloc`/`xmalloc`  
**Impact**: Inconsistent error handling  
**Files**: All source files

**Solution**:
1. Use `xmalloc`/`xcalloc` everywhere
2. Add `xrealloc` wrapper
3. Add documentation for memory conventions

**Effort**: 2-3 days  
**Risk**: Low  
**Verification**:
```bash
grep -rn "malloc\|calloc\|realloc" src/ | grep -v "xmalloc\|xcalloc\|xrealloc\|// " | wc -l
# Should return 0
```

---

## P3: Low Priority Improvements

### P3-1: Implement Graph Coloring Register Allocation

**Problem**: Current register allocation is basic  
**Impact**: Suboptimal code generation  
**Location**: `src/backend/regalloc.c`

**Solution**: Implement graph coloring algorithm

**Effort**: 2-4 weeks  
**Risk**: Medium  
**Verification**: Performance benchmarks

---

### P3-2: Add SSA-Based IR

**Problem**: Current IR is not in SSA form  
**Impact**: Limits optimization opportunities  

**Solution**: Convert to SSA IR with phi nodes

**Effort**: 4-6 weeks  
**Risk**: High  
**Verification**: SSA validation passes

---

### P3-3: Add Debug Information Generation

**Problem**: No DWARF debug info  
**Impact**: Hard to debug compiled programs  

**Solution**: Implement DWARF generation in backend

**Effort**: 2-3 weeks  
**Risk**: Low  
**Verification**: GDB can set breakpoints in compiled code

---

## Implementation Schedule

### Sprint 1: Critical Cleanup (Week 1)

| Task | Priority | Effort | Owner |
|------|----------|--------|-------|
| Remove DEBUG statements | P0 | 2h | - |
| Delete backup files | P0 | 15m | - |
| Fix BUG comment | P0 | 4h | - |
| Complete WASM tests (start) | P1 | - | - |

### Sprint 2: WASM Completion (Week 2-3)

| Task | Priority | Effort | Owner |
|------|----------|--------|-------|
| Fix wasm_test_cmp | P1 | 2d | - |
| Fix wasm_test_loop | P1 | 2d | - |
| Fix wasm_test_memory | P1 | 2d | - |
| Verification | P1 | 1d | - |

### Sprint 3: Self-Hosting Features (Week 4-5)

| Task | Priority | Effort | Owner |
|------|----------|--------|-------|
| Implement global variables | P1 | 1w | - |
| Complete struct field access | P1 | 1w | - |
| sizeof for structs | P1 | 3d | - |

### Sprint 4: Code Quality (Week 6-8)

| Task | Priority | Effort | Owner |
|------|----------|--------|-------|
| Split parser.c | P2 | 1w | - |
| Split lowerer.c | P2 | 1-2w | - |
| Standardize memory mgmt | P2 | 2d | - |

### Sprint 5: Architecture (Week 9-12)

| Task | Priority | Effort | Owner |
|------|----------|--------|-------|
| Target abstraction layer | P2 | 2-3w | - |
| Remove ARM64 IR opcodes | P2 | 1w | - |
| Verification | P2 | 1w | - |

---

## Verification Strategy

### Continuous Verification

After each refactoring step:
1. Run full test suite: `make test`
2. Run WASM tests: `make test-wasm`
3. Check for regressions in generated code

### Quality Gates

| Gate | Metric | Threshold |
|------|--------|-----------|
| Test Coverage | Pass rate | 100% puzzle, 95%+ comprehensive |
| Code Quality | DEBUG statements | 0 |
| Code Quality | Backup files | 0 |
| Architecture | Target-specific IR | 0 ARM64 opcodes |

---

## Rollback Plan

For each refactoring:
1. Create feature branch
2. Make incremental commits
3. Run tests after each commit
4. If tests fail, identify and fix before continuing
5. If unfixable, revert to main branch

---

## Success Metrics

| Metric | Current | Target | Timeline |
|--------|---------|--------|----------|
| DEBUG statements | 20+ | 0 | Week 1 |
| Backup files | 3 | 0 | Week 1 |
| WASM tests passing | 3/6 | 6/6 | Week 3 |
| Self-hosting | Incomplete | Complete | Week 12 |
| God files (>1000 lines) | 3 | 0 | Week 8 |

---

## Appendix: File Line Counts

**Before Refactoring**:
```
src/parser/parser.c      2,118 lines
src/ir/lowerer.c         1,953 lines
src/backend/codegen.c      909 lines
```

**After Refactoring (Target)**:
```
src/parser/parser.c        200 lines (driver)
src/parser/parse_expr.c    500 lines
src/parser/parse_stmt.c    400 lines
src/parser/parse_decl.c    500 lines
src/parser/parse_type.c    300 lines
src/parser/parse_utils.c   200 lines

src/ir/lowerer.c           300 lines (driver)
src/ir/lower_expr.c        600 lines
src/ir/lower_stmt.c        400 lines
src/ir/lower_decl.c        300 lines
src/ir/scope.c             200 lines
```

---

*This refactoring plan is designed to be executed incrementally with continuous verification at each step.*
