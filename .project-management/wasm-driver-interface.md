# WASM Driver Interface Specification

## Overview

This document specifies the driver interface extensions needed to support multi-target compilation, with a focus on WASM target selection and output handling.

## Command Line Interface

### New Options

```
--target=<arch>    Specify target architecture (default: arm64)
                   Supported: arm64, wasm

--wasm-format=<fmt>  WASM output format (default: wat)
                     Supported: wat, wasm

--wasm-imports=<file>  Import definitions file for WASM

--list-targets     List supported target architectures
```

### Updated Usage

```
Usage: c-compiler [options] <input>

Options:
  -o <file>              Output to file
  -S                     Compile to assembly
  -c                     Compile only, do not link
  -E                     Preprocess only
  -O<0-3>                Set optimization level
  --target=<arch>        Target architecture (arm64, wasm)
  --wasm-format=<fmt>    WASM output format (wat, wasm)
  --list-targets         List supported targets
  --dump-tokens          Print tokens
  --dump-ast             Print AST
  --dump-ir              Print IR
  --dump-asm             Print assembly/WAT
  -v, --version          Print version
  -h, --help             Print this help
```

## Data Structure Changes

### CompileOptions Extension

```c
// src/driver.h

typedef enum {
    TARGET_ARM64,
    TARGET_WASM,
    TARGET_COUNT
} CompileTarget;

typedef enum {
    WASM_FORMAT_WAT,
    WASM_FORMAT_WASM
} WasmFormat;

typedef struct {
    bool dump_tokens;
    bool dump_ast;
    bool dump_ir;
    bool dump_asm;
    bool syntax_only;
    bool preprocess_only;
    bool generate_debug;
    int optimization_level;
    const char *output_file;
    const char *input_file;
    
    // New fields for multi-target support
    CompileTarget target;
    WasmFormat wasm_format;
    const char *wasm_imports_file;
} CompileOptions;
```

### Default Options

```c
// src/driver.c

CompileOptions default_options(void) {
    return (CompileOptions) {
        .dump_tokens = false,
        .dump_ast = false,
        .dump_ir = false,
        .dump_asm = false,
        .syntax_only = false,
        .preprocess_only = false,
        .generate_debug = false,
        .optimization_level = 0,
        .output_file = NULL,
        .input_file = NULL,
        .target = TARGET_ARM64,      // Default to ARM64
        .wasm_format = WASM_FORMAT_WAT,
        .wasm_imports_file = NULL
    };
}
```

## Argument Parsing

### Target Option Parser

```c
// src/driver.c

static CompileTarget parse_target(const char *target_str) {
    if (strcmp(target_str, "arm64") == 0 || 
        strcmp(target_str, "aarch64") == 0) {
        return TARGET_ARM64;
    } else if (strcmp(target_str, "wasm") == 0 ||
               strcmp(target_str, "webassembly") == 0) {
        return TARGET_WASM;
    } else {
        error("Unknown target: %s\n", target_str);
        error("Supported targets: arm64, wasm\n");
        return TARGET_ARM64;  // Default
    }
}

static WasmFormat parse_wasm_format(const char *fmt_str) {
    if (strcmp(fmt_str, "wat") == 0) {
        return WASM_FORMAT_WAT;
    } else if (strcmp(fmt_str, "wasm") == 0) {
        return WASM_FORMAT_WASM;
    } else {
        error("Unknown WASM format: %s\n", fmt_str);
        return WASM_FORMAT_WAT;
    }
}
```

### Main Argument Loop Extension

```c
// src/main.c

for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--target") == 0) {
        if (i + 1 < argc) {
            options.target = parse_target(argv[++i]);
        } else {
            error("--target requires an argument\n");
            return 1;
        }
    } else if (strncmp(argv[i], "--target=", 9) == 0) {
        options.target = parse_target(argv[i] + 9);
    } else if (strcmp(argv[i], "--wasm-format") == 0) {
        if (i + 1 < argc) {
            options.wasm_format = parse_wasm_format(argv[++i]);
        } else {
            error("--wasm-format requires an argument\n");
            return 1;
        }
    } else if (strncmp(argv[i], "--wasm-format=", 14) == 0) {
        options.wasm_format = parse_wasm_format(argv[i] + 14);
    } else if (strcmp(argv[i], "--list-targets") == 0) {
        printf("Supported targets:\n");
        printf("  arm64    - ARM 64-bit (Apple Silicon)\n");
        printf("  wasm     - WebAssembly\n");
        return 0;
    }
    // ... existing options ...
}
```

## Output Routing

### Target-Specific Code Generation

```c
// src/driver.c

int compile_file(const char *filename, CompileOptions *options) {
    // ... existing preprocessing, lexing, parsing, analysis ...
    
    // IR generation
    IRModule *module = lowerer_lower(ast, result->symtab);
    
    // Target-specific code generation
    int codegen_result = 0;
    
    switch (options->target) {
        case TARGET_ARM64:
            codegen_result = generate_arm64(module, options);
            break;
        case TARGET_WASM:
            codegen_result = generate_wasm(module, options);
            break;
        default:
            error("Unsupported target\n");
            codegen_result = 1;
    }
    
    // ... cleanup ...
    return codegen_result;
}
```

### WASM Output Handler

