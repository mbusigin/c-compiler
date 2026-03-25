# WASM Module Structure and Export/Import Design

## Overview

This document defines the module structure, function signatures, memory layout, and import/export conventions for the WASM backend.

## Module Structure

### Standard Module Layout

```wat
(module
  ;; 1. Type definitions
  (type $t_main (func (result i32)))
  (type $t_printf (func (param i32 i32 i32 i32 i32) (result i32)))
  
  ;; 2. Imports
  (import "env" "printf" (func $printf (type $t_printf)))
  (import "env" "putchar" (func $putchar (param i32) (result i32)))
  (import "env" "malloc" (func $malloc (param i32) (result i32)))
  (import "env" "free" (func $free (param i32)))
  
  ;; 3. Memory
  (memory (export "memory") 2)  ;; 2 pages = 128KB
  
  ;; 4. Globals
  (global $sp (mut i32) (i32.const 65536))  ;; Stack pointer starts after first page
  (global $heap_ptr (mut i32) (i32.const 65536))
  
  ;; 5. Functions
  (func $main (type $t_main) (result i32)
    ;; ... body ...
  )
  
  ;; 6. Exports
  (export "main" (func $main))
  (export "_main" (func $main))
  
  ;; 7. Data
  (data (i32.const 0) "string data\00")
  
  ;; 8. Start function (optional)
  ;; (start $init)
)
```

## Memory Layout

### Linear Memory Map

```
Address Range    | Usage
-----------------|----------------------------------
0 - 1023         | Static data (strings, constants)
1024 - 65535     | Global variables, heap
65536 - 131071   | Stack (grows upward)
131072+          | Additional pages (if allocated)
```

### Stack Layout

```
High Addresses
+------------------+
| Function locals  |
+------------------+
| Saved frame ptr  | <- fp
+------------------+
| Arguments        |
+------------------+
| Return slot      |
+------------------+  <- sp (stack pointer)
Low Addresses
```

## Function Signatures

### Main Function

```wat
;; C: int main(void)
(type $t_main (func (result i32)))
(func $main (type $t_main) (result i32)
  i32.const 0
  return
)
(export "main" (func $main))
(export "_main" (func $main))  ;; macOS convention
```

### Functions with Parameters

```wat
;; C: int add(int a, int b)
(type $t_add (func (param i32 i32) (result i32)))
(func $add (type $t_add) (param $a i32) (param $b i32) (result i32)
  local.get $a
  local.get $b
  i32.add
  return
)
(export "add" (func $add))
(export "_add" (func $add))
```

### Void Functions

```wat
;; C: void foo(void)
(type $t_void (func))
(func $foo (type $t_void)
  ;; ... body ...
  return
)
```

### Functions with Pointers

```wat
;; C: int deref(int *p)
(type $t_deref (func (param i32) (result i32)))
(func $deref (type $t_deref) (param $p i32) (result i32)
  local.get $p
  i32.load
  return
)
```

## Import Conventions

### Standard Library Imports

```wat
;; I/O functions
(import "env" "printf" (func $printf (param i32 i32 i32 i32 i32) (result i32)))
(import "env" "putchar" (func $putchar (param i32) (result i32)))
(import "env" "gets" (func $gets (param i32) (result i32)))

;; Memory functions
(import "env" "malloc" (func $malloc (param i32) (result i32)))
(import "env" "free" (func $free (param i32)))
(import "env" "memcpy" (func $memcpy (param i32 i32 i32) (result i32)))
(import "env" "memset" (func $memset (param i32 i32 i32) (result i32)))

;; String functions
(import "env" "strlen" (func $strlen (param i32) (result i32)))
(import "env" "strcmp" (func $strcmp (param i32 i32) (result i32)))
```

### JavaScript Host Implementation

```javascript
const imports = {
  env: {
    memory: new WebAssembly.Memory({ initial: 2 }),
    
    printf: (fmt, a1, a2, a3, a4, a5) => {
      // Read format string from memory
      const format = readString(fmt);
      // Simple printf implementation
      let output = format.replace(/%d/g, () => {
        // Get next argument
        return a1;
      });
      console.log(output);
      return output.length;
    },
    
    putchar: (c) => {
      process.stdout.write(String.fromCharCode(c));
      return c;
    },
    
    malloc: (size) => {
      // Simple bump allocator
      const ptr = heapPtr;
      heapPtr += size;
      return ptr;
    },
    
    free: (ptr) => {
      // No-op for bump allocator
    },
    
    memcpy: (dst, src, len) => {
      const mem = new Uint8Array(memory.buffer);
      for (let i = 0; i < len; i++) {
        mem[dst + i] = mem[src + i];
      }
      return dst;
    },
    
    memset: (ptr, val, len) => {
      const mem = new Uint8Array(memory.buffer);
      for (let i = 0; i < len; i++) {
        mem[ptr + i] = val;
      }
      return ptr;
    }
  }
};
```

