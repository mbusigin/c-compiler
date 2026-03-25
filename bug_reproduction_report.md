# Bug Reproduction Report - C Compiler

**Date**: 2024
**Compiler**: GCC-built stage0

## Bug #1: Function Pointer with Named Parameters

### Status: CONFIRMED

### Failing Syntax
```c
int (*callback)(int x, int y);  // Named parameters - FAILS
int (*callback)(int, int);      // Unnamed parameters - WORKS
```

### Error Messages
```
error: [1:21] expected ')' after type in cast
error: [1:24] expected ';'
error: [1:25] expected ';'
```

### Root Cause
Parser doesn't handle parameter names within function pointer declarators.

### Test Case
```c
// File: /tmp/test4.c
int (*callback)(int x, int y);
```

### Impact
- Cannot declare function pointers with parameter names
- qsort-style callbacks must omit parameter names

---

## Bug #2: Function Pointer in Variable Declaration Context

### Status: PARTIAL

### Failing Syntax
```c
typedef int (*CompFn)(const void*, const void*);  // WORKS
void sort(int (*cmp)(int, int));                  // WORKS  
int (*callback)(int x, int y);                    // FAILS (Bug #1)
```

The parser fails when there's a standalone function pointer variable declaration with named parameters.

### Test Case
```c
// File: /tmp/fp_complete.c
int (*callback)(int x, int y);  // Fails
```

---

## Working Cases

### Typedef for function pointer - WORKS
```c
typedef int (*CompFn)(int, int);
```

### Function parameter with function pointer - WORKS
```c
void sort(int (*cmp)(int, int));
void qsort(void *base, size_t n, size_t s, int (*c)(const void*, const void*));
```

### Function definition with function pointer parameter - WORKS
```c
int apply(int (*fn)(int), int x) {
    return fn(x);
}
```

---

## Files That Compile Successfully

All 20 source files compile without hanging:
- asm.c ✓
- codegen.c ✓
- dwarf.c ✓
- regalloc.c ✓
- error.c ✓
- list.c ✓
- util.c ✓
- driver.c ✓
- ir.c ✓
- lowerer.c ✓
- lexer.c ✓
- preproc.c ✓
- main.c ✓
- constfold.c ✓
- dce.c ✓
- optimizer.c ✓
- ast.c ✓
- parser.c ✓
- analyzer.c ✓
- symtab.c ✓

---

## Summary

| Bug | Status | Impact |
|-----|--------|-------|
| Function pointer named params | CONFIRMED | Cannot use named params in fp declarations |
| Infinite loop (ir.c) | NOT REPRODUCED | All files compile without hanging |
| Struct member access | NOT REPRODUCED | Simple cases work |
| Type compatibility | NOT REPRODUCED | Basic cases work |
| Output file handling | WORKS | -o flag works correctly |

**Action Required**: Fix function pointer parameter name parsing in `src/parser/parser.c`
