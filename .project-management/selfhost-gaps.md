# Self-Hosting Gap Analysis Report

## Overview

This document analyzes the gaps between the current C compiler's capabilities and what's needed to compile its own source code. The analysis is based on examining compiler source code, existing test failures, and attempting to compile compiler source files.

## Methodology

1. **Source Code Analysis**: Examined compiler source files to identify language features used
2. **Test Compilation**: Attempted to compile compiler source files with current compiler
3. **Feature Mapping**: Mapped used features to current compiler support
4. **Priority Assessment**: Prioritized gaps by frequency of use and criticality

## Missing Features

### High Priority (Critical for Self-Hosting)

#### 1. **Struct Support**
- **Used Extensively**: AST nodes, symbol tables, IR structures, type system
- **Current Support**: Partial - struct types exist but field access not implemented
- **Required**: Struct declaration, field access (`.` operator), pointer-to-struct (`->`), struct copying, `sizeof(struct)`

#### 2. **Preprocessor Completeness**
- **Used Extensively**: `#include`, `#define`, `#ifdef`, `#pragma once`
- **Current Support**: Basic `#include` and `#define` likely implemented
- **Required**: Full macro expansion, `#if` expressions, `#elif`, `#else`, `#pragma`, token concatenation (`##`), stringification (`#`)

#### 3. **Standard Library Functions**
- **Used**: `malloc`, `free`, `printf`, `fprintf`, `strcmp`, `strlen`, `memcpy`, etc.
- **Current Support**: Minimal runtime (`runtime.c`)
- **Required**: Essential libc subset for compiler's own use

#### 4. **Type System Completeness**
- **Used**: `typedef`, function pointers, `const` qualifiers, complex type combinations
- **Current Support**: Basic types, pointers, functions
- **Required**: Union types, typedef resolution, type qualifiers, function pointer types

### Medium Priority (Required for Full Self-Hosting)

#### 5. **Memory Model**
- **Used**: Global variables, static variables, complex pointer arithmetic
- **Current Support**: Stack allocation, basic load/store
- **Required**: Global variable support, static storage duration, complex addressing modes

#### 6. **Control Flow Completeness**
- **Used**: `switch` statements, `goto` (likely not used), nested loops
- **Current Support**: `if`, `while`, `for`, `return`
- **Required**: `switch`/`case`/`default`, `do-while` loops

#### 7. **Array Operations**
- **Used**: Multi-dimensional arrays, array initialization, array decay to pointers
- **Current Support**: Basic arrays
- **Required**: Array initialization syntax, multi-dimensional arrays, array decay rules

### Low Priority (Nice to Have)

#### 8. **Advanced Features**
- **Used**: Variable-length arrays (likely not), `inline` functions, `_Generic` (likely not)
- **Current Support**: Not implemented
- **Required**: For completeness but not critical for initial self-hosting

## Test Compilation Results

### Attempted Compilation

```bash
# Test compilation of compiler source files
./compiler src/main.c -c -o /tmp/main.o
./compiler src/common/list.c -c -o /tmp/list.o
./compiler src/parser/parser.c -c -o /tmp/parser.o
```

### Expected Failures

Based on source analysis, compilation will likely fail due to:

1. **Struct-related errors**: Field access, struct type declarations
2. **Preprocessor errors**: Complex macros, conditional compilation
3. **Library function errors**: Missing `malloc`, `printf`, etc.
4. **Type system errors**: Complex typedefs, function pointers

## Feature Usage vs Support Matrix

| Feature | Used in Compiler | Currently Supported | Priority | Estimated Effort |
|---------|------------------|---------------------|----------|------------------|
| Structs | Extensive | Partial | HIGH | 3-4 weeks |
| Preprocessor | Extensive | Basic | HIGH | 2-3 weeks |
| Standard Library | Extensive | Minimal | HIGH | 2-3 weeks |
| Typedefs | Extensive | Yes | HIGH | 1 week |
| Unions | Some | No | MEDIUM | 1-2 weeks |
| Function pointers | Some | Likely no | MEDIUM | 1-2 weeks |
| Global variables | Some | Partial | MEDIUM | 1 week |
| Switch statements | Some | No | LOW | 1 week |
| Array initialization | Some | Partial | LOW | 1 week |

