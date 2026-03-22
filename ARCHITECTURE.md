# C Compiler Architecture

## Overview

A self-hosted, ISO C11-compliant C compiler targeting x86-64 Linux with support for RISC-V. Built entirely from scratch in C.

## Project Goals

- **ISO C11 Compliance** (full language support)
- **Multiple Targets**: x86-64 Linux (primary), RISC-V64 Linux (secondary)
- **Self-hostable**: Compiler can compile itself
- **Performance**: Competitive with GCC -O2 for typical workloads
- **Debug Info**: DWARF debugging format support

---

## Compiler Pipeline

```
Source Code (.c)
      │
      ▼
┌─────────────────┐
│     LEXER       │  Tokenize source into tokens
│  (scanner.c)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     PARSER      │  Build AST from tokens
│   (parser.c)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   SEMANTIC      │  Type checking, scope resolution
│   ANALYZER      │
│ (analyzer.c)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│    AST IR       │  High-level IR (tree-based)
│    (ast.c)      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ LOWERING PASS   │  Lower AST to linear IR
│  (lowerer.c)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   MIDDLE END    │
│  ┌───────────┐  │
│  │ CFG Build │  │  Build control flow graph
│  └───────────┘  │
│  ┌───────────┐  │
│  │ SSA Form  │  │  Convert to Static Single Assignment
│  └───────────┘  │
│  ┌───────────┐  │
│  │ Optimizer │  │  Optimization passes
│  └───────────┘  │
│  (optimizer.c)  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  LINEAR IR      │  Low-level linear IR (machine-independent)
│   (lir.c)       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   BACKEND       │
│  ┌───────────┐  │
│  │Reg Alloc  │  │  Graph coloring register allocation
│  └───────────┘  │
│  ┌───────────┐  │
│  │Code Emit  │  │  Generate assembly
│  └───────────┘  │
│  (backend.c)    │
└────────┬────────┘
         │
         ▼
Assembly (.s) ──► Assembler ──► Object (.o) ──► Linker ──► Executable
```

---

## Component Specifications

### 1. Lexer (scanner.c)

**Purpose**: Convert source text into sequence of tokens

**Tokens**:
```
KEYWORD:    auto, break, case, char, const, continue, default, do, double,
            else, enum, extern, float, for, goto, if, inline, int, long,
            register, restrict, return, short, signed, sizeof, static,
            struct, switch, typedef, union, unsigned, void, volatile, while
OPERATORS:  +, -, *, /, %, =, ==, !=, <, >, <=, >=, &&, ||, !, &, |, ^,
            <<, >>, ++, --, ->, ., sizeof, alignof, ?:, ...
PUNCTUATION: (, ), {, }, [, ], ;, ,, :, ...
LITERALS:   Integer (decimal, hex, octal, binary), Float, Character, String
IDENTIFIERS: Letters, digits, underscores (cannot start with digit)
```

