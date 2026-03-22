# C Compiler TODO - Detailed Task List

## Project Structure

```
project/
├── ARCHITECTURE.md      # Compiler architecture design
├── SPRINTS.md           # Sprint breakdown
├── TODO.md              # This file - detailed tasks
├── src/
│   ├── main.c
│   ├── driver.c
│   ├── common/
│   ├── lexer/
│   ├── parser/
│   ├── sema/
│   ├── ir/
│   ├── optimize/
│   └── backend/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── benchmarks/
└── docs/
```

---

## Core Infrastructure

### [ ] 1. Build System Setup
- [ ] Create Makefile with all targets
- [ ] Define compiler flags (-Wall, -Wextra, -Werror)
- [ ] Set up debug and release builds
- [ ] Configure test targets
- **Test**: `make clean && make && ./compiler --version`

### [ ] 2. Memory Management
- [ ] Arena allocator for AST nodes
- [ ] String interning
- [ ] Memory tracking for debugging
- [ ] Cleanup functions
- **Test**: `make test 2>&1 | head -20 | grep -q "Memory"`

### [ ] 3. Dynamic Arrays (List)
- [ ] Generic list implementation
- [ ] Push, pop, get, set operations
- [ ] Iterator support
- [ ] Memory management
- **Test**: `./test_list | grep -q "passed"`

### [ ] 4. String Utilities
- [ ] String concatenation
- [ ] String comparison
- [ ] String hashing
- [ ] String formatting (snprintf wrapper)
- **Test**: `grep -c "str_" src/common/util.c | [ $(cat) -ge 10 ]`

### [ ] 5. Error Reporting
- [ ] Source location tracking
- [ ] Error message formatting
- [ ] Warning system
- [ ] Note system for context
- [ ] Error recovery support
- **Test**: `echo 'invalid' | ./compiler 2>&1 | grep -q "error" && echo "Error reporting works"`

---

## Lexer (scanner.c)

### [ ] 6. Token Definitions
- [ ] Define TokenType enum
- [ ] Token structure with location
- [ ] Token kinds (keyword, identifier, literal, operator, punctuation)
- [ ] Preprocessor token types
- **Test**: `grep -E "TOKEN_[A-Z]+\s*=" src/lexer/tokens.h | wc -l | [ $(cat) -ge 40 ]`

### [ ] 7. Character Classification
- [ ] Is digit, is hex digit
- [ ] Is alphabetic, is alphanumeric
- [ ] Is whitespace, is newline
- [ ] Is identifier start, is identifier character
- [ ] Escape sequence classification
- **Test**: `./test_lexer --test-classification`

### [ ] 8. Keyword Recognition
- [ ] All C11 keywords (32 keywords)
- [ ] Context-sensitive keyword detection (typedef vs variable)
- **Test**: `grep -c "KEYWORD" src/lexer/tokens.h | [ $(cat) -ge 32 ]`

### [ ] 9. Number Literal Parsing
- [ ] Decimal integers (123, 0)
- [ ] Hexadecimal (0x, 0X prefixes)
- [ ] Octal (0 prefix: 0755)
- [ ] Binary (0b, 0B prefixes)
- [ ] Suffixes (u, ul, ull, l, f)
- [ ] Floating point (1.0, .5, 1e10, 1.0e-5f)
- [ ] Scientific notation
- **Test**: `echo '0xFF 0755 0b1010 3.14e-10' | ./compiler --dump-tokens | grep -c "NUMBER"`

