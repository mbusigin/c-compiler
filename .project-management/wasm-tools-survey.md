# WASM Runtime Tools and Validation Utilities Survey

## Overview

This document surveys the available tools for WASM development, validation, and runtime execution, with recommendations for our test infrastructure.

## Core Toolchain

### WABT (WebAssembly Binary Toolkit)

**Purpose**: Essential tools for WASM text/binary conversion and validation

**Tools Included**:
- `wat2wasm` - Convert WAT to WASM binary
- `wasm2wat` - Decompile WASM to WAT
- `wasm-validate` - Validate WASM binary
- `wasm-interp` - Interpreter for WASM
- `wasm-objdump` - Disassemble WASM binaries
- `wasm-strip` - Remove sections from WASM
- `wasm-decompile` - High-level decompilation

**Installation**:
```bash
# macOS
brew install wabt

# Ubuntu/Debian
sudo apt-get install wabt

# From source
git clone https://github.com/WebAssembly/wabt
cd wabt && make && sudo make install
```

**Usage Examples**:
```bash
# Convert WAT to WASM
wat2wasm module.wat -o module.wasm

# Validate WASM
wasm-validate module.wasm

# Decompile WASM
wasm2wat module.wasm -o module.wat

# Run with interpreter
wasm-interp module.wasm --run-all
```

**Recommendation**: **Required** - Core toolchain for all WASM development

---

### Wasmtime

**Purpose**: High-performance WASM runtime for testing and execution

**Features**:
- Fast JIT compilation
- WASI support (system interface)
- Debugging capabilities
- Multiple language bindings

**Installation**:
```bash
# Install script (all platforms)
curl https://wasmtime.dev/install.sh -sSf | bash

# macOS
brew install wasmtime

# Ubuntu/Debian
sudo apt-get install wasmtime
```

**Usage Examples**:
```bash
# Run WASM module
wasmtime module.wasm

# Invoke specific function
wasmtime module.wasm --invoke main

# Invoke with arguments
wasmtime module.wasm --invoke add -- 3 5

# With WASI support
wasmtime --dir=. module.wasm

# Debug mode
wasmtime --debug module.wasm
```

**CLI Options**:
```
--invoke <func>    Invoke a specific function
--dir <path>       Grant access to directory (WASI)
--env <VAR=val>    Set environment variable
--preload <mod>    Preload a module
--trace            Enable tracing
```

**Recommendation**: **Required** - Primary runtime for integration tests

---

### Node.js

**Purpose**: Browser-like WASM execution environment

**Features**:
- Web-compatible WASM API
- JavaScript integration
- Widely available

**Installation**:
```bash
# Node.js includes WASM support by default
# Install from https://nodejs.org/
```

**Usage Example**:
```javascript
// run-wasm.js
const fs = require('fs');
const wasm = fs.readFileSync('module.wasm');

async function run() {
    const module = await WebAssembly.compile(wasm);
    const instance = await WebAssembly.instantiate(module, {
        env: {
            memory: new WebAssembly.Memory({ initial: 1 }),
            printf: (fmt) => console.log('printf called with', fmt),
        }
    });
    
    const result = instance.exports.main();
    console.log('Result:', result);
}

run();
```

**Run**:
```bash
node run-wasm.js
```

**Recommendation**: **Optional** - Useful for browser compatibility testing

---

## Validation Tools

### wasm-validate (WABT)

**Purpose**: Validate WASM binary format

**Usage**:
```bash
wasm-validate module.wasm
echo $?  # 0 = valid, non-zero = invalid
```

**Error Output**:
```
error: type mismatch in i32.add, expected [i32, i32] but got [i32]
```

**Recommendation**: **Required** - Essential for CI/CD validation

---

### wasmparser (Rust)

**Purpose**: Low-level WASM parsing and validation

**Installation**:
```bash
cargo install wasmparser
```

**Usage**:
```bash
wasmparser module.wasm
```

**Recommendation**: **Optional** - For advanced validation needs

---

## Development Tools

### wasm2c (WABT)

**Purpose**: Convert WASM to C code

**Usage**:
```bash
wasm2c module.wasm -o module.c
wasm2c --header module.wasm -o module.h
```

**Use Case**: Embedding WASM in C programs

**Recommendation**: **Optional** - Not needed for our use case

---

### wasm-opcodecnt (WABT)

**Purpose**: Count opcodes in WASM binary

**Usage**:
```bash
wasm-opcodecnt module.wasm
```

**Use Case**: Performance analysis, code size optimization

**Recommendation**: **Optional** - Useful for optimization work

---

### wasm-metrics

**Purpose**: Analyze WASM binary metrics

**Installation**:
```bash
npm install -g wasm-metrics
```

**Usage**:
```bash
wasm-metrics module.wasm
```

