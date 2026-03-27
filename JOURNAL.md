# Distinguished Engineer Review: C Compiler Project

## Executive Summary

This is a comprehensive code and architecture review of a C compiler project written from scratch. The compiler implements a traditional compiler pipeline (Lexer → Parser → Semantic Analyzer → IR → Optimizer → Code Generator) with support for ARM64 and WebAssembly backends.

**Overall Assessment**: The project demonstrates solid compiler construction fundamentals but suffers from architecture debt, premature self-hosting focus, and iterative debugging patterns that indicate missing upfront design.

---

## Project Overview

| Attribute | Value |
|-----------|-------|
| **Author** | Matt Busigin |
| **Language** | C11 |
| **Lines of Code** | ~11,600 lines |
| **Source Files** | 36 C files |
| **Architecture** | Traditional compiler pipeline |
| **Targets** | ARM64, WebAssembly |
| **Test Coverage** | 52 regression tests, 75+ puzzle tests |
| **Status** | Functional (self-hosting blocked) |

---

## Architecture Analysis

### Strengths

1. **Target Abstraction**: Clean separation between IR and backends via target interface
2. **Modular Structure**: Components well-organized by compiler phase
3. **Comprehensive Testing**: Large test suite with categorized puzzle tests
4. **IR Design**: Well-defined intermediate representation with basic blocks

### Weaknesses

1. **Global State**: Multiple static variables in lowerer.c for state management
2. **Hybrid Architecture**: IR contains ARM64-specific opcodes (IR_SAVE_X8, IR_STORE_INDIRECT)
3. **Incomplete Refactoring**: lower_expr.c is a placeholder file
4. **Self-Hosting Tunnel Vision**: Features added for self-hosting compromise type system

---

## Code Smells Identified

### 1. Global State Abuse
```c
// src/ir/lowerer.c
static IRModule *current_module = NULL;
static IRFunction *current_function = NULL;
static IRBasicBlock *current_block = NULL;
static IRValue *param_values[16];
static char *param_names[16];
static LocalVar *locals_hash[LOCALS_BUCKETS];
static Scope scope_stack[SCOPE_STACK_DEPTH];
static LoopContext *loop_stack = NULL;
```

### 2. Permissive Type System for Self-Hosting
```c
// src/sema/analyzer.c
static bool types_compatible(...) {
    // For self-hosting, be very permissive
    return true;  // Bypasses type safety
}
```

### 3. Hardcoded Builtin Types
```c
// src/parser/parser.c
static bool is_builtin_type(const char *name) {
    // 50+ special cases for self-hosting
    return strcmp(name, "Type") == 0 ||
           strcmp(name, "ASTNode") == 0 ||
           // ... 48 more cases
}
```

### 4. IR Opcode Explosion
The IR contains 50+ opcodes, including ARM64-specific ones:
- IR_SAVE_X8, IR_SAVE_X8_TO_X20, IR_SAVE_X8_TO_X22
- IR_STORE_INDIRECT, IR_STORE_INDIRECT_X22
- IR_ADD_X21, IR_ADD_IMM64

This violates the target-independence principle.

### 5. Duplicate Epilogue Emission
The code generator emits return sequences twice due to:
1. Explicit `ret` instruction
2. Implicit `ret_void` at function end

---

## Git History Pattern Analysis

