/**
 * main.c - Entry point for the C compiler
 */

#include "driver.h"
#include "common/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    CompileOptions options = {
        .dump_tokens = false,
        .dump_ast = false,
        .dump_ir = false,
        .dump_asm = false,
        .syntax_only = false,
        .generate_debug = false,
        .optimization_level = 0,
        .output_file = NULL,
        .input_file = NULL
    };
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            show_usage();
            return 0;
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            show_version();
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                options.output_file = argv[++i];
            } else {
                fprintf(stderr, "Error: -o requires a filename\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            options.syntax_only = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            // Just produce assembly (default behavior)
        } else if (strcmp(argv[i], "-g") == 0) {
            options.generate_debug = true;
        } else if (strcmp(argv[i], "-O0") == 0) {
            options.optimization_level = 0;
        } else if (strcmp(argv[i], "-O1") == 0) {
            options.optimization_level = 1;
        } else if (strcmp(argv[i], "-O2") == 0) {
            options.optimization_level = 2;
        } else if (strcmp(argv[i], "-O3") == 0) {
            options.optimization_level = 3;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            options.dump_tokens = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            options.dump_ast = true;
        } else if (strcmp(argv[i], "--dump-ir") == 0) {
            options.dump_ir = true;
        } else if (strcmp(argv[i], "--dump-asm") == 0) {
            options.dump_asm = true;
        } else if (argv[i][0] != '-') {
            options.input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            show_usage();
            return 1;
        }
    }
    
    // Need an input file unless dumping something or compiling to assembly
    if (!options.input_file && !options.dump_tokens && !options.dump_ast && 
        !options.dump_ir && !options.dump_asm) {
        // stdin is OK, compilation will proceed
    }
    
    // Run compilation
    int result = 0;
    
    if (options.input_file) {
        result = compile_file(options.input_file, &options);
    } else {
        // Compile from stdin
        char *source = read_stdin();
        if (!source || strlen(source) == 0) {
            fprintf(stderr, "Error: No input\n");
            return 1;
        }
        
        CompileOptions stdin_options = options;
        stdin_options.input_file = "<stdin>";
        
        // For stdin, we need to create a temp file
        FILE *tmp = fopen("/tmp/cc_stdin.c", "w");
        if (tmp) {
            fprintf(tmp, "%s", source);
            fclose(tmp);
            result = compile_file("/tmp/cc_stdin.c", &stdin_options);
            remove("/tmp/cc_stdin.c");
        }
        
        free(source);
    }
    
    return result;
}
