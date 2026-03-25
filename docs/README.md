# Compiler Documentation

See the project documentation at the root level:
- `ARCHITECTURE.md` - Detailed compiler architecture
- `SPRINTS.md` - Development sprint plan
- `TODO.md` - Detailed task list

## WASM Target

This compiler supports WebAssembly (WASM) as a compilation target.

### Usage

```bash
# Compile C to WAT (WebAssembly Text format)
./compiler --target=wasm input.c -o output.wat

# Compile and validate
./scripts/validate-wasm.sh input.c

# List supported targets
./compiler --list-targets
```

### WASM Test Suite

Run the WASM test suite:

```bash
make test-wasm
```

### Example

```c
// hello.c
#include <stdio.h>

int main() {
    printf("Hello, WASM!\n");
    return 0;
}
```

Compile and validate:

```bash
./compiler --target=wasm hello.c -o hello.wat
wat2wasm hello.wat -o hello.wasm
```

### Supported Features

- Integer arithmetic (add, sub, mul, div)
- Bitwise operations (and, or, xor, shl, shr)
- Comparison operations (lt, gt, le, ge, eq, ne)
- Control flow (if/else, while, for)
- Local variables
- Memory operations (load, store)
- Function calls
- String literals

### Requirements

- WABT (WebAssembly Binary Toolkit) for validation
- wasmtime or Node.js for execution

Install WABT:
```bash
# macOS
brew install wabt

# Ubuntu/Debian
sudo apt-get install wabt
```

### Known Limitations

- WASM structured control flow validation requires proper block nesting
- printf variadic arguments need full implementation
- Floating point operations are partially implemented

For more details, see `.project-management/wasm-*.md` documentation files.
