# Research: Self-Hosting Compilers and Bootstrapping

## Bootstrapping Approaches

### 1. **Full Bootstrapping**
- Compiler written in its own language
- Requires initial compiler (seed) written in another language
- Classic approach: N → N+1 → N+2 generations
- Example: GCC (C → C), Go (Go → Go)

### 2. **Incremental Bootstrapping**
- Start with minimal subset compiler
- Extend compiler using its own features
- Gradually add more language features
- Example: TinyCC (C subset → full C)

### 3. **Cross-Compilation Bootstrapping**
- Compiler runs on platform A, targets platform B
- Build compiler for B on A, then use it on B
- Common for new architectures
- Example: Rust cross-compilation

### 4. **Trusting Trust Attack Prevention**
- Ken Thompson's "Reflections on Trusting Trust"
- Requires diverse compiler sources or verification
- Techniques: diverse double-compiling, reproducible builds

## Existing Self-Hosting Compilers

### **GCC (GNU Compiler Collection)**
- **Language**: C, C++, Fortran, etc.
- **Bootstrapping**: Written in C, compiles itself
- **Approach**: Full bootstrapping with multiple stages
- **Complexity**: Very high, supports many targets

### **Clang/LLVM**
- **Language**: C++ 
- **Bootstrapping**: C++ compiles itself
- **Approach**: Modular, library-based design
- **Note**: Not written in C, but demonstrates self-hosting concept

### **TinyCC (Tiny C Compiler)**
- **Language**: C
- **Bootstrapping**: Can compile itself (~200KB binary)
- **Approach**: Minimal, single-pass compiler
- **Relevance**: Good reference for our project size

### **Chicken Scheme**
- **Language**: Scheme
- **Bootstrapping**: Bootstrap compiler in C, then Scheme
- **Approach**: Multiple generation bootstrapping
- **Interesting**: Demonstrates bootstrapping functional languages

### **Oberon System**
- **Language**: Oberon
- **Bootstrapping**: Complete system written in itself
- **Approach**: From Pascal to Oberon evolution
- **Philosophy**: "The compiler must be able to compile itself"

## Key Challenges for Self-Hosting

### 1. **Feature Completeness**
- Compiler must support all language features it uses
- Common pitfalls: preprocessor, complex types, libraries
- Solution: Start with minimal subset, expand gradually

### 2. **Performance**
- Bootstrap compiler may be slow
- Multiple compilation stages increase build time
- Solution: Optimize only after basic functionality works

### 3. **Testing and Verification**
- How to verify self-compiled compiler is correct?
- Need comprehensive test suite
- Solution: Compare outputs with trusted compiler

### 4. **Circular Dependencies**
- Compiler needs certain features to compile itself
- Chicken-and-egg problem
- Solution: Temporary workarounds, then remove

### 5. **Platform Dependencies**
- System calls, libraries, ABI differences
- Solution: Abstract platform-specific code

## Bootstrapping Strategies for Our C Compiler

### **Assessment Needed**
1. What C subset does our compiler currently support?
2. Is the compiler written in C?
3. What missing features would prevent self-compilation?

### **Potential Path**
1. **Phase 1**: Make compiler support its own source language features
2. **Phase 2**: Compile compiler with itself (stage1)
3. **Phase 3**: Use stage1 to compile compiler (stage2)
4. **Phase 4**: Verify stage1 and stage2 produce identical output
5. **Phase 5**: Remove any bootstrap dependencies

### **Minimal Viable Self-Hosting**
1. Support all C constructs used in compiler source
2. Handle preprocessor directives (#include, #define, #ifdef)
3. Support necessary standard library functions
4. Generate correct code for target architecture

## Technical Considerations

### **Compiler Source Analysis**
- Need to examine `src/` directory structure
- Identify language features used
- Map features to current compiler capabilities

### **Architecture Support**
- Current backend: ARM64 (based on exploration)
- Self-hosting requires same target as host
- May need x86_64 support for development machines

### **Standard Library**
- Compiler likely uses minimal libc functions
- Need to implement or stub missing functions
- Consider linking with system libc for bootstrap

## Risk Assessment

### **High Risk Areas**
1. **Preprocessor**: Complex #ifdef chains, macros
2. **Type System**: struct, union, typedef, function pointers
3. **Optimizations**: May reveal bugs in code generation
4. **Platform Differences**: ARM64 vs x86_64 issues

### **Mitigation Strategies**
1. Start with simplest possible self-compilation
2. Use comparison testing with known-good compiler
3. Implement features incrementally
4. Maintain ability to fall back to bootstrap compiler

## Success Metrics

### **Phase 1: Feature Parity**
- [ ] Compiler can parse its own source files
- [ ] All syntax constructs are recognized
- [ ] Semantic analysis passes without errors

### **Phase 2: Compilation**
- [ ] Compiler can compile individual source files
- [ ] Object files link successfully
- [ ] Resulting binary passes basic tests

### **Phase 3: Self-Compilation**
- [ ] Stage1 compiler produces working stage2
- [ ] Stage2 compiler produces identical stage3
- [ ] All test suites pass with self-compiled compiler

### **Phase 4: Verification**
- [ ] Output comparison with bootstrap compiler
- [ ] Performance benchmarks within tolerance
- [ ] No regressions in supported features

## References

1. **"Bootstrapping a Simple Compiler from Nothing"** - Jack Crenshaw
2. **"Reflections on Trusting Trust"** - Ken Thompson
3. **GCC Bootstrapping Guide** - GNU Documentation
4. **TinyCC Self-Compilation** - Fabrice Bellard
5. **"The Oberon System"** - Niklaus Wirth

## Next Steps

1. **Analyze compiler source code** to identify required features
2. **Test current compiler** with its own source files
3. **Create feature gap analysis** between current and required capabilities
4. **Develop implementation roadmap** for self-hosting