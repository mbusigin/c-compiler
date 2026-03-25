# Root Cause Analysis Report - C Compiler Self-Hosting Failures

## Executive Summary

The C compiler cannot achieve self-hosting due to **6 critical bugs** in the compiler implementation. Analysis of compilation failures reveals issues in the parser, semantic analyzer, and code generator.

---

## Critical Bug #1: INFINITE LOOP IN CODE GENERATION

### Severity: CRITICAL
### Impact: 6 files cannot be compiled (28.6% of source)

### Affected Files:
- test_framework.c (hangs)
- ir.c (hangs)
- lowerer.c (hangs)
- optimizer.c (hangs)
- codegen.c (hangs)
- driver.c (hangs)

### Symptoms:
- Compiler enters infinite loop during compilation
- No error messages output
- Process must be killed after timeout (5+ seconds)
- CPU remains at 100% usage

### Root Cause Analysis:

**test_framework.c** is only 2 lines:
```c
#include "backend/codegen.h"
IRModule *m;
```

This minimal file causes a hang, suggesting the issue is in:
1. Include processing for `backend/codegen.h`
2. IR module handling
3. Global variable declaration processing

**Investigation Path:**
1. codegen.h includes ir.h
2. ir.h defines IRModule structure
3. IRModule uses List type from common/list.h
4. Possible circular dependency or infinite recursion in type resolution

**Hypothesis:** The compiler has an infinite loop when:
- Processing struct types that contain List pointers
- Resolving forward declarations for IR types
- Computing type sizes for struct members

**Location in Compiler:** 
- Likely in `src/sema/analyzer.c` - type resolution
- Or in `src/backend/codegen.c` - IR generation
- Or in `src/ir/ir.c` - IR module creation

### Recommended Fix:

1. **Add debug logging** to identify exact hang location:
```c
// In analyzer.c, codegen.c, ir.c
fprintf(stderr, "DEBUG: Entering function %s\n", __func__);
```

2. **Check for infinite recursion** in:
   - Type size computation (`type_compute_struct_size`)
   - Member offset calculation
   - List operations

3. **Add recursion depth limits** with clear error messages

4. **Fix circular dependency** in IR/codegen includes

---

## Critical Bug #2: PARSER CANNOT PARSE UNION MEMBER ACCESS

### Severity: HIGH
### Impact: Parser cannot compile itself

### Affected Files:
- parser.c
- ast.c

### Error Messages:
```
error: [837:27] expected ')'
error: struct has no member named 'data'
error: struct has no member named 'type'
error: struct has no member named 'name'
```

### Root Cause:

The ASTNode structure uses a **union** with nested anonymous structs:
```c
struct ASTNode {
    ASTNodeType type;
    // ...
    union {
        struct { List *declarations; } unit;
        struct { char *name; Type *func_type; /* ... */ } function;
        struct { List *stmts; } compound;
        // ... more anonymous structs
    } data;
};
```

The code accesses this as: `node->data.compound.stmts`

**The Problem:** The parser is failing to correctly handle:
1. Anonymous struct members within unions
2. Nested member access through union members
3. Member lookup for union-contained structs

**Location in Compiler:**
- `src/parser/parser.c` - member access parsing (lines ~600-700)
- `src/sema/analyzer.c` - member access semantic checking

### Recommended Fix:

1. **In parser.c**, when parsing `node->data.member`, check:
   - If `node` is a struct/union pointer
   - If `data` is a union member
   - If `member` is a field within that union variant

2. **In analyzer.c**, for member access:
   - Handle union member types specially
   - Look up members in the correct union variant
   - Support anonymous struct/union members

---

## Critical Bug #3: TYPE COMPATIBILITY CHECKING BROKEN

### Severity: HIGH  
### Impact: Type errors prevent compilation

### Error Messages:
```
error: assignment of incompatible types (pointer vs pointer)
error: assignment of incompatible types (int vs pointer)
error: comparison between incompatible types
```

### Root Cause:

The type system does not properly handle:
1. **Pointer compatibility** - All pointers should be assignable to each other (with warnings)
2. **Integer-pointer conversions** - Should allow with proper casting rules
3. **Void pointer** - Should be compatible with any pointer type

**Location:** `src/sema/analyzer.c` - type checking functions

### Recommended Fix:

Implement proper type compatibility rules:
```c
bool types_compatible(Type *t1, Type *t2, bool is_assignment) {
    // Same type is always compatible
    if (types_equal(t1, t2)) return true;
    
    // Void pointer compatibility
    if (is_void_pointer(t1) || is_void_pointer(t2)) return true;
    
    // Pointer to pointer assignments
    if (t1->kind == TYPE_POINTER && t2->kind == TYPE_POINTER) {
        return true;  // Allow with warning
    }
    
    // Integer to pointer (with warning)
    if (is_integer(t1) && t2->kind == TYPE_POINTER) {
        return true;  // Allow with warning
    }
    
    // ... other rules
}
```

---

