# Technical Debt Remediation & Self-Hosting Epic
## C Compiler Project

**Epic ID**: COMPILER-2026-001  
**Status**: Planning Complete  
**Duration**: 12 weeks (6 sprints)  
**Owner**: Development Team

---

## Epic Summary

**Title**: Resolve technical debt and achieve self-hosting for the C compiler

**Goal**: Transform the compiler from a functional prototype with technical debt into a production-quality, self-hosting compiler with clean architecture and comprehensive test coverage.

**Business Value**:
1. Enable compiler to compile itself (educational and practical milestone)
2. Improve maintainability for future development
3. Establish quality standards for the codebase
4. Enable future multi-target support

---

## Sprint Overview

| Sprint | Duration | Focus | Key Deliverable |
|--------|----------|-------|-----------------|
| 1 | Week 1 | Critical Cleanup | Zero DEBUG statements, zero backup files |
| 2 | Weeks 2-3 | WASM Completion | 6/6 WASM tests passing |
| 3 | Weeks 4-5 | Self-Hosting Features | Global variables, struct access, sizeof |
| 4 | Weeks 6-7 | Code Quality | Split god files, standardize memory |
| 5 | Weeks 8-10 | Architecture | Target abstraction layer |
| 6 | Weeks 11-12 | Validation | Self-hosting verification, documentation |

---

## Sprint 1: Critical Cleanup (Week 1)

### Objective
Remove all technical debt artifacts that degrade code quality and professionalism.

### Tasks

#### Task 1.1: Remove DEBUG Statements
- **Description**: Delete all `fprintf(stderr, "DEBUG:...")` statements from source files
- **Files**: `src/sema/analyzer.c`, `src/backend/codegen.c`, `src/parser/parser.c`
- **Effort**: 2 hours
- **Acceptance Criteria**:
  ```bash
  grep -rn "fprintf(stderr, \"DEBUG" src/ | wc -l  # Returns 0
  ```
- **Gate Test**: `grep -rn "fprintf(stderr, \"DEBUG" src/ | wc -l | grep -q "^0$"`

#### Task 1.2: Delete Backup Files
- **Description**: Remove all .bak files from source tree and update .gitignore
- **Files**: `src/driver.c.bak`, `src/main.c.bak2`, `src/main.c.bak3`
- **Effort**: 15 minutes
- **Acceptance Criteria**:
  ```bash
  find src -name "*.bak*" | wc -l  # Returns 0
  grep -q "*.bak" .gitignore
  ```
- **Gate Test**: `find src -name "*.bak*" | wc -l | grep -q "^0$"`

#### Task 1.3: Address BUG Comment
- **Description**: Investigate and fix the documented bug in codegen.c:621
- **File**: `src/backend/codegen.c`
- **Effort**: 4 hours
- **Acceptance Criteria**: Bug fixed or comment updated with resolution
- **Gate Test**: `grep -c "BUG:" src/backend/codegen.c | grep -q "^0$"`

#### Task 1.4: Code Formatting Pass
- **Description**: Run code formatter on modified files
- **Effort**: 1 hour
- **Acceptance Criteria**: Consistent formatting per project style

### Sprint 1 Success Criteria
- [ ] Zero DEBUG statements in source
- [ ] Zero backup files in repository
- [ ] Zero BUG comments remaining
- [ ] All existing tests still pass

---

## Sprint 2: WASM Completion (Weeks 2-3)

### Objective
Complete the WASM backend to enable full test suite passing.

### Tasks

#### Task 2.1: Fix wasm_test_cmp
- **Description**: Fix comparison value generation in WASM backend
- **File**: `src/backend/wasm_codegen.c`
- **Effort**: 2 days
- **Root Cause**: Comparison operands not correctly loaded
- **Acceptance Criteria**:
  ```bash
  ./compiler --target=wasm tests/wasm/wasm_test_cmp.c -o /tmp/cmp.wat
  wat2wasm /tmp/cmp.wat -o /tmp/cmp.wasm
  wasmtime /tmp/cmp.wasm  # Returns expected output
  ```
- **Gate Test**: `make test-wasm 2>&1 | grep -q "wasm_test_cmp.*PASS"`

#### Task 2.2: Fix wasm_test_loop
- **Description**: Fix loop label tracking and br/br_if generation
- **File**: `src/backend/wasm_codegen.c`
- **Effort**: 2 days
- **Root Cause**: Label depth calculation incorrect for nested loops
- **Acceptance Criteria**: Loop test compiles and executes correctly
- **Gate Test**: `make test-wasm 2>&1 | grep -q "wasm_test_loop.*PASS"`

#### Task 2.3: Fix wasm_test_memory
- **Description**: Implement proper memory operations
- **Files**: `src/backend/wasm_codegen.c`, `src/ir/lowerer.c`
- **Effort**: 2 days
- **Root Cause**: IR_LOAD/IR_STORE not mapping correctly to WASM memory ops
- **Acceptance Criteria**: Memory test compiles and accesses array correctly
- **Gate Test**: `make test-wasm 2>&1 | grep -q "wasm_test_memory.*PASS"`

