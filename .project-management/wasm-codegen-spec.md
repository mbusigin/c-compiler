# WASM Code Generator Specification

## Overview

This document provides the detailed technical specification for the WASM code generator, including the complete IR-to-WASM instruction mapping and implementation guidelines.

## Module Structure

### WAT Module Template

```wat
(module
  ;; 1. Types Section - Function signatures
  (type $t0 (func (param i32 i32) (result i32)))
  (type $t1 (func (result i32)))
  
  ;; 2. Imports Section - Host functions
  (import "env" "printf" (func $printf (param i32 i32 i32 i32 i32) (result i32)))
  (import "env" "putchar" (func $putchar (param i32) (result i32)))
  
  ;; 3. Functions Section - Type indices
  (func $add (type $t0) (param $a i32) (param $b i32) (result i32)
    ;; 4. Locals Section - Local variables
    (local $result i32)
    
    ;; 5. Code Section - Function body
    local.get $a
    local.get $b
    i32.add
    local.set $result
    local.get $result
  )
  
  ;; 6. Exports Section
  (export "add" (func $add))
  (export "_add" (func $add))
  
  ;; 7. Data Section - Static data
  (data (i32.const 1024) "Hello\00")
)
```

## IR to WASM Instruction Mapping

### Control Flow Instructions

#### IR_NOP
```c
// IR: NOP
// WASM: (omitted)
```

#### IR_LABEL
```c
// IR: LABEL loop_start
// WASM: Named block target
block $loop_start
  ...
end
```

#### IR_JMP
```c
// IR: JMP end_label
// WASM: br $end_label
br $end_label
```

#### IR_JMP_IF
```c
// IR: JMP_IF cond, false_label
// WASM: 
local.get $cond
br_if $false_label

// Alternative with if-block:
local.get $cond
if
  ;; true branch
else
  ;; false branch  
end
```

#### IR_RET
```c
// IR: RET value
// WASM:
local.get $value
return
```

#### IR_RET_VOID
```c
// IR: RET_VOID
// WASM:
return
```

### Function Call Instructions

#### IR_CALL
```c
// IR: CALL printf, [fmt, arg1, arg2]
// WASM:
local.get $fmt
local.get $arg1
local.get $arg2
call $printf
```

#### IR_CALL_INDIRECT
```c
// IR: CALL_INDIRECT fn_ptr, [args...]
// WASM:
;; Push arguments
local.get $arg1
;; Push function index
local.get $fn_ptr
;; Indirect call with type
call_indirect (type $t0)
```

### Memory Instructions

#### IR_LOAD
```c
// IR: LOAD [addr] -> result
// WASM for i32:
local.get $addr
i32.load

// WASM for i64:
local.get $addr
i64.load
```

#### IR_STORE
```c
// IR: STORE [addr], value
// WASM for i32:
local.get $addr
local.get $value
i32.store

// WASM for i64:
local.get $addr
local.get $value
i64.store
```

#### IR_ALLOCA
```c
// IR: ALLOCA size -> local
// WASM: Allocate as local variable
(local $var i32)
```

### Arithmetic Instructions

#### IR_ADD
```c
// IR: ADD left, right -> result
// WASM i32:
local.get $left
local.get $right
i32.add

// WASM i64:
local.get $left
local.get $right
i64.add

// WASM f32:
local.get $left
local.get $right
f32.add

// WASM f64:
local.get $left
local.get $right
f64.add
```

#### IR_SUB
```c
// IR: SUB left, right -> result
// WASM:
local.get $left
local.get $right
i32.sub
```

#### IR_MUL
```c
// IR: MUL left, right -> result
// WASM:
local.get $left
local.get $right
i32.mul
```

#### IR_DIV
```c
// IR: DIV left, right -> result
// WASM signed:
local.get $left
local.get $right
i32.div_s

// WASM unsigned:
local.get $left
local.get $right
i32.div_u
```

#### IR_MOD
```c
// IR: MOD left, right -> result
// WASM signed:
local.get $left
local.get $right
i32.rem_s

// WASM unsigned:
local.get $left
local.get $right
i32.rem_u
```