### [ ] 10. String Literal Parsing
- [ ] Basic strings ("hello")
- [ ] Escape sequences (\n, \t, \\, \", etc.)
- [ ] Hex escapes (\xFF)
- [ ] Unicode escapes (\u, \U)
- [ ] String concatenation ("hello" "world")
- [ ] Wide strings (L"")
- **Test**: `echo '"Hello\nWorld"' | ./compiler --dump-tokens | grep -c "STRING"`

### [ ] 11. Character Literal Parsing
- [ ] Single characters ('a')
- [ ] Escape sequences
- [ ] Multi-character constants
- **Test**: `echo "'\\n' '\\x41'" | ./compiler --dump-tokens | grep -c "CHAR"`

### [ ] 12. Comment Handling
- [ ] Single-line comments (//)
- [ ] Multi-line comments (/* */)
- [ ] Nested comments (non-standard, optional)
- [ ] Comment preservation (for -C option)
- **Test**: `echo '/* comment */ // comment' | ./compiler --dump-tokens | grep -qv "comment"`

### [ ] 13. Lexer Driver
- [ ] Next token function
- [ ] Peek ahead
- [ ] Position tracking
- [ ] Error reporting
- [ ] End of file handling
- **Test**: `echo 'int main() { return 0; }' | ./compiler --dump-tokens | grep -c "TOKEN"`

### [ ] 14. Preprocessor Lexing
- [ ] #define directive
- [ ] #include directive
- [ ] #if, #ifdef, #ifndef, #else, #elif, #endif
- [ ] #pragma directive
- **Test**: `echo '#define X 42' | ./compiler --dump-tokens | grep "HASH"`

---

## Parser (parser.c)

### [ ] 15. AST Infrastructure
- [ ] ASTNode structure
- [ ] Node type enum
- [ ] Node allocation
- [ ] Node list management
- [ ] AST printing (debug)
- [ ] AST serialization (optional)
- **Test**: `echo 'int x;' | ./compiler --dump-ast | grep -q "VariableDecl"`

### [ ] 16. Type System - Basics
- [ ] Primitive types enum
- [ ] Type structure
- [ ] Type equality
- [ ] Type printing
- [ ] Integer types (char, short, int, long, long long)
- [ ] Floating types (float, double, long double)
- **Test**: `grep -c "TYPE_INT\|TYPE_CHAR\|TYPE_FLOAT" src/sema/type.c | [ $(cat) -ge 5 ]`

### [ ] 17. Type System - Derived Types
- [ ] Pointer type
- [ ] Array type (with and without size)
- [ ] Function type
- [ ] Type qualifier (const, volatile, restrict)
- [ ] Type specifier combinations
- **Test**: `echo 'int (*p)[10];' | ./compiler --dump-ast | grep -q "PointerType"`

### [ ] 18. Type System - Struct/Union/Enum
- [ ] Struct type with members
- [ ] Union type with members
- [ ] Enum type with values
- [ ] Bit-field support (struct members)
- [ ] Incomplete types
- **Test**: `echo 'struct S { int x; };' | ./compiler --dump-ast | grep -q "StructType"`

### [ ] 19. Type System - Utilities
- [ ] Type size calculation
- [ ] Type alignment
- [ ] Integer promotions
- [ ] Usual arithmetic conversions
- [ ] Type compatibility
- [ ] Composite types
- **Test**: `./test_types | grep -q "Type system tests passed"`

### [ ] 20. Declaration Parsing - Basics
- [ ] Simple variable declarations
- [ ] Multiple declarators
- [ ] Initialization (simple)
- [ ] Storage class specifiers (auto, static, extern, register)
- **Test**: `echo 'int x, y = 5;' | ./compiler --dump-ast | grep -c "VarDecl"`

### [ ] 21. Declaration Parsing - Declarators
- [ ] Simple declarator (identifier)
- [ ] Pointer declarator (*)
- [ ] Array declarator []
- [ ] Function declarator ()
- [ ] Abstract declarator
- [ ] Parameter declarations
- [ ] Function definition with body
- **Test**: `echo 'int (*fp)(int*, char[]);' | ./compiler --dump-ast | grep -q "FunctionType"`

### [ ] 22. Declaration Parsing - Complex
- [ ] Complex declarators: int **ptr;
- [ ] Function returning pointer: int *f(void);
- [ ] Pointer to function: int (*fp)(void);
- [ ] Array of pointers: int *arr[10];
- [ ] Pointer to array: int (*p)[10];
- [ ] Toplevel qualifiers: const int x;
- **Test**: `echo 'int **matrix[5][10];' | ./compiler --dump-ast | verify correct nesting`

### [ ] 23. Declaration Parsing - Typedef
- [ ] Basic typedef
- [ ] Complex typedef
- [ ] Typedef redeclaration
- **Test**: `echo 'typedef int* IntPtr;' | ./compiler --dump-ast | grep -q "TypedefDecl"`

### [ ] 24. Expression Parsing - Primary
- [ ] Identifier reference
- [ ] Integer constant
- [ ] Float constant
- [ ] Character constant
- [ ] String literal
- [ ] Parenthesized expression
- **Test**: `echo 'x + 42 + 3.14 + "str"' | ./compiler --dump-ast | grep -c "Expr"`

### [ ] 25. Expression Parsing - Postfix
- [ ] Array subscript: a[i]
- [ ] Function call: f(a, b)
- [ ] Member access: s.member
- [ ] Pointer member: p->member
- [ ] Postfix ++: i++
- [ ] Postfix --: i--
- [ ] Compound literal (C99)
- [ ] Designated initializer access
- **Test**: `echo 'a[0]->b.c[i](x, y)++' | ./compiler --dump-ast | grep -q "Postfix"`

### [ ] 26. Expression Parsing - Unary
- [ ] Prefix ++: ++i
- [ ] Prefix --: --i
- [ ] Unary +: +x
- [ ] Unary -: -x
- [ ] Logical not: !x
- [ ] Bitwise not: ~x
- [ ] Address-of: &x
- [ ] Indirection: *p
- [ ] sizeof (expression form)
- [ ] sizeof (type form)
- [ ] _Alignof (C11)
- **Test**: `echo '++x --y +x -x !a ~b &c *d sizeof(*p)' | ./compiler --dump-ast | grep -c "Unary"`

### [ ] 27. Expression Parsing - Cast
- [ ] Cast expression
- [ ] Parenthesized type name
- [ ] Cast precedence
- **Test**: `echo '(int)(char)x + (long)y' | ./compiler --dump-ast | grep -c "Cast"`

### [ ] 28. Expression Parsing - Binary
- [ ] Multiplicative: *, /, %
- [ ] Additive: +, -
- [ ] Shift: <<, >>
- [ ] Relational: <, >, <=, >=
- [ ] Equality: ==, !=
- [ ] Bitwise AND: &
- [ ] Bitwise XOR: ^
- [ ] Bitwise OR: |
- [ ] Logical AND: &&
- [ ] Logical OR: ||
- [ ] Correct precedence
- **Test**: `echo 'a+b*c-d/e | f&g ^ h|i && j||k' | ./compiler --dump-ast | verify precedence`

### [ ] 29. Expression Parsing - Conditional
- [ ] Ternary conditional: a ? b : c
- [ ] Nested conditionals
- [ ] Omitted true expression
- **Test**: `echo 'a ? b ? c : d : e' | ./compiler --dump-ast | grep -c "Conditional"`

### [ ] 30. Expression Parsing - Assignment
- [ ] Simple assignment: =
- [ ] Compound assignment: +=, -=, *=, /=, %=, <<=, >>=, &=, ^=, |=
- [ ] Lvalue check
- **Test**: `echo 'a += b -= c *= d' | ./compiler --dump-ast | grep -c "Assign"`

### [ ] 31. Expression Parsing - Comma
- [ ] Comma operator
- [ ] Comma in function arguments
- [ ] Sequence evaluation
- **Test**: `echo 'a = (b, c, d)' | ./compiler --dump-ast | grep -c "Comma"`

### [ ] 32. Statement Parsing - Compound
- [ ] Block statement { }
- [ ] Local variable declarations
- [ ] Statement list
- **Test**: `echo '{ int x = 1; x++; }' | ./compiler --dump-ast | grep -q "Compound"`

### [ ] 33. Statement Parsing - Selection
- [ ] if statement
- [ ] if-else statement
- [ ] Dangling else resolution
- [ ] switch statement
- [ ] case label
- [ ] default label
- [ ] Case fall-through
- **Test**: `echo 'if (x) if (y) a; else b; else c;' | ./compiler --dump-ast | verify correct structure`

### [ ] 34. Statement Parsing - Iteration
- [ ] while loop
- [ ] do-while loop
- [ ] for loop (all forms)
- [ ] for with declarations (C99)
- [ ] Comma operator in for
- **Test**: `echo 'for (int i=0; i<n; i++) sum += i;' | ./compiler --dump-ast | grep -q "For"`

### [ ] 35. Statement Parsing - Jump
- [ ] return (with value)
- [ ] return (without value)
- [ ] break statement
- [ ] continue statement
- [ ] goto statement
- [ ] Labeled statement
- **Test**: `echo 'goto L; L: return x;' | ./compiler --dump-ast | grep -c "Goto\|Label\|Return"`

### [ ] 36. Statement Parsing - Expression
- [ ] Expression statement
- [ ] Null statement
- **Test**: `echo 'x++; ;' | ./compiler --dump-ast | grep -c "Null"`

### [ ] 37. Function Parsing
- [ ] Function definition
- [ ] Parameter list
- [ ] Old-style parameter declarations (K&R)
- [ ] Function body
- [ ] Empty parameter list (void)
- **Test**: `echo 'int f(int x, char* y) { return x; }' | ./compiler --dump-ast | grep -q "FunctionDecl"`

### [ ] 38. Translation Unit
- [ ] Multiple top-level declarations
- [ ] Function prototypes
- [ ] External declarations
- [ ] Linkage specifications
- **Test**: `echo 'int a; int b = 5; int f() { return a+b; }' | ./compiler --dump-ast | grep -c "VarDecl\|FunctionDecl"`

### [ ] 39. Initializer Lists
- [ ] Scalar initializer
- [ ] Array initializer
- [ ] Struct initializer
- [ ] Designated initializers (C99)
- [ ] Nested initializers
- **Test**: `echo 'int arr[3] = { [1] = 5 };' | ./compiler --dump-ast | verify structure`

### [ ] 40. Error Recovery
- [ ] Synchronization points
- [ ] Panic mode recovery
- [ ] Error message improvement
- **Test**: `echo 'int x = ;' | ./compiler 2>&1 | grep -q "error"`

---

## Semantic Analysis (analyzer.c)

### [ ] 41. Symbol Table - Basics
- [ ] Symbol structure
- [ ] Symbol kinds (variable, function, type, label, param)
- [ ] Insert symbol
- [ ] Lookup symbol
- [ ] Scope management
- **Test**: `grep -c "sym_insert\|sym_lookup" src/sema/symtab.c | [ $(cat) -ge 5 ]`

### [ ] 42. Symbol Table - Scopes
- [ ] Global scope
- [ ] Function scope
- [ ] Block scope
- [ ] Scope stack
- [ ] Scope enter/exit
- **Test**: `./test_symtab | grep -q "Symbol table tests passed"`

### [ ] 43. Declaration Processing
- [ ] Variable declarations
- [ ] Function declarations
- [ ] Type declarations
- [ ] Label declarations
- [ ] Shadow handling
- **Test**: `echo 'int x; int x;' | ./compiler -c 2>&1 | grep -q "warning.*shadow"`

### [ ] 44. Type Checking - Expressions
- [ ] Determine expression type
- [ ] Check assignment types
- [ ] Check arithmetic operand types
- [ ] Check comparison types
- [ ] Check logical operator types
- **Test**: `echo 'int x; long y; x + y' | ./compiler -fsyntax-only 2>&1 | verify no errors`

### [ ] 45. Type Checking - Functions
- [ ] Function call checking
- [ ] Argument type matching
- [ ] Return type checking
- [ ] Function declaration matching
- **Test**: `echo 'void f(int); f(1.0);' | ./compiler -c 2>&1 | grep -q "warning"`

### [ ] 46. Type Checking - Lvalue
- [ ] Modify lvalue check
- [ ] Address-of lvalue check
- [ ] Assignment lvalue check
- **Test**: `echo '3 = 4;' | ./compiler -c 2>&1 | grep -q "error"`

### [ ] 47. Type Checking - Conversions
- [ ] Integer promotions
- [ ] Usual arithmetic conversions
- [ ] Implicit casts
- [ ] Float to int conversions
- [ ] Pointer conversions
- **Test**: `echo 'char c = 300;' | ./compiler -c 2>&1 | grep -q "warning"`

### [ ] 48. Control Flow Analysis
- [ ] Break scope checking
- [ ] Continue scope checking
- [ ] Return path analysis
- [ ] Unreachable code detection
- **Test**: `echo 'int f() { break; }' | ./compiler -c 2>&1 | grep -q "error"`

### [ ] 49. Constant Expression Evaluation
- [ ] Integer constant expressions
- [ ] Enum value evaluation
- [ ] sizeof constant expressions
- [ ] Array size evaluation
- **Test**: `echo 'int arr[1+2];' | ./compiler --dump-ast | verify arr size is 3`

### [ ] 50. Struct/Union Analysis
- [ ] Member type checking
- [ ] Member access type checking
- [ ] Size calculation
- [ ] Offset calculation
- [ ] Incomplete type checking
- **Test**: `echo 'struct S s; s.x = 1;' | ./compiler -c && echo "OK"`

### [ ] 51. Linkage Analysis
- [ ] extern linkage
- [ ] static linkage
- [ ] tentative definition
- [ ] weak linkage
- **Test**: `echo 'static int x; int f() { return x; }' | ./compiler -c && echo "OK"`

### [ ] 52. Symbol Resolution
- [ ] Resolve identifier references
- [ ] Resolve type names
- [ ] Resolve labels
- [ ] Forward reference handling
- **Test**: `echo 'int f() { return g(); } int g() { return 1; }' | ./compiler -c && echo "OK"`

---

## IR Generation (lowerer.c)

### [ ] 53. IR Definitions
- [ ] IROpcode enum
- [ ] IRType enum
- [ ] Value structure
- [ ] Instruction structure
- [ ] Basic block structure
- [ ] Function structure
- **Test**: `grep -c "IR_ADD\|IR_LOAD\|IR_CALL\|IR_JMP" src/ir/ir.h | [ $(cat) -ge 25 ]`

### [ ] 54. IR Builder
- [ ] Create basic blocks
- [ ] Append instructions
- [ ] Manage block list
- [ ] Block utility functions
- **Test**: `echo 'int f() { return 0; }' | ./compiler --dump-ir | grep -q "function"`

### [ ] 55. AST to IR Lowering
- [ ] Walk AST
- [ ] Create IR instructions
- [ ] Map AST nodes to IR values
- **Test**: `echo 'int f() { return 1; }' | ./compiler --dump-ir | grep -q "ret"`

### [ ] 56. Expression Lowering
- [ ] Constant lowering
- [ ] Identifier lowering
- [ ] Binary expression lowering
- [ ] Unary expression lowering
- [ ] Cast lowering
- **Test**: `echo 'int f(int a, int b) { return a + b; }' | ./compiler --dump-ir | grep -q "add"`

### [ ] 57. Call Lowering
- [ ] Function call lowering
- [ ] Argument lowering
- [ ] Return value handling
- [ ] Indirect calls
- **Test**: `echo 'int g(int x) { return x+1; } int f() { return g(5); }' | ./compiler --dump-ir | grep -q "call"`

### [ ] 58. Memory Lowering
- [ ] Load instruction
- [ ] Store instruction
- [ ] Alloca instruction
- [ ] GEP instruction
- **Test**: `echo 'int f(int* p) { int x = *p; return x; }' | ./compiler --dump-ir | grep -q "load"`

### [ ] 59. Control Flow Lowering
- [ ] Branch instruction
- [ ] Conditional branch
- [ ] Return instruction
- [ ] Phi node preparation
- **Test**: `echo 'int f(int x) { if (x) return 1; else return 0; }' | ./compiler --dump-ir | grep -q "br"`

### [ ] 60. Loop Lowering
- [ ] While loop
- [ ] For loop
- [ ] Do-while loop
- [ ] Break/continue
- **Test**: `echo 'int sum(int n) { int s=0; for(int i=0;i<n;i++) s+=i; return s; }' | ./compiler --dump-ir | grep -c "br\|label"`

### [ ] 61. Switch Lowering
- [ ] Jump table generation
- [ ] Case handling
- [ ] Default handling
- **Test**: `echo 'int f(int x) { switch(x) { case 1: return 1; case 2: return 2; default: return 0; } }' | ./compiler --dump-ir`

### [ ] 62. Global Variable Lowering
- [ ] Global variable IR
- [ ] Static variables
- [ ] Constant folding for globals
- **Test**: `echo 'int x = 42; int f() { return x; }' | ./compiler --dump-ir | grep -q "global"`

### [ ] 63. String Literal Lowering
- [ ] String literal storage
- [ ] .rodata section
- [ ] String addressing
- **Test**: `echo 'char* s = "hello";' | ./compiler --dump-ir | grep -q "string"`

---

## Optimization (optimizer.c)

### [ ] 64. CFG Construction
- [ ] Build control flow graph
- [ ] Compute predecessors
- [ ] Find dominators
- [ ] Find loop headers
- **Test**: `./test_cfg | grep -q "CFG tests passed"`

### [ ] 65. Constant Folding
- [ ] Fold arithmetic constants
- [ ] Fold comparison constants
- [ ] Fold cast constants
- [ ] Fold logical expressions
- **Test**: `echo 'int x = 5 + 3;' | ./compiler -O1 --dump-ir | grep "8" | grep -q "const"`

### [ ] 66. Copy Propagation
- [ ] Track copies
- [ ] Replace uses
- [ ] Remove redundant copies
- **Test**: `echo 'int f(int a) { int b = a; return b + 1; }' | ./compiler -O1 --dump-ir | verify a used directly`

### [ ] 67. Dead Code Elimination
- [ ] Remove unreachable blocks
- [ ] Remove unused instructions
- [ ] Remove dead stores
- [ ] Remove dead calls
- **Test**: `echo 'int f() { 5; return 0; }' | ./compiler -O1 --dump-ir | verify no dead store`

### [ ] 68. Common Subexpression Elimination
- [ ] Find available expressions
- [ ] Eliminate redundant expressions
- [ ] Handle memory dependencies
- **Test**: `echo 'int f(int a, int b) { int x = a + b; int y = a + b; return x + y; }' | ./compiler -O2 --dump-ir | grep "add" | wc -l | [ $(cat) -eq 2 ]`

### [ ] 69. Loop Optimizations
- [ ] Loop invariant code motion
- [ ] Induction variable elimination
- [ ] Strength reduction (multiplication to shifts)
- **Test**: `echo 'int f(int n) { int s=0; for(int i=0;i<n;i++) s+=5; return s; }' | ./compiler -O2 | verify efficient code`

### [ ] 70. Function Inlining
- [ ] Identify inlineable functions
- [ ] Inline small functions
- [ ] Handle recursive cases
- [ ] Update call graph
- **Test**: `echo 'inline int sq(int x) { return x*x; } int f() { return sq(5); }' | ./compiler -O2 --dump-ir | grep -v "define @sq" | grep -q "mul"`

### [ ] 71. DCE (Full)
- [ ] Iterative DCE
- [ ] Handle function returns
- [ ] Clean up dead functions
- **Test**: `echo 'void unused() { } int main() { return 0; }' | ./compiler -O2 --dump-ir | grep -q "define.*main"`

### [ ] 72. Optimization Pipeline
- [ ] Pass ordering
- [ ] Pass manager
- [ ] Pass statistics
- [ ] -O0, -O1, -O2, -O3 levels
- **Test**: `echo 'int f(int x) { return x+1; }' | ./compiler -O0 --dump-ir | grep -c "add" && echo "int f(int x) { return x+1; }" | ./compiler -O2 --dump-ir | grep -c "add" | [ 1 -eq 1 ]`

---

## Code Generation (codegen.c)

### [ ] 73. Register Set Definition
- [ ] x86-64 register list
- [ ] Register classes
- [ ] Caller/callee saved
- [ ] Special registers (SP, BP, PC)
- **Test**: `grep -c "rax\|rbx\|rcx\|rdx\|rsi\|rdi\|rsp\|rbp" src/backend/regalloc.c | [ $(cat) -ge 8 ]`

### [ ] 74. Calling Convention
- [ ] Integer argument registers (rdi, rsi, rdx, rcx, r8, r9)
- [ ] Float argument registers (xmm0-xmm7)
- [ ] Return value registers
- [ ] Stack alignment
- [ ] Shadow space
- **Test**: `echo 'int f(int a, int b, int c, int d, int e, int f) { return a+b+c+d+e+f; }' | ./compiler --dump-asm | grep "mov"`

### [ ] 75. Register Allocation - Basics
- [ ] Build interference graph
- [ ] Simplify/push/pop stack
- [ ] Assign registers
- [ ] Handle spills
- **Test**: `echo 'int f() { int a,b,c,d,e,f,g,h,i,j,k,l; return a+b+c+d+e+f+g+h+i+j+k+l; }' | ./compiler --dump-asm | verify spills handled`

### [ ] 76. Instruction Selection - Basics
- [ ] Move instructions
- [ ] Arithmetic instructions (add, sub, mul, div)
- [ ] Bitwise instructions (and, or, xor, not, shl, shr)
- **Test**: `echo 'int f(int a, int b) { return a + b - a * b; }' | ./compiler --dump-asm | grep -q "addl"`

### [ ] 77. Instruction Selection - Memory
- [ ] Load instructions
- [ ] Store instructions
- [ ] Addressing modes
- [ ] Stack frame access
- **Test**: `echo 'int f(int* p) { return p[2]; }' | ./compiler --dump-asm | grep -q "movl"`

### [ ] 78. Instruction Selection - Control Flow
- [ ] Conditional jumps (je, jne, jl, jg, etc.)
- [ ] Unconditional jumps (jmp)
- [ ] Labels
- [ ] Function calls/returns
- **Test**: `echo 'int f(int x) { if (x > 0) return 1; return 0; }' | ./compiler --dump-asm | grep -q "jg"`

### [ ] 79. Function Prologue/Epilogue
- [ ] Push callee-saved registers
- [ ] Allocate stack frame
- [ ] Pop callee-saved registers
- [ ] Return instruction
- **Test**: `echo 'int f() { return 0; }' | ./compiler --dump-asm | grep -E "push.*rbp|mov.*rbp.*rsp" | head -2`

### [ ] 80. Global Variable Emission
- [ ] .data section
- [ ] .rodata section
- [ ] .bss section
- [ ] Global variable addresses
- **Test**: `echo 'int x = 42; int f() { return x; }' | ./compiler --dump-asm | grep -E "\\.data|\\.globl"`

### [ ] 81. String Literal Emission
- [ ] String in .rodata
- [ ] Label for string
- [ ] Null termination
- [ ] Escape processing
- **Test**: `echo 'char* s = "hello";' | ./compiler --dump-asm | grep -E "ascii|\\.rodata"`

### [ ] 82. Function Call Emission
- [ ] Prepare arguments
- [ ] Call instruction
- [ ] Clean up stack
- [ ] Handle return value
- **Test**: `echo 'int g(int x) { return x+1; } int f() { return g(5); }' | ./compiler --dump-asm | grep -q "call"`

### [ ] 83. Switch Emission
- [ ] Jump table generation
- [ ] Sparse switch handling
- [ ] Range checking
- **Test**: `echo 'int f(int x) { switch(x) { case 0: return 0; case 1: return 1; case 2: return 2; } return -1; }' | ./compiler --dump-asm | grep ".rodata"`

### [ ] 84. Assembly Formatting
- [ ] GAS syntax
- [ ] Directives
- [ ] Labels
- [ ] Comments (optional)
- **Test**: `echo 'int main() { return 0; }' | ./compiler --dump-asm | grep -E "\\.file|\\.text|\\.globl|\\.type"`

### [ ] 85. DWARF Debug Info
- [ ] .debug_line section
- [ ] .debug_info section
- [ ] .debug_abbrev section
- [ ] Variable locations
- **Test**: `echo 'int main() { return 0; }' | ./compiler -g --dump-asm | grep -E "DWARF|\\.uleb128" | head -5`

---

## Integration & Testing

### [ ] 86. End-to-End Compilation
- [ ] Full pipeline execution
- [ ] Output verification
- [ ] Error handling
- **Test**: `echo '#include <stdio.h>\nint main() { printf("Hello\\n"); return 0; }' > /tmp/hello.c && ./compiler /tmp/hello.c -o /tmp/hello && /tmp/hello | grep -q "Hello"`

### [ ] 87. Standard Library Integration
- [ ] Link with libc
- [ ] Link with libm (math)
- [ ] Standard headers
- **Test**: `echo '#include <math.h>\nint main() { return (int)sin(0.0); }' > /tmp/t.c && ./compiler /tmp/t.c -o /tmp/t && /tmp/t && echo "Math works"`

### [ ] 88. Unit Tests - Lexer
- [ ] Test all token types
- [ ] Test edge cases
- [ ] Test error handling
- **Test**: `./test_lexer | grep -q "All lexer tests passed"`

### [ ] 89. Unit Tests - Parser
- [ ] Test all productions
- [ ] Test error recovery
- [ ] Test AST output
- **Test**: `./test_parser | grep -q "All parser tests passed"`

### [ ] 90. Unit Tests - Type System
- [ ] Test type equality
- [ ] Test type conversions
- [ ] Test type printing
- **Test**: `./test_types | grep -q "Type system tests passed"`

### [ ] 91. Integration Tests
- [ ] Test compilation of sample programs
- [ ] Compare with GCC output
- [ ] Test optimization levels
- **Test**: `make test-integration | grep -q "passed"`

### [ ] 92. Regression Tests
- [ ] Capture and replay bugs
- [ ] Continuous verification
- **Test**: `make test-regression | grep -q "0 failures"`

### [ ] 93. Performance Tests
- [ ] Compilation speed
- [ ] Generated code speed
- [ ] Memory usage
- **Test**: `time ./compiler tests/compiler.c -o /tmp/cc_out 2>&1 | grep -E "real|user|sys" | head -3`

---

## Project Management

### [ ] 94. Code Style
- [ ] Consistent formatting
- [ ] Naming conventions
- [ ] Comment standards
- **Test**: `make style-check | grep -q "Style OK"`

### [ ] 95. Documentation - User
- [ ] Compiler usage (--help)
- [ ] Command line options
- [ ] Installation guide
- **Test**: `./compiler --help | grep -E "Usage|Options" | head -5`

### [ ] 96. Documentation - Developer
- [ ] Architecture document
- [ ] Code documentation
- [ ] Design decisions
- **Test**: `test -f project/docs/architecture.md && test -f project/docs/sprints.md`

### [ ] 97. CI/CD
- [ ] Test automation
- [ ] Build automation
- [ ] Release process
- **Test**: `test -f .github/workflows/test.yml`

### [ ] 98. Versioning
- [ ] Semantic versioning
- [ ] Changelog
- [ ] Release notes
- **Test**: `./compiler --version | grep -q "[0-9]\\.[0-9]\\.[0-9]"`

---

## Polish

### [ ] 99. Error Messages
- [ ] Clear error text
- [ ] Source location
- [ ] Suggestion for fix
- **Test**: `echo 'int x = ;' | ./compiler 2>&1 | grep -E "error|.*:[0-9]+:[0-9]+"`

### [ ] 100. Warnings
- [ ] Unused variables
- [ ] Unused functions
- [ ] Implicit function declarations
- [ ] Missing return
- **Test**: `echo 'int f() { int x; return 0; }' | ./compiler -Wall 2>&1 | grep -q "warning"`

### [ ] 101. Performance Optimization
- [ ] Profile compiler
- [ ] Optimize hot paths
- [ ] Reduce allocations
- **Test**: `time ./compiler tests/large.c -o /tmp/out 2>&1 | grep "real" | awk '{if ($2 < 10) print "PASS"}'`

### [ ] 102. Self-Hosting Preparation
- [ ] Compiler compiles itself
- [ ] Bootstrap binary
- **Test**: `./compiler src/*.c -o compiler_new 2>&1 | grep -v "warning" | head -5`

---

## Optional Enhancements (Future)

### [ ] 103. RISC-V Backend
- [ ] RISC-V instruction set
- [ ] RISC-V calling convention
- [ ] RISC-V assembly output
- **Test**: `echo 'int main() { return 0; }' | ./compiler --target=riscv64 --dump-asm | grep -q "ret"`

### [ ] 104. Link-Time Optimization
- [ ] IR object format
- [ ] Whole program analysis
- [ ] Cross-module inlining
- **Test**: `./compiler -flto test1.c test2.c -o test`

### [ ] 105. WebAssembly Target
- [ ] WASM instruction set
- [ ] WASM ABI
- [ ] Binary output
- **Test**: `echo 'int main() { return 0; }' | ./compiler --target=wasm --dump-asm | grep -q "module"`

### [ ] 106. Profile-Guided Optimization
- [ ] Instrumentation
- [ ] Profile collection
- [ ] Profile-guided compilation
- **Test**: `./compiler -fprofile-generate test.c -o test && ./test && ./compiler -fprofile-use test.c -o test_opt`

### [ ] 107. Sanitizers
- [ ] AddressSanitizer support
- [ ] UndefinedBehaviorSanitizer support
- **Test**: `echo 'int main() { int* p = 0; return *p; }' > /tmp/t.c && ./compiler -fsanitize=address /tmp/t.c -o /tmp/t && /tmp/t 2>&1 | grep -q "AddressSanitizer"`
