# WASM Backend Design Document

## Overview

This document describes the design for adding WebAssembly (WASM) target support to the C compiler. The WASM backend will generate WebAssembly Text format (WAT) from the compiler's intermediate representation (IR).

## WASM Module Structure

### Module Components

A WASM module consists of:

1. **Types Section** - Function signatures (parameter and result types)
2. **Imports Section** - External functions (e.g., printf from host)
3. **Functions Section** - Function type indices
4. **Tables Section** - Function tables (for indirect calls)
5. **Memories Section** - Linear memory declarations
6. **Globals Section** - Module-level variables
7. **Exports Section** - Exported functions/globals
8. **Start Section** - Optional start function
9. **Code Section** - Function bodies (for binary format)
10. **Data Section** - Static data initialization

### WAT Format Example

```wat
(module
  ;; Import host functions
  (import "env" "printf" (func $printf (param i32 i32) (result i32)))
  
  ;; Memory declaration (1 page = 64KB)
  (memory (export "memory") 1)
  
  ;; Static data
  (data (i32.const 0) "Hello, World!\00")
  
  ;; Function type
  (type $main_type (func (result i32)))
  
  ;; Function declaration
  (func $main (type $main_type) (result i32)
    ;; Function body
    i32.const 0        ;; Load format string address
    i32.const 0        ;; Load argument
    call $printf       ;; Call printf
    i32.const 0        ;; Return 0
  )
  
  ;; Export main function
  (export "main" (func $main))
  (export "_main" (func $main))  ;; macOS convention
)
```

## Instruction Mapping (instruction mapping)

### IR to WASM Instruction Mapping

| IR Opcode | WASM Instructions | Notes |
|-----------|-------------------|-------|
| IR_NOP | - | No-op, omitted |
| IR_LABEL | - | Labels become branch targets |
| IR_JMP | `br` | Unconditional branch |
| IR_JMP_IF | `if`/`else`/`end` or `br_if` | Conditional branch |
| IR_RET | `return` | Return from function |
| IR_RET_VOID | `return` | Void return |
| IR_CALL | `call` | Direct function call |
| IR_CALL_INDIRECT | `call_indirect` | Indirect call through table |
| IR_LOAD | `i32.load` / `i64.load` | Memory load |
| IR_STORE | `i32.store` / `i64.store` | Memory store |
| IR_ALLOCA | `local.set` | Allocate in locals |
| IR_ADD | `i32.add` / `i64.add` | Integer addition |
| IR_ADD_F | `f32.add` / `f64.add` | Float addition |
| IR_SUB | `i32.sub` / `i64.sub` | Integer subtraction |
| IR_SUB_F | `f32.sub` / `f64.sub` | Float subtraction |
| IR_MUL | `i32.mul` / `i64.mul` | Integer multiplication |
| IR_MUL_F | `f32.mul` / `f64.mul` | Float multiplication |
| IR_DIV | `i32.div_s` / `i64.div_s` | Signed division |
| IR_DIV_F | `f32.div` / `f64.div` | Float division |
| IR_MOD | `i32.rem_s` / `i64.rem_s` | Signed remainder |
| IR_NEG | `i32.neg` / `i64.neg` | Negation |
| IR_NEG_F | `f32.neg` / `f64.neg` | Float negation |
| IR_AND | `i32.and` / `i64.and` | Bitwise AND |
| IR_OR | `i32.or` / `i64.or` | Bitwise OR |
| IR_XOR | `i32.xor` / `i64.xor` | Bitwise XOR |
| IR_SHL | `i32.shl` / `i64.shl` | Shift left |
| IR_SHR | `i32.shr_s` / `i64.shr_s` | Arithmetic shift right |
| IR_NOT | `i32.eqz` | Logical NOT (compare to zero) |
| IR_CMP_LT | `i32.lt_s` | Signed less than |
| IR_CMP_GT | `i32.gt_s` | Signed greater than |
| IR_CMP_LE | `i32.le_s` | Signed less or equal |
| IR_CMP_GE | `i32.ge_s` | Signed greater or equal |
| IR_CMP_EQ | `i32.eq` | Equal |
| IR_CMP_NE | `i32.ne` | Not equal |
| IR_BOOL_AND | `i32.and` | Both operands normalized to 0/1 |
| IR_BOOL_OR | `i32.or` | Both operands normalized to 0/1 |
| IR_SEXT | `i64.extend_i32_s` | Sign extend |
| IR_ZEXT | `i64.extend_i32_u` | Zero extend |
| IR_TRUNC | `i32.wrap_i64` | Truncate to 32-bit |
| IR_SITOFP | `f64.convert_i32_s` | Signed int to float |
| IR_FPTOSI | `i32.trunc_f64_s` | Float to signed int |
| IR_CONST_INT | `i32.const` / `i64.const` | Integer constant |
| IR_CONST_FLOAT | `f32.const` / `f64.const` | Float constant |
| IR_CONST_STRING | `i32.const` | String address |

### Control Flow Translation

WASM uses structured control flow. The IR's label/jump model must be converted:

```
IR Pattern:
  LABEL start
  JMP_IF cond, end
  ... body ...
  JMP end
  LABEL end

WASM Pattern:
  block $end
    local.get $cond
    br_if $end
    ... body ...
  end
```

## Memory Model (memory model)

