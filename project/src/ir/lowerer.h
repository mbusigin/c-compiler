#ifndef LOWERER_H
#define LOWERER_H

#include "ir.h"
#include "../parser/ast.h"
#include "../sema/symtab.h"

IRModule *lowerer_lower(ASTNode *ast, SymbolTable *symtab);
void lowerer_free_module(IRModule *module);

#endif // LOWERER_H
