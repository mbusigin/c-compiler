# WASM String Literals and Data Section Implementation Plan

## String Literals Support

### Current Status

The WASM backend currently has **no support** for string literals:

**What's Missing:**
1. **Data section** - No mechanism to store string data in WASM binary
2. **String constant emission** - `IR_CONST_STRING` not implemented in WASM backend
3. **Address calculation** - No way to get pointer to string in linear memory
4. **Escape sequence handling** - String literals with escapes not processed

**Current Behavior:**
- `IR_CONST_STRING` likely generates ARM64 code, not WASM
- String literals passed to `printf` use `i32.const 0` (null pointer)
- No data section in generated WAT files

### Requirements for String Support

#### 1. Data Section Structure
WASM data sections store initialized data in linear memory:
```wasm
(data (i32.const 0) "Hello, World!\00")
```

#### 2. String Literal Representation
- Strings need null termination
- Escape sequences must be processed (`\n`, `\t`, `\\`, etc.)
- Multiple strings need unique addresses

#### 3. Address Calculation
- Each string gets unique offset in data section
- Functions need to reference these addresses
- Pointers are 32-bit offsets in linear memory

#### 4. Integration with IR
- `IR_CONST_STRING` should generate data section entry
- String references need address constants
- Multiple references to same string should reuse storage

### Implementation Plan

#### Phase 1: Data Section Infrastructure

**Task 1.1: Data Section Management**
- Add data section tracking to `WasmContext`
- Track string literals with unique IDs
- Calculate offsets and addresses

**Task 1.2: Data Section Emission**
- Implement `wasm_emit_data_section()` function
- Emit `(data ...)` segments in WAT
- Handle alignment (WASM pages are 64KB)

**Task 1.3: String Literal Collection**
- During IR generation, collect all string literals
- Assign unique IDs and offsets
- Store in `WasmContext` for later emission

#### Phase 2: String Constant Implementation

**Task 2.1: IR_CONST_STRING Handling**
- Implement `case IR_CONST_STRING:` in `emit_instruction()`
- Generate `i32.const <address>` for string pointer
- Map string index to data section offset

**Task 2.2: Escape Sequence Processing**
- Process C escape sequences in string literals
- Convert to actual bytes for data section
- Handle null terminator automatically

**Task 2.3: String Reuse Optimization**
- Detect duplicate string literals
- Reuse same data section entry
- Maintain mapping from string content to offset

#### Phase 3: Integration with Functions

**Task 3.1: Function Argument Passing**
- String pointers as 32-bit integers
- Integrate with `printf` calls
- Handle string concatenation scenarios

**Task 3.2: Global String Support**
- Support for global string variables
- Initialized data in data section
- Address calculation at compile time

**Task 3.3: Character Arrays**
- Support for character array initialization
- `char str[] = "hello";` syntax
- Array bounds calculation

### Technical Design

#### WASM Context Extension

```c
typedef struct StringLiteral {
    const char *content;     // String content (with escapes processed)
    size_t length;           // Length in bytes (including null terminator)
    size_t offset;           // Offset in data section
    int id;                  // Unique identifier
} StringLiteral;

typedef struct WasmContext {
    // ... existing fields ...
    
    // String literal management
    StringLiteral **strings;
    size_t string_count;
    size_t string_capacity;
    size_t data_offset;      // Current offset in data section
    
    // String reuse optimization
    Dict *string_map;        // Map from content to StringLiteral*
} WasmContext;
```

#### Data Section Emission

```c
void wasm_emit_data_section(WasmContext *ctx) {
    if (ctx->string_count == 0) return;
    
    wasm_emit_comment(ctx, "Data section");
    
    size_t current_offset = 0;
    for (size_t i = 0; i < ctx->string_count; i++) {
        StringLiteral *str = ctx->strings[i];
        str->offset = current_offset;
        
        // Emit data segment
        wasm_emit_indented(ctx, 1, "(data (i32.const %zu) \"", str->offset);
        
        // Escape string content for WAT format
        for (size_t j = 0; j < str->length; j++) {
            unsigned char c = str->content[j];
            if (c >= 32 && c <= 126 && c != '"' && c != '\\') {
                wasm_emit(ctx, "%c", c);
            } else if (c == '\n') {
                wasm_emit(ctx, "\\n");
            } else if (c == '\t') {
                wasm_emit(ctx, "\\t");
            } else if (c == '\"') {
                wasm_emit(ctx, "\\\"");
            } else if (c == '\\') {
                wasm_emit(ctx, "\\\\");
            } else {
                wasm_emit(ctx, "\\%02x", c);
            }
        }
        
        wasm_emit(ctx, "\")\n");
        current_offset += str->length;
    }
    
    wasm_emit(ctx, "\n");
}
```

#### IR_CONST_STRING Implementation

