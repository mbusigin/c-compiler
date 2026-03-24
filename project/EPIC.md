# C Compiler Self-Hosting Epic
**Goal**: Enable the compiler to compile itself (achieve stage1 self-hosting)

**Status**: Planning Complete - Ready for Implementation

**Created**: 2024-03-24

**Estimated Duration**: 3-4 weeks (with iterative approach)

---

## Executive Summary

This Epic delivers a **self-hosting C compiler** capable of compiling its own source code. The primary blocker is **missing function pointer support** in the parser, preventing compilation of standard library headers and core compiler files.

### Key Metrics
- **Current State**: 0/17 source files compile with stage0 compiler
- **Target State**: 17/17 source files compile, producing working stage1 compiler
- **Success Criteria**: `make stage1` produces `build/compiler_stage1` that can compile "hello world"

### Critical Path
```
Function Pointers → Self-Compile Stage1 → Bootstrap Verification → Done
     2-3 days            1 day                1 day              5 days
```

---

## Sprint Structure

We use **iterative, high-entropy sprints** with rapid validation:

- **Sprint Length**: 2-3 days
- **Validation**: Working code that compiles real files
- **Approach**: Spike solutions first, then refine
- **Philosophy**: "Make it work, make it right, make it fast"

---

## Sprint 1: Function Pointer Foundation (CRITICAL PATH)

**Goal**: Parse function pointer parameters in function declarations

**Duration**: 2-3 days

**Why This Sprint**: Blocks ALL self-hosting - no stdlib headers can be included without this

### Task 1.1: Spike - Minimal Function Pointer Parsing
**Type**: Spike/Prototype  
**Time**: 4 hours  
**Approach**: Quick hack to validate approach

**Implementation**:
```c
// In parse_parameter(), add special case:
if (check(p, TOKEN_LPAREN)) {
    Token next = peek_token(p, 1);
    if (next.type == TOKEN_STAR) {
        return parse_function_pointer_param(p, param_type);
    }
}
```

**Validation**:
```bash
# Test case: test_func_ptr_param.c
echo 'void qsort(void *base, int (*compar)(const void*, const void*));' > /tmp/test.c
./build/compiler_stage0 /tmp/test.c -o /tmp/test.s
# Should produce assembly, not error
```

**Success Criteria**: 
- Test case compiles without parse errors
- Can run on actual util.c without crashing

**Deliverable**: Working prototype (may be ugly, but works)

---

### Task 1.2: Implement Declarator Parser (Proper)
**Type**: Core Implementation  
**Time**: 8 hours  
**Approach**: Replace spike with clean implementation

**Implementation**:
1. Create `parse_declarator()` function in parser.c
2. Handle declarator grammar:
   ```
   declarator := pointer? direct_declarator
   direct_declarator := identifier 
                      | '(' declarator ')' 
                      | direct_declarator '[' size? ']'
                      | direct_declarator '(' parameter_list? ')'
   ```

3. Support both named and abstract declarators:
   - Named: `int (*compar)(const void*, const void*)`
   - Abstract: `int (*)(const void*, const void*)`

**Validation**:
```bash
# Test suite
tests/
  test_func_ptr_named.c       # Named function pointer param
  test_func_ptr_abstract.c    # Abstract function pointer param
  test_func_ptr_variable.c    # Function pointer variable
  test_func_ptr_typedef.c     # Function pointer typedef
```

**Success Criteria**:
- All test cases compile
- Generated assembly is syntactically valid
- Can compile `<stdlib.h>` header without errors

**Deliverable**: Production-ready declarator parser

---

### Task 1.3: Semantic Analysis for Function Pointers
**Type**: Core Implementation  
**Time**: 4 hours  
**Prerequisite**: Task 1.2

**Implementation**:
1. Add function pointer type representation:
   ```c
   struct Type {
       TypeKind kind;
       union {
           struct {
               Type *return_type;
               List *param_types;
               bool is_variadic;
           } function;
           // ... other types
       };
   };
   ```

2. Type checking for function pointer assignments
3. Validate function pointer calls match signature

**Validation**:
```bash
# Type checking tests
tests/
  test_func_ptr_type_match.c      # Valid assignments
  test_func_ptr_type_mismatch.c   # Should error on mismatch
```

**Success Criteria**:
- Correct type inference for function pointers
- Error on type mismatches
- Symbol table tracks function pointer variables

**Deliverable**: Complete semantic analysis for function pointers

---

### Task 1.4: Code Generation for Function Pointer Calls
**Type**: Core Implementation  
**Time**: 4 hours  
**Prerequisite**: Task 1.3

