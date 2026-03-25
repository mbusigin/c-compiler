# C Compiler Project

A simple C compiler written from scratch.

## Building

```bash
make          # Build the compiler
make debug    # Build debug version
make release  # Build optimized version
make clean    # Clean build artifacts
```

## Testing

```bash
make test           # Run all tests
make test-lexer     # Run lexer tests
make test-parser    # Run parser tests
make test-types     # Run type system tests
make test-symtab    # Run symbol table tests
```

## Usage

```bash
./compiler <source_file.c>           # Compile to assembly
./compiler <source_file.c> -o out    # Specify output file
./compiler --dump-tokens file.c      # Print tokens
./compiler --dump-ast file.c         # Print AST
./compiler --dump-ir file.c          # Print IR
./compiler --dump-asm file.c         # Print assembly
./compiler -O2 file.c                # Optimize
./compiler --version                 # Show version
```

## Architecture

The compiler follows a traditional pipeline:

1. **Lexer** - Tokenizes source code
2. **Parser** - Builds AST from tokens
3. **Semantic Analyzer** - Type checking, scope analysis
4. **IR Generator** - Creates intermediate representation
5. **Optimizer** - Applies optimization passes
6. **Code Generator** - Generates x86-64 assembly

See `docs/ARCHITECTURE.md` for detailed architecture documentation.
