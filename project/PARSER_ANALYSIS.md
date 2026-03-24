# Parser Architecture Analysis

## Current State

The parser is a **recursive descent parser** that handles most C11 syntax but has critical gaps.

## Architecture Overview

### Core Functions

```
parse() 
  └─> parse_translation_unit()
       └─> parse_declaration() [handles functions, variables, typedefs]
            ├─> parse_type() [handles type specifiers]
            ├─> parse_function_definition()
            │    └─> parse_parameter()
            └─> parse_statement()
```

### Key Data Structures

1. **Parser State** (`Parser` struct in `parser.h`)
   - Current and previous token
   - Error flags (had_error, panic_mode)
   - Root AST node

2. **Type System** (`Type` struct)
   - Primitive types (int, char, float, etc.)
   - Derived types (pointer, array, function, struct)
   - Type qualifiers (const, unsigned)

3. **Symbol Registries** (static globals)
   - `typedef_list` - typedef name → type mapping
   - `struct_registry` - struct name → type mapping
   - `enum_constants` - enum name → value mapping

## The Bug: Missing Function Pointer Support

### Location
`parse_parameter()` function (lines 693-727 in parser.c)

### Current Implementation
```c
static ASTNode *parse_parameter(Parser *p) {
    Type *param_type = parse_type(p);
    
    ASTNode *param = ast_create(AST_PARAMETER_DECL);
    param->data.parameter.param_type = param_type;
    
    if (check(p, TOKEN_IDENTIFIER)) {
        param->data.parameter.name = token_name(p);
        advance(p);
    } else {
        param->data.parameter.name = xstrdup("");
    }
    
    // Only handles array parameters becoming pointers
    while (check(p, TOKEN_LBRACKET)) {
        // ... array handling ...
    }
    
    return param;
}
```

### What's Missing

The function does NOT handle:
1. **Parenthesized declarators**: `(*name)`
2. **Function pointer declarators**: `(*name)(params)`
3. **Abstract function pointer declarators**: `(*)(params)`

### How It Should Work

For a parameter like `int (*compar)(const void *, const void *)`:

1. Parse base type: `int`
2. Check for `(` → indicates declarator needs parsing
3. Consume `(`
4. Check for `*` → indicates pointer
5. Check for identifier → `compar`
6. Consume `)`
7. Check for `(` → indicates function type
8. Parse parameter list: `(const void *, const void *)`
9. Create function pointer type:
   ```
   TYPE_FUNCTION
     return_type: int
     params: [const void*, const void*]
     is_pointer: true
   ```

## Fix Strategy

### Option 1: Add Declarator Parsing Function

Create a new function to parse declarators:

```c
typedef struct {
    Type *type;
    char *name;
    bool is_function_pointer;
} Declarator;

static Declarator parse_declarator(Parser *p, Type *base_type) {
    Declarator decl = {0};
    decl.type = base_type;
    
    // Handle pointer suffixes: * * *
    while (check(p, TOKEN_STAR)) {
        advance(p);
        decl.type = type_pointer(decl.type);
    }
    
    // Handle parenthesized declarator: (*name)
    if (check(p, TOKEN_LPAREN)) {
        advance(p);
        
        // Could be: (*name) or (*name)(params)
        if (check(p, TOKEN_STAR)) {
            // Function pointer: (*name)(params)
            advance(p);  // consume *
            
            if (check(p, TOKEN_IDENTIFIER)) {
                decl.name = token_name(p);
                advance(p);
            }
            
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance(p);
            
            // Now parse function params
            if (check(p, TOKEN_LPAREN)) {
                advance(p);
                decl.type = parse_function_type_params(p, decl.type);
                decl.is_function_pointer = true;
            }
        } else {
            // Parenthesized declarator - recurse
            decl = parse_declarator(p, decl.type);
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance(p);
        }
    } else if (check(p, TOKEN_IDENTIFIER)) {
        decl.name = token_name(p);
        advance(p);
    }
    
    // Handle array suffixes: [size]
    while (check(p, TOKEN_LBRACKET)) {
        advance(p);
        // ... parse array size ...
        decl.type = type_array(decl.type, size);
    }
    
    return decl;
}
```

### Option 2: Extend parse_parameter

Modify `parse_parameter` inline:

```c
static ASTNode *parse_parameter(Parser *p) {
    Type *param_type = parse_type(p);
    char *name = NULL;
    
    // NEW: Check for function pointer declarator
    if (check(p, TOKEN_LPAREN)) {
        advance(p);  // consume (
        
        if (check(p, TOKEN_STAR)) {
            // Function pointer: int (*name)(params)
            advance(p);  // consume *
            
            if (check(p, TOKEN_IDENTIFIER)) {
                name = token_name(p);
                advance(p);
            }
            
            expect(p, TOKEN_RPAREN, "expected ')'");
            advance(p);  // consume )
            
            if (check(p, TOKEN_LPAREN)) {
                advance(p);  // consume (
                param_type = parse_function_params(p, param_type);
            }
        } else {
            // Parenthesized declarator - backup and handle differently
            // ... handle regular parenthesized declarator ...
        }
    } else if (check(p, TOKEN_IDENTIFIER)) {
        name = token_name(p);
        advance(p);
    }
    
    // Create parameter node
    ASTNode *param = ast_create(AST_PARAMETER_DECL);
    param->data.parameter.param_type = param_type;
    param->data.parameter.name = name ? name : xstrdup("");
    
    return param;
}
```

### Recommended Approach

**Option 1** is better because:
- Separates concerns (declarator parsing is complex)
- Reusable for variable declarations, not just parameters
- Easier to test in isolation
- Matches the C grammar structure

## Test Suite Expectations

From `tests/unit/test_parser.c`:
- Tests basic declarations and expressions
- Does NOT appear to have function pointer tests (gap in test coverage!)

From `tests/puzzles/`:
- Various C programs that should compile
- Likely include function pointer usage

## Implementation Checklist

- [ ] Create `parse_declarator()` function
- [ ] Handle `(*identifier)` syntax
- [ ] Handle `(*)(params)` abstract declarator
- [ ] Create `TYPE_FUNCTION_POINTER` type or use `TYPE_FUNCTION` with pointer flag
- [ ] Update `parse_parameter()` to use new declarator parser
- [ ] Update `parse_declaration()` to use new declarator parser
- [ ] Add codegen support for function pointer calls
- [ ] Add tests for function pointer parameters
- [ ] Add tests for function pointer variables
- [ ] Add tests for function pointer typedefs

## Estimated Effort

- Parser changes: 1-2 days
- Semantic analysis: 0.5 day
- Codegen: 0.5 day
- Testing: 0.5 day
- **Total: 2-3 days**

## Dependencies

- Type system must support function types
- AST must have `AST_FUNCTION_POINTER_DECL` or reuse `AST_VARIABLE_DECL` with function type
- Code generator must handle indirect function calls through pointers