## Export Conventions

### Function Exports

```wat
;; Export with both C and macOS naming
(export "function_name" (func $function_name))
(export "_function_name" (func $function_name))
```

### Memory Export

```wat
;; Always export memory for host access
(memory (export "memory") 2)
```

### Global Exports

```wat
;; Export globals if needed by host
(global $errno (mut i32) (i32.const 0))
(export "errno" (global $errno))
```

## Data Section

### String Literals

```wat
;; Strings placed at fixed addresses
(data (i32.const 0) "Hello, World!\00")
(data (i32.const 14) "Value: %d\00")
(data (i32.const 25) "Error\00")
```

### String Address Calculation

```
String Index | Address | Content
-------------|---------|----------
0            | 0       | "Hello, World!\00"
1            | 14      | "Value: %d\00"
2            | 25      | "Error\00"
```

### Global Variable Initialization

```wat
;; Initialized global
(global $counter (mut i32) (i32.const 0))

;; Uninitialized (in BSS equivalent)
(global $buffer (mut i32) (i32.const 0))  ;; Will be allocated
```

## Calling Convention

### Parameter Passing

```
C Function        | WASM Signature
------------------|----------------------------------
int f()           | (func (result i32))
int f(int a)      | (func (param i32) (result i32))
int f(int a, b)   | (func (param i32 i32) (result i32))
void f(int a)     | (func (param i32))
```

### Return Values

```
C Type   | WASM Result
---------|------------
int      | i32
long     | i64
float    | f32
double   | f64
void     | (no result)
pointer  | i32
```

### Variadic Functions

For printf and other variadic functions:
```wat
;; All arguments passed explicitly
call $printf  ;; Arguments on stack before call
```

## Module Generation

### Module Header

```c
void wasm_emit_module_header(WasmContext *ctx) {
    fprintf(ctx->out, "(module\n");
    fprintf(ctx->out, "  ;; Generated by c-compiler\n\n");
}
```

### Type Section

```c
void wasm_emit_types(WasmContext *ctx, IRModule *mod) {
    fprintf(ctx->out, "  ;; Type definitions\n");
    for (each function in mod) {
        fprintf(ctx->out, "  (type $t_%s (func", func->name);
        for (each param) fprintf(ctx->out, " (param i32)");
        if (has_return) fprintf(ctx->out, " (result i32)");
        fprintf(ctx->out, "))\n");
    }
}
```

### Import Section

```c
void wasm_emit_imports(WasmContext *ctx) {
    fprintf(ctx->out, "\n  ;; Imports\n");
    if (uses_printf) {
        fprintf(ctx->out, "  (import \"env\" \"printf\" (func $printf (param i32 i32 i32 i32 i32) (result i32)))\n");
    }
    if (uses_putchar) {
        fprintf(ctx->out, "  (import \"env\" \"putchar\" (func $putchar (param i32) (result i32)))\n");
    }
}
```

### Export Section

```c
void wasm_emit_exports(WasmContext *ctx, IRModule *mod) {
    fprintf(ctx->out, "\n  ;; Exports\n");
    for (each function in mod) {
        if (is_exported(func)) {
            fprintf(ctx->out, "  (export \"%s\" (func $%s))\n", func->name, func->name);
            fprintf(ctx->out, "  (export \"_%s\" (func $%s))\n", func->name, func->name);
        }
    }
    fprintf(ctx->out, "  (export \"memory\" (memory 0))\n");
}
```

## Special Considerations

### Main Function

The main function is always exported:
```wat
(export "main" (func $main))
(export "_main" (func $main))
```

### Runtime Initialization

If initialization is needed:
```wat
(func $init
  ;; Initialize globals
  i32.const 0
  global.set $sp
)
(start $init)
```

### Multiple Modules

For larger programs, consider:
- Separate modules for libraries
- Linking at binary level
- Shared memory between modules

## References

- [WebAssembly Module Structure](https://webassembly.github.io/spec/core/binary/modules.html)
- [WASM Linking](https://github.com/WebAssembly/tool-conventions/blob/main/Linking.md)