## Critical Bug #4: FORWARD DECLARATION NOT WORKING

### Severity: MEDIUM
### Impact: Undeclared identifier errors

### Error Messages:
```
error: undeclared identifier 'next'
error: undeclared identifier 'error_count'
error: undeclared identifier 'warning_count'
error: undeclared identifier 'scope_level'
```

### Root Cause:

The symbol table is not properly handling:
1. Forward declarations
2. Out-of-order declarations
3. Global scope in multi-file compilation

**Example:** In symtab.c, the Symbol struct has a `next` pointer:
```c
typedef struct Symbol {
    char *name;
    // ...
    struct Symbol *next;  // This might not be recognized
} Symbol;
```

**Location:** `src/sema/symtab.c` - symbol lookup
**Also:** `src/parser/parser.c` - declaration processing

### Recommended Fix:

1. **Two-pass compilation:**
   - First pass: Collect all declarations (prototypes, structs, globals)
   - Second pass: Process function bodies with all symbols known

2. **Or:** Fix forward reference handling in symbol table:
   - When encountering struct X*, don't require X to be fully defined yet
   - Mark as "incomplete type" and resolve later

---

## Critical Bug #5: SYNTAX PARSING ERRORS

### Severity: MEDIUM
### Impact: Parser fails on valid C code

### Error Messages:
```
error: [319:27] expected ')'
error: [319:29] expected ')'  
error: [319:39] expected ';'
```

### Root Cause:

Parser is failing on specific C constructs, possibly:
1. **Complex macro expansions** in preprocessor
2. **Multi-line conditionals** 
3. **Compound literals** or **designated initializers**
4. **__attribute__** or compiler-specific extensions

**Location:** 
- `src/parser/parser.c` - expression/statement parsing
- `src/lexer/preproc.c` - macro expansion

### Recommended Fix:

1. Add detailed error location reporting:
   - Print the actual token being parsed
   - Show the context around the error
   - Indicate what was expected vs what was found

2. Test parser incrementally:
   - Extract problematic lines
   - Test in isolation
   - Identify missing grammar rules

---

## Critical Bug #6: OUTPUT FILE HANDLING

### Severity: LOW
### Impact: Workaround exists (redirect stdout)

### Issue:
The `-o` flag does not create the output file:
```bash
./compiler -o output.s input.c  # Fails: "Could not open output file"
./compiler -S input.c > output.s  # Works: redirect stdout
```

### Root Cause:

File I/O handling in `src/driver.c` is broken for the `-o` flag.

**Location:** `src/driver.c` - output file handling

### Recommended Fix:

Check the file opening code:
```c
FILE *outfile = fopen(output_filename, "w");
if (!outfile) {
    // Error handling
    fprintf(stderr, "Could not open output file: %s\n", output_filename);
    return false;
}
```

Verify the filename is being passed correctly from command-line argument parsing.

---

## Priority Order for Fixes

1. **Bug #1: Infinite Loop** (CRITICAL - blocks 28.6% of files)
   - Add debug logging to locate exact hang
   - Fix circular dependency or infinite recursion
   - Add depth limits with error messages

2. **Bug #2: Union Member Access** (HIGH - parser can't compile itself)
   - Fix parser to handle union.anonymous_struct.member
   - Update semantic analyzer for union member lookup
   - Test with ASTNode pattern

3. **Bug #3: Type Compatibility** (HIGH - many type errors)
   - Implement proper compatibility rules
   - Handle void* specially
   - Allow pointer-pointer assignment

4. **Bug #4: Forward Declarations** (MEDIUM - undeclared identifiers)
   - Implement two-pass or fix symbol resolution
   - Handle incomplete types
   - Fix scope handling

5. **Bug #5: Syntax Parsing** (MEDIUM - specific constructs fail)
   - Add better error reporting
   - Identify missing grammar rules
   - Test incrementally

6. **Bug #6: Output File** (LOW - workaround exists)
   - Fix file I/O in driver.c
   - Verify argument parsing

---

## Testing Strategy

After each fix:

1. **Unit test**: Compile the fixed component with GCC
2. **Self-test**: Compile the component with the fixed compiler
3. **Regression**: Run full test suite: `make test`
4. **Self-hosting**: Run `make self` and check success rate
5. **Verification**: Ensure stage1 and stage2 produce identical output

---

## Estimated Effort

- Bug #1 (Infinite Loop): 2-4 hours
- Bug #2 (Union Access): 3-6 hours  
- Bug #3 (Type Compatibility): 2-3 hours
- Bug #4 (Forward Decls): 3-5 hours
- Bug #5 (Syntax Parsing): 2-4 hours
- Bug #6 (Output File): 30 minutes

**Total: 12-22 hours of focused development**

---

## Success Criteria

After fixes, the compiler should:
1. Compile all 21 source files without hanging
2. Successfully build stage1 compiler
3. Use stage1 to build stage2 compiler
4. Stage2 should compile stage3 (convergence test)
5. All three stages should produce identical output for test programs
