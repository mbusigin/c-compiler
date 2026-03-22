# C Compiler Development Sprints

## Overview

This document outlines 8 sprints to build a complete, production-quality C compiler. Each sprint delivers working software incrementing in capability.

**Total Estimated Duration**: ~16 weeks (self-paced)
**Prerequisites**: C programming, data structures, basic compiler theory

---

## Sprint 1: Foundation & Lexer (Week 1-2)

**Goal**: Create project infrastructure and working lexer

### Tasks

#### 1.1 Project Setup
- Create directory structure
- Set up build system (Makefile)
- Create common utilities (list, memory, errors)
- Set up test framework
```
Deliverable: `make` builds successfully
Test: cd project && make clean && make
```

#### 1.2 Token Definitions
- Define all C11 token types in tokens.h
- Implement token structure with location info
```
Deliverable: tokens.h with complete token enum
Test: grep -c "TOKEN_" project/src/lexer/tokens.h | [ $(cat) -ge 50 ]
```

#### 1.3 Lexer Implementation
- Character classification utilities
- Keyword recognition
- Number literal parsing (decimal, hex, octal, binary)
- String and character literal parsing
- Identifier recognition
- Comment handling
- Whitespace handling
```
Deliverable: Functional lexer that tokenizes C source
Test: echo 'int main() { return 0; }' | ./compiler --dump-tokens - | grep -q "INT"
```

#### 1.4 Lexer Testing
- Test all token types
- Test escape sequences in strings
- Test various number formats
- Error handling for invalid input
```
Deliverable: Lexer passes all unit tests
Test: ./test_lexer | grep -q "All lexer tests passed"
```

---

## Sprint 2: Parser - Declarations (Week 2-3)

**Goal**: Parse C declarations and basic types

### Tasks

#### 2.1 AST Infrastructure
- Define AST node structures for declarations
- Implement AST node allocation and management
- Implement AST printing/debugging
```
Deliverable: AST data structures ready
Test: ./test_ast | grep -q "AST infrastructure OK"
```

#### 2.2 Type System
- Implement primitive types (void, char, int, float, etc.)
- Implement derived types (pointer, array, function)
- Implement struct/union types
- Type comparison and promotion
```
Deliverable: Complete type system
Test: ./test_types | grep -q "Type tests passed"
```

#### 2.3 Declaration Parsing
- Parse variable declarations
- Parse function declarations
- Parse typedef
- Parse extern and static storage
- Parse type qualifiers (const, volatile)
- Parse type specifiers
```
Deliverable: Parser handles all declaration forms
Test: echo 'extern const int x; typedef unsigned long ulong;' | ./compiler --dump-ast | grep -q "Declaration"
```

#### 2.4 Declarator Parsing
- Parse simple declarators
- Parse pointer declarators
- Parse array declarators
- Parse function declarators (with parameter lists)
- Parse abstract declarators
```
Deliverable: Full declarator support
Test: echo 'int (*fp)(int *, const char[]);' | ./compiler --dump-ast | grep -q "FunctionType"
```

#### 2.5 Declaration Testing
- Comprehensive declaration tests
- Error recovery tests
```
Deliverable: All declaration tests pass
Test: ./test_declarations | grep -q "All declaration tests passed"
```

---

## Sprint 3: Parser - Expressions (Week 3-4)

**Goal**: Parse all C expression syntax

### Tasks

#### 3.1 Expression Infrastructure
- Add expression AST nodes
- Expression result types
```
Deliverable: Expression AST nodes defined
Test: grep -c "AST_BINARY_EXPR\|AST_UNARY_EXPR\|AST_CALL_EXPR" project/src/parser/ast.h | [ $(cat) -ge 10 ]
```

#### 3.2 Primary Expressions
- Parse identifiers
- Parse constants (integer, float, character)
- Parse string literals
- Parse parenthesized expressions
```
Deliverable: Primary expression parsing works
Test: echo 'x 42 3.14 "hello"' | ./compiler --dump-tokens | grep -c "IDENTIFIER\|CONSTANT\|STRING" | [ $(cat) -ge 4 ]
```

