# WASM Implementation Sprint Plan

## Overview

This document outlines the phased implementation plan for adding WebAssembly (WASM) target support to the C compiler. The implementation is divided into 5 sprints over approximately 10-12 weeks.

## Sprint 1: Foundation (Weeks 1-2)

### Goals
- Set up WASM backend infrastructure
- Implement basic WAT text emission
- Support simple functions with integer operations

### Tasks

#### Phase 1.1: Backend Infrastructure
- [ ] Create `src/backend/wasm_codegen.h` - Public interface
- [ ] Create `src/backend/wasm_codegen.c` - Main codegen module
- [ ] Create `src/backend/wasm_emit.c` - WAT text emission helpers
- [ ] Create `src/backend/wasm_emit.h` - Emission interface
- [ ] Add WASM backend to Makefile build

#### Phase 1.2: Basic Code Generation
- [ ] Implement module header/footer emission
- [ ] Implement function signature emission
- [ ] Implement basic block structure
- [ ] Support integer constants (i32.const, i64.const)
- [ ] Support integer arithmetic (add, sub, mul, div)

#### Phase 1.3: Driver Integration
- [ ] Add `--target` option to driver
- [ ] Add WASM target enum to CompileOptions
- [ ] Route WASM output to codegen

### Deliverables
- Compiler can emit valid WAT for simple arithmetic functions
- `./compiler --target=wasm simple.c -o out.wat` produces valid WAT
- wat2wasm validation passes

### Test Cases
```c
// test_add.c
int add(int a, int b) {
    return a + b;
}
```

---

## Sprint 2: Control Flow (Weeks 3-4)

### Goals
- Implement control flow constructs
- Support conditionals and loops
- Handle basic block jumps

### Tasks

#### Phase 2.1: Conditional Branches
- [ ] Implement IR_JMP_IF to br_if conversion
- [ ] Implement IR_JMP to br conversion
- [ ] Handle label tracking for branch targets
- [ ] Support if/else/endif blocks

#### Phase 2.2: Comparison Operations
- [ ] Implement all comparison ops (lt, gt, le, ge, eq, ne)
- [ ] Support signed/unsigned variants
- [ ] Handle comparison result normalization (0/1)

#### Phase 2.3: Loop Constructs
- [ ] Support loop blocks
- [ ] Handle back-edges (loop continuation)
- [ ] Implement break/continue patterns

### Deliverables
- Compiler can emit WAT for functions with if/else and while loops
- Control flow validation passes with wat2wasm

### Test Cases
```c
// test_if.c
int max(int a, int b) {
    if (a > b) return a;
    return b;
}

// test_loop.c
int sum(int n) {
    int s = 0;
    while (n > 0) {
        s += n;
        n--;
    }
    return s;
}
```

---

## Sprint 3: Memory and Variables (Weeks 5-6)

### Goals
- Implement local variable support
- Add memory operations
- Support stack frame management

### Tasks

#### Phase 3.1: Local Variables
- [ ] Implement IR_ALLOCA for local allocation
- [ ] Map locals to WASM locals
- [ ] Support IR_LOAD_STACK / IR_STORE_STACK
- [ ] Handle variable scoping

#### Phase 3.2: Memory Operations
- [ ] Implement IR_LOAD / IR_STORE
- [ ] Support memory.load and memory.store
- [ ] Handle pointer arithmetic
- [ ] Implement linear memory declaration

#### Phase 3.3: Stack Frame
- [ ] Implement function prologue
- [ ] Implement function epilogue
- [ ] Support frame pointer management
- [ ] Handle nested function calls

### Deliverables
- Compiler can emit WAT for functions with local variables
- Memory operations work correctly
- Multi-function programs compile

### Test Cases
```c
// test_locals.c
int factorial(int n) {
    int result = 1;
    while (n > 1) {
        result *= n;
        n--;
    }
    return result;
}

// test_memory.c
int array_sum(int *arr, int len) {
    int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += arr[i];
    }
    return sum;
}
```

