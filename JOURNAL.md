# C Compiler Bootstrap Journal

## Iteration 3 Summary

### Goal
Execute OODA iteration 3/10: make self-verify, identify bugs, fix, and journal results.

### Progress Made

#### ✅ Major Bug Fixes

1. **Added implicit return for void functions** - Functions that don't end with a return statement now get an implicit `ret_void` instruction, preventing undefined behavior.

2. **Fixed stderr/stdout/stdin handling** - Changed from macros to external variable declarations in stdio.h, and updated the lowerer to recognize these symbols even if not in the symbol table.

3. **Improved short-circuit evaluation for && and ||** - Implemented proper control flow with labels for logical operators.

### Current Status

| Stage | Status |
|-------|--------|
| Stage0 (GCC) | ✅ Builds successfully, compiles all 36/36 files |
| Stage1 (Self-compiled) | ✅ Builds and runs `--help` successfully |
| Stage2 (Stage1-compiled) | ❌ Crashes when compiling files |

### Remaining Issues

**Stage1 crashes when compiling source files:**
- `./build/compiler_stage1 -S /tmp/tiny.c` crashes
- Crash occurs in `strlen` called from `vfprintf` inside `vreport` (error reporting)
- The issue appears to be with string address calculations or memory access patterns

**Investigation findings:**
- Stage1 correctly runs `--help` (no file input needed)
- Stage1 crashes when it tries to report an error (like "Could not read file")
- The crash address (0x4b8d0) corresponds to the string "error" in the binary
- The issue is likely related to how stage0-generated code handles certain memory operations

### Files Modified

1. `src/ir/lowerer.c` - Added implicit ret_void, fixed conditional expressions, improved stderr handling
2. `src/backend/codegen.c` - Frame pointer setup for callee-saved registers
3. `include/stdio.h` - Changed stdin/stdout/stderr from macros to extern declarations
4. `JOURNAL.md` - Documentation

### Next Steps for Iteration 4

1. Debug why stage1 crashes when compiling files
2. Check string literal address calculation in generated code
3. Investigate memory access patterns for error reporting
4. Consider if the issue is in how we're loading string addresses

### Test Commands

```bash
# Build and test
make clean && make self

# Test stage1 with --help
./build/compiler_stage1 --help

# Test stage1 compiling a file
./build/compiler_stage1 -S /tmp/tiny.c -o /tmp/tiny.s

# Full self-verify
make self-verify
```

### Convergence Status

```
CONVERGENCE: IN PROGRESS - Stage1 runs --help but crashes when compiling files
```

### Technical Details

**Stage1 vreport function analysis:**
- Correctly loads stderr via GOT
- Uses fprintf with format string "%s%s:"
- Crashes when strlen tries to read the format string
- String addresses appear to be calculated incorrectly in some cases

**Hypothesis:**
The issue may be related to how the compiler handles PC-relative addressing for string literals when compiled by itself. The first instruction that accesses a string literal after a function call may have incorrect address calculation.