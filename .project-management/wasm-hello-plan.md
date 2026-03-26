# Hello World Implementation Plan for WASM

## Goal

Make the compiler produce a working "Hello, World!" program in WebAssembly that can run in a browser or WASM runtime.

## Current Status

**Test Program**: `tests/hello.c`
```c
#include <stdio.h>

int main(void) {
    printf("Hello, World!\n");
    return 0;
}
```

**Current Issues**:
1. **Missing string literal support** - No data section for "Hello, World!\n"
2. **Wrong printf signature** - Expects 5 arguments but only 1 provided
3. **No import implementation** - `printf` import exists but no host implementation
4. **Return value issues** - Function signatures don't match

## Required Components

### 1. String Literal Support
- Data section for storing string constants
- `IR_CONST_STRING` implementation for WASM
- Address calculation for string pointers

### 2. Function Call Fixes
- Correct `printf` import signature
- Proper argument passing
- Return value handling

### 3. Host Integration
- JavaScript/Node.js host for testing
- Simple `printf` implementation
- Memory access for string printing

### 4. Validation
- WAT validation with `wat2wasm`
- Runtime execution test
- Output verification

## Implementation Steps

### Phase 1: String Literal Foundation (Week 1)

#### Task 1.1: Data Section Infrastructure
- Add `StringLiteral` struct to track string data
- Implement data section emission in `wasm_codegen.c`
- Test with simple data section output

**Test**: Compiler produces WAT with `(data ...)` section

#### Task 1.2: IR_CONST_STRING Implementation
- Handle `case IR_CONST_STRING:` in `emit_instruction()`
- Generate `i32.const <address>` for string pointers
- Map string indices to data section offsets

**Test**: `IR_CONST_STRING` generates correct address constants

#### Task 1.3: String Processing
- Process escape sequences (`\n`, `\t`, etc.)
- Add null terminator automatically
- Handle empty strings

**Test**: String "Hello\n" appears as "Hello\0a\00" in data section

### Phase 2: Function Call Fixes (Week 2)

#### Task 2.1: Import Signature Correction
- Change `printf` import to `(func (param i32) (result i32))`
- Update import detection logic
- Handle variable arguments properly

**Test**: `wat2wasm` validates WAT without signature errors

#### Task 2.2: Argument Passing
- Ensure only actual arguments are pushed
- Fix argument count tracking in IR
- Handle void return types

**Test**: Function calls compile without stack imbalance errors

#### Task 2.3: Return Value Fixes
- Fix `IR_RET_VOID` emission
- Ensure function signatures match calls
- Handle main function return value

**Test**: `wat2wasm` validates entire module

### Phase 3: Host Integration (Week 3)

#### Task 3.1: JavaScript Host Implementation
- Create `wasm_host.js` with `printf` implementation
- Implement memory access for string printing
- Handle newline and other escape sequences

**Test**: JavaScript host can load and run WASM module

#### Task 3.2: Simple printf Implementation
- Basic string printing to console
- Handle format strings (simplified)
- Return value simulation

**Test**: "Hello, World!" appears in console output

#### Task 3.3: Integration Testing
- End-to-end test: compile → WAT → WASM → run
- Output verification
- Error handling

**Test**: Complete Hello World pipeline works

### Phase 4: Polish and Validation (Week 4)

#### Task 4.1: Error Handling
- Graceful handling of missing imports
- Better error messages for WASM validation failures
- Debug output options

**Test**: Clear error messages for common issues

#### Task 4.2: Performance Optimization
- String deduplication in data section
- Efficient memory layout
- Minimal binary size

**Test**: WASM binary size is reasonable (< 1KB for Hello World)

#### Task 4.3: Documentation
- Update README with WASM usage
- Example Hello World program
- Troubleshooting guide

**Test**: Documentation exists and is accurate

## Technical Details

### Expected WAT Output

```wasm
(module
  ;; Memory declaration
  (memory (export "memory") 1)
  
  ;; Data section with string
  (data (i32.const 0x1000) "Hello, World!\0a\00")
  
  ;; Import declaration
  (import "env" "printf" (func $printf (param i32) (result i32)))
  
  ;; Main function
  (func $main (export "main") (result i32)
    ;; Push string address
    i32.const 0x1000
    
    ;; Call printf
    call $printf
    
    ;; Return 0
    i32.const 0
    return
  )
)
```

### JavaScript Host Implementation