#### 3.3 Postfix Expressions
- Array subscript operator
- Function call operator
- Member access operator (.)
- Pointer member access (->)
- Postfix ++ and --
```
Deliverable: Postfix expressions fully parsed
Test: echo 'a[0]->b.c[i](x, y)' | ./compiler --dump-ast | grep -q "ArraySubscript"
```

#### 3.4 Unary Expressions
- Prefix ++ and --
- Unary +, -
- Logical NOT (!)
- Bitwise NOT (~)
- Address-of (&)
- Indirection (*)
- sizeof (both forms)
```
Deliverable: All unary operators parsed
Test: echo '!x ~y &z *w sizeof(int)' | ./compiler --dump-ast | grep -q "Unary"
```

#### 3.5 Cast Expressions
- Parse type casts
- Cast precedence rules
```
Deliverable: Cast expressions work
Test: echo '(int)(char)x' | ./compiler --dump-ast | grep -q "Cast"
```

#### 3.6 Binary Expressions
- Implement precedence climbing parser
- Parse all binary operators with correct precedence
- 15 precedence levels per C standard
```
Deliverable: Full operator precedence
Test: echo 'a+b*c-d/e%f' | ./compiler --dump-ast | verify precedence is correct
```

#### 3.7 Conditional & Assignment
- Ternary conditional (?:)
- All assignment operators (=, +=, -=, etc.)
```
Deliverable: Conditional and assignment expressions
Test: echo 'a ? b : c = d += e' | ./compiler --dump-ast | grep -q "Conditional"
```

#### 3.8 Comma Operator
- Parse comma/sequence operator
```
Deliverable: Comma operator support
Test: echo '{ a, b, c; }' | ./compiler --dump-ast | verify comma operator parsed
```

---

## Sprint 4: Parser - Statements (Week 4-5)

**Goal**: Parse all C statement syntax

### Tasks

#### 4.1 Statement AST Nodes
- Add statement AST node types
- Statement result structure
```
Deliverable: All statement types in AST
Test: grep -c "AST_IF_STMT\|AST_FOR_STMT\|AST_RETURN_STMT" project/src/parser/ast.h | [ $(cat) -ge 8 ]
```

#### 4.2 Compound Statement
- Parse block statements ({ })
- Parse local declarations in blocks
```
Deliverable: Block statements work
Test: echo '{ int x; x = 1; }' | ./compiler --dump-ast | grep -q "Compound"
```

#### 4.3 Selection Statements
- if/else statement
- switch statement
- case and default labels
```
Deliverable: if/else and switch statements
Test: echo 'if (x) if (y) a; else b; else c;' | ./compiler --dump-ast | verify dangling else resolved correctly
```

#### 4.4 Iteration Statements
- while loop
- do-while loop
- for loop (all forms)
```
Deliverable: All loop forms
Test: echo 'for (int i=0; i<n; i++) { sum += i; }' | ./compiler --dump-ast | grep -q "For"
```

#### 4.5 Jump Statements
- return (with and without value)
- break
- continue
- goto and labels
```
Deliverable: All jump statements
Test: echo 'goto label; label: return x;' | ./compiler --dump-ast | grep -q "Goto"
```

#### 4.6 Expression Statement
- Expression followed by semicolon
- Null statement
```
Deliverable: Expression statements work
Test: echo 'x++; ;' | ./compiler --dump-ast | verify null statement parsed
```

#### 4.7 Full Program Parsing
- Parse complete translation units
- Handle multiple declarations and functions
```
Deliverable: Full C programs parse correctly
Test: ./compiler tests/hello.c --dump-ast | verify complete AST output
```

---

## Sprint 5: Semantic Analysis (Week 5-7)

**Goal**: Implement type checking and symbol management

### Tasks

#### 5.1 Symbol Table
- Implement scope management
- Stack-based scope traversal
- Insert, lookup, shadow handling
```
Deliverable: Working symbol table
Test: ./test_symtab | grep -q "Symbol table tests passed"
```