**Implementation**:
1. Generate indirect call instructions:
   ```asm
   mov rax, [function_pointer]  ; Load function address
   call rax                      ; Indirect call
   ```

2. Handle function pointer in assignments
3. Support taking address of functions: `&func_name`

**Validation**:
```bash
# Runtime tests
tests/
  test_func_ptr_call.c    # Call through function pointer
  test_func_ptr_assign.c  # Assign function to pointer
  test_qsort.c           # Use qsort with comparison function
```

**Success Criteria**:
- Compiled code executes correctly
- Can implement and use qsort
- Function pointers work in real programs

**Deliverable**: Working function pointer code generation

---

### Sprint 1 Acceptance Criteria

**Must Have**:
- [ ] Parse function pointer parameters
- [ ] Parse function pointer variables
- [ ] Parse function pointer typedefs
- [ ] Semantic analysis validates types
- [ ] Code generation produces correct assembly
- [ ] Can compile `<stdlib.h>` header
- [ ] Can compile `src/common/util.c`

**Definition of Done**:
```bash
# Compile util.c successfully
./build/compiler_stage0 -I src -I include src/common/util.c -o /tmp/util.s
test -f /tmp/util.s && echo "Sprint 1: PASS"
```

---

## Sprint 2: Compile All Source Files

**Goal**: Successfully compile all 17 compiler source files

**Duration**: 2-3 days

**Why This Sprint**: Verify no other blockers before attempting stage1 build

### Task 2.1: Compile File Inventory
**Type**: Investigation  
**Time**: 2 hours

**Process**:
```bash
# Try compiling each source file
for file in src/**/*.c; do
    echo "=== $file ==="
    ./build/compiler_stage0 -I src -I include $file -o /tmp/test.s
done
```

**Deliverable**: List of files that compile vs fail, with error analysis

---

### Task 2.2: Fix Remaining Parse Errors
**Type**: Bug Fixes  
**Time**: Variable (estimate 8 hours)

**Potential Issues**:
- Struct member access errors (seen in parser.c)
- Missing C syntax features
- Preprocessor edge cases

**Approach**:
1. Fix one file at a time
2. Verify fix with test case
3. Move to next file
4. Refactor common patterns

**Success Criteria**: Each file compiles to valid assembly

**Deliverable**: All source files compile individually

---

### Task 2.3: Integration Testing
**Type**: Testing  
**Time**: 4 hours

**Test Cases**:
```c
// test_self_compile_fragment.c
// Compile a fragment that uses multiple compiler features

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int (*compare)(const void*, const void*);
    void *data;
    size_t size;
} Container;

int my_compare(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

int main() {
    Container c;
    c.compare = my_compare;
    c.size = 100;
    
    int arr[] = {3, 1, 2};
    qsort(arr, 3, sizeof(int), c.compare);
    
    return arr[0]; // Should be 1
}
```

**Validation**: Compile and run, verify correct output

**Deliverable**: Integration test suite

---

### Sprint 2 Acceptance Criteria

**Must Have**:
- [ ] All 17 source files compile to assembly
- [ ] Integration test passes
- [ ] No parse errors in any compiler file
- [ ] Generated assembly is syntactically valid

**Definition of Done**:
```bash
# All files compile
count=0
for file in src/**/*.c; do
    if ./build/compiler_stage0 -I src -I include $file -o /tmp/$file.s 2>/dev/null; then
        count=$((count + 1))
    fi
done
test $count -eq 17 && echo "Sprint 2: PASS"
```

---

## Sprint 3: Self-Compilation (Stage1)

**Goal**: Build compiler with itself

**Duration**: 2 days

**Why This Sprint**: The ultimate goal - self-hosting

### Task 3.1: Assemble Compiled Files
**Type**: Build Process  
**Time**: 4 hours

**Process**:
```bash
# Compile all source files with stage0
mkdir -p build/stage1_obj
for file in src/**/*.c; do
    base=$(basename $file .c)
    ./build/compiler_stage0 -I src -I include $file -o build/stage1_obj/$base.s
    clang -c build/stage1_obj/$base.s -o build/stage1_obj/$base.o
done

# Link all objects
clang build/stage1_obj/*.o -o build/compiler_stage1 -lm
```

**Validation**:
```bash
test -x build/compiler_stage1 && echo "Stage1 built"
```

**Success Criteria**: stage1 executable exists and is executable

**Deliverable**: Working stage1 compiler binary

---

### Task 3.2: Stage1 Smoke Test
**Type**: Testing  
**Time**: 2 hours

