#ifndef ANALYZER_H
#define ANALYZER_H

#include "../parser/ast.h"
#include "symtab.h"

// Semantic analysis result
typedef struct {
    bool success;
    int error_count;
    int warning_count;
    SymbolTable *symtab;
} AnalyzeResult;

// Functions
AnalyzeResult *analyzer_analyze(ASTNode *ast);
void analyzer_free_result(AnalyzeResult *result);

// Error reporting
void error(const char *fmt, ...);
void warning(const char *fmt, ...);

#endif // ANALYZER_H