```c
case IR_CONST_STRING:
    if (instr->result && instr->result->kind == IR_VALUE_STRING) {
        int string_id = instr->result->string_index;
        
        // Find or create string literal
        StringLiteral *str = wasm_get_string_literal(ctx, string_id);
        if (str) {
            // Emit address constant
            wasm_emit_instr(ctx, "i32.const %zu", str->offset);
            ctx->stack_depth++;
            instr->result->emitted = true;
        } else {
            wasm_emit_comment(ctx, "String literal %d not found", string_id);
        }
    }
    break;
```

### String Processing Pipeline

#### 1. Frontend Processing
```
Source: printf("Hello\n");
↓
Lexer: STRING_LITERAL '"Hello\n"'
↓
Parser: String literal node
↓
IR Generator: IR_CONST_STRING with content "Hello\n"
```

#### 2. WASM Backend Processing
```
IR_CONST_STRING with "Hello\n"
↓
Escape processing: "Hello\x0a\x00"
↓
Data section entry at offset 0
↓
Code generation: i32.const 0
```

#### 3. WAT Output
```wasm
(data (i32.const 0) "Hello\0a\00")

(func $main (result i32)
  i32.const 0    ; Pointer to "Hello\n"
  call $printf
  ...)
```

### Test Plan

#### Test Cases

1. **Basic string literal**:
```c
int main() {
    const char *msg = "Hello";
    return 0;
}
```

2. **String with escapes**:
```c
int main() {
    printf("Line1\nLine2\tTab\\Backslash\"Quote");
    return 0;
}
```

3. **Multiple strings**:
```c
int main() {
    printf("First");
    printf("Second");
    return 0;
}
```

4. **Duplicate strings** (should reuse):
```c
int main() {
    printf("Hello");
    printf("Hello");  // Same string, same address
    return 0;
}
```

5. **Character array**:
```c
int main() {
    char buffer[] = "Initialized";
    return buffer[0];
}
```

6. **Empty string**:
```c
int main() {
    printf("");
    return 0;
}
```

### Implementation Steps

#### Week 1: Foundation
1. Implement `StringLiteral` struct and management
2. Add data section emission to `wasm_codegen_generate()`
3. Basic `IR_CONST_STRING` support

#### Week 2: String Processing
1. Implement escape sequence processing
2. Add null terminator automatically
3. String reuse optimization

#### Week 3: Integration
1. Integrate with `printf` calls
2. Support character arrays
3. Performance optimization

### Success Criteria

1. **All test cases compile** with valid WAT
2. **Strings appear in data section** with correct content
3. **Escape sequences handled** correctly
4. **Duplicate strings reused** (optimization)
5. **`printf("Hello")` works** with JavaScript host

### Challenges and Solutions

#### Challenge 1: WAT String Encoding
- **Problem**: WAT has its own string escape rules
- **Solution**: Implement proper WAT string escaping

#### Challenge 2: Memory Alignment
- **Problem**: WASM has 64KB page alignment
- **Solution**: Start data section at reasonable offset (e.g., 0x1000)

#### Challenge 3: Large Strings
- **Problem**: Strings longer than page boundaries
- **Solution**: Handle across multiple data segments if needed

#### Challenge 4: International Characters
- **Problem**: UTF-8 in source, bytes in WASM
- **Solution**: Treat as raw bytes, preserve UTF-8 encoding

### Dependencies

1. **Function call fixes** - Needed for `printf` integration
2. **Import handling** - `printf` needs to be importable
3. **Memory declaration** - Data section requires memory

### Future Work

1. **String concatenation** - Compile-time concatenation
2. **Wide strings** - `wchar_t` and UTF-16 support
3. **String functions** - `strlen`, `strcpy` runtime support
4. **String pooling** - More advanced deduplication
5. **Read-only data** - Mark data section as read-only

### Integration with Existing Code

#### Modifications Needed:

1. **`wasm_codegen.c`**:
   - Add string literal management
   - Implement data section emission
   - Handle `IR_CONST_STRING`

2. **`wasm_codegen.h`**:
   - Add `StringLiteral` struct definition
   - Add data section functions

3. **IR Generation**:
   - Ensure `IR_CONST_STRING` created for string literals
   - Proper string index assignment

4. **Frontend**:
   - Escape sequence processing in lexer/parser
   - String literal normalization

### Performance Considerations

1. **String deduplication** - Reduces binary size
2. **Data section layout** - Minimize memory fragmentation
3. **Address calculation** - Compile-time constants
4. **Escape processing** - Done once at compile time

### Security Considerations

1. **Null termination** - Ensure all strings are properly terminated
2. **Buffer boundaries** - Don't overflow data section
3. **Injection prevention** - Proper escaping for WAT format

### Conclusion

String literal support is essential for practical WASM programs. The implementation requires data section support, string processing, and integration with the existing IR system. The plan provides a phased approach starting with basic string support and adding optimizations and advanced features incrementally.