---

## Sprint 4: Functions and I/O (Weeks 7-8)

### Goals
- Implement function calls
- Add I/O support via imports
- Support string literals

### Tasks

#### Phase 4.1: Function Calls
- [ ] Implement IR_CALL emission
- [ ] Handle parameter passing
- [ ] Handle return values
- [ ] Support multiple arguments

#### Phase 4.2: Function Imports
- [ ] Define import section for host functions
- [ ] Import printf/putchar for I/O
- [ ] Import malloc/free for memory
- [ ] Create JavaScript host bindings

#### Phase 4.3: String Literals
- [ ] Implement IR_CONST_STRING
- [ ] Add data section for strings
- [ ] Calculate string addresses
- [ ] Handle escape sequences

### Deliverables
- Compiler can emit WAT for programs with function calls
- Hello World program runs in wasmtime
- String output works

### Test Cases
```c
// test_call.c
int double_it(int x) {
    return x * 2;
}

int main() {
    return double_it(21);
}

// test_hello.c
#include <stdio.h>

int main() {
    printf("Hello, WASM!\n");
    return 0;
}
```

---

## Sprint 5: Advanced Features and Testing (Weeks 9-10)

### Goals
- Implement remaining IR operations
- Complete test suite
- Documentation and validation

### Tasks

#### Phase 5.1: Advanced Operations
- [ ] Implement floating point operations
- [ ] Implement type conversions (sext, zext, trunc)
- [ ] Implement bitwise operations
- [ ] Support IR_CALL_INDIRECT (function pointers)

#### Phase 5.2: Test Suite
- [ ] Create src/tests/test_wasm.c
- [ ] Create tests/wasm/ directory with test cases
- [ ] Add test-wasm target to Makefile
- [ ] Integrate with CI/CD

#### Phase 5.3: Validation and Documentation
- [ ] Create validation scripts
- [ ] Document usage and limitations
- [ ] Performance benchmarking
- [ ] Known issues and future work

### Deliverables
- Full test suite passes
- Documentation complete
- Compiler self-compilation assessment

### Test Cases
```c
// test_float.c
double square(double x) {
    return x * x;
}

// test_types.c
long long big_add(long long a, long long b) {
    return a + b;
}

// test_ptr.c
int deref(int *p) {
    return *p;
}
```

---

## Phase Summary

| Sprint | Duration | Focus | Key Deliverable |
|--------|----------|-------|-----------------|
| 1 | Weeks 1-2 | Foundation | Basic WAT emission |
| 2 | Weeks 3-4 | Control Flow | Conditionals and loops |
| 3 | Weeks 5-6 | Memory | Local variables, memory ops |
| 4 | Weeks 7-8 | Functions | Calls, I/O, strings |
| 5 | Weeks 9-10 | Complete | Full test suite |

## Risk Mitigation

### Technical Risks
1. **Structured control flow conversion**: May require IR modifications
   - Mitigation: Start with simple patterns, iterate
   
2. **WASM specification changes**: New features may affect design
   - Mitigation: Target MVP features first, extend later

3. **Runtime library complexity**: I/O implementation may be complex
   - Mitigation: Use existing WASM runtime imports

### Schedule Risks
1. **Scope creep**: Feature requests during implementation
   - Mitigation: Strict sprint boundaries, defer to future work

2. **Testing complexity**: WASM runtime setup may be time-consuming
   - Mitigation: Use wat2wasm for syntax validation first

## Success Criteria

- [ ] Compiler produces valid WAT for all test cases
- [ ] wat2wasm validation passes
- [ ] wasmtime execution produces correct output
- [ ] Test suite integrated into CI/CD
- [ ] Documentation complete

## Future Work (Post-Sprint)

- Direct WASM binary output (skip WAT intermediate)
- WASM SIMD support
- Exception handling
- Multi-value returns optimization
- Tail call optimization
- Garbage collection integration (when standardized)
