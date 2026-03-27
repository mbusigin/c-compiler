# C Compiler Bootstrap Journal

## convergence Progress Analysis

### Initial State
- **Stage0 Compiler**: Built with GCC, works correctly
- **Stage1 Compiler**: Self-hosted, crashes in argument parsing
- **Critical Bug**: `options.input_file` not being set, causing NULL FILE* crash

### Bug Fixes Applied (Convergence Metrics)

| Fix # | Component | Bug Description | Status |
|-------|-----------|-----------------|--------|
| 1 | lexer.c | Character constants hardcoded to 0 | ✅ FIXED |
| 2 | lowerer.c | Array subscript for non-identifier bases | ✅ FIXED |
| 3 | codegen.c | Byte loads for char arrays (ldrb vs ldr) | ✅ FIXED |
| 4 | lowerer.c | Boolean OR operand saving | ✅ FIXED |
| 5 | codegen.c | Binary ops with two constants | ✅ FIXED |
| 6 | lowerer.c | Implicit return for void functions | ✅ FIXED |
| 7 | ir.h | Added elem_size field to IRValue | ✅ FIXED |
| 8 | lowerer.c | Struct member assignment IR generation | ✅ ADDED |
| 9 | codegen.c | 64-bit add for pointer arithmetic | ✅ FIXED |
| 10 | lowerer.c | String literal handling for struct stores | ✅ FIXED |
| 11 | ir.h | IR_SAVE_X8_TO_X22, IR_STORE_INDIRECT_X22 | ✅ ADDED |
| 12 | codegen.c | Use x22 for struct member address | ✅ FIXED |

### Remaining Issue: Stack Layout Conflict

The assembly shows that sp+56 is being used for BOTH:
1. `options.input_file` member (struct at sp+16, member offset 40)
2. Temporary boolean values

This causes input_file to be overwritten after being set correctly.

### Convergence Measurement

```
Stage0 (GCC-compiled): 100% functional
Stage1 (self-hosted):  CRASHES - stack layout conflict

Character comparison test: PASS
Byte load test: PASS
Array subscript test: PASS
Struct member store IR: GENERATED
```

### Root Cause

The IR uses `store_stack` with fixed offsets that don't account for struct layouts properly. Temporaries and struct members share the same stack space.

### Solution Needed

1. Proper stack frame management in IR generation
2. Separate allocation for struct locals vs temporaries
3. Or: Use register-based approach for struct member stores

### Test Results

| Test | Result |
|------|--------|
| `./build/compiler_stage0 -S test.c` | ✅ PASS |
| `./build/compiler_stage1 test.c -o test.s` | ❌ CRASH |
| Character comparison `argv[i][0] != '-'` | ✅ PASS |
| Byte load for char array | ✅ PASS |
| Nested array subscript | ✅ PASS |
| Struct member store IR generation | ✅ PASS (but overwritten) |
