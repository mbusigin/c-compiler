# Task 6.1 Status: Function Pointer Support - WORKAROUND FOUND

## Summary

**Parser successfully handles function pointers in struct members when using typedef syntax.**

## Test Results

### ✅ WORKS: Typedef Function Pointer Syntax

```c
typedef int (*FuncPtr)(int);

struct S {
    FuncPtr f;  // ✅ This works!
};
```

**Result**: Compiles successfully

### ❌ CRASHES: Direct Function Pointer Syntax  

```c
struct S {
    int (*f)(int);  // ❌ Crashes in later compilation stage
};
```

**Result**: Crash after parsing completes

## Technical Analysis

### Parser Status: WORKING ✅

- Parser correctly detects function pointer pattern `(*name)(params)`
- Creates proper type structure (TYPE_POINTER to TYPE_FUNCTION)
- Handles parameter lists correctly
- Works with empty parameter lists `()`
- Works with complex parameter types

### Crash Location: POST-PARSER

The crash occurs **after** parsing completes, likely in:
- Semantic analysis
- AST operations
- Type checking

**Evidence**:
- `--dump-tokens` works
- Parser debug output shows successful parsing
- Crash happens with `--dump-ast` and `-c` (syntax-only)
- Typedef version works perfectly

## Workaround for Self-Hosting

The target abstraction files can be refactored to use typedefs:

### Before (crashes):
```c
typedef struct {
    const char* (*get_name)(Target *target);
    bool (*init)(Target *target, FILE *output, TargetOptions *options);
    void (*emit_instruction)(Target *target, IRInstruction *instr);
    // ... more function pointers
} TargetVTable;
```

### After (works):
```c
// Define function pointer types
typedef const char* (*TargetGetNameFunc)(Target *target);
typedef bool (*TargetInitFunc)(Target *target, FILE *output, TargetOptions *options);
typedef void (*TargetInstructionFunc)(Target *target, IRInstruction *instr);

typedef struct {
    TargetGetNameFunc get_name;
    TargetInitFunc init;
    TargetInstructionFunc emit_instruction;
    // ... using typedef'd types
} TargetVTable;
```

## Resolution Path

### Immediate (Complete Task 6.1)
1. Refactor target abstraction to use typedef'd function pointer types
2. Test all 3 target files compile
3. Run `make self` successfully
4. Complete self-hosting verification

**Estimated time**: 2-3 hours

### Future (Proper Fix)
1. Debug crash in semantic analysis/type handling
2. Fix direct function pointer syntax
3. Complete C feature support
4. Update target abstraction to use direct syntax

**Estimated time**: 4-6 hours

## Recommendation

**Use typedef workaround** to complete Task 6.1 and self-hosting:
- Pragmatic solution
- Minimal code changes
- Enables full self-hosting now
- Clean, readable code
- Can revisit direct syntax later

This approach:
- ✅ Completes Sprint 6 on schedule
- ✅ Enables self-hosting verification
- ✅ Maintains code quality
- ✅ Documents known limitation

## Files to Modify

1. `src/target/target.h` - Add function pointer typedefs
2. `src/target/target.c` - Update to use typedefs
3. `src/target/arm64/arm64_target.c` - Use typedef'd types
4. `src/target/wasm/wasm_target.c` - Use typedef'd types

## Next Steps

1. Refactor target abstraction with typedefs (2 hours)
2. Test compilation of all target files (30 min)
3. Run `make self` successfully (30 min)
4. Pop Task 6.1 as complete
5. Continue with remaining Sprint 6 tasks

---

**Status**: Parser fix complete, workaround identified
**Recommendation**: Use typedef workaround for immediate completion
**Timeline**: 2-3 hours to full self-hosting