**Tests**:
```bash
# Test 1: Version check
./build/compiler_stage1 --version

# Test 2: Compile hello world
echo 'int main() { return 42; }' > /tmp/hello.c
./build/compiler_stage1 /tmp/hello.c -o /tmp/hello.s
clang /tmp/hello.s -o /tmp/hello
/tmp/hello
test $? -eq 42 && echo "Smoke test: PASS"

# Test 3: Compile a real program
./build/compiler_stage1 tests/hello.c -o /tmp/hello_real.s
```

**Success Criteria**: stage1 can compile basic programs

**Deliverable**: Verified working stage1 compiler

---

### Task 3.3: Bootstrap Convergence (Optional Stretch Goal)
**Type**: Verification  
**Time**: 4 hours

**Process**:
```bash
# Compile compiler with stage1 to produce stage2
mkdir -p build/stage2_obj
for file in src/**/*.c; do
    base=$(basename $file .c)
    ./build/compiler_stage1 -I src -I include $file -o build/stage2_obj/$base.s
    clang -c build/stage2_obj/$base.s -o build/stage2_obj/$base.o
done
clang build/stage2_obj/*.o -o build/compiler_stage2 -lm

# Compare stage1 and stage2 output
./build/compiler_stage1 tests/hello.c -o /tmp/stage1_output.s
./build/compiler_stage2 tests/hello.c -o /tmp/stage2_output.s
diff /tmp/stage1_output.s /tmp/stage2_output.s
test $? -eq 0 && echo "Bootstrap: CONVERGED"
```

**Success Criteria**: stage1 and stage2 produce identical output

**Deliverable**: Proven bootstrap convergence (gold standard for compiler correctness)

---

### Sprint 3 Acceptance Criteria

**Must Have**:
- [ ] stage1 compiler exists
- [ ] stage1 can compile hello world
- [ ] stage1 produces working executables

**Nice to Have**:
- [ ] Bootstrap convergence verified

**Definition of Done**:
```bash
# Stage1 compiles and runs
./build/compiler_stage1 tests/hello.c -o /tmp/test.s
clang /tmp/test.s -o /tmp/test
/tmp/test
test $? -eq 0 && echo "Sprint 3: PASS"
```

---

## Sprint 4: Polish & Testing (Post-Self-Hosting)

**Goal**: Improve quality, fix edge cases, add tests

**Duration**: 3-5 days

**Why This Sprint**: Production-quality compiler needs comprehensive testing

### Task 4.1: Comprehensive Test Suite
**Type**: Testing  
**Time**: 2 days

**Test Categories**:
1. **Unit Tests**
   - Function pointer parsing
   - Type checking
   - Code generation
   
2. **Integration Tests**
   - Compile real C programs
   - Compare output with GCC
   - Runtime behavior verification

3. **Stress Tests**
   - Large files
   - Deeply nested function pointers
   - Complex type combinations

**Deliverable**: Test suite with >80% code coverage

---

### Task 4.2: Error Message Improvements
**Type**: UX  
**Time**: 1 day

**Improvements**:
1. Better source location in errors
2. Show context (surrounding lines)
3. Suggest fixes for common mistakes
4. Limit cascading errors

**Example**:
```
error: parser.c:645: undeclared identifier 'compar'
  643 | void qsort(void *base, size_t n, size_t size,
  644 |            int (*compar)(const void *, const void *));
      |                ^^^^^^
  note: did you mean to declare 'compar' as a parameter?
```

**Deliverable**: Clear, actionable error messages

---

### Task 4.3: Documentation
**Type**: Documentation  
**Time**: 1 day

**Documents**:
1. **User Guide**: How to use the compiler
2. **Implementation Guide**: Architecture decisions
3. **Self-Hosting Guide**: How the bootstrap works
4. **Contributing Guide**: How to add features

**Deliverable**: Complete documentation in docs/

---

### Sprint 4 Acceptance Criteria

**Must Have**:
- [ ] Test suite passes all tests
- [ ] Error messages are helpful
- [ ] Documentation is complete

**Definition of Done**:
```bash
make test  # All tests pass
make docs  # Documentation builds
```

---

## Risk Mitigation

### Risk 1: More Complex Than Estimated
**Probability**: Medium  
**Impact**: Delays by 1-2 weeks

**Mitigation**:
- Spike solutions validate approach early
- Can pivot to simpler implementation
- Accept "ugly but working" for first version

### Risk 2: Other Missing C Features
**Probability**: Medium  
**Impact**: Additional sprints needed

**Mitigation**:
- Sprint 2 discovers all blockers
- Prioritize by impact
- Can use system headers vs. our headers if needed

