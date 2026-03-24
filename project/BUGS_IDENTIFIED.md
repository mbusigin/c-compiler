# Compiler Bugs Preventing Self-Hosting (Stage1)

## Executive Summary

The compiler cannot compile itself (stage1) due to **missing support for function pointer types** in various contexts. This is a critical blocker for self-hosting.

---

## Bug #1: Function Pointer Parameters in Function Declarations (CRITICAL)

### Status
**BLOCKER** - Prevents compilation of any file including `<stdlib.h>`

### Description
The parser fails to parse function pointer types when used as function parameters.

### Failing Syntax
```c
// Function with function pointer parameter
void qsort(void *base, size_t nmemb, size_t size, 
           int (*compar)(const void *, const void *));

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size, 
              int (*compar)(const void *, const void *));
```

### Error Messages
```
error: [124:81] expected ')'
error: [124:89] expected ';'
error: [124:103] expected ')' after type in cast
error: [124:105] expected ';'
error: [0:0] undeclared identifier 'compar'
```

### Root Cause
The parser treats `(*compar)` as a dereference expression rather than recognizing it as a function pointer declarator. It then fails to parse the subsequent `(const void *, const void *)` as the function's parameter list.

### Test Case
```c
// File: test_func_ptr_param.c
void myfunc(int (*callback)(int)) {
}

int main() { return 0; }
```

**Result**: Fails with parse errors

### Impact
- All files that include `<stdlib.h>` fail to compile
- Blocks self-hosting completely
- Affects `src/common/util.c`, `src/common/list.c`, `src/common/error.c`, and others

### Fix Required
Update `src/parser/parser.c` to handle function pointer declarators in parameter lists:
1. Recognize `(*identifier)` as a pointer declarator within a parameter
2. Parse the following `(parameter-list)` as the function type
3. Support both named parameters: `int (*compar)(const void *, const void *)`
4. Support abstract declarators: `int (*)(const void *, const void *)`

---

## Bug #2: Standalone Function Pointer Declarations (CRITICAL)

### Status
**BLOCKER** - Related to Bug #1

### Description
The parser cannot parse function pointer declarations outside of function parameters.

### Failing Syntax
```c
// Function pointer variable declaration
int (*func_ptr)(int x);

// Typedef for function pointer
typedef int (*comparator_t)(const void *, const void *);
```

### Test Case
```c
// File: test_func_ptr_decl.c
int (*func)(int x);
int main() { return 0; }
```

**Result**: 
```
error: [1:17] expected ')' after type in cast
error: [1:18] expected ';'
```

### Root Cause
Same as Bug #1 - parser doesn't recognize function pointer declarator syntax

### Impact
- Cannot declare function pointer variables
- Cannot create typedefs for function pointers
- Blocks use of callbacks and function pointer patterns

---

## Bug #3: Struct Member Access (MAJOR)

### Status
**PARTIAL** - Some struct access works, but complex cases fail

### Description
The semantic analyzer incorrectly reports missing struct members in certain contexts.

### Error Pattern
```
error: [0:0] struct has no member named 'data'
```

### Context
This error appeared when compiling `src/parser/parser.c`, suggesting the analyzer doesn't properly track struct member definitions in some scopes.

### Potential Causes
1. Forward declarations not handled properly
2. Struct definitions in different scopes not merged
3. Typedef'd structs not resolving correctly

### Test Case
```c
// Needs investigation - some struct access works, some doesn't
typedef struct {
    int data;
    int size;
} List;

int main() {
    List l;
    l.data = 42;  // May fail in certain contexts
    return l.size;
}
```

**Result**: Works in isolation, but fails in complex files

---

## Bug #4: Parse Error Recovery (MODERATE)

### Status
**DEGRADES UX** - Makes debugging harder

### Description
When the parser encounters an error, it generates cascading errors and doesn't recover gracefully.

### Example
From a single missing feature (function pointer), the parser generates 12+ errors:
```
error: [124:81] expected ')'
error: [124:89] expected ';'
error: [124:103] expected ')' after type in cast
error: [124:105] expected ';'
error: [124:117] expected ';'
error: [124:118] expected ';'
... (6 more errors)
```

### Impact
- Makes it hard to identify the root cause
- Overwhelming error output
- Poor developer experience

### Fix Required
Improve parser synchronization after errors:
1. Better panic mode recovery
2. Synchronize to statement/declaration boundaries
3. Limit cascading errors

---

## Bug #5: Missing Type Information in Error Messages (MODERATE)

### Status
**DEGRADES UX** - Reduces debugging efficiency

### Description
Error messages lack source location context.

### Example
```
error: [0:0] undeclared identifier 'compar'
```

The `[0:0]` location makes it impossible to find where the error occurred.

### Root Cause
Errors detected during semantic analysis or later stages lose source location tracking.

### Fix Required
1. Preserve source locations through all compilation stages
2. Include source line in error output
3. Show context (surrounding lines)

---

## Priority Order

1. **Bug #1 & #2 (Function Pointers)** - MUST FIX for self-hosting
   - Blocks all stdlib includes
   - Estimated effort: 2-3 days
   
2. **Bug #3 (Struct Member Access)** - SHOULD FIX for self-hosting
   - May be needed for complex code
   - Estimated effort: 1-2 days
   
3. **Bug #4 & #5 (Error Handling)** - NICE TO HAVE
   - Improves usability but not blocking
   - Estimated effort: 1 day each

---

## Verification Tests

After fixes, these test cases should compile successfully:

```c
// test_func_pointer_basic.c
int (*callback)(int);
int main() { return 0; }

// test_func_pointer_param.c
void sort(int (*compare)(int, int)) {}
int main() { return 0; }

// test_func_pointer_typedef.c
typedef int (*comparator_t)(const void*, const void*);
int main() { return 0; }

// test_stdlib_include.c
#include <stdlib.h>
int main() { 
    int arr[] = {3, 1, 2};
    // qsort(arr, 3, sizeof(int), compare);  // After qsort is callable
    return 0; 
}
```

---

## Next Steps

1. Implement function pointer declarator parsing in `src/parser/parser.c`
2. Add semantic analysis for function pointer types
3. Add code generation for function pointer calls
4. Test with `<stdlib.h>` includes
5. Run full stage1 build to verify self-hosting capability