#### Task 2.4: WASM Test Validation
- **Description**: Run full WASM test suite and verify all pass
- **Effort**: 4 hours
- **Acceptance Criteria**: `make test-wasm` shows 6/6 passing
- **Gate Test**: `make test-wasm 2>&1 | grep -q "6 passed, 0 failed"`

### Sprint 2 Success Criteria
- [ ] All 6 WASM tests pass
- [ ] No wat2wasm validation errors
- [ ] Generated WASM executes correctly in wasmtime

---

## Sprint 3: Self-Hosting Features (Weeks 4-5)

### Objective
Implement missing language features required for self-hosting.

### Tasks

#### Task 3.1: Implement Global Variables
- **Description**: Add support for global variable declarations and access
- **Files**: `src/ir/ir.h`, `src/ir/lowerer.c`, `src/backend/*.c`
- **Effort**: 1 week
- **Implementation**:
  1. Add `IR_GLOBAL_LOAD` and `IR_GLOBAL_STORE` opcodes
  2. Track globals in IR module
  3. Generate proper assembly/WASM for globals
- **Acceptance Criteria**: Compile and run program with global variables
- **Gate Test**: 
  ```bash
  echo 'int x = 5; int main() { return x; }' > /tmp/test_global.c
  ./compiler /tmp/test_global.c -o /tmp/test_global && /tmp/test_global
  # Returns exit code 5
  ```

#### Task 3.2: Complete Struct Field Access
- **Description**: Implement `.` and `->` operators for struct member access
- **Files**: `src/parser/parser.c`, `src/ir/lowerer.c`, `src/backend/*.c`
- **Effort**: 1 week
- **Implementation**:
  1. Parse struct member expressions
  2. Calculate field offsets
  3. Generate proper load/store with offset
- **Acceptance Criteria**: Compile and run struct test program
- **Gate Test**:
  ```bash
  cat > /tmp/test_struct.c << 'EOF'
  struct Point { int x; int y; };
  int main() { struct Point p = {1, 2}; return p.x + p.y; }
  EOF
  ./compiler /tmp/test_struct.c -o /tmp/test_struct && /tmp/test_struct
  # Returns exit code 3
  ```

#### Task 3.3: Implement sizeof for Structs
- **Description**: Calculate struct size including padding
- **Files**: `src/sema/analyzer.c`, `src/parser/parser.c`
- **Effort**: 3 days
- **Acceptance Criteria**: `sizeof(struct Foo)` returns correct size
- **Gate Test**:
  ```bash
  cat > /tmp/test_sizeof.c << 'EOF'
  struct S { char a; int b; };
  int main() { return sizeof(struct S); }
  EOF
  ./compiler /tmp/test_sizeof.c -o /tmp/test_sizeof && /tmp/test_sizeof
  # Returns exit code 8 (padded)
  ```

### Sprint 3 Success Criteria
- [ ] Global variables compile and work
- [ ] Struct field access works
- [ ] sizeof() works for all types
- [ ] Self-hosting compilation progresses

---

## Sprint 4: Code Quality (Weeks 6-7)

### Objective
Split monolithic files into maintainable modules.

### Tasks

#### Task 4.1: Split parser.c
- **Description**: Refactor 2,118-line parser into modules
- **Target Structure**:
  ```
  src/parser/
  ├── parser.c        (200 lines - driver)
  ├── parse_expr.c    (500 lines)
  ├── parse_stmt.c    (400 lines)
  ├── parse_decl.c    (500 lines)
  ├── parse_type.c    (300 lines)
  └── parse_utils.c   (200 lines)
  ```
- **Effort**: 1 week
- **Acceptance Criteria**: All tests pass, no behavior change
- **Gate Test**: `make test && find src/parser -name "*.c" | wc -l | grep -q "7"`

#### Task 4.2: Split lowerer.c
- **Description**: Refactor 1,953-line lowerer into modules
- **Target Structure**:
  ```
  src/ir/
  ├── lowerer.c       (300 lines - driver)
  ├── lower_expr.c    (600 lines)
  ├── lower_stmt.c    (400 lines)
  ├── lower_decl.c    (300 lines)
  └── scope.c         (200 lines)
  ```
- **Effort**: 1 week
- **Acceptance Criteria**: All tests pass, no behavior change
- **Gate Test**: `make test && find src/ir -name "lower*.c" | wc -l | grep -q "4"`

#### Task 4.3: Standardize Memory Management
- **Description**: Use `xmalloc`/`xcalloc` consistently everywhere
- **Effort**: 2 days
- **Acceptance Criteria**: No direct `malloc`/`calloc` calls
- **Gate Test**: `grep -rn "malloc\|calloc" src/ | grep -v "xmalloc\|xcalloc\|// " | wc -l | grep -q "^0$"`

### Sprint 4 Success Criteria
- [ ] No file > 1,000 lines
- [ ] All memory allocation uses safe wrappers
- [ ] All tests pass after refactoring

---

## Sprint 5: Architecture (Weeks 8-10)

### Objective
Add target abstraction layer for clean multi-target support.

### Tasks

