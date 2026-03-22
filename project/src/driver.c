/**
 * driver.c - Compiler driver
 * 
 * Orchestrates the compilation pipeline: Preprocess -> Lex -> Parse -> Analyze -> IR -> Optimize -> Codegen
 */

#include "driver.h"
#include "lexer/lexer.h"
#include "lexer/preproc.h"
#include "parser/parser.h"
#include "sema/analyzer.h"
#include "ir/ir.h"
#include "ir/lowerer.h"
#include "optimize/optimizer.h"
#include "backend/codegen.h"
#include "backend/asm.h"
#include "backend/dwarf.h"
#include "common/util.h"
#include "common/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *VERSION = "0.1.0";
const char *COMPILER_NAME = "c-compiler";

int compile_file(const char *filename, CompileOptions *options) {
    // Read source file
    char *source = read_file(filename);
    if (!source) {
        error("Could not read file: %s\n", filename);
        return 1;
    }
    
    // Preprocess
    char *preprocessed = preprocess(source);
    free(source);
    source = preprocessed;
    
    // Lexical analysis
    Lexer *lexer = lexer_create(source);
    
    if (options->dump_tokens) {
        printf("Tokens for %s:\n", filename);
        Token tok = lexer_next_token(lexer);
        while (tok.type != TOKEN_EOF) {
            token_print(&tok, stdout);
            tok = lexer_next_token(lexer);
        }
        token_print(&tok, stdout);
        lexer_destroy(lexer);
        free(source);
        return 0;
    }
    
    // Create parser - it will fetch the first token internally
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parse(parser);
    
    if (options->dump_ast) {
        printf("AST for %s:\n", filename);
        translation_unit_print(ast);
    }
    
    // Semantic analysis
    AnalyzeResult *result = analyzer_analyze(ast);
    
    if (options->syntax_only || result->error_count > 0) {
        analyzer_free_result(result);
        parser_destroy(parser);
        if (error_count == 0 && result->error_count == 0) {
            printf("No errors found.\n");
            return 0;
        }
        return 1;
    }
    
    // IR generation
    IRModule *module = lowerer_lower(ast, result->symtab);
    
    if (options->dump_ir) {
        printf("IR for %s:\n", filename);
        ir_module_print(module);
    }
    
    // Optimization (placeholder)
    // module = optimizer_optimize(module, (OptLevel)options->optimization_level);
    
    // Code generation
    char *asm_file = NULL;
    if (options->output_file) {
        // Determine if we should produce executable, assembly, or object
        bool produce_executable = !options->syntax_only && !options->dump_asm;
        
        // Check if output file ends with .s (assembly request)
        size_t len = strlen(options->output_file);
        bool output_is_assembly = (len >= 2 && strcmp(options->output_file + len - 2, ".s") == 0);
        
        if (produce_executable && !output_is_assembly) {
            // Generate assembly to a temp file, then assemble and link
            asm_file = xstrdup(options->output_file);
            char *dot_s = xmalloc(strlen(options->output_file) + 10);
            strcpy(dot_s, options->output_file);
            strcat(dot_s, ".s");
            
            FILE *asm_out = fopen(dot_s, "w");
            if (asm_out) {
                codegen_generate(module, asm_out);
                fclose(asm_out);
            }
            
            // Assemble to object file using clang (handles macOS conventions)
            char *dot_o = xstrdup(dot_s);
            dot_o[strlen(dot_o) - 1] = 'o';  // Change .s to .o
            
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "clang -c '%s' -o '%s' && clang '%s' -o '%s'", dot_s, dot_o, dot_o, options->output_file);
            int ret = system(cmd);
            
            // Cleanup temp files (keep for debugging)
            // remove(dot_s);
            // remove(dot_o);
            // free(dot_s);
            // free(dot_o);
            
            if (ret != 0) {
                error("Linking failed\n");
                ir_module_destroy(module);
                analyzer_free_result(result);
                parser_destroy(parser);
                free(source);
                free(asm_file);
                return 1;
            }
        } else if (output_is_assembly) {
            // Output assembly text directly
            FILE *out = fopen(options->output_file, "w");
            if (out) {
                codegen_generate(module, out);
                fclose(out);
            } else {
                error("Could not open output file: %s\n", options->output_file);
                ir_module_destroy(module);
                analyzer_free_result(result);
                parser_destroy(parser);
                free(source);
                return 1;
            }
        } else {
            // Output assembly to file (non-.s extension)
            FILE *out = fopen(options->output_file, "w");
            if (out) {
                codegen_generate(module, out);
                fclose(out);
            } else {
                error("Could not open output file: %s\n", options->output_file);
                ir_module_destroy(module);
                analyzer_free_result(result);
                parser_destroy(parser);
                free(source);
                return 1;
            }
        }
    } else if (!options->dump_tokens && !options->dump_ast && !options->dump_ir) {
        // Default: output to stdout
        codegen_generate(module, stdout);
    }
    
    // Cleanup
    analyzer_free_result(result);
    parser_destroy(parser);
    ir_module_destroy(module);
    free(source);
    
    if (error_count > 0) {
        return 1;
    }
    
    return 0;
}

void show_version(void) {
    printf("%s version %s\n", COMPILER_NAME, VERSION);
}

void show_usage(void) {
    printf("Usage: %s [options] <input>\n", "c-compiler");
    printf("Options:\n");
    printf("  -o <file>      Output to file\n");
    printf("  -S             Compile to assembly\n");
    printf("  -c             Compile only, do not link\n");
    printf("  -O<0-3>        Set optimization level\n");
    printf("  --dump-tokens  Print tokens\n");
    printf("  --dump-ast     Print AST\n");
    printf("  --dump-ir      Print IR\n");
    printf("  -v, --version  Print version\n");
    printf("  -h, --help     Print this help\n");
}
