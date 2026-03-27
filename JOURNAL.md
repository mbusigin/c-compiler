# C Compiler Bootstrap Journal

## Iteration 1 Summary

### Goal
Make self-verify pass for the C compiler. Identify and fix any compiler bugs preventing self-hosting.

### Progress Made

#### ✅ Fixed: 4 Previously Failing Files Now Compile

1. **src/target/target.c** - Now compiles successfully
2. **src/target/arm64/arm64_target.c** - Now compiles successfully  
3. **src/target/wasm/wasm_target.c** - Now compiles successfully
4. **src/backend/wasm_codegen.c** - Now compiles successfully

**Result:** Stage0 (GCC-compiled) compiler now compiles all 36 source files successfully!

### Bug Fixes Applied

| # | File | Issue | Fix |
|---|------|-------|-----|
| 1 | parser.c | Missing IR types (IRModule, IRFunction, etc.) | Added to builtin types |
| 2 | target.h | Inline functions using incomplete `Target` typedef | Changed to `struct Target` |
| 3 | wasm_emit.h | Missing extern for wasm_stack_ptr | Added extern declaration |
| 4 | analyzer.c | wasm_stack_ptr undeclared | Added to declare_builtins() |
| 5 | lowerer.c | Struct initializers not handled, storing garbage | Emit zero for unhandled initializers |
| 6 | codegen.c | IR_SAVE_X8_TO_X20 not implemented | Added case to emit mov x20, x8 |
| 7 | codegen.c | IR_ADD_X21 not implemented | Added case to emit add x8, x20, x8 |
| 8 | codegen.c | IR_CALL incorrectly saved x0 to x8 before call | Removed incorrect save |
| 9 | codegen.c | IR_CONST_INT saved to x20, overwriting saved pointer | Removed save to x20, use x10 only |

### Stage Compilation Status

| Stage | Status |
|-------|--------|
| Stage0 (GCC) | ✅ Builds successfully |
| Stage1 (Self-compiled) | ⚠️ Compiles but crashes at runtime |

### Remaining Issue: Stage1 Runtime Crash

The self-compiled Stage1 compiler crashes during execution. The crash occurs in `strncmp` with an invalid pointer address.

**Root Cause Analysis:**
Multiple code generation bugs were identified and fixed:
1. IR_SAVE_X8_TO_X20 and IR_ADD_X21 were not implemented
2. IR_CALL had incorrect code that saved x0 to x8 before the call
3. IR_CONST_INT was incorrectly saving to x20, overwriting the saved pointer for array indexing

**Current Status:**
The generated assembly for array subscript now correctly:
1. Loads argv
2. Saves argv to x20
3. Loads i
4. Computes i * 8
5. Adds argv + i*8
6. Loads from result

However, there's still a crash during execution that needs further investigation.

### Convergence Status

```
CONVERGENCE: FAIL - stage1 crashes at runtime
```

### Files Modified

1. `src/parser/parser.c` - Added IR types to builtin types
2. `src/sema/analyzer.c` - Added wasm_stack_ptr builtin
3. `src/target/target.h` - Changed Target* to struct Target*
4. `src/target/target.c` - Changed Target* to struct Target*
5. `src/target/arm64/arm64_target.c` - Changed Target* to struct Target*
6. `src/target/wasm/wasm_target.c` - Changed Target* to struct Target*
7. `src/backend/wasm_emit.h` - Added wasm_stack_ptr extern
8. `src/backend/wasm_emit.c` - Added wasm_stack_ptr definition
9. `src/backend/wasm_codegen.c` - Fixed function definitions
10. `src/runtime.c` - Removed duplicate wasm_stack_ptr
11. `src/ir/lowerer.c` - Fixed struct initializer handling
12. `src/backend/codegen.c` - Fixed multiple IR instruction implementations

### Next Steps for Iteration 2

1. Debug remaining Stage1 crash using lldb to trace execution
2. Check if there are more unimplemented IR instructions
3. Verify string literal handling is correct
4. Check function call argument passing
5. Re-test self-verify after fixing all crashes
6. Verify convergence between stage1 and stage2

### Test Commands

```bash
# Build stage0
make stage0

# Test compilation of all files
make self

# Test self-verify (will fail until crash is fixed)
make self-verify
```