### Commit Pattern
- **30+ commits** over 5 days
- Heavy use of "Fix" in messages (15+ fix commits)
- Large refactoring commits (3a51a11: 41 files changed)
- Deleted documentation files (.project-management/*)

### Anti-Patterns Identified

1. **OODA Loop Iteration Comments**: "OODA iteration 1", "OODA iteration 3"
2. **Rapid Trial-and-Error**: Each fix reveals new gaps
3. **Documentation Churn**: Planning docs started then abandoned
4. **Self-Hosting Premature Optimization**: Tests pass (52/52) but self-hosting blocked

### Root Cause
Missing upfront architecture specification. The author is building without a complete design document, leading to iterative discovery of issues.

---

## Bugs Found and Fixed

### Bug 1: IR_ALLOCA for Parameters (CRITICAL)
**Location**: `src/ir/lowerer.c`
**Issue**: Parameters generated IR_ALLOCA instructions, causing duplicate stack allocations
**Impact**: Stack frame misalignment, crashes
**Fix**: Removed IR_ALLOCA emission for parameters; use IR_STORE_PARAM only

### Bug 2: Parameter Stack Space Calculation (CRITICAL)
**Location**: `src/backend/codegen.c`
**Issue**: After removing IR_ALLOCA, codegen didn't count parameter stack slots
**Fix**: Added IR_STORE_PARAM offset tracking for locals_size calculation

### Bug 3: WASM Drop Instruction (HIGH)
**Location**: `src/backend/wasm_codegen.c`
**Issue**: Unconditional `drop` after IR_RESTORE_X8_RESULT when nothing on stack
**Fix**: Removed unnecessary `drop` emission

### Bug 4: WASM Loop Code Generation (HIGH)
**Location**: `src/backend/wasm_codegen.c`
**Issue**: Invalid WAT structure for loops, duplicate local.set instructions
**Fix**: Fixed loop structure reconstruction algorithm

---

## Remaining Issues

### Issue 1: Stage1 Compiler Segfault
The self-hosted compiler crashes when compiling source files.
- **Root Cause**: Large stack frames exceed ARM64 immediate offset range ([-512, 504])
- **Status**: Unresolved - requires fundamental stack frame redesign

### Issue 2: Duplicate Return Sequences
Generated assembly contains unreachable duplicate epilogues.
- **Root Cause**: Explicit `ret` + implicit `ret_void`
- **Status**: Harmless but indicates incomplete control flow handling

### Issue 3: IR Target Independence Violation
ARM64-specific opcodes in IR violate target abstraction.
- **Status**: Design issue, would require IR redesign

---

## Test Results

### Before Fixes
| Test Suite | Passed | Failed |
|------------|--------|--------|
| Comprehensive | 48/52 | 4 |
| Puzzle | 68 | 0 |
| WASM | 5/6 | 1 |

### After Fixes
| Test Suite | Passed | Failed |
|------------|--------|--------|
| Comprehensive | 52/52 | 0 |
| Puzzle | 68 | 0 |
| WASM | 6/6 | 0 |

---

## Recommendations

### Short Term (1-2 weeks)

1. **Fix Stack Frame Management**
   - Implement dynamic stack frame for large locals
   - Use indirect addressing for offsets > 504

2. **Add Regression Tests**
   - Before each bug fix, add a failing test
   - Use test-driven development

3. **Complete Refactoring**
   - Migrate code from lower_expr.c placeholder
   - Remove global state via context passing

### Medium Term (1-2 months)

1. **Define IR Semantics**
   - Write formal IR specification
   - Remove target-specific opcodes from IR
   - Separate target code generation from IR

2. **Build Type System Properly**
   - Remove "self-hosting permissive" mode
   - Implement proper type checking

3. **Document Architecture**
   - Create ARCHITECTURE.md
   - Document IR semantics
   - Document code generation approach

### Long Term (3+ months)

1. **Self-Hosting as Milestone**
   - Current focus is premature
   - Build stable compiler first
   - Self-hosting as verification, not goal

2. **Additional Backends**
   - x86_64 support
   - RISC-V consideration

3. **Performance Optimization**
   - Better register allocation
   - Peephole optimization
   - Benchmark against GCC

---

## Sprint Epics (for future work)

### Epic 1: Stabilize Compiler (4 sprints)
- Fix stack frame issues
- Remove global state
- Add comprehensive tests

### Epic 2: Architecture Refactoring (3 sprints)
- Define IR semantics formally
- Remove target-specific IR opcodes
- Implement proper context passing

### Epic 3: Feature Completion (4 sprints)
- Complete C99/C11 feature support
- Better error messages
- Improved optimization

### Epic 4: Self-Hosting (6 sprints)
- Bootstrap verification
- Performance benchmarking
- Documentation update

---

## Conclusion

This C compiler project demonstrates solid fundamentals in compiler construction but suffers from architecture debt accumulated through iterative debugging without upfront design. The author shows high velocity (30 commits in 5 days) but the pattern of "fix bug → reveal new bug" indicates missing formal specifications.

**Key Takeaway**: Build the architecture document BEFORE writing code. A compiler's IR and type system must be formally defined before implementation begins.

The fixes applied resolve the immediate test failures, but the fundamental architecture issues (global state, target-specific IR, permissive type system) remain and will continue to cause problems until addressed.

---

## Appendix: Files Modified

1. `src/ir/lowerer.c` - Removed IR_ALLOCA for parameters
2. `src/backend/codegen.c` - Fixed locals_size calculation
3. `src/backend/wasm_codegen.c` - Removed unnecessary drop instructions

---

*Review Date: 2026-03-27*
*Reviewer: Distinguished Engineer Assessment*
*Project Status: Functional, requires architecture refactoring*
