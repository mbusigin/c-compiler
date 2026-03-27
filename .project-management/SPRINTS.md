# Detailed Sprint Breakdown

## Sprint 1.1: Fix Stack Frame Management

### Goal
Handle stack frames larger than 504 bytes (ARM64 immediate offset limit)

### Tasks
1. [ ] Audit current stack frame implementation
2. [ ] Implement indirect addressing for large frames
3. [ ] Add dynamic offset calculation
4. [ ] Write regression test for 100+ local variables
5. [ ] Verify all existing tests pass

### Acceptance Criteria
- [ ] Functions with 100 local variables compile correctly
- [ ] Assembly uses indirect addressing for offsets > 504
- [ ] No regression in existing test suite

### Deliverables
- Fixed `src/backend/codegen.c` stack frame handling
- New test in `tests/regression/stack-overflow.c`

---

## Sprint 1.2: Remove Duplicate Epilogues

### Goal
Emit exactly one return sequence per function

### Tasks
1. [ ] Trace control flow to identify duplicate emission
2. [ ] Implement single exit point tracking
3. [ ] Remove duplicate ret_void emission
4. [ ] Verify clean assembly output

### Acceptance Criteria
- [ ] No duplicate `ldp x20, x21...` sequences
- [ ] No unreachable code after `ret`
- [ ] Clean disassembly

### Deliverables
- Fixed control flow emission
- Updated IR lowering to prevent duplicates

---

## Sprint 1.3: Fix Self-Hosting Bootstrap

### Goal
Get stage1 compiler to compile all source files without crashing

### Tasks
1. [ ] Debug stage1 crash with lldb
2. [ ] Fix stack frame issues causing crashes
3. [ ] Handle assembly errors in large files
4. [ ] Verify stage2 converges

### Acceptance Criteria
- [ ] stage1 compiles without segfault
- [ ] stage1 produces valid assembly
- [ ] stage2 produces identical output to stage1

### Deliverables
- Working self-hosting bootstrap
- Convergence verification passing

---

## Sprint 1.4: Add Comprehensive Tests

### Goal
Achieve 90% code coverage

### Tasks
1. [ ] Run coverage analysis
2. [ ] Identify uncovered code paths
3. [ ] Write unit tests for each component
4. [ ] Add integration tests
5. [ ] Set up CI/CD coverage reporting

### Acceptance Criteria
- [ ] 90%+ line coverage
- [ ] 90%+ branch coverage
- [ ] All new tests pass

### Deliverables
- Comprehensive test suite
- Coverage reports in CI

---

## Sprint 2.1: Define IR Semantics

### Goal
Create formal IR specification

### Tasks
1. [ ] Document all IR opcodes with semantics
2. [ ] Define type system formally
3. [ ] Document basic block structure
4. [ ] Create IR verification passes

### Acceptance Criteria
- [ ] IR specification document exists
- [ ] All opcodes documented
- [ ] Type rules formally specified

### Deliverables
- `docs/IR_SPEC.md`
- IR verification pass

---

## Sprint 2.2: Remove Target-Specific IR Opcodes

### Goal
Make IR truly target-independent

### Tasks
1. [ ] Identify all ARM64-specific opcodes
2. [ ] Design target-independent alternatives
3. [ ] Migrate logic to backend
4. [ ] Test both backends

### Acceptance Criteria
- [ ] No IR_SAVE_X8, IR_STORE_INDIRECT in IR
- [ ] WASM and ARM64 backends work identically
- [ ] New test for target abstraction

### Deliverables
- Clean target-independent IR
- Both backends passing tests

---

## Sprint 2.3: Remove Global State

### Goal
Make compiler reentrant via context passing

### Tasks
1. [ ] Audit all static variables in lowerer.c
2. [ ] Design context struct
3. [ ] Refactor to use context
4. [ ] Add thread-safety consideration

### Acceptance Criteria
- [ ] No static variables in lowerer.c
4. [ ] Compiler can be instantiated multiple times
- [ ] All tests pass

### Deliverables
- Context-based architecture
- Unit tests for IR generation

---

## Sprint 3.1: Complete Type System

### Goal
Implement proper C type checking

### Tasks
1. [ ] Remove permissive type compatibility
2. [ ] Implement strict type checking
3. [ ] Add type coercion rules
4. [ ] Improve error messages

### Acceptance Criteria
- [ ] Type errors caught correctly
- [ ] Error messages are helpful
- [ ] Standard compliance tests pass