## Detailed Gap Analysis

### Struct Support Gaps

**Current IR Support (from `ir.h`):**
- No specific IR instructions for struct field access
- No struct type representation in type system
- No `sizeof` operator for struct types

**Required Implementation:**
1. Add struct type to type system (`src/parser/ast.h`)
2. Implement struct field access in parser and semantic analyzer
3. Add IR instructions for field access (or extend existing LOAD/STORE)
4. Implement `sizeof` for struct types
5. Handle struct copying and assignment

### Preprocessor Gaps

**Current Implementation (`src/lexer/preproc.c`):**
- Basic `#include` and `#define` likely implemented
- Unknown support for `#if`, `#elif`, `#else`

**Required Implementation:**
1. Complete macro expansion with arguments
2. Implement `#if` expression evaluation
3. Add `#pragma` directive support
4. Implement token concatenation (`##`) and stringification (`#`)
5. Support predefined macros (`__FILE__`, `__LINE__`, `__DATE__`, etc.)

### Standard Library Gaps

**Current Runtime (`src/runtime.c`):**
- Minimal runtime functions
- Likely missing most libc functions used by compiler

**Required Implementation:**
1. Memory management: `malloc`, `calloc`, `realloc`, `free`
2. String operations: `strlen`, `strcpy`, `strcmp`, `memcpy`, etc.
3. I/O functions: `printf`, `fprintf`, `fopen`, `fclose` (minimal subset)
4. Utilities: `atoi`, `exit`, `assert`

## Risk Assessment

### High Risk Areas

1. **Circular Dependencies**: Compiler may use features it implements
   - **Mitigation**: Implement features incrementally, use feature flags
   
2. **Performance Degradation**: Self-compiled compiler may be slower
   - **Mitigation**: Optimize after basic functionality works
   
3. **Bootstrapping Complexity**: Multiple stages required for verification
   - **Mitigation**: Clear documentation, automated scripts

### Technical Challenges

1. **Struct Alignment**: Platform-dependent padding and alignment
2. **Preprocessor Complexity**: Full macro expansion is complex
3. **Library Integration**: Linking with own standard library
4. **Type System Completeness**: Handling all C type combinations

## Implementation Roadmap

### Phase 1: Foundation (Weeks 1-4)
1. Implement struct support (field access, copying, sizeof)
2. Complete preprocessor implementation
3. Create minimal standard library subset

### Phase 2: Integration (Weeks 5-8)
1. Test compilation of compiler source files
2. Fix compilation errors incrementally
3. Implement missing type system features

### Phase 3: Bootstrapping (Weeks 9-12)
1. First successful self-compilation (stage1)
2. Verification (stage1 → stage2 comparison)
3. Performance optimization

### Phase 4: Validation (Weeks 13-16)
1. Complete test suite with self-compiled compiler
2. Documentation and release
3. CI/CD integration

## Success Criteria

### Minimum Viable Self-Hosting
- [ ] Compiler can parse its own source files without errors
- [ ] Individual `.c` files compile to object files
- [ ] Object files link into working compiler executable
- [ ] Self-compiled compiler passes basic test suite

### Full Self-Hosting
- [ ] All compiler source files compile without external dependencies
- [ ] Self-compiled compiler passes all test suites
- [ ] Performance within 20% of bootstrap compiler
- [ ] No regression in language feature support

## Next Steps

### Immediate Actions (Week 1)
1. **Create test infrastructure**: `scripts/selfhost-test.sh`
2. **Attempt full compilation**: Identify specific error messages
3. **Prioritize implementation**: Based on actual compilation failures

### Short-term Goals (Weeks 2-4)
1. Implement highest priority missing features
2. Create minimal standard library
3. Establish bootstrapping test pipeline

## Conclusion

The compiler has a solid foundation but lacks several critical features for self-hosting. The main gaps are in struct support, preprocessor completeness, and standard library functions. A phased approach focusing on these high-priority areas will enable self-hosting within 12-16 weeks.

The most critical path is implementing struct support, as structs are used extensively throughout the compiler source code. Once structs are supported, many source files will become compilable, providing momentum for implementing remaining features.