**Features**:
- Unicode support (UTF-8 source files)
- Line/column tracking for error reporting
- Preprocessor directive recognition
- Comment stripping (/* */ and //)
- String literal concatenation

**Output**: Token stream with source location info

---

### 2. Parser (parser.c)

**Purpose**: Build Abstract Syntax Tree from token stream

**Grammar (simplified C11 subset)**:

```bnf
translation_unit    := (external_declaration)*
external_declaration:= function_definition | declaration
function_definition := declaration_specifiers declarator declaration* compound_statement
declarator          := pointer? direct_declarator
direct_declarator   := IDENTIFIER | '(' declarator ')' | direct_declarator '[' constant_expression? ']' | direct_declarator '(' parameter_list ')'
compound_statement  := '{' block_item* '}'
block_item          := statement | declaration
statement           := labeled_statement | compound_statement | expression_statement |
                       selection_statement | iteration_statement | jump_statement
expression_statement:= expression? ';'
selection_statement := IF '(' expression ')' statement ('ELSE' statement)?
iteration_statement := WHILE '(' expression ')' statement | DO statement WHILE '(' expression ')' ';' |
                       FOR '(' expression? ';' expression? ';' expression? ')' statement
jump_statement      := RETURN expression? ';'
declaration          := declaration_specifiers init_declarator_list? ';'
init_declarator_list:= init_declarator (',' init_declarator)*
init_declarator     := declarator ('=' initializer)?
declaration_specifiers := (storage_class_specifier | type_specifier | type_qualifier | function_specifier)*
type_specifier      := VOID | CHAR | SHORT | INT | LONG | FLOAT | DOUBLE |
                       SIGNED | UNSIGNED | struct_or_union_specifier | enum_specifier | typedef_name
```

**Parser Algorithm**: Recursive descent with operator precedence parsing for expressions

**AST Node Types**:
```c
enum ASTNodeType {
    // Declarations
    AST_FUNCTION_DECL,
    AST_VARIABLE_DECL,
    AST_PARAMETER_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF,
    
    // Types
    AST_TYPE_SPECIFIER,
    AST_POINTER_TYPE,
    AST_ARRAY_TYPE,
    AST_FUNCTION_TYPE,
    AST_STRUCT_TYPE,
    AST_UNION_TYPE,
    
    // Statements
    AST_COMPOUND_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_LABEL_STMT,
    
    // Expressions
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CAST_EXPR,
    AST_CONDITIONAL_EXPR,
    AST_CALL_EXPR,
    AST_ARRAY_SUBSCRIPT,
    AST_MEMBER_ACCESS,
    AST_POINTER_MEMBER_ACCESS,
    AST_IDENTIFIER,
    AST_CONSTANT,
    AST_STRING_LITERAL,
    ASTInitializer,
    
    // Literals
    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL
};
```

---

### 3. Semantic Analyzer (analyzer.c)

**Purpose**: Perform type checking, scope analysis, and semantic validation

**Responsibilities**:

1. **Symbol Table Management**
   - Global scope for functions and global variables
   - Block scopes for local variables
   - Struct/union scopes for member access
   - Function prototype scope for parameter matching

2. **Type System**
   - Primitive types: void, char, short, int, long, float, double
   - Qualifiers: const, volatile, restrict
   - Derived types: pointer, array, function, struct, union
   - Integer promotions and usual arithmetic conversions

3. **Type Checking Rules**
   - Operands of binary operators must have compatible types
   - Assignment requires compatible types
   - Function calls must match parameter types
   - Array subscript requires integer index
   - Address-of operator applied to lvalues only

4. **Semantic Checks**
   - All identifiers declared before use
   - No redefinition in same scope
   - break/continue only in loops/switches
   - return type matches function signature
   - sizeof cannot be applied to incomplete types
   - Case labels only in switch
   - No void variables

**Output**: Annotated AST with resolved types and symbol references

---

### 4. IR Generation (lowerer.c)

**Purpose**: Lower AST to machine-independent linear IR

**IR Design (3-address code style)**:

```c
// Instructions
enum IROpcode {
    // Control flow
    IR_LABEL,
    IR_JMP,
    IR_JMP_IF,
    IR_RET,
    IR_RET_VOID,
    
    // Memory
    IR_LOAD,
    IR_STORE,
    IR_ALLOCA,
    IR_GEP,           // Get element pointer
    
    // Arithmetic
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD,
    IR_NEG,
    
    // Bitwise
    IR_AND,
    IR_OR,
    IR_XOR,
    IR_SHL,
    IR_SHR,
    IR_NOT,
    
    // Comparison
    IR_CMP,
    IR_CMP_F,
    
    // Function calls
    IR_CALL,
    IR_CALL_INDIRECT,
    
    // Casts
    IR_SEXT,
    IR_ZEXT,
    IR_TRUNC,
    IR_FPEXT,
    IR_FPTRUNC,
    IR_SITOFP,
    IR_FPTOSI,
    IR_INTTOPTR,
    IR_PTRTOINT,
    
    // Phi nodes (SSA)
    IR_PHI
};
```

**Function Structure**:
```c
struct IRFunction {
    char *name;
    Type *return_type;
    List<IRParameter*> params;
    List<IRBasicBlock*> blocks;
    Map<String, IRBasicBlock*> block_map;
};
```

---

### 5. Optimizer (optimizer.c)

**Purpose**: Apply optimization passes to improve code quality

**Optimization Passes** (in order):

1. **Constant Folding**
   - Evaluate constant expressions at compile time
   - Example: `5 + 3` → `8`

2. **Copy Propagation**
   - Replace uses of a variable with the value it's assigned to
   - Example: `a = b; c = a + 1;` → `c = b + 1;`

3. **Dead Code Elimination**
   - Remove unreachable code and unused computations
   - Remove dead stores, unused parameters

4. **Common Subexpression Elimination (CSE)**
   - Eliminate redundant computations
   - Example: `a = x + y; b = x + y;` → `a = x + y; b = a;`

5. **Loop Invariant Code Motion (LICM)**
   - Move loop-invariant code outside loops

6. **Strength Reduction**
   - Replace expensive operations with cheaper ones
   - Example: `a * 2` → `a << 1`

7. **Loop Unrolling** (optional)
   - Unroll small loops for better performance

8. **Inlining**
   - Inline small functions
   - Reduces function call overhead

9. **Dead Store Elimination**
   - Remove stores to variables that are never used

10. **Control Flow Simplification**
    - Remove unreachable blocks
    - Simplify branches

---

### 6. Register Allocator (regalloc.c)

**Purpose**: Map infinite virtual registers to finite physical registers

**Algorithm**: Graph Coloring Register Allocation (Chaitin-Briggs)

**Steps**:
1. Build interference graph
2. Simplify stack
3. Coalesce moves
4. Select registers
5. Spill if necessary

**Registers for x86-64**:
```
Caller-saved: rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11
Callee-saved: rbx, rbp, r12, r13, r14, r15
Special: rsp (stack pointer), rbp (frame pointer)
```

---

### 7. Code Generator (codegen.c)

**Purpose**: Generate x86-64 assembly from lowered IR

**x86-64 Calling Convention (System V ABI)**:
```
Integer arguments: rdi, rsi, rdx, rcx, r8, r9
Float arguments:   xmm0-xmm7
Return value:      rax (integer), xmm0 (float)
Stack alignment:   16 bytes
```

**Instruction Selection**: Pattern matching on IR instructions

**Key Features**:
- Dwarf debug information generation
- Function prologue/epilogue generation
- Stack frame layout
- Prologue:
  ```
  push rbp
  mov rbp, rsp
  sub rsp, <frame_size>
  ```
- Epilogue:
  ```
  mov rsp, rbp
  pop rbp
  ret
  ```

---

## Data Structures

### Symbol Table Entry
```c
struct Symbol {
    char *name;
    Type *type;
    SymbolKind kind;  // FUNCTION, VARIABLE, PARAMETER, TYPE, LABEL
    Scope *scope;
    bool is_static;
    bool is_extern;
    Value *value;     // For constants
    int stack_offset; // For locals
};
```

### Type System
```c
struct Type {
    TypeKind kind;
    int size;         // In bytes
    int alignment;
    // Kind-specific fields:
    Type *base_type;  // For pointer, array
    int array_size;   // For array
    List<Field*> *members; // For struct/union
    Type *return_type;    // For function
    List<Type*> *param_types; // For function
};
```

---

## Error Handling

**Error Types**:
1. Lexical errors (invalid characters)
2. Syntax errors (parse failures)
3. Semantic errors (type mismatches, undefined symbols)
4. Codegen errors (internal compiler errors)

**Error Reporting**:
```c
void error(SourceLocation loc, const char *fmt, ...);
void warning(SourceLocation loc, const char *fmt, ...);
void note(SourceLocation loc, const char *fmt, ...);
```

**Error Recovery**: Panic mode with synchronization

---

## File Organization

```
src/
├── main.c           # Entry point, driver
├── common/
│   ├── util.c       # Utilities, memory management
│   ├── error.c      # Error reporting
│   └── list.c       # Dynamic array implementation
├── lexer/
│   ├── lexer.h      # Lexer interface
│   ├── lexer.c      # Lexer implementation
│   └── tokens.h     # Token definitions
├── parser/
│   ├── parser.h     # Parser interface
│   ├── parser.c     # Recursive descent parser
│   └── ast.h        # AST node definitions
├── sema/
│   ├── analyzer.h   # Analyzer interface
│   ├── analyzer.c   # Semantic analysis
│   ├── symtab.c     # Symbol table implementation
│   └── type.c       # Type system
├── ir/
│   ├── ir.h         # IR definitions
│   ├── ir.c         # IR builder
│   └── lowerer.c    # AST to IR lowering
├── optimize/
│   ├── optimize.h   # Optimizer interface
│   ├── passes.c     # Optimization passes
│   ├── constfold.c  # Constant folding
│   ├── cprop.c      # Copy propagation
│   └── dce.c        # Dead code elimination
├── backend/
│   ├── backend.h    # Backend interface
│   ├── regalloc.c   # Register allocation
│   ├── codegen.c    # x86-64 code generation
│   ├── asm.c        # Assembly output
│   └── dwarf.c      # DWARF debug info
├── target/
│   ├── target.h     # Target architecture interface
│   ├── x86_64.c     # x86-64 specific code
│   └── riscv64.c    # RISC-V specific code
└── driver.c         # Compiler driver, passes pipeline
```

---

## Testing Strategy

1. **Unit Tests**: Test each component in isolation
2. **Integration Tests**: Test full compilation pipeline
3. **Reference Tests**: Compare output with GCC/Clang
4. **Conformance Tests**: C11 standard compliance tests
5. **Stress Tests**: Large real-world programs

---

## Performance Targets

- Compile 10,000 lines/second (single thread)
- Generated code within 20% of GCC -O2
- Memory usage under 200MB for typical compilation

---

## Future Enhancements

- Self-hosting (compiler compiles itself)
- Additional targets (ARM64, WebAssembly)
- Link-time optimization (LTO)
- Profile-guided optimization (PGO)
- Better error messages
- Language server protocol (LSP) support
