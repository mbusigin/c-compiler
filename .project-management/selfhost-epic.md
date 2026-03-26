# Self-Hosting Epic

## Epic Summary

**Title**: Make the C compiler capable of compiling itself

**Goal**: Extend the compiler to support all C language features used in its own source code, enabling self-compilation and bootstrapping.

**Duration**: 12-16 weeks (6 sprints)

**Status**: Planning

---

## Business Value

### Why Self-Hosting?

1. **Educational Milestone**: Demonstrates compiler completeness and correctness
2. **Bootstrapping Independence**: Eliminates dependency on external compilers
3. **Trust Verification**: Enables diverse double-compiling to detect "trusting trust" attacks
4. **Quality Assurance**: Forces implementation of all language features actually used
5. **Performance Benchmark**: Provides baseline for compiler optimization

### Success Metrics

- [ ] Compiler can parse its own source files without errors
- [ ] Compiler can compile individual source files to object files
- [ ] Object files link into working compiler executable
- [ ] Self-compiled compiler passes all test suites
- [ ] Stage1 and Stage2 compilers produce identical output
- [ ] Performance within 20% of bootstrap compiler

---

## Sprint Overview

| Sprint | Duration | Focus | Key Deliverable |
|--------|----------|-------|-----------------|
| 1 | Weeks 1-2 | Gap Analysis | Complete feature gap analysis and test compilation |
| 2 | Weeks 3-5 | Core Features | Struct support, preprocessor completion |
| 3 | Weeks 6-8 | Standard Library | Essential libc subset implementation |
| 4 | Weeks 9-11 | Bootstrapping | First successful self-compilation |
| 5 | Weeks 12-13 | Validation | Stage comparison and test verification |
| 6 | Weeks 14-16 | Optimization | Performance tuning and documentation |

---

## Task Breakdown

### Sprint 1: Gap Analysis (Weeks 1-2)

#### Goals
- Identify exact language features missing for self-hosting
- Create comprehensive test suite for compiler source compilation
- Establish baseline measurements

#### Tasks

**Phase 1.1: Compilation Test**
- [ ] Create `scripts/selfhost-test.sh` - Compile all compiler source files
- [ ] Test compilation of each `.c` file individually
- [ ] Record compilation errors and missing features
- [ ] Generate feature gap report

**Phase 1.2: Feature Analysis**
- [ ] Analyze compiler source for language feature usage
- [ ] Map used features to current compiler support
- [ ] Prioritize features by frequency of use
- [ ] Create detailed implementation roadmap

**Phase 1.3: Test Infrastructure**
- [ ] Create self-hosting test suite
- [ ] Add to Makefile (`make test-selfhost`)
- [ ] Set up continuous integration for self-hosting progress

#### Deliverables
- Complete feature gap analysis report
- Self-hosting test suite
- Prioritized implementation roadmap

#### Gate Tests
```bash
# Test: Self-hosting gap analysis complete
test -f /Users/mbusigin/c-compiler/.project-management/selfhost-gaps.md && \
grep -q "Missing Features" /Users/mbusigin/c-compiler/.project-management/selfhost-gaps.md && \
grep -q "Priority" /Users/mbusigin/c-compiler/.project-management/selfhost-gaps.md

# Test: Self-hosting test script exists and runs
cd /Users/mbusigin/c-compiler && ./scripts/selfhost-test.sh --list 2>&1 | head -5
```

---

### Sprint 2: Core Features (Weeks 3-5)

#### Goals
- Implement struct and union support
- Complete preprocessor implementation
- Add missing type system features

#### Tasks

**Phase 2.1: Struct Support**
- [ ] Implement struct type definition and declaration
- [ ] Add struct field access (`.` operator)
- [ ] Implement pointer-to-struct field access (`->` operator)
- [ ] Handle struct copying and assignment
- [ ] Add `sizeof(struct)` support
- [ ] Implement struct alignment and padding

**Phase 2.2: Preprocessor Completion**
- [ ] Complete macro expansion with arguments
- [ ] Implement `#if`, `#elif`, `#else` with expressions
- [ ] Add `#pragma` directive support (minimal)
- [ ] Implement `##` token concatenation
- [ ] Add `#` stringification operator
- [ ] Support predefined macros (`__FILE__`, `__LINE__`, etc.)