**Output**:
```
File: module.wasm
Size: 1024 bytes
Functions: 5
Types: 3
Imports: 2
Exports: 1
```

**Recommendation**: **Optional** - Useful for size optimization

---

## Debugging Tools

### wasm-debug

**Purpose**: Debug WASM with source maps

**Installation**:
```bash
npm install -g @wasmer/wasm-debug
```

**Recommendation**: **Future** - When source map support is added

---

### Chrome DevTools

**Purpose**: Browser-based WASM debugging

**Usage**:
1. Open Chrome DevTools
2. Load HTML with WASM module
3. Debug in Sources panel

**Recommendation**: **Optional** - For browser testing

---

## Build Tools

### Binaryen

**Purpose**: WASM compiler toolchain infrastructure

**Tools**:
- `wasm-opt` - Optimize WASM binaries
- `wasm-as` - Assemble WASM
- `wasm-dis` - Disassemble WASM
- `wasm-ctor-eval` - Evaluate constructors

**Installation**:
```bash
# macOS
brew install binaryen

# Ubuntu/Debian
sudo apt-get install binaryen
```

**Usage**:
```bash
# Optimize WASM
wasm-opt module.wasm -o module.opt.wasm -O3

# Disassemble
wasm-dis module.wasm
```

**Recommendation**: **Recommended** - For optimization passes

---

### Emscripten

**Purpose**: C/C++ to WASM compiler

**Note**: We're building our own compiler, but Emscripten is useful for:
- Reference implementation
- Testing compatibility
- Standard library implementation

**Installation**:
```bash
git clone https://github.com/emscripten-core/emsdk
cd emsdk && ./emsdk install latest && ./emsdk activate latest
```

**Recommendation**: **Reference** - For compatibility testing

---

## Recommended Tool Set

### Required (Must Have)

| Tool | Purpose | Installation |
|------|---------|--------------|
| wat2wasm | WAT → WASM | wabt |
| wasm2wat | WASM → WAT | wabt |
| wasm-validate | Validation | wabt |
| wasmtime | Runtime | wasmtime.dev |

### Recommended (Should Have)

| Tool | Purpose | Installation |
|------|---------|--------------|
| wasm-opt | Optimization | binaryen |
| wasm-interp | Reference interpreter | wabt |
| Node.js | JS runtime | nodejs.org |

### Optional (Nice to Have)

| Tool | Purpose | Installation |
|------|---------|--------------|
| wasm-metrics | Size analysis | npm |
| wasm-debug | Debugging | npm |
| Chrome DevTools | Browser debug | Chrome |

---

## Tool Version Requirements

```bash
# Minimum versions for compatibility
wat2wasm --version  # >= 1.0.0
wasmtime --version  # >= 1.0.0
node --version      # >= 14.0.0
```

---

## CI/CD Tool Installation

### GitHub Actions

```yaml
- name: Install WABT
  run: sudo apt-get install wabt

- name: Install Wasmtime
  run: |
    curl https://wasmtime.dev/install.sh -sSf | bash
    echo "$HOME/.wasmtime/bin" >> $GITHUB_PATH

- name: Verify tools
  run: |
    wat2wasm --version
    wasmtime --version
    wasm-validate --version
```

### Docker

```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    wabt \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN curl https://wasmtime.dev/install.sh -sSf | bash
ENV PATH="/root/.wasmtime/bin:${PATH}"

WORKDIR /app
```

---

## Tool Comparison

### Runtime Comparison

| Feature | Wasmtime | Node.js | wasm-interp |
|---------|----------|---------|-------------|
| Speed | Fast (JIT) | Fast (JIT) | Slow (interp) |
| WASI | Yes | Limited | No |
| Debugging | Good | Excellent | Basic |
| CLI | Excellent | Good | Basic |

### Validation Comparison

| Feature | wasm-validate | wasmparser |
|---------|---------------|------------|
| Speed | Fast | Fast |
| Error Detail | Good | Excellent |
| Integration | Easy | Moderate |

---

## Testing Strategy with Tools

### Development Workflow

```bash
# 1. Compile C to WAT
./compiler --target=wasm source.c -o output.wat

# 2. Validate WAT syntax
wat2wasm output.wat -o /dev/null

# 3. Convert to WASM
wat2wasm output.wat -o output.wasm

# 4. Validate WASM binary
wasm-validate output.wasm

# 5. Run with wasmtime
wasmtime output.wasm --invoke main

# 6. Compare output
```

### CI/CD Pipeline

```bash
# In CI:
make test-wasm           # Run test suite
make test-wasm-validate  # Validate all binaries
```

---

## References

- [WABT GitHub](https://github.com/WebAssembly/wabt)
- [Wasmtime](https://wasmtime.dev/)
- [Binaryen](https://github.com/WebAssembly/binaryen)
- [WebAssembly.org Tools](https://webassembly.org/docs/tools/)
