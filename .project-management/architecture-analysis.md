# C Compiler Architecture Analysis

## Overview

This is a C compiler written from scratch that currently targets **ARM64 (Apple Silicon)**. The compiler follows a traditional compilation pipeline.

## Compilation Pipeline

1. **Preprocessor** (`src/lexer/preproc.c`) - Handles `#include`, `#define`, etc.
2. **Lexer** (`src/lexer/lexer.c`) - Tokenizes source code into tokens
3. **Parser** (`src/parser/parser.c`) - Builds AST from tokens using recursive descent
4. **Semantic Analyzer** (`src/sema/analyzer.c`) - Type checking, scope analysis with symbol tables
5. **IR Generator** (`src/ir/lowerer.c`) - Lowers AST to intermediate representation
6. **Optimizer** (`src/optimize/`) - Optimization passes (constfold, DCE)
7. **Code Generator** (`src/backend/codegen.c`) - Generates ARM64 assembly

## IR Format (IR format)

The intermediate representation is defined in `src/ir/ir.h`:

### IR Structure
- **IRModule**: Top-level container with functions and string literals
- **IRFunction**: Contains function name, parameters, and basic blocks
- **IRBasicBlock**: Named block with a list of instructions
- **IRInstruction**: Single operation with opcode, result, and up to 4 arguments

### IR Value Types (`IRValueKind`)
- `IR_VALUE_INT` - Integer values
- `IR_VALUE_FLOAT` - Floating point values  
- `IR_VALUE_PTR` - Pointer values
- `IR_VALUE_STRING` - String literal references

### IR Instructions (IROpcode)
Key instruction categories:
- **Control Flow**: `IR_LABEL`, `IR_JMP`, `IR_JMP_IF`, `IR_RET`, `IR_RET_VOID`
- **Function Calls**: `IR_CALL`, `IR_CALL_INDIRECT`
- **Memory**: `IR_LOAD`, `IR_STORE`, `IR_ALLOCA`, `IR_LOAD_STACK`, `IR_STORE_STACK`
- **Arithmetic**: `IR_ADD`, `IR_SUB`, `IR_MUL`, `IR_DIV`, `IR_MOD`, `IR_NEG`
- **Bitwise**: `IR_AND`, `IR_OR`, `IR_XOR`, `IR_SHL`, `IR_SHR`, `IR_NOT`
- **Comparisons**: `IR_CMP_LT`, `IR_CMP_GT`, `IR_CMP_LE`, `IR_CMP_GE`, `IR_CMP_EQ`, `IR_CMP_NE`
- **Logical**: `IR_BOOL_AND`, `IR_BOOL_OR` (with short-circuit support)
- **Type Conversion**: `IR_SEXT`, `IR_ZEXT`, `IR_TRUNC`, `IR_SITOFP`, `IR_FPTOSI`
- **Constants**: `IR_CONST_INT`, `IR_CONST_FLOAT`, `IR_CONST_STRING`
- **ARM64-specific**: `IR_SAVE_X8`, `IR_RESTORE_X8_RESULT`, `IR_LEA`, etc.

### IR Value Metadata
- `is_constant` - Compile-time constant
- `is_temp` - Result of previous instruction (in x8 register)
- `is_address` - Value is an address (from LEA)
- `is_pointer` - Value is a pointer (needs 8-byte load/store)
- `param_reg` - Parameter register (x0-x3) or -2 for local variable
- `offset` - Stack offset for local variables

## Current Backend

### Target Architecture: ARM64 (Apple Silicon/macOS)

The code generator (`src/backend/codegen.c`) produces ARM64 assembly with macOS conventions:
- Uses underscore prefix for symbols (`_main`, `_printf`)
- `.cstring` section for string literals
- Standard ARM64 calling convention (x0-x3 for arguments, x0 for return)
- Callee-saved registers: x19-x28, x29 (frame pointer), x30 (link register)

### Register Allocation Strategy
- **x0-x3**: Function arguments and return value
- **x8**: Primary temp register for intermediate results
- **x9**: Secondary temp register
- **x10**: Saves previous x8 value for binary operations
- **x19, x20, x21, x22, x23**: Callee-saved registers for preserving values across calls
- **x29**: Frame pointer
- **x30**: Link register

### Key Code Generation Patterns
- Prologue/Epilogue with 16-byte stack alignment
- Local variables accessed via `[sp, #offset]`
- Large immediates handled with `movz`/`movk` sequences
- Conditional operations use `cmp` + `cset` pattern

## Existing Backends

**Current**: Single backend targeting ARM64 assembly

**No existing backends for**:
- x86-64
- WebAssembly (WASM)
- LLVM IR
- C (transpilation)

## Driver Interface

The driver (`src/driver.c`) orchestrates the pipeline:
```
CompileOptions {
    dump_tokens, dump_ast, dump_ir, dump_asm,
    syntax_only, preprocess_only, generate_debug,
    optimization_level, output_file, input_file
}
```

Command line interface supports:
- `-o <file>` - Output file
- `-S` - Produce assembly
- `-c` - Compile only
- `-E` - Preprocess only
- `-O0` to `-O3` - Optimization levels
- `--dump-tokens`, `--dump-ast`, `--dump-ir`, `--dump-asm` - Debug dumps

## Test Infrastructure

- **Unit tests**: Lexer, parser, types, symbol table
- **Integration tests**: Code generation tests
- **Regression tests**: Full compilation tests
- **Puzzle tests**: Algorithm and stress tests
- **Self-hosting**: Bootstrap compiler with itself

## Key Files for Backend Implementation

| File | Purpose |
|------|---------|
| `src/ir/ir.h` | IR definition |
| `src/ir/ir.c` | IR creation/destruction |
| `src/ir/lowerer.c` | AST to IR lowering |
| `src/backend/codegen.c` | ARM64 code generation |
| `src/backend/codegen.h` | Codegen interface |
| `src/backend/asm.c` | Assembly utilities |
| `src/backend/regalloc.c` | Register allocation |
| `src/backend/dwarf.c` | Debug info generation |
| `src/driver.c` | Compilation pipeline |

## WASM Integration Points

To add WASM support, the following components need to be created/modified:

1. **New Backend Module** (`src/backend/wasm_codegen.c/h`)
   - Generate WASM binary or WAT (WebAssembly Text format)
   - Map IR instructions to WASM opcodes
   - Handle WASM stack-based execution model

2. **Driver Extension**
   - Add `--target=wasm` or `-t wasm` flag
   - Add `.wasm` or `.wat` output file support

3. **IR Considerations**
   - Current IR is register-based; WASM is stack-based
   - May need IR extensions for WASM-specific features
   - Memory model differences (linear memory vs stack)

4. **Runtime/Library Support**
   - WASM imports for I/O (printf, etc.)
   - WASM module structure and exports
   - JavaScript glue code for browser execution

## Architecture Strengths for Multi-Backend

- Clean separation between IR and codegen
- Well-defined IR with explicit instruction set
- Driver already supports multiple output modes
- Modular backend structure (`src/backend/`)

## Architecture Challenges for WASM

- IR assumes register machine; WASM is stack-based
- Current backend tightly coupled to ARM64 conventions
- No abstraction layer for target-specific details
- Memory model assumptions (stack frames, local variables)