```c
// src/driver.c

static int generate_wasm(IRModule *module, CompileOptions *options) {
    if (options->output_file) {
        // Check for .wat or .wasm extension
        size_t len = strlen(options->output_file);
        bool is_wat = (len >= 4 && strcmp(options->output_file + len - 4, ".wat") == 0);
        bool is_wasm = (len >= 5 && strcmp(options->output_file + len - 5, ".wasm") == 0);
        
        if (is_wat || options->wasm_format == WASM_FORMAT_WAT) {
            // Generate WAT text
            FILE *out = fopen(options->output_file, "w");
            if (!out) {
                error("Could not open output file: %s\n", options->output_file);
                return 1;
            }
            wasm_codegen_generate(module, out);
            fclose(out);
        } else if (is_wasm || options->wasm_format == WASM_FORMAT_WASM) {
            // Generate WAT to temp file, then convert
            char *wat_file = xstrdup(options->output_file);
            wat_file[len] = 'a';
            wat_file[len+1] = 't';
            wat_file[len+2] = '\0';
            
            FILE *out = fopen(wat_file, "w");
            wasm_codegen_generate(module, out);
            fclose(out);
            
            // Convert to binary using wat2wasm
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "wat2wasm '%s' -o '%s'", wat_file, options->output_file);
            if (system(cmd) != 0) {
                error("wat2wasm conversion failed\n");
                remove(wat_file);
                return 1;
            }
            remove(wat_file);  // Clean up temp file
            free(wat_file);
        }
    } else if (options->dump_asm) {
        // Default: output WAT to stdout
        wasm_codegen_generate(module, stdout);
    }
    
    return 0;
}
```

### ARM64 Output Handler (Existing)

```c
// src/driver.c

static int generate_arm64(IRModule *module, CompileOptions *options) {
    // ... existing ARM64 codegen ...
    codegen_generate(module, out);
    // ...
}
```

## Help and Version Updates

### Updated Help Message

```c
// src/driver.c

void show_usage(void) {
    printf("Usage: c-compiler [options] <input>\n");
    printf("\nCompilation Options:\n");
    printf("  -o <file>              Output to file\n");
    printf("  -S                     Compile to assembly\n");
    printf("  -c                     Compile only, do not link\n");
    printf("  -E                     Preprocess only\n");
    printf("  -O<0-3>                Set optimization level\n");
    printf("\nTarget Options:\n");
    printf("  --target=<arch>        Target architecture (arm64, wasm)\n");
    printf("  --wasm-format=<fmt>    WASM output format (wat, wasm)\n");
    printf("  --list-targets         List supported targets\n");
    printf("\nDebug Options:\n");
    printf("  --dump-tokens          Print tokens\n");
    printf("  --dump-ast             Print AST\n");
    printf("  --dump-ir              Print IR\n");
    printf("  --dump-asm             Print assembly/WAT\n");
    printf("\nOther:\n");
    printf("  -v, --version          Print version\n");
    printf("  -h, --help             Print this help\n");
}
```

### Updated Version Output

```c
// src/driver.c

void show_version(void) {
    printf("c-compiler version %s\n", VERSION);
    printf("Targets: arm64, wasm\n");
}
```

## Error Handling

### Target-Specific Errors

```c
typedef enum {
    ERR_TARGET_UNSUPPORTED,
    ERR_WASM_CONVERSION_FAILED,
    ERR_WASM_INVALID_FORMAT,
    ERR_WASM_IMPORTS_MISSING
} WasmError;

static void wasm_error(WasmError err, const char *detail) {
    switch (err) {
        case ERR_TARGET_UNSUPPORTED:
            error("Target not supported: %s\n", detail);
            break;
        case ERR_WASM_CONVERSION_FAILED:
            error("WASM conversion failed: %s\n", detail);
            break;
        case ERR_WASM_INVALID_FORMAT:
            error("Invalid WASM format: %s\n", detail);
            break;
        case ERR_WASM_IMPORTS_MISSING:
            error("Required WASM imports not found: %s\n", detail);
            break;
    }
}
```

## Example Usage

### Compile to WAT

```bash
./compiler --target=wasm hello.c -o hello.wat
```

### Compile to WASM Binary

```bash
./compiler --target=wasm --wasm-format=wasm hello.c -o hello.wasm
```

### Quick WASM Output

```bash
./compiler --target=wasm hello.c -o hello.wat
wat2wasm hello.wat -o hello.wasm
```

### Run with Wasmtime

```bash
wasmtime hello.wasm --invoke main
```

### List Targets

```bash
./compiler --list-targets
# Output:
# Supported targets:
#   arm64    - ARM 64-bit (Apple Silicon)
#   wasm     - WebAssembly
```

## Backward Compatibility

### Default Behavior

- Default target remains `arm64` for backward compatibility
- Existing command lines continue to work unchanged
- No breaking changes to existing options

### Migration Path

Users wanting WASM output:
1. Add `--target=wasm` to existing commands
2. Change output extension to `.wat` or `.wasm`
3. Optionally specify `--wasm-format=wasm` for binary output

## Testing the Interface

### CLI Tests

```bash
# Test target listing
./compiler --list-targets | grep -q "wasm"

# Test WASM compilation
./compiler --target=wasm tests/hello.c -o /tmp/test.wat
test -f /tmp/test.wat

# Test default target (should be arm64)
./compiler tests/hello.c -o /tmp/test.s
file /tmp/test.s | grep -q "ASCII"  # Assembly text
```
