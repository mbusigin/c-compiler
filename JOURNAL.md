# C Compiler Bootstrap Journal

## Iteration 1 Summary

### Goal
Execute OODA iteration 1/10: make self-verify, identify compiler bugs preventing test completion/execution, fix bugs, journal results.

### Progress Made

#### ✅ Major Bug Fixes

1. **Stack frame addressing bug** - Changed from SP-relative to FP-relative (x29) addressing
   - Fixed IR_LOAD_STACK, IR_STORE_STACK, IR_STORE_PARAM to use `[x29, #offset]`
   - Fixed emit_load_value for local variables
   - Fixed prologue/epilogue for correct frame pointer setup

2. **Printf variadic argument handling** - Fixed multiple issues:
   - Added support for 3+ arguments in printf calls
   - Fixed argument ordering to preserve temp values in x8
   - Fixed stack allocation for variadic args

3. **Call argument preservation** - Fixed temp value handling
   - Changed to check ALL remaining arguments (not just next one) for potential x8 modification
   - Saves temp values to stack when any later argument might modify x8

4. **Global variable support** - Added IR_LOAD_EXTERNAL for external globals
   - Added stderr, stdout as builtin symbols
   - Uses `adrp + ldr` from GOT for external symbols
   - Fixed symbol name mapping (stderr -> __stderrp on macOS)

5. **Large stack frame handling** - Fixed ldp/stp offset limits
   - Added manual address computation for frames > 504 bytes

### Current Status

| Stage | Status |
|-------|--------|
| Stage0 (GCC) | ✅ Builds successfully, compiles all 36/36 files |
| Stage1 (Self-compiled) | ⚠️ Builds but crashes at runtime due to missing static variable definitions |

### Remaining Issues

1. **Static variable definitions not emitted** - The compiler generates references to static variables (like `arm64_vtable`, `MAX_RECURSION`) but doesn't emit the actual data definitions in the assembly output.

2. **Workaround in place** - Added stub definitions in `runtime.s` for critical static variables, but this is not a proper solution.

### Files Modified

1. `src/backend/codegen.c` - Frame pointer addressing, printf handling, external symbol loading
2. `src/ir/lowerer.c` - Call argument preservation, global variable handling
3. `src/sema/analyzer.c` - Added stderr, stdout, __stderrp, __stdoutp builtins
4. `src/parser/parser.c` - Removed stderr/stdout null-pointer workaround
5. `src/runtime.s` - Added stubs for global/static variables

### Next Steps for Iteration 2

1. Implement proper static/global variable data emission in assembly output
2. Handle static const variables as compile-time constants (inline them)
3. Test self-verify after fixing static variable issues
4. Work towards convergence where stage1 and stage2 produce identical output

### Test Commands

```bash
# Build and test
make clean && make self

# Test stage1
./build/compiler_stage1 --help

# Full self-verify
make self-verify
```

### Convergence Status

```
CONVERGENCE: IN PROGRESS - Stage1 builds, runtime crashes due to missing static variable definitions
```