**Phase 2.3: Type System Enhancements**
- [ ] Add union type support
- [ ] Implement `typedef` resolution for complex types
- [ ] Add `const` and `volatile` qualifiers (semantic only)
- [ ] Support function pointer types
- [ ] Implement `void` pointer arithmetic restrictions

#### Deliverables
- Compiler supports structs and unions
- Complete preprocessor implementation
- Enhanced type system

#### Gate Tests
```bash
# Test: Struct support works
cd /Users/mbusigin/c-compiler && ./compiler tests/selfhost/test_struct.c -o /tmp/test_struct.o 2>&1 && \
echo "Struct test compiled successfully"

# Test: Preprocessor handles complex macros
cd /Users/mbusigin/c-compiler && ./compiler -E tests/selfhost/test_macro.c 2>&1 | \
grep -q "EXPANDED_MACRO" && echo "Macro expansion works"

# Test: Compiler can parse its own headers
cd /Users/mbusigin/c-compiler && ./compiler -c src/common/list.c -o /tmp/list.o 2>&1 | \
grep -v "DEBUG:" | wc -l | grep -q "^0$"
```

---

### Sprint 3: Standard Library (Weeks 6-8)

#### Goals
- Implement essential libc subset used by compiler
- Add memory management support
- Implement I/O functions for compiler's needs

#### Tasks

**Phase 3.1: Memory Management**
- [ ] Implement `malloc`, `calloc`, `realloc`, `free`
- [ ] Add `memcpy`, `memmove`, `memset`, `memcmp`
- [ ] Implement `strdup`, `strndup`

**Phase 3.2: String Operations**
- [ ] Implement `strlen`, `strcpy`, `strncpy`
- [ ] Add `strcmp`, `strncmp`, `strcat`, `strncat`
- [ ] Implement `strchr`, `strstr`, `strtok`

**Phase 3.3: I/O and Utilities**
- [ ] Implement `printf`, `fprintf`, `sprintf` subset
- [ ] Add `fopen`, `fclose`, `fread`, `fwrite`
- [ ] Implement `atoi`, `atol`, `atoll`
- [ ] Add `exit`, `abort`, `assert`

**Phase 3.4: Compiler-specific Functions**
- [ ] Create minimal runtime for compiler needs
- [ ] Implement `xmalloc`, `xcalloc` with error checking
- [ ] Add debugging and error reporting utilities

#### Deliverables
- Essential libc subset implemented
- Compiler can use its own standard library
- Memory management working

#### Gate Tests
```bash
# Test: Memory allocation works
cd /Users/mbusigin/c-compiler && ./compiler tests/selfhost/test_malloc.c -o /tmp/test_malloc 2>&1 && \
/tmp/test_malloc 2>&1 | grep -q "Allocation test passed"

# Test: String operations work
cd /Users/mbusigin/c-compiler && ./compiler tests/selfhost/test_string.c -o /tmp/test_string 2>&1 && \
/tmp/test_string 2>&1 | grep -q "String tests passed"

# Test: Compiler links with its own runtime
cd /Users/mbusigin/c-compiler && ./compiler tests/hello.c -o /tmp/hello 2>&1 && \
/tmp/hello 2>&1 | grep -q "Hello"
```

---

### Sprint 4: Bootstrapping (Weeks 9-11)

#### Goals
- First successful self-compilation
- Link compiler executable from own object files
- Establish bootstrapping pipeline

#### Tasks

**Phase 4.1: Source Compilation**
- [ ] Compile all `.c` files individually with current compiler
- [ ] Fix remaining compilation errors
- [ ] Generate object files for all compiler modules
- [ ] Verify object file compatibility

**Phase 4.2: Linking**
- [ ] Create linker script for compiler executable
- [ ] Link object files into stage1 compiler
- [ ] Verify stage1 compiler basic functionality
- [ ] Test stage1 compiler with simple programs

**Phase 4.3: Bootstrapping Pipeline**
- [ ] Create `scripts/bootstrap.sh` - Automated bootstrapping
- [ ] Document bootstrapping process
- [ ] Add bootstrap tests to CI/CD
- [ ] Verify stage1 can compile compiler source

#### Deliverables
- Stage1 compiler executable (self-compiled)
- Bootstrapping automation scripts
- Documentation of bootstrapping process

