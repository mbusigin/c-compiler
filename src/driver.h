#ifndef DRIVER_H
#define DRIVER_H

#include <stdbool.h>

// Target architecture
typedef enum {
    TARGET_ARM64,
    TARGET_WASM
} CompileTarget;

// Compilation options
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
    CompileTarget target;
} CompileOptions;

// Compile a source file
int compile_file(const char *filename, CompileOptions *options);

// Show usage
void show_usage(void);

// Show version
void show_version(void);

#endif // DRIVER_H