#### 5.2 Declaration Processing
- Register declarations in symbol table
- Handle forward declarations
- Process function prototypes
- Process extern declarations
```
Deliverable: Declarations properly registered
Test: echo 'int f(int x); int f(int x) { return x; }' | ./compiler --dump-ast | verify symbol registered
```

#### 5.3 Expression Type Checking
- Determine expression result types
- Check assignment compatibility
- Check binary operator operand types
- Check unary operator operand types
```
Deliverable: Expression types correctly computed
Test: echo 'int x; long y; x + y' | ./compiler --check 2>&1 | verify long result type
```

#### 5.4 Function Type Checking
- Check function call arguments
- Check function return types
- Check function definition matches prototype
```
Deliverable: Function calls type-checked
Test: echo 'void f(int); f(1.0);' | ./compiler --check 2>&1 | grep -q "warning\|error"
```

#### 5.5 Control Flow Analysis
- Verify break/continue in loops
- Verify return statements
- Warn about missing returns
```
Deliverable: Control flow validated
Test: echo 'int f() { }' | ./compiler --check 2>&1 | grep -q "warning.*return"
```

#### 5.6 Struct/Union Analysis
- Build struct member symbol tables
- Type checking for member access
- Size calculation for structs/unions
```
Deliverable: Struct/union semantics work
Test: echo 'struct S { int x; char y; }; struct S s; s.x = 1;' | ./compiler --check
```

#### 5.7 Constant Expression Evaluation
- Evaluate integer constant expressions
- Evaluate enum values
- Handle sizeof with constant expressions
```
Deliverable: Constant expressions evaluated
Test: echo 'enum { A = 1 + 2 * 3 }; int x[4];' | ./compiler --check | verify array size is 4
```

#### 5.8 Semantic Error Reporting
- Source location tracking
- Clear error messages
- Recovery from errors
```
Deliverable: Good error messages
Test: echo 'int x = y + 1;' | ./compiler --check 2>&1 | grep -q "undeclared"
```

---

## Sprint 6: IR Generation & Lowering (Week 7-9)

**Goal**: Generate intermediate representation from AST

### Tasks

#### 6.1 IR Definitions
- Define IR instruction types
- Define basic block structure
- Define function structure
```
Deliverable: IR data structures complete
Test: grep -c "IR_ADD\|IR_LOAD\|IR_CALL" project/src/ir/ir.h | [ $(cat) -ge 30 ]
```

#### 6.2 AST to IR Lowering
- Walk AST and generate IR
- Lower expressions to sequences
- Lower statements to control flow
```
Deliverable: AST lowered to IR
Test: echo 'int f() { return 1; }' | ./compiler --dump-ir | grep -q "ret"
```

#### 6.3 Control Flow Lowering
- Generate basic blocks
- Handle if/else branching
- Handle loops (while, for)
- Handle switch statements
```
Deliverable: Control flow IR generated
Test: echo 'if (x) a; else b;' | ./compiler --dump-ir | verify jmp_if generated
```

#### 6.4 Expression Lowering
- Lower arithmetic expressions
- Lower function calls
- Lower memory operations (load/store)
- Lower casts
```
Deliverable: All expressions lowered
Test: echo 'int f(int a, int b) { return a + b * 2; }' | ./compiler --dump-ir | verify multiplication
```

#### 6.5 Function Call Lowering
- Parameter passing
- Return value handling
- Caller/callee saved registers
```
Deliverable: Function calls work
Test: echo 'int g(int x) { return x + 1; } int f() { return g(5); }' | ./compiler --dump-ir | verify call
```

#### 6.6 Global Variables
- Global variable IR generation
- Static vs extern linkage
```
Deliverable: Global variables in IR
Test: echo 'int x = 5; int f() { return x; }' | ./compiler --dump-ir | verify global x
```

#### 6.7 String Literals
- String literal storage
- String to data section
```
Deliverable: String literals generated
Test: echo 'char* s = "hello";' | ./compiler --dump-ir | verify string literal
```

---

## Sprint 7: Optimization (Week 9-11)

**Goal**: Implement optimization passes

### Tasks

#### 7.1 CFG Construction
- Build control flow graph
- Compute dominators
- Find loops and loop headers
```
Deliverable: CFG built correctly
Test: ./test_cfg | grep -q "CFG tests passed"
```

#### 7.2 Constant Folding
- Fold constant arithmetic
- Fold constant comparisons
- Fold constant casts
```
Deliverable: Constant expressions folded
Test: echo 'int x = 5 + 3;' | ./compiler -O1 --dump-ir | verify x = 8
```

#### 7.3 Copy Propagation
- Track variable assignments
- Replace uses with copies
- Remove redundant copies
```
Deliverable: Copy propagation working
Test: echo 'int x = a; y = x + 1;' | ./compiler -O1 --dump-ir | verify y = a + 1
```

#### 7.4 Dead Code Elimination
- Remove unreachable blocks
- Remove unused computations
- Remove dead stores
```
Deliverable: Dead code removed
Test: echo 'int f() { 5; return 0; }' | ./compiler -O1 --dump-ir | verify no store to discarded value
```

#### 7.5 Common Subexpression Elimination
- Find redundant expressions
- Eliminate repeated computations
```
Deliverable: CSE implemented
Test: echo 'int x = a + b; int y = a + b;' | ./compiler -O1 --dump-ir | verify only one addition
```

#### 7.6 Loop Optimizations
- Loop invariant code motion
- Induction variable elimination
- Strength reduction
```
Deliverable: Loop optimizations work
Test: echo 'for (int i=0; i<n; i++) a[i] = i * 2;' | ./compiler -O2 | verify efficient code
```