#### Gate Tests
```bash
# Test: Stage1 compiler exists and runs
cd /Users/mbusigin/c-compiler && test -f build/stage1/compiler && \
build/stage1/compiler --version 2>&1 | grep -q "C Compiler"

# Test: Stage1 can compile simple program
cd /Users/mbusigin/c-compiler && build/stage1/compiler tests/hello.c -o /tmp/hello-stage1 2>&1 && \
/tmp/hello-stage1 2>&1 | grep -q "Hello"

# Test: Bootstrap script works
cd /Users/mbusigin/c-compiler && ./scripts/bootstrap.sh --stage1 2>&1 | \
tail -5 | grep -q "Stage1 compilation successful"
```

---

### Sprint 5: Validation (Weeks 12-13)

#### Goals
- Verify self-compiled compiler correctness
- Compare stage1 and stage2 outputs
- Ensure test suite passes with self-compiled compiler

#### Tasks

**Phase 5.1: Stage Comparison**
- [ ] Use stage1 to compile compiler source (stage2)
- [ ] Compare stage1 and stage2 binaries
- [ ] Verify identical behavior for test programs
- [ ] Document any differences

**Phase 5.2: Test Suite Verification**
- [ ] Run all existing tests with stage1 compiler
- [ ] Fix any test failures
- [ ] Add self-hosting specific tests
- [ ] Ensure 100% test pass rate

**Phase 5.3: Correctness Validation**
- [ ] Compare output with reference compiler (clang/gcc)
- [ ] Verify language standard compliance
- [ ] Test edge cases and corner cases
- [ ] Document known limitations

#### Deliverables
- Stage1 and Stage2 comparison report
- All tests pass with self-compiled compiler
- Validation documentation

#### Gate Tests
```bash
# Test: Stage1 and Stage2 produce identical output for simple program
cd /Users/mbusigin/c-compiler && build/stage1/compiler tests/hello.c -S -o /tmp/hello-stage1.s 2>&1 && \
build/stage2/compiler tests/hello.c -S -o /tmp/hello-stage2.s 2>&1 && \
diff /tmp/hello-stage1.s /tmp/hello-stage2.s && echo "Output identical"

# Test: All tests pass with stage1 compiler
cd /Users/mbusigin/c-compiler && make test COMPILER=build/stage1/compiler 2>&1 | \
tail -10 | grep -q "All tests passed"

# Test: Bootstrap validation passes
cd /Users/mbusigin/c-compiler && ./scripts/bootstrap.sh --validate 2>&1 | \
grep -q "Bootstrap validation successful"
```

---

### Sprint 6: Optimization and Documentation (Weeks 14-16)

#### Goals
- Optimize self-compiled compiler performance
- Complete documentation
- Integrate into CI/CD pipeline

#### Tasks

**Phase 6.1: Performance Optimization**
- [ ] Profile stage1 compiler performance
- [ ] Identify and fix performance bottlenecks
- [ ] Optimize critical code paths
- [ ] Achieve performance targets (within 20% of bootstrap)

**Phase 6.2: Documentation**
- [ ] Document self-hosting process
- [ ] Create user guide for bootstrapping
- [ ] Document known issues and limitations
- [ ] Update README with self-hosting status

**Phase 6.3: Integration**
- [ ] Add self-hosting to CI/CD pipeline
- [ ] Create release process including bootstrapping
- [ ] Add performance regression tests
- [ ] Finalize project structure

#### Deliverables
- Optimized self-compiled compiler
- Complete documentation
- CI/CD integration
- Release-ready project

#### Gate Tests
```bash
# Test: Performance within target range
cd /Users/mbusigin/c-compiler && ./scripts/benchmark.sh --compare 2>&1 | \
grep -q "Performance: OK" || echo "Performance check passed"

# Test: Documentation exists
test -f /Users/mbusigin/c-compiler/docs/SELFHOSTING.md && \
test -f /Users/mbusigin/c-compiler/docs/BOOTSTRAPPING.md

# Test: CI/CD integration works
cd /Users/mbusigin/c-compiler && make ci-test-selfhost 2>&1 | \
tail -5 | grep -q "Self-hosting CI tests passed"
```

---

## Technical Architecture

### Bootstrapping Process

```
┌─────────────────────────────────────────────────────────────┐
│                   Bootstrap Compiler                        │
│                   (clang/gcc)                               │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   Stage 0 Compiler                          │
│                   (compiled with bootstrap)                 │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   Stage 1 Compiler                          │
│                   (compiled with stage 0)                   │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   Stage 2 Compiler                          │
│                   (compiled with stage 1)                   │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                   Verification:                             │
│                   stage1 ≡ stage2                           │
└─────────────────────────────────────────────────────────────┘
```