#### IR_NEG
```c
// IR: NEG value -> result
// WASM:
local.get $value
i32.neg
```

### Bitwise Instructions

#### IR_AND
```c
// IR: AND left, right -> result
// WASM:
local.get $left
local.get $right
i32.and
```

#### IR_OR
```c
// IR: OR left, right -> result
// WASM:
local.get $left
local.get $right
i32.or
```

#### IR_XOR
```c
// IR: XOR left, right -> result
// WASM:
local.get $left
local.get $right
i32.xor
```

#### IR_SHL
```c
// IR: SHL value, shift -> result
// WASM:
local.get $value
local.get $shift
i32.shl
```

#### IR_SHR
```c
// IR: SHR value, shift -> result
// WASM arithmetic:
local.get $value
local.get $shift
i32.shr_s

// WASM logical:
local.get $value
local.get $shift
i32.shr_u
```

#### IR_NOT
```c
// IR: NOT value -> result
// WASM:
local.get $value
i32.eqz
```

### Comparison Instructions

#### IR_CMP_LT
```c
// IR: CMP_LT left, right -> result
// WASM signed:
local.get $left
local.get $right
i32.lt_s

// WASM unsigned:
local.get $left
local.get $right
i32.lt_u

// WASM float:
local.get $left
local.get $right
f32.lt
```

#### IR_CMP_GT
```c
// IR: CMP_GT left, right -> result
// WASM:
local.get $left
local.get $right
i32.gt_s
```

#### IR_CMP_LE
```c
// IR: CMP_LE left, right -> result
// WASM:
local.get $left
local.get $right
i32.le_s
```

#### IR_CMP_GE
```c
// IR: CMP_GE left, right -> result
// WASM:
local.get $left
local.get $right
i32.ge_s
```

#### IR_CMP_EQ
```c
// IR: CMP_EQ left, right -> result
// WASM:
local.get $left
local.get $right
i32.eq
```

#### IR_CMP_NE
```c
// IR: CMP_NE left, right -> result
// WASM:
local.get $left
local.get $right
i32.ne
```

### Logical Instructions

#### IR_BOOL_AND
```c
// IR: BOOL_AND left, right -> result
// WASM (both normalized to 0/1):
local.get $left
local.get $right
i32.and
```

#### IR_BOOL_OR
```c
// IR: BOOL_OR left, right -> result
// WASM (both normalized to 0/1):
local.get $left
local.get $right
i32.or
```

### Type Conversion Instructions

#### IR_SEXT (Sign Extend)
```c
// IR: SEXT i32 -> i64
// WASM:
local.get $value
i64.extend_i32_s
```

#### IR_ZEXT (Zero Extend)
```c
// IR: ZEXT i32 -> i64
// WASM:
local.get $value
i64.extend_i32_u
```

#### IR_TRUNC
```c
// IR: TRUNC i64 -> i32
// WASM:
local.get $value
i32.wrap_i64
```

#### IR_SITOFP (Signed Int to Float)
```c
// IR: SITOFP i32 -> f64
// WASM:
local.get $value
f64.convert_i32_s
```

#### IR_FPTOSI (Float to Signed Int)
```c
// IR: FPTOSI f64 -> i32
// WASM:
local.get $value
i32.trunc_f64_s
```

### Constant Instructions

#### IR_CONST_INT
```c
// IR: CONST_INT 42
// WASM i32:
i32.const 42

// WASM i64:
i64.const 42
```

#### IR_CONST_FLOAT
```c
// IR: CONST_FLOAT 3.14
// WASM f32:
f32.const 3.14

// WASM f64:
f64.const 3.14
```

#### IR_CONST_STRING
```c
// IR: CONST_STRING "hello" (index 0)
// WASM:
i32.const 1024  ;; Address in data section
```

### Stack Instructions

#### IR_LOAD_STACK
```c
// IR: LOAD_STACK [sp+offset]
// WASM:
global.get $sp
i32.const offset
i32.add
i32.load
```