### Linear Memory Layout

WASM uses a single linear memory. Our layout:

```
Address 0:     Static data (strings, globals)
Address N:     Stack (grows upward)
               ^
               |
               SP (stack pointer, in $sp local)
```

### Stack Frame Layout

```
High addresses
+------------------+
| Local variables  |  <- accessed via $fp + offset
+------------------+
| Saved frame ptr  |  <- $fp points here
+------------------+
| Return address   |  (implicit in WASM)
+------------------+
| Function args    |  (in locals for WASM)
+------------------+  <- $sp points here
Low addresses
```

### Memory Operations

- **Stack allocation**: Use WASM `local` variables where possible
- **Heap allocation**: Not supported initially (requires malloc implementation)
- **Global variables**: Use WASM `global` or fixed memory addresses

### Stack Pointer Management

```wat
;; Prologue
(func $example
  (local $sp i32)    ;; Stack pointer
  (local $fp i32)    ;; Frame pointer
  (local $locals i32) ;; Space for locals
  
  ;; Save frame pointer
  local.get $sp
  local.set $fp
  
  ;; Allocate locals
  local.get $sp
  i32.const 32       ;; 32 bytes for locals
  i32.add
  local.set $sp
  
  ;; ... function body ...
  
  ;; Epilogue
  local.get $fp
  local.set $sp
  return
)
```

## I/O Strategy

### Import Model

WASM modules import host functions for I/O:

```wat
(import "env" "printf" (func $printf (param i32 i32 i32 i32 i32) (result i32)))
(import "env" "putchar" (func $putchar (param i32) (result i32)))
(import "env" "malloc" (func $malloc (param i32) (result i32)))
(import "env" "free" (func $free (param i32)))
```

### JavaScript Host Integration

For browser execution, provide JavaScript glue:

```javascript
const imports = {
  env: {
    printf: (fmt, ...args) => {
      // Format and print
    },
    putchar: (c) => {
      process.stdout.write(String.fromCharCode(c));
    },
    memory: new WebAssembly.Memory({ initial: 1 })
  }
};
```

### Runtime Library

Provide a minimal C runtime for WASM:

```c
// wasm_runtime.c
int putchar(int c);
int printf(const char *fmt, ...);
void *malloc(unsigned size);
void free(void *ptr);
```

## Type System Mapping

### C to WASM Types

| C Type | WASM Type | Notes |
|--------|-----------|-------|
| char | i32 | Sign-extended |
| short | i32 | Sign-extended |
| int | i32 | Direct mapping |
| long | i64 | On 64-bit systems |
| long long | i64 | Direct mapping |
| float | f32 | Direct mapping |
| double | f64 | Direct mapping |
| pointer | i32 | Memory address |
| struct | i32 | Pointer to memory |

### Function Signatures

```wat
;; C: int add(int a, int b)
(type $add (func (param i32 i32) (result i32)))

;; C: void foo(void)
(type $foo (func))

;; C: double sqrt(double x)
(type $sqrt (func (param f64) (result f64)))
```

## Implementation Considerations

### Register vs Stack Machine

The current IR assumes a register machine (ARM64). WASM is stack-based:

**Register IR:**
```
ADD x0, x1, x2  ;; x0 = x1 + x2
```

**Stack WASM:**
```
local.get $x1
local.get $x2
i32.add
local.set $x0
```

### Optimization Opportunities

1. **Local variable allocation**: Keep frequently used values in locals
2. **Stack depth reduction**: Reorder operations to minimize stack usage
3. **Dead code elimination**: Remove unused computations before codegen

### Challenges

1. **Structured control flow**: IR uses goto-style jumps; WASM requires structured blocks
2. **Multiple return values**: WASM supports multi-value returns (recent addition)
3. **Tail calls**: Not yet standardized in WASM
4. **Exception handling**: Not in base WASM spec

## File Structure

```
src/backend/
  wasm_codegen.c    # Main WASM code generator
  wasm_codegen.h    # Public interface
  wasm_emit.c       # WAT text emission
  wasm_emit.h       # Emission helpers
  wasm_runtime.c    # WASM runtime library (C source)
  
.project-management/
  wasm-design.md         # This document
  wasm-codegen-spec.md   # Detailed codegen specification
  wasm-module-design.md  # Module structure design
```

## Output Format

### WAT (Text Format)

Primary output for debugging and validation:
```bash
./compiler --target=wasm input.c -o output.wat
```

### WASM (Binary Format)

Future: Direct binary output or pipe through wat2wasm:
```bash
./compiler --target=wasm --binary input.c -o output.wasm
# Or
wat2wasm output.wat -o output.wasm
```

## Validation

### Tools

- **wat2wasm**: Validate WAT syntax, produce binary
- **wasm2wat**: Decompile binary, verify round-trip
- **wasmtime**: Runtime execution and testing
- **wasm-validate**: Binary validation

### Test Commands

```bash
# Validate syntax
wat2wasm output.wat -o /dev/null

# Run with wasmtime
wasmtime output.wasm --invoke main

# Validate binary
wasm-validate output.wasm
```

## References

- [WebAssembly Specification](https://webassembly.github.io/spec/core/)
- [WAT Format](https://webassembly.github.io/wabt/doc/wat2wasm.html)
- [WASM by Example](https://webassemblybyexample.com/)