### Deliverables
- Strict type checker
- Improved error messages

---

## Sprint 3.2: Complete Expression Support

### Goal
Implement all C expressions

### Tasks
1. [ ] Migrate code from lower_expr.c placeholder
2. [ ] Implement comma operator
3. [ ] Implement compound literals
4. [ ] Test complex expressions

### Acceptance Criteria
- [ ] All C11 expressions work
- [ ] Expression precedence correct
- [ ] Puzzle tests for expressions pass

### Deliverables
- Complete expression support
- Expression test suite

---

## Sprint 3.3: Complete Statement Support

### Goal
Implement all C statements

### Tasks
1. [ ] Implement switch/case
2. [ ] Fix do-while loops
3. [ ] Add minimal goto support
4. [ ] Test all statement types

### Acceptance Criteria
- [ ] Switch statement compiles correctly
- [ ] All loop types work
- [ ] Statement tests pass

### Deliverables
- Complete statement support
- Statement test suite

---

## Sprint 3.4: Error Handling

### Goal
Improve error reporting

### Tasks
1. [ ] Fix line/column reporting
2. [ ] Implement multiple error reporting
3. [ ] Add warning count
4. [ ] Test error recovery

### Acceptance Criteria
- [ ] Errors point to correct location
- [ ] Multiple errors reported
- [ ] Warnings displayed

### Deliverables
- Improved error handling
- Error message tests

---

## Sprint 4.1: Bootstrap Infrastructure

### Goal
Set up self-hosting build system

### Tasks
1. [ ] Verify make self target works
2. [ ] Implement convergence testing
3. [ ] Add bootstrap verification
4. [ ] Document bootstrap process

### Acceptance Criteria
- [ ] make self succeeds
- [ ] Convergence.txt shows PASS
- [ ] Documentation complete

### Deliverables
- Working bootstrap
- Documentation

---

## Sprint 4.2: Missing Features for Self-Hosting

### Goal
Implement features required to compile compiler source

### Tasks
1. [ ] Complete typedef support
2. [ ] Implement static functions
3. [ ] Add #include search paths
4. [ ] Test with compiler source

### Acceptance Criteria
- [ ] Compiler source compiles
- [ ] All features work
- [ ] Stage1 builds

### Deliverables
- Self-compiling compiler

---

## Sprint 4.3: Preprocessor Completion

### Goal
Implement full preprocessor

### Tasks
1. [ ] Complete #define support
2. [ ] Implement #ifdef
3. [ ] Add #include guards
4. [ ] Test preprocessor

### Acceptance Criteria
- [ ] All macros work
- [ ] Conditional compilation works
- [ ] Header guards handled

### Deliverables
- Complete preprocessor
- Preprocessor tests

---

## Sprint 4.4: Library Functions

### Goal
Provide standard library support

### Tasks
1. [ ] Implement stdlib.h
2. [ ] Implement stdio.h
3. [ ] Implement string.h
4. [ ] Test with real code

### Acceptance Criteria
- [ ] malloc/free work
- [ ] printf works
- [ ] String functions work

### Deliverables
- Standard library support
- Library function tests

---

## Sprint 4.5: Performance Benchmarking

### Goal
Measure and optimize compiler performance

### Tasks
1. [ ] Create benchmark suite
2. [ ] Measure current performance
3. [ ] Identify bottlenecks
4. [ ] Optimize hot paths

### Acceptance Criteria
- [ ] Benchmarks run
- [ ] Performance measured
- [ ] Gap vs GCC known

### Deliverables
- Performance report
- Optimization improvements

---

## Sprint 4.6: Documentation Update

### Goal
Document self-hosting process

### Tasks
1. [ ] Write self-hosting guide
2. [ ] Update README
3. [ ] Create contributing guide
4. [ ] Update architecture docs

### Acceptance Criteria
- [ ] Self-hosting documented
- [ ] README is current
- [ ] Contributing guide exists

### Deliverables
- Complete documentation
- Contribution guidelines

---

## Burndown Chart (Ideal)

```
Sprint: 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17
Story:  10 10 10 10  8  8  8  8  8  8  8  8  8  8  8  8  8
        |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
        v  v  v  v  v  v  v  v  v  v  v  v  v  v  v  v  v
        90 80 70 60 52 44 36 28 20 12  4  0  (epic 1+2 done)
        
        120 110 100 90 82 74 66 58 50 42 34 26 18 10  2  0 (all done)
```

---

*Document Version: 1.0*
*Created: 2026-03-27*