#### 7.7 Function Inlining
- Inline small functions
- Handle recursive functions (don't infinite inline)
```
Deliverable: Inlining works
Test: echo 'inline int sq(int x) { return x*x; } int f() { return sq(5); }' | ./compiler -O2 --dump-ir | verify inlined
```

#### 7.8 Optimization Pipeline
- Order optimization passes
- Handle pass dependencies
- Make optimization optional (-O0, -O1, -O2)
```
Deliverable: Optimization levels work
Test: ./compiler tests/loop.c -O0 --dump-ir | wc -l > /tmp/o0.txt; ./compiler tests/loop.c -O2 --dump-ir | wc -l > /tmp/o2.txt; [ $(cat /tmp/o2.txt) -lt $(cat /tmp/o0.txt) ]
```

---

## Sprint 8: Code Generation (Week 11-14)

**Goal**: Generate x86-64 assembly from IR

### Tasks

#### 8.1 Register Allocation Infrastructure
- Define register set
- Track register availability
- Handle register spilling
```
Deliverable: Register allocator skeleton
Test: grep -c "rax\|rsp\|rbp\|rdi" project/src/backend/regalloc.c | [ $(cat) -ge 20 ]
```

#### 8.2 Register Allocation
- Build interference graph
- Implement graph coloring
- Handle spills
```
Deliverable: Working register allocation
Test: echo 'int f(int a, int b, int c, int d, int e, int f, int g, int h) { return a+b+c+d+e+f+g+h; }' | ./compiler --dump-asm | verify all args used
```

#### 8.3 x86-64 Instruction Selection
- Map IR to x86-64 instructions
- Handle addressing modes
- Implement function prologue/epilogue
```
Deliverable: Basic instruction selection
Test: echo 'int f() { return 0; }' | ./compiler --dump-asm | grep -q "xorl.*%eax"
```

#### 8.4 Arithmetic Instruction Generation
- Generate add, sub, mul, div
- Generate bitwise operations
- Generate shifts
```
Deliverable: All arithmetic ops emit x86-64
Test: echo 'int f(int a, int b) { return (a + b) * (a - b); }' | ./compiler --dump-asm | verify imul, add, sub
```

#### 8.5 Control Flow Generation
- Generate conditional jumps
- Generate loops
- Generate switch (jump table)
```
Deliverable: Control flow x86-64 code
Test: echo 'int f(int x) { if (x > 0) return 1; else return 0; }' | ./compiler --dump-asm | verify jg
```

#### 8.6 Function Call Generation
- Implement calling convention
- Pass parameters in registers
- Handle stack parameters
- Handle return values
```
Deliverable: Function calls work
Test: echo 'int g(int x) { return x + 1; } int f() { return g(5); }' | ./compiler --dump-asm | verify call
```

#### 8.7 Memory Operations
- Generate load/store
- Handle structure copies
- Handle stack frames
```
Deliverable: Memory operations work
Test: echo 'int f(int* p) { return *p; }' | ./compiler --dump-asm | verify movl
```

#### 8.8 Global Variables & Strings
- Generate .data section
- Generate .rodata for constants
- Handle string literals
```
Deliverable: Data section generation
Test: echo 'int x = 42; char* s = "hello";' | ./compiler --dump-asm | grep -q "\.data"
```

#### 8.9 Full Compilation Pipeline
- End-to-end compilation
- Link with libc
- Generate working executables
```
Deliverable: Compiler produces working executables
Test: echo '#include <stdio.h>\nint main() { printf("Hello, World!\\n"); return 0; }' > /tmp/hello.c && ./compiler /tmp/hello.c -o /tmp/hello && /tmp/hello | grep -q "Hello, World!"
```

---

## Sprint 9: Testing & Polish (Week 14-16)

**Goal**: Comprehensive testing and bug fixes

### Tasks

#### 9.1 Test Suite
- Write comprehensive unit tests
- Write integration tests
- Write regression tests
```
Deliverable: Full test suite
Test: make test | grep -q "All tests passed"
```

#### 9.2 Compiler Self-Hosting Preparation
- Identify compiler-specific features needed
- Fix any hardcoded limitations
```
Deliverable: Compiler ready for self-hosting
Test: ./compiler --version | grep -q "C-Compiler"
```

#### 9.3 Reference Comparison
- Compare with GCC/Clang output
- Fix correctness issues
- Optimize generated code
```
Deliverable: Code quality comparable to GCC
Test: Compare execution results of programs compiled with ./compiler vs gcc
```

#### 9.4 Error Handling Polish
- Improve error messages
- Better source location reporting
- Recovery from errors
```
Deliverable: Good error messages
Test: echo 'int x = ;' | ./compiler -c 2>&1 | grep -q "expected"
```

#### 9.5 Documentation
- Write user documentation
- Document internals
- Write examples
```
Deliverable: Complete documentation
Test: test -f project/docs/compiler.md && test -f project/docs/architecture.md
```

#### 9.6 Performance Optimization
- Profile compiler performance
- Optimize hot paths
- Reduce memory usage
```
Deliverable: Fast compiler
Test: time ./compiler tests/large_program.c -o /tmp/out 2>&1 | verify completes in reasonable time
```

---

## Sprint Summary

| Sprint | Duration | Deliverable |
|--------|----------|-------------|
| 1 | 2 weeks | Working lexer |
| 2 | 2 weeks | Declarations parser |
| 3 | 2 weeks | Expression parser |
| 4 | 2 weeks | Statement parser |
| 5 | 3 weeks | Semantic analyzer |
| 6 | 3 weeks | IR generation |
| 7 | 3 weeks | Optimizer |
| 8 | 4 weeks | x86-64 codegen |
| 9 | 3 weeks | Testing & polish |
| **Total** | **~24 weeks** | **Production C compiler** |

---

## Milestones

1. **M1 (Week 2)**: Lexer complete - can tokenize simple C
2. **M2 (Week 5)**: Parser complete - can parse any valid C
3. **M3 (Week 7)**: Semantic analysis complete - type checking works
4. **M4 (Week 9)**: IR generation complete - IR from AST
5. **M5 (Week 11)**: Optimization complete - -O1 works
6. **M6 (Week 14)**: Code generation complete - executables work
7. **M7 (Week 16)**: Production ready - passes test suite

---

## Daily Workflow

1. Review task for the day
2. Implement feature in isolation
3. Write tests first (TDD)
4. Verify with test command
5. Commit with clear message
6. Move to next task

## Definition of Done

Each task is complete when:
- Feature implemented
- All tests pass
- Code reviewed
- Documentation updated