### Key Components for Self-Hosting

1. **Complete Parser**: Must handle all constructs used in compiler source
2. **Full Type System**: Structs, unions, typedefs, function pointers
3. **Preprocessor**: Complete macro expansion and conditional compilation
4. **Standard Library**: Essential subset for compiler's own use
5. **Linker Support**: Ability to link multiple object files
6. **Runtime**: Minimal runtime for memory management and I/O

### Risk Assessment

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| Circular dependencies in compiler source | High | Medium | Incremental implementation, feature flags |
| Performance degradation in self-compiled version | Medium | High | Profiling, optimization passes |
| Bootstrapping infinite loop | High | Low | Validation checks, stage comparison |
| Missing subtle language features | Medium | High | Comprehensive testing, comparison with reference compiler |
| Platform-specific issues | Medium | Medium | Abstract platform dependencies, CI on multiple platforms |

### Success Criteria

#### Phase 1: Feature Completeness
- [ ] Compiler source parses without errors
- [ ] All language features used in compiler are supported
- [ ] Preprocessor handles all directives in compiler source

#### Phase 2: Compilation
- [ ] Individual `.c` files compile to object files
- [ ] Object files are compatible with system linker
- [ ] Compiler executable links successfully

#### Phase 3: Bootstrapping
- [ ] Stage1 compiler produces working Stage2
- [ ] Stage1 and Stage2 produce identical output for test programs
- [ ] Bootstrap process is documented and repeatable

#### Phase 4: Validation
- [ ] All test suites pass with self-compiled compiler
- [ ] Performance within acceptable range
- [ ] No regression in language support

### Future Work (Post-Epic)

- Cross-compilation self-hosting (compile for different target)
- Diverse double-compiling for security verification
- Performance optimization beyond 20% target
- Support for compiling other compilers (C++, etc.)
- Formal verification of compiler correctness

---

## Stakeholders

- **Compiler Team**: Implementation and testing
- **Project Lead**: Architecture oversight and planning
- **QA Team**: Validation and verification
- **Documentation**: User guides and technical documentation
- **Community**: Users interested in bootstrapping and compiler education

---

## References

1. **"Reflections on Trusting Trust"** - Ken Thompson
2. **"Bootstrapping a Simple Compiler from Nothing"** - Jack Crenshaw
3. **GCC Bootstrapping Documentation** - GNU Project
4. **TinyCC Self-Compilation** - Fabrice Bellard
5. **"The Oberon System"** - Niklaus Wirth (complete self-hosting system)

---

## Approval

- [ ] Architecture review complete
- [ ] Sprint plan approved by team
- [ ] Resource allocation confirmed
- [ ] Timeline agreed with stakeholders

---

## Appendices

### Appendix A: Compiler Source Feature Usage

Based on initial analysis, the compiler uses:

1. **Structs extensively** (AST nodes, symbol tables, IR structures)
2. **Pointers** for dynamic data structures
3. **Preprocessor macros** for configuration and utilities
4. **Standard I/O** for error reporting and debugging
5. **Memory allocation** for all dynamic structures
6. **String operations** for symbol names and error messages

### Appendix B: Test Programs for Self-Hosting

1. `tests/selfhost/test_struct.c` - Struct operations
2. `tests/selfhost/test_macro.c` - Complex macro expansion
3. `tests/selfhost/test_stdlib.c` - Standard library functions
4. `tests/selfhost/test_compiler.c` - Compiler source compilation
5. `tests/selfhost/test_bootstrap.c` - Bootstrapping verification

### Appendix C: Performance Targets

1. **Compilation speed**: Within 20% of bootstrap compiler
2. **Memory usage**: Within 30% of bootstrap compiler
3. **Binary size**: Within 50% of bootstrap compiler (initially)
4. **Test pass rate**: 100% of existing test suite

---

## Revision History

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.0 | 2024 | Initial epic creation | Planning Team |
| 1.1 | 2024 | Added gate tests and validation details | Planning Team |

---

*This epic represents a significant milestone in compiler development. Successful completion will demonstrate compiler maturity and enable future work in verification, optimization, and language extension.*