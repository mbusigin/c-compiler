# Known Issue: Function Pointers in Struct Members

## Status
**Known Limitation** - Parser crashes when parsing function pointers in struct members

## Problem Description

The parser successfully handles:
- ✅ Global function pointers: `int (*f)(int);`
- ✅ Function pointers as local variables

But crashes when parsing:
- ❌ Function pointers in struct members: `struct S { int (*f)(int); };`
- ❌ Function pointers in typedef'd structs: `typedef struct { int (*f)(int); } S;`

## Technical Details

**Symptoms**:
- Parser enters parsing code for function pointers
- Crashes with Trace/BPT trap (assertion or runtime error)
- Likely issue: type system not handling function types as struct members

**Investigation**:
- `token_name()` allocates memory correctly
- Function pointer pattern detection works
- Crash occurs after `type_add_member()` is called with function type
- Issue may be in later compilation stages expecting only data types

## Impact

**Blocks compilation of**:
- `src/target/target.c` (15 function pointers in TargetVTable struct)
- `src/target/arm64/arm64_target.c` (uses TargetVTable)
- `src/target/wasm/wasm_target.c` (uses TargetVTable)

**Self-hosting status**: 33/36 files compile (92%)

## Workaround Options

### Option 1: Refactor Target Abstraction (RECOMMENDED)

Replace function pointer table with switch-based dispatch:

```c
typedef enum {
    TARGET_ARM64,
    TARGET_WASM
} TargetKind;

typedef struct {
    TargetKind kind;
    FILE *output;
    // Remove function pointers, use kind field for dispatch
} Target;

// In implementation:
void target_emit_instruction(Target *target, IRInstruction *instr) {
    switch (target->kind) {
        case TARGET_ARM64:
            arm64_emit_instruction(target, instr);
            break;
        case TARGET_WASM:
            wasm_emit_instruction(target, instr);
            break;
    }
}
```

**Pros**:
- Avoids function pointer syntax
- Maintains clean API
- Works with current parser

**Cons**:
- Loses some abstraction elegance
- Requires refactoring target.c files

**Estimated effort**: 3-4 hours

### Option 2: Fix Parser Issue

Debug and fix the root cause:
1. Investigate why function types as struct members crash
2. Fix type system to handle function types
3. Test thoroughly

**Pros**:
- Fixes the underlying issue
- Completes C feature support

**Cons**:
- Could take 4-8+ hours
- May require deep debugging
- Risk of introducing other issues

## Recommendation

For **Task 6.1 completion**, use **Option 1** (refactor target abstraction):
- Faster path to self-hosting
- Clean, maintainable code
- Can revisit function pointer support later

For **production compiler**, implement **Option 2**:
- Function pointers are essential for real C code
- Should be fixed eventually
- Not blocking for current sprint

## Files to Modify (Option 1)

1. `src/target/target.h` - Replace function pointers with enum dispatch
2. `src/target/target.c` - Implement dispatch functions
3. `src/target/arm64/arm64_target.c` - Adapt to new interface
4. `src/target/wasm/wasm_target.c` - Adapt to new interface

## Testing

After refactoring:
1. Compile all target files: `./compiler src/target/*.c`
2. Build stage1: `make self`
3. Verify all 36 files compile

## Next Steps

1. Document this limitation (this file)
2. Refactor target abstraction (Option 1)
3. Complete self-hosting verification
4. Create future task to fix function pointers properly

---

**Priority**: High (blocks self-hosting)
**Effort**: 3-4 hours for workaround, 4-8+ hours for proper fix
**Decision**: Use workaround to complete Sprint 6, fix properly in future sprint