### Risk 3: Generated Code Incorrect
**Probability**: Low  
**Impact**: stage1 doesn't work correctly

**Mitigation**:
- Test generated code at each sprint
- Compare with GCC output
- Use existing test suite

---

## Dependencies

### External
- **clang** or **gcc** for assembling/linking (already used in Makefile)
- System libc for running compiled programs

### Internal
- **Type system** must support function types (partially done)
- **AST** needs function pointer nodes (may need new node types)
- **Code generator** needs indirect call support (straightforward)

---

## Success Metrics

### Quantitative
- ✅ Number of source files that compile: 0 → 17
- ✅ Test coverage: Unknown → >80%
- ✅ Number of bugs in tracker: Current → All resolved

### Qualitative
- ✅ Can compile real C programs
- ✅ Error messages are helpful
- ✅ Code is maintainable
- ✅ Documentation is complete

---

## Post-Epic Work (Future Sprints)

After achieving self-hosting:

1. **Optimization**: Implement -O2 level optimizations
2. **More Targets**: Add RISC-V or ARM64 backend
3. **Language Server**: LSP support for IDE integration
4. **Debug Info**: DWARF generation for debugging
5. **C11 Compliance**: Fill remaining gaps
6. **Performance**: Improve compilation speed

---

## Appendix A: File Inventory

Files to compile for self-hosting:

```
src/main.c                    - Entry point
src/driver.c                  - Compiler driver
src/common/util.c             - Utilities (BLOCKER: function pointers)
src/common/list.c             - Dynamic arrays (BLOCKER: function pointers)
src/common/error.c            - Error reporting (BLOCKER: function pointers)
src/common/test_framework.c   - Testing framework
src/lexer/lexer.c             - Lexer
src/lexer/preproc.c           - Preprocessor
src/parser/parser.c           - Parser (BLOCKER: struct member access?)
src/parser/ast.c              - AST nodes
src/sema/analyzer.c           - Semantic analysis
src/sema/symtab.c             - Symbol table
src/ir/ir.c                   - IR generation
src/ir/lowerer.c              - AST to IR
src/optimize/optimizer.c      - Optimizations
src/optimize/constfold.c      - Constant folding
src/optimize/dce.c            - Dead code elimination
src/backend/codegen.c         - Code generation
src/backend/regalloc.c        - Register allocation
src/backend/asm.c             - Assembly output
src/backend/dwarf.c           - Debug info
```

**Total**: 17 files (22 source files in project, but some are tests)

---

## Appendix B: Test Cases

### Function Pointer Tests

```c
// test_func_ptr_1.c - Named function pointer parameter
int apply(int (*func)(int), int x) {
    return func(x);
}

int square(int x) { return x * x; }
int main() { return apply(square, 5); } // Returns 25
```

```c
// test_func_ptr_2.c - Function pointer variable
int (*fp)(int, int);

int add(int a, int b) { return a + b; }
int main() {
    fp = add;
    return fp(3, 4); // Returns 7
}
```

```c
// test_func_ptr_3.c - Function pointer typedef
typedef int (*Comparator)(const void*, const void*);

int compare_ints(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

int main() {
    Comparator cmp = compare_ints;
    int arr[] = {3, 1, 2};
    qsort(arr, 3, sizeof(int), cmp);
    return arr[0]; // Returns 1
}
```

---

## Appendix C: Architecture Decisions

### Decision 1: Declarator Parser Approach
**Choice**: Implement dedicated `parse_declarator()` function  
**Rationale**: Matches C grammar, reusable, testable  
**Alternatives Considered**: Inline parsing in `parse_parameter()` (rejected: too complex)

### Decision 2: Function Pointer Type Representation
**Choice**: Use `TYPE_FUNCTION` with `is_pointer` flag  
**Rationale**: Function pointers are just pointers to functions  
**Alternatives**: New `TYPE_FUNCTION_POINTER` kind (rejected: redundant)

### Decision 3: Implementation Order
**Choice**: Parser → Semantic → Codegen  
**Rationale**: Linear dependency chain  
**Alternatives**: Parallel implementation (rejected: higher risk)

---

## Appendix D: References

- **ARCHITECTURE.md**: Detailed compiler architecture
- **TODO.md**: Original task list (107 items)
- **BUGS_IDENTIFIED.md**: Bug analysis from stage1 failures
- **PARSER_ANALYSIS.md**: Parser implementation analysis
- **CRITICAL_THINKING.md**: Meta-analysis of approach

---

**Epic Owner**: Development Team  
**Last Updated**: 2024-03-24  
**Status**: Ready for Sprint 1