#### Task 5.1: Define Target Interface
- **Description**: Create abstract target interface
- **File**: `src/target/target.h`
- **Effort**: 2 days
- **Interface**:
  ```c
  typedef struct Target {
      const char *name;
      void (*emit_prologue)(int locals_size);
      void (*emit_epilogue)(int locals_size);
      void (*emit_instruction)(IRInstruction *instr);
      // ... other methods
  } Target;
  ```

#### Task 5.2: Refactor ARM64 as Target
- **Description**: Move ARM64 codegen to target interface
- **Files**: `src/target/arm64/*.c`
- **Effort**: 1 week
- **Acceptance Criteria**: ARM64 tests pass via target interface

#### Task 5.3: Refactor WASM as Target
- **Description**: Move WASM codegen to target interface
- **Files**: `src/target/wasm/*.c`
- **Effort**: 1 week
- **Acceptance Criteria**: WASM tests pass via target interface

#### Task 5.4: Remove ARM64-Specific IR Opcodes
- **Description**: Replace target-specific opcodes with target-independent alternatives
- **Opcodes to Remove**: `IR_SAVE_X8`, `IR_ADD_X21`, `IR_RESTORE_X8_RESULT`, etc.
- **Effort**: 1 week
- **Acceptance Criteria**: IR dump contains no target-specific opcodes

### Sprint 5 Success Criteria
- [ ] Target abstraction layer implemented
- [ ] Both ARM64 and WASM use target interface
- [ ] IR is target-independent
- [ ] All tests pass

---

## Sprint 6: Validation (Weeks 11-12)

### Objective
Verify self-hosting capability and complete documentation.

### Tasks

#### Task 6.1: Self-Hosting Compilation
- **Description**: Compile all compiler source files with itself
- **Effort**: 3 days
- **Acceptance Criteria**: `make self` completes stage1 compiler
- **Gate Test**: `make self 2>&1 | grep -q "Self-hosting bootstrap SUCCESS"`

#### Task 6.2: Bootstrap Verification
- **Description**: Verify stage1 and stage2 produce identical output
- **Effort**: 2 days
- **Acceptance Criteria**: `make self-verify` shows convergence
- **Gate Test**: `make self-verify 2>&1 | grep -q "CONVERGENCE: PASS"`

#### Task 6.3: Performance Benchmarking
- **Description**: Compare self-compiled vs GCC-compiled performance
- **Effort**: 2 days
- **Target**: Within 20% of bootstrap compiler

#### Task 6.4: Documentation
- **Description**: Update README, ARCHITECTURE.md, and developer docs
- **Files**: `README.md`, `docs/ARCHITECTURE.md`, `docs/CONTRIBUTING.md`
- **Effort**: 2 days

### Sprint 6 Success Criteria
- [ ] Compiler self-compiles successfully
- [ ] Bootstrap verification passes
- [ ] Performance within target
- [ ] Documentation complete

---

## Dependencies

```
Sprint 1 ──────────────────────────────────────────────────────┐
                                                                │
Sprint 2 ──────────────────────────────────────────────────────┤
                                                                │
Sprint 3 ──────────────────────────────────────────────────────┤
                                                                │
Sprint 4 ──────────────────────────────────────────────────────┤
                                                                │
Sprint 5 ──────────────────────────────────────────────────────┤
                                                                │
Sprint 6 ──────────────────────────────────────────────────────┘
```

All sprints are sequential due to dependencies. Each sprint must be complete before the next begins.

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Refactoring introduces bugs | Medium | High | Comprehensive test suite, incremental commits |
| Self-hosting blocked by edge case | Medium | Critical | Early identification, prioritize blocking issues |
| Schedule overrun | Medium | Medium | Buffer time in each sprint |
| Knowledge silo | Low | High | Documentation, pair programming |

---

## Success Metrics

| Metric | Baseline | Target | Sprint |
|--------|----------|--------|--------|
| DEBUG statements | 20+ | 0 | 1 |
| Backup files | 3 | 0 | 1 |
| WASM tests passing | 3/6 | 6/6 | 2 |
| Files > 1000 lines | 3 | 0 | 4 |
| Self-hosting | Incomplete | Complete | 6 |

---

## Stakeholders

- **Development Team**: Implementation
- **QA**: Testing and validation
- **Documentation**: Technical writing
- **Project Lead**: Oversight and planning

---

## Approval

- [ ] Architecture review complete
- [ ] Sprint plan approved
- [ ] Resource allocation confirmed
- [ ] Timeline agreed

---

## Appendix A: Glossary

- **Self-hosting**: Compiler able to compile its own source code
- **Bootstrap**: Process of building compiler with itself
- **Stage1**: Compiler built by bootstrap (GCC/clang)
- **Stage2**: Compiler built by Stage1
- **Convergence**: Stage1 and Stage2 producing identical output

## Appendix B: References

1. Review: `.project-management/review.md`
2. Refactoring Plan: `.project-management/refactoring-plan.md`
3. Architecture: `.project-management/architecture-analysis.md`
4. Self-hosting Epic: `.project-management/selfhost-epic.md`
5. WASM Epic: `.project-management/wasm-epic.md`

---

*This epic consolidates all technical debt remediation and self-hosting work into a structured, executable plan.*