#### IR_STORE_STACK
```c
// IR: STORE_STACK [sp+offset], value
// WASM:
global.get $sp
i32.const offset
i32.add
local.get $value
i32.store
```

## Function Generation

### Function Prologue

```wat
(func $example (param $a i32) (param $b i32) (result i32)
  (local $fp i32)      ;; Frame pointer
  (local $sp i32)      ;; Stack pointer
  (local $result i32)  ;; Local variables
  
  ;; Save frame pointer
  global.get $sp
  local.set $fp
  
  ;; Allocate stack space
  global.get $sp
  i32.const 16         ;; Stack space needed
  i32.add
  global.set $sp
  
  ;; ... function body ...
)
```

### Function Epilogue

```wat
  ;; Restore stack pointer
  local.get $fp
  global.set $sp
  
  ;; Return value
  local.get $result
  return
)
```

## Data Section Generation

### String Literals

```wat
(data (i32.const 0) "Hello, World!\00")
(data (i32.const 14) "Format: %d\00")
```

### Global Variables

```wat
(global $counter (mut i32) (i32.const 0))
```

## Implementation Guidelines

### Code Organization

```
src/backend/
  wasm_codegen.c    # Main entry point, module generation
  wasm_codegen.h    # Public interface
  wasm_func.c       # Function generation
  wasm_func.h       # Function helpers
  wasm_instr.c      # Instruction emission
  wasm_instr.h      # Instruction helpers
  wasm_emit.c       # Text emission utilities
  wasm_emit.h       # Emission interface
```

### Key Data Structures

```c
typedef struct WasmContext {
    FILE *out;
    int func_index;
    int string_index;
    int local_index;
    int label_count;
    List *types;
    List *imports;
    List *functions;
    List *exports;
    List *data;
} WasmContext;

typedef struct WasmFunction {
    char *name;
    char *wat_name;
    List *params;
    List *locals;
    List *results;
    List *blocks;
    bool is_export;
} WasmFunction;
```

### Emission Functions

```c
// Main entry point
void wasm_codegen_generate(IRModule *module, FILE *out);

// Module level
void wasm_emit_module_header(WasmContext *ctx);
void wasm_emit_module_footer(WasmContext *ctx);

// Function level
void wasm_emit_function(WasmContext *ctx, IRFunction *func);
void wasm_emit_prologue(WasmContext *ctx, IRFunction *func);
void wasm_emit_epilogue(WasmContext *ctx, IRFunction *func);

// Instruction level
void wasm_emit_instruction(WasmContext *ctx, IRInstruction *instr);

// Value level
void wasm_emit_value(WasmContext *ctx, IRValue *val);
```

## Error Handling

### Unsupported Operations

```c
if (!wasm_supports_opcode(instr->opcode)) {
    error("Unsupported IR opcode for WASM target: %d\n", instr->opcode);
    return -1;
}
```

### Type Mismatches

```c
if (!wasm_type_compatible(expected, actual)) {
    error("Type mismatch in WASM codegen: expected %d, got %d\n", expected, actual);
    return -1;
}
```

## Testing the Codegen

### Unit Test Example

```c
void test_wasm_emit_add(void) {
    WasmContext *ctx = wasm_context_create();
    IRInstruction *instr = ir_instruction_create(IR_ADD);
    instr->args[0] = ir_value_int(5);
    instr->args[1] = ir_value_int(3);
    
    wasm_emit_instruction(ctx, instr);
    
    char *output = wasm_context_get_output(ctx);
    assert(strstr(output, "i32.const 5") != NULL);
    assert(strstr(output, "i32.const 3") != NULL);
    assert(strstr(output, "i32.add") != NULL);
    
    wasm_context_destroy(ctx);
    ir_instruction_destroy(instr);
}
```

## References

- [WebAssembly Instruction Set](https://webassembly.github.io/spec/core/binary/instructions.html)
- [WAT Format Specification](https://webassembly.github.io/wabt/doc/wat2wasm.html)
