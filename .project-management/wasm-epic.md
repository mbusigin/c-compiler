# WASM Implementation Epic

## Epic Summary

**Title**: Add fully functioning and tested support for WebAssembly (WASM) target

**Goal**: Extend the C compiler to generate WebAssembly binaries, enabling C programs to run in browsers and WASM runtimes.

**Duration**: 10-12 weeks (5 sprints)

**Status**: Planning Complete

---

## Business Value

### Why WASM Support?

1. **Portability**: Run C programs in browsers, servers, and edge devices
2. **Security**: WASM sandboxing provides memory safety
3. **Performance**: Near-native execution speed
4. **Ecosystem**: Access to WebAssembly's growing ecosystem
5. **Education**: Demonstrate compiler multi-target capabilities

### Success Metrics

- [ ] Compiler produces valid WAT for all test cases
- [ ] wat2wasm validation passes 100%
- [ ] wasmtime execution produces correct output
- [ ] Test suite integrated into CI/CD
- [ ] Documentation complete and usable

---

## Sprint Overview

| Sprint | Duration | Focus | Key Deliverable |
|--------|----------|-------|-----------------|
| 1 | Weeks 1-2 | Foundation | Basic WAT emission |
| 2 | Weeks 3-4 | Control Flow | Conditionals and loops |
| 3 | Weeks 5-6 | Memory | Local variables, memory ops |
| 4 | Weeks 7-8 | Functions | Calls, I/O, strings |
| 5 | Weeks 9-10 | Complete | Full test suite |

---

## Task Breakdown

### Sprint 1: Foundation

#### Tasks
- [ ] #8 Research WASM specification, binary format, and design the WASM backend architecture
- [ ] #11 Design WASM code generator architecture and IR mapping specification
- [ ] Create `src/backend/wasm_codegen.h` - Public interface
- [ ] Create `src/backend/wasm_codegen.c` - Main codegen module
- [ ] Create `src/backend/wasm_emit.c/h` - WAT text emission helpers
- [ ] Implement module header/footer emission
- [ ] Implement function signature emission
- [ ] Support integer constants and arithmetic
- [ ] Add WASM backend to Makefile build

#### Deliverables
- Compiler can emit valid WAT for simple arithmetic functions
- `./compiler --target=wasm simple.c -o out.wat` produces valid WAT
- wat2wasm validation passes

---

### Sprint 2: Control Flow

#### Tasks
- [ ] Implement IR_JMP_IF to br_if conversion
- [ ] Implement IR_JMP to br conversion
- [ ] Handle label tracking for branch targets
- [ ] Support if/else/endif blocks
- [ ] Implement all comparison ops (lt, gt, le, ge, eq, ne)
- [ ] Support loop constructs
- [ ] Handle back-edges (loop continuation)

#### Deliverables
- Compiler can emit WAT for functions with if/else and while loops
- Control flow validation passes with wat2wasm

---

### Sprint 3: Memory and Variables

#### Tasks
- [ ] Implement IR_ALLOCA for local allocation
- [ ] Map locals to WASM locals
- [ ] Support IR_LOAD_STACK / IR_STORE_STACK
- [ ] Implement IR_LOAD / IR_STORE
- [ ] Support memory.load and memory.store
- [ ] Handle pointer arithmetic
- [ ] Implement linear memory declaration
- [ ] Implement function prologue/epilogue

#### Deliverables
- Compiler can emit WAT for functions with local variables
- Memory operations work correctly
- Multi-function programs compile

---

### Sprint 4: Functions and I/O

#### Tasks
- [ ] Implement IR_CALL emission
- [ ] Handle parameter passing and return values
- [ ] Define import section for host functions
- [ ] Import printf/putchar for I/O
- [ ] Implement IR_CONST_STRING
- [ ] Add data section for strings
- [ ] Calculate string addresses
- [ ] Create JavaScript host bindings

#### Deliverables
- Compiler can emit WAT for programs with function calls
- Hello World program runs in wasmtime
- String output works

---

### Sprint 5: Advanced Features and Testing

#### Tasks
- [ ] Implement floating point operations
- [ ] Implement type conversions (sext, zext, trunc)
- [ ] Implement bitwise operations
- [ ] Support IR_CALL_INDIRECT (function pointers)
- [ ] Create `src/tests/test_wasm.c`
- [ ] Create `tests/wasm/` directory with test cases
- [ ] Add test-wasm target to Makefile
- [ ] Create validation scripts
- [ ] Document usage and limitations