```javascript
const fs = require('fs');
const memory = new WebAssembly.Memory({ initial: 1 });

const imports = {
  env: {
    memory: memory,
    printf: (ptr) => {
      // Read null-terminated string from memory
      const bytes = new Uint8Array(memory.buffer);
      let str = '';
      for (let i = ptr; bytes[i] !== 0; i++) {
        str += String.fromCharCode(bytes[i]);
      }
      console.log(str);
      return str.length;
    }
  }
};

async function runHelloWorld() {
  const wasmBuffer = fs.readFileSync('hello.wasm');
  const module = await WebAssembly.compile(wasmBuffer);
  const instance = await WebAssembly.instantiate(module, imports);
  
  // Call main
  const result = instance.exports.main();
  console.log(`Program returned: ${result}`);
}
```

### IR Generation Changes

#### Current IR for Hello World:
```
main:
  const_int 0      ; Wrong: should be string address
  call printf
  const_int 0
  ret
```

#### Required IR:
```
main:
  const_string 0   ; String index 0 -> "Hello, World!\n"
  call printf      ; With correct argument count
  const_int 0
  ret
```

## Test Plan

### Unit Tests

1. **Data section generation**
   - Input: String literal "Hello"
   - Expected: `(data (i32.const ...) "Hello\00")`

2. **IR_CONST_STRING emission**
   - Input: `IR_CONST_STRING` with index 0
   - Expected: `i32.const <correct-address>`

3. **Import signature**
   - Input: `printf` call with string argument
   - Expected: `(import "env" "printf" (func (param i32) (result i32)))`

4. **Function call**
   - Input: `call $printf` with string address
   - Expected: Valid WAT without stack errors

### Integration Tests

1. **End-to-end compilation**
   ```
   ./compiler --target=wasm tests/hello.c -o hello.wat
   wat2wasm hello.wat -o hello.wasm
   node wasm_host.js
   ```
   Expected: "Hello, World!" printed

2. **WAT validation**
   ```
   wat2wasm hello.wat -v
   ```
   Expected: No errors

3. **Binary execution**
   ```
   wasmtime hello.wasm
   ```
   Expected: Program runs (may need custom host)

### Regression Tests

1. **Existing tests still pass**
   - Arithmetic tests
   - Control flow tests
   - Memory tests

2. **No new compiler crashes**
   - All source files still compile
   - No memory leaks
   - No assertion failures

## Success Criteria

### Minimum Viable Hello World
- [ ] Compiler produces valid WAT for `tests/hello.c`
- [ ] `wat2wasm` validation passes without errors
- [ ] JavaScript host can load and instantiate WASM
- [ ] "Hello, World!" appears in console output
- [ ] Program returns 0

### Complete Implementation
- [ ] All string escape sequences handled
- [ ] Multiple string literals work
- [ ] `printf` with multiple arguments works
- [ ] Memory efficient (binary < 2KB)
- [ ] Good error messages for failures
- [ ] Documentation complete

## Risks and Mitigation

### Risk 1: WASM Validation Failures
- **Risk**: Complex WAT syntax errors
- **Mitigation**: Incremental testing, use `wat2wasm` after each change

### Risk 2: Host Integration Complexity
- **Risk**: JavaScript/WASM memory access issues
- **Mitigation**: Start with simple string reading, add complexity gradually

### Risk 3: Performance Issues
- **Risk**: Large binaries or slow execution
- **Mitigation**: Optimize data section, reuse strings

### Risk 4: Platform Differences
- **Risk**: Works on one platform but not others
- **Mitigation**: Test on Node.js, browsers, wasmtime

## Dependencies

1. **String literal support** (Phase 1)
2. **Function call fixes** (Phase 2)
3. **Import handling** (Phase 2)
4. **JavaScript host** (Phase 3)

## Timeline

### Week 1: String Infrastructure
- Day 1-2: Data section implementation
- Day 3-4: IR_CONST_STRING handling
- Day 5: String processing and testing

### Week 2: Function Calls
- Day 1-2: Import signature fixes
- Day 3-4: Argument passing fixes
- Day 5: Integration testing

### Week 3: Host Integration
- Day 1-2: JavaScript host implementation
- Day 3-4: End-to-end testing
- Day 5: Bug fixes and polish

### Week 4: Validation and Documentation
- Day 1-2: Comprehensive testing
- Day 3-4: Performance optimization
- Day 5: Documentation and release

## Resources Needed

### Development
- WASM specification reference
- WAT syntax guide
- JavaScript WebAssembly API documentation
- Existing compiler codebase

### Testing
- `wat2wasm` tool (from WABT)
- Node.js for JavaScript host
- Browser for web testing
- `wasmtime` for standalone testing

### Debugging
- WAT text output for inspection
- JavaScript debugger
- Console logging in host
- Compiler debug flags

## Conclusion

Hello World is a critical milestone for the WASM backend. It requires string literal support, function call fixes, and host integration. The phased approach ensures each component works before integration. Success will demonstrate the compiler's ability to produce runnable WASM programs and provide a foundation for more complex features.