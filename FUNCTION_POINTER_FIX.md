# Function Pointer Parser Fix - Technical Details

## Root Cause

**File**: `src/parser/parser.c`
**Lines**: 327-377 (struct member parsing)

**Problem**: Struct member parsing expects simple declarators:
```c
Type *member_type = type_copy(member_base_type);
while (check_p(p, TOKEN_STAR)) {
    advance_p(p);
    Type *ptr = type_pointer(member_type);
    member_type = ptr;
}

if (check_p(p, TOKEN_IDENTIFIER)) {
    char *member_name = token_name(p);
    // ... handle array
}
```

This works for: `int x;`, `int *ptr;`, `int arr[10];`

**But fails for**: `int (*f)(int);` (function pointer)

The function pointer syntax is:
```
int (*f)(int)
    ^^^^^ - LPAREN STAR IDENTIFIER RPAREN LPAREN params RPAREN
```

The parser sees LPAREN and doesn't know it's a function pointer.

## The Fix

Add function pointer detection BEFORE checking for identifier in struct member parsing.

### Code Location

File: `src/parser/parser.c`
Function: parse_type() (struct body parsing section)
Lines: ~327-377

### Proposed Fix

```c
// After parsing member_base_type and handling stars, add:

// Check for function pointer declarator: (*name)(params)
if (check_p(p, TOKEN_LPAREN)) {
    size_t saved_pos = p->lexer->position;
    // ... similar to existing function pointer parsing code
    // ... (lines 1071-1140 can be adapted)
    
    if (/* successfully parsed function pointer */) {
        // Add member with function pointer type
        type_add_member(t, member_name, func_ptr_type);
        continue;
    } else {
        // Restore position and try regular identifier
        // ...
    }
}
```

### Implementation Steps

1. **Extract function pointer parsing** into a helper function
2. **Call from struct member parsing** when LPAREN encountered
3. **Test with target abstraction files**
4. **Verify no regression** in existing tests

### Time Estimate

- Extract helper function: 1 hour
- Integrate into struct parsing: 1 hour
- Testing: 1 hour
- **Total**: 3 hours

## Workaround Options

### Option 1: Use Typedef

Define function pointer types outside struct:

```c
typedef int (*IntFunc)(int);

typedef struct {
    IntFunc f;  // Works because it's a typedef
} S;
```

**Pros**: Might compile now
**Cons**: Requires refactoring target abstraction

### Option 2: Refactor Target Abstraction

Replace function pointer table with switch statements:

```c
typedef enum {
    TARGET_ARM64,
    TARGET_WASM
} TargetKind;

typedef struct {
    TargetKind kind;
    // ... use switch instead of function pointers
} TargetVTable;
```

**Pros**: Avoids function pointers
**Cons**: Major refactoring, loses clean abstraction

## Recommendation

**Implement the parser fix** (Option 1 above)

**Why**:
1. It's the right fix - complete C support
2. Function pointers are essential for real code
3. Only 3 hours of work
4. Enables full self-hosting

## Files to Modify

1. `src/parser/parser.c` - Add function pointer detection in struct member parsing
2. Test files - Verify no regression

## Testing Plan

1. **Test simple function pointer**:
   ```c
   int (*f)(int);
   ```

2. **Test in struct**:
   ```c
   typedef struct {
       int (*f)(int);
   } S;
   ```

3. **Test target abstraction**:
   ```c
   ./build/compiler_stage0 -I src -I include -o /tmp/target.s src/target/target.c
   ```

4. **Run full test suite**:
   ```bash
   make test
   ```

5. **Attempt self-hosting**:
   ```bash
   make self
   ```

## Success Criteria

- [ ] Function pointers in structs parse correctly
- [ ] All target abstraction files compile
- [ ] All existing tests pass
- [ ] `make self` succeeds (builds stage1)

---

**Status**: Fix identified, ready for implementation
**Estimated Time**: 3 hours
**Impact**: Enables full self-hosting (100% of Task 6.1)