#### Deliverables
- Full test suite passes
- Documentation complete
- Compiler self-compilation assessment

---

## Planning Documents

| Document | Purpose | Location |
|----------|---------|----------|
| Architecture Analysis | Current compiler structure | `.project-management/architecture-analysis.md` |
| WASM Design | Backend architecture | `.project-management/wasm-design.md` |
| Sprint Plan | Implementation timeline | `.project-management/wasm-sprints.md` |
| Test Strategy | Testing approach | `.project-management/wasm-test-strategy.md` |
| Codegen Spec | IR mapping details | `.project-management/wasm-codegen-spec.md` |
| Module Design | Export/import conventions | `.project-management/wasm-module-design.md` |
| Driver Interface | CLI extensions | `.project-management/wasm-driver-interface.md` |
| Build System | Makefile changes | `.project-management/wasm-build-system.md` |
| Test Cases | Test specifications | `.project-management/wasm-test-cases.md` |
| Integration Tests | Test framework | `.project-management/wasm-integration-tests.md` |
| Tools Survey | Tool recommendations | `.project-management/wasm-tools-survey.md` |
| Validation Workflow | CI/CD integration | `.project-management/wasm-validation-workflow.md` |
| Risk Assessment | Risk mitigation | `.project-management/wasm-risks.md` |

---

## Technical Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      C Source Code                          │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    Compiler Frontend                        │
│  (Lexer → Parser → Semantic Analysis → IR Generation)       │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    IR Module                                │
│  (Functions, Basic Blocks, Instructions, Values)            │
└─────────────────────────────────────────────────────────────┘
                            │
            ┌───────────────┴───────────────┐
            │                               │
            ▼                               ▼
┌───────────────────────┐       ┌───────────────────────┐
│   ARM64 Backend       │       │    WASM Backend       │
│   (codegen.c)         │       │   (wasm_codegen.c)    │
│   → Assembly          │       │   → WAT/WASM          │
└───────────────────────┘       └───────────────────────┘
            │                               │
            ▼                               ▼
┌───────────────────────┐       ┌───────────────────────┐
│   .s (Assembly)       │       │   .wat / .wasm        │
│   → Executable        │       │   → wasmtime/node     │
└───────────────────────┘       └───────────────────────┘
```

### IR to WASM Mapping Summary

| IR Category | WASM Equivalent |
|-------------|-----------------|
| Arithmetic | i32.add, i32.sub, i32.mul, i32.div |
| Comparison | i32.lt_s, i32.gt_s, i32.eq, etc. |
| Control Flow | block, loop, br, br_if, return |
| Memory | local.get, local.set, i32.load, i32.store |
| Functions | func, call, call_indirect |
| Constants | i32.const, i64.const |

---

## Dependencies

### External Tools

| Tool | Purpose | Required For |
|------|---------|--------------|
| wat2wasm | WAT → WASM | Integration tests |
| wasm2wat | WASM → WAT | Debugging |
| wasmtime | WASM runtime | Execution tests |
| wasm-validate | Validation | CI/CD |

### Internal Dependencies

- IR module must support all required opcodes
- Driver must support multi-target selection
- Test framework must support WASM execution

---

## Risk Summary

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Structured control flow complexity | High | Medium | Start simple, iterate |
| WASM spec changes | Medium | Low | Target stable features |
| Runtime library complexity | Medium | Medium | Use existing imports |
| Schedule slippage | High | Medium | Strict sprint boundaries |

See `.project-management/wasm-risks.md` for detailed risk assessment.

---

## Future Work (Post-Epic)

- Direct WASM binary output (skip WAT intermediate)
- WASM SIMD support
- Exception handling
- Multi-value returns optimization
- Tail call optimization
- Garbage collection integration
- WASI full support
- Browser integration examples

---

## Stakeholders

- **Project Lead**: Implementation oversight
- **Compiler Team**: Code review and integration
- **QA Team**: Test validation
- **Documentation**: User guides and examples

---

## Approval

- [ ] Architecture review complete
- [ ] Sprint plan approved
- [ ] Resource allocation confirmed
- [ ] Timeline agreed

---

## References

- [WebAssembly Specification](https://webassembly.github.io/spec/core/)
- [WAT Format](https://webassembly.github.io/wabt/doc/wat2wasm.html)
- [WASM by Example](https://webassemblybyexample.com/)
- [Compiler Architecture](./.project-management/architecture-analysis.md)
