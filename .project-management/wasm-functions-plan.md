# WASM Function Calls and Imports Implementation Plan

## Function Calls

### Current Status

The WASM backend currently has partial support for function calls:

**What Works:**
- Basic `call` instruction emission for internal functions
- Function signatures with parameters and return types
- Local function calls within the same module

**What's Broken:**
1. **External function calls** (imports like `printf`) have wrong signature
2. **Argument passing** doesn't match expected parameter counts
3. **Return value handling** for void functions
4. **Function pointer calls** (`IR_CALL_INDIRECT`) not implemented

### Issues Identified

#### 1. Import Function Signature Mismatch
Current import declaration for `printf`:
```wasm
(import "env" "printf" (func $printf (param i32 i32 i32 i32 i32) (result i32)))
```

But actual call in code:
```wasm
i32.const 0  ; Only one argument (string pointer)
call $printf
```

**Problem**: WASM expects 5 arguments but only 1 is provided.

#### 2. Argument Count Tracking
The `IR_CALL` instruction needs to track how many arguments are actually passed, not just emit all argument slots.

#### 3. String Literal Arguments
`printf("Hello")` needs:
1. String literal in data section
2. Pointer to string as argument
3. Correct number of additional arguments (0 for simple case)

### Implementation Plan

#### Phase 1: Fix Internal Function Calls

**Task 1.1: Track Argument Count in IR**
- Modify `IR_CALL` instruction to store actual argument count
- Update IR generator to count arguments properly
- Ensure WASM emitter uses correct argument count

**Task 1.2: Fix Parameter Passing**
- Ensure arguments are pushed in correct order (WASM expects left-to-right)
- Match parameter count in function signature with actual arguments
- Handle variable argument functions specially

**Task 1.3: Return Value Handling**
- Fix `IR_RET_VOID` emission for functions without return values
- Ensure stack balance after function calls

#### Phase 2: Implement Import Functions

**Task 2.1: Import Signature Database**
Create a mapping of common C library functions to WASM signatures:

| C Function | WASM Signature |
|------------|----------------|
| `printf` | `(func (param i32) (result i32))` (simplified) |
| `putchar` | `(func (param i32) (result i32))` |
| `malloc` | `(func (param i32) (result i32))` |
| `free` | `(func (param i32))` |

**Task 2.2: Dynamic Import Detection**
- Scan IR for `IR_CALL` to known external functions
- Generate appropriate import declarations
- Handle variable arguments (like `printf`) with simplified signatures

**Task 2.3: Runtime Integration**
- Create JavaScript host bindings for imports
- Implement simple `printf` that handles basic string output
- Provide memory allocation wrappers

#### Phase 3: Advanced Call Features

**Task 3.1: Function Pointers**
- Implement `IR_CALL_INDIRECT` for function pointers
- WASM equivalent: `call_indirect` with type index
- Need function table declaration

**Task 3.2: Variable Arguments**
- Design for `va_start`, `va_arg`, `va_end`
- WASM doesn't have native varargs, need ABI design
- Option: Pass additional arguments on stack/linear memory

**Task 3.3: Calling Convention**
- Standardize parameter passing order
- Handle return value placement
- Consider stack-based vs register-based for complex types

### Technical Design

#### IR Call Instruction Enhancement

```c
// Current IR_CALL structure needs enhancement
typedef struct IRInstruction {
    IROpcode opcode;
    IRValue *result;
    IRValue *args[4];
    int num_args;
    const char *label;  // Function name
    
    // Add these fields:
    int is_import;      // 1 for external/imported functions
    int is_varargs;     // 1 for variable argument functions
    int call_abi;       // Calling convention (default, varargs, etc.)
} IRInstruction;
```

#### WASM Import Handling

```c
// Import signature structure
typedef struct {
    const char *name;
    const char *wasm_signature;
    int param_count;
    int has_return;
} WasmImportSignature;

// Database of known imports
static WasmImportSignature import_db[] = {
    {"printf", "(param i32) (result i32)", 1, 1},
    {"putchar", "(param i32) (result i32)", 1, 1},
    {"malloc", "(param i32) (result i32)", 1, 1},
    {"free", "(param i32)", 1, 0},
    {NULL, NULL, 0, 0}
};
```

#### WASM Emitter Changes

1. **During IR scanning phase**:
   - Detect calls to external functions
   - Record required imports
   - Validate argument counts

2. **During code generation**:
   - Emit import declarations before function definitions
   - Generate correct `call` instructions with proper argument count
   - Handle return values appropriately

### Test Plan

#### Test Cases to Implement

1. **Simple internal call** (already works):
```c
int add(int a, int b) { return a + b; }
int main() { return add(3, 5); }
```

2. **Multiple arguments**:
```c
int sum4(int a, int b, int c, int d) { return a + b + c + d; }
```

3. **Void function**:
```c
void do_nothing() {}
int main() { do_nothing(); return 0; }
```

4. **External import**:
```c
int main() {
    // Should generate import for putchar
    putchar('A');
    return 0;
}
```

5. **Function pointer**:
```c
int (*func_ptr)(int, int);
int add(int a, int b) { return a + b; }
int main() {
    func_ptr = add;
    return func_ptr(3, 5);
}
```

### Implementation Steps

#### Week 1: Foundation
1. Enhance IR_CALL instruction with argument tracking
2. Fix basic internal function calls
3. Implement test suite for function calls

#### Week 2: Imports
1. Create import signature database
2. Implement import detection and declaration
3. Fix `printf`/`putchar` calls

#### Week 3: Advanced Features
1. Implement function pointers (`call_indirect`)
2. Design variable argument handling
3. Optimize calling convention

### Success Criteria

1. **All test cases compile** without WASM validation errors
2. **Import functions work** with JavaScript host
3. **Function pointers work** for internal functions
4. **Performance** comparable to direct calls

### Risks and Mitigation

#### Risk 1: WASM Signature Complexity
- **Risk**: C functions with complex signatures hard to map to WASM
- **Mitigation**: Start with simple signatures, expand gradually

#### Risk 2: Variable Arguments
- **Risk**: WASM has no native varargs support
- **Mitigation**: Implement simplified varargs using linear memory

#### Risk 3: Performance
- **Risk**: Indirect calls slower than direct
- **Mitigation**: Optimize common cases, use direct calls when possible

### Dependencies

1. **String literal support** needed for `printf` tests
2. **Memory allocation** needed for `malloc`/`free` tests
3. **Data section** needed for global function pointer tables

### Future Work

1. **WASI support** - Standardized system interface
2. **Dynamic linking** - Load external WASM modules
3. **Optimized calls** - Tail call optimization
4. **Exception handling** - Try/catch for WASM exceptions

### Conclusion

Function call implementation is critical for WASM backend completeness. The plan focuses on fixing existing issues with argument passing and imports, then adding advanced features like function pointers. Implementation should proceed incrementally with comprehensive testing at each stage.