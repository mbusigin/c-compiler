/**
 * analyzer.c - Semantic analyzer
 */

#include "analyzer.h"
#include "../common/util.h"
#include "../common/error.h"
#include <stdlib.h>

static SymbolTable *current_symtab = NULL;
static ASTNode *current_function = NULL;
static int recursion_depth = 0;
static const int MAX_RECURSION = 200;

// Forward declarations
static void analyze_node(ASTNode *node);
static void analyze_expression(ASTNode *node);
static Type *analyze_expression_with_type(ASTNode *node);

// Check if a type is an integer type
static bool is_integer_type(Type *t) {
    if (!t) return false;
    switch (t->kind) {
        case TYPE_CHAR:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_LONGLONG:
            return true;
        default:
            return false;
    }
}

// Check if two types are compatible
static bool types_compatible(Type *t1, Type *t2) {
    if (!t1 || !t2) return true;
    
    // Allow implicit conversions between integer types
    if (is_integer_type(t1) && is_integer_type(t2)) {
        return true;
    }
    
    // Allow implicit pointer conversions (void* to any pointer, any pointer to void*)
    if (t1->kind == TYPE_POINTER && t2->kind == TYPE_POINTER) {
        // Allow any pointer to be assigned to void* and vice versa
        if (t1->base && t1->base->kind == TYPE_VOID) return true;
        if (t2->base && t2->base->kind == TYPE_VOID) return true;
        // Otherwise check base types
        if (t1->base && t2->base) {
            return types_compatible(t1->base, t2->base);
        }
        return true;
    }
    
    if (t1->kind != t2->kind) return false;
    
    // Check base types for arrays
    if (t1->kind == TYPE_ARRAY && t1->base && t2->base) {
        return types_compatible(t1->base, t2->base);
    }
    
    return true;
}

// Analyze a declaration
static void analyze_declaration(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VARIABLE_DECL: {
            Symbol *sym = symtab_lookup(current_symtab, node->data.variable.name);
            if (!sym) {
                sym = symtab_add(current_symtab, node->data.variable.name, 
                                SYMBOL_VARIABLE, node->data.variable.var_type);
                if (!sym) {
                    error("[%d:%d] redeclaration of '%s'\n", 
                          node->line, node->column, node->data.variable.name);
                } else {
                    sym->is_defined = true;
                }
            }
            if (node->data.variable.init) {
                analyze_expression(node->data.variable.init);
            }
            break;
        }
        
        case AST_FUNCTION_DECL: {
            Symbol *sym = symtab_lookup(current_symtab, node->data.function.name);
            if (!sym) {
                sym = symtab_add(current_symtab, node->data.function.name,
                                SYMBOL_FUNCTION, node->data.function.func_type);
            }
            
            if (sym && !sym->is_defined && node->data.function.body) {
                sym->is_defined = true;
                symtab_enter_scope(current_symtab);
                
                // Save current function context
                ASTNode *prev_func = current_function;
                current_function = node;
                
                // Add parameters to scope
                for (size_t i = 0; i < list_size(node->data.function.params); i++) {
                    ASTNode *param = list_get(node->data.function.params, i);
                    if (param->type == AST_PARAMETER_DECL) {
                        symtab_add(current_symtab, param->data.parameter.name,
                                  SYMBOL_PARAMETER, param->data.parameter.param_type);
                    }
                }
                
                // Analyze function body
                if (node->data.function.body) {
                    analyze_node(node->data.function.body);
                }
                
                // Restore function context
                current_function = prev_func;
                symtab_exit_scope(current_symtab);
            } else if (sym && sym->is_defined) {
                error("[%d:%d] redefinition of '%s'\n",
                      node->line, node->column, node->data.function.name);
            }
            break;
        }
        
        case AST_PARAMETER_DECL:
            // Parameters are handled in function declaration
            break;
            
        default:
            break;
    }
}

// Analyze a statement
static void analyze_statement(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_COMPOUND_STMT: {
            symtab_enter_scope(current_symtab);
            for (size_t i = 0; i < list_size(node->data.compound.stmts); i++) {
                analyze_node(list_get(node->data.compound.stmts, i));
            }
            symtab_exit_scope(current_symtab);
            break;
        }
        
        case AST_IF_STMT: {
            if (node->data.if_stmt.condition) {
                analyze_expression(node->data.if_stmt.condition);
            }
            if (node->data.if_stmt.then_stmt) {
                analyze_node(node->data.if_stmt.then_stmt);
            }
            if (node->data.if_stmt.else_stmt) {
                analyze_node(node->data.if_stmt.else_stmt);
            }
            break;
        }
        
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT: {
            if (node->data.while_stmt.condition) {
                analyze_expression(node->data.while_stmt.condition);
            }
            if (node->data.while_stmt.body) {
                analyze_node(node->data.while_stmt.body);
            }
            break;
        }
        
        case AST_FOR_STMT: {
            symtab_enter_scope(current_symtab);
            if (node->data.for_stmt.init) {
                analyze_node(node->data.for_stmt.init);
            }
            if (node->data.for_stmt.condition) {
                analyze_expression(node->data.for_stmt.condition);
            }
            if (node->data.for_stmt.increment) {
                analyze_expression(node->data.for_stmt.increment);
            }
            if (node->data.for_stmt.body) {
                analyze_node(node->data.for_stmt.body);
            }
            symtab_exit_scope(current_symtab);
            break;
        }
        
        case AST_RETURN_STMT: {
            if (node->data.return_stmt.expr) {
                analyze_expression(node->data.return_stmt.expr);
            }
            break;
        }
        
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            // TODO: Check that we're inside a loop
            break;
            
        case AST_GOTO_STMT:
            // TODO: Label resolution
            break;
            
        case AST_EXPRESSION_STMT: {
            if (node->data.expr_stmt.expr) {
                analyze_expression(node->data.expr_stmt.expr);
            }
            break;
        }
        
        case AST_NULL_STMT:
            break;
            
        default:
            break;
    }
}

// Analyze an expression
static void analyze_expression(ASTNode *node) {
    analyze_expression_with_type(node);
}

// Analyze expression and return its type
static Type *analyze_expression_with_type(ASTNode *node) {
    if (!node) return NULL;
    
    switch (node->type) {
        case AST_IDENTIFIER_EXPR: {
            Symbol *sym = symtab_lookup(current_symtab, node->data.identifier.name);
            if (!sym) {
                error("[%d:%d] undeclared identifier '%s'\n",
                      node->line, node->column, node->data.identifier.name);
                return NULL;
            }
            sym->is_used = true;
            // Store type info on node
            node->type_info = sym->type;
            return sym->type;
        }
        
        case AST_INTEGER_LITERAL_EXPR:
            return NULL;  // int type
            
        case AST_FLOAT_LITERAL_EXPR:
            return NULL;  // float type
            
        case AST_STRING_LITERAL_EXPR:
            return NULL;  // char* type
            
        case AST_BINARY_EXPR: {
            Type *left = analyze_expression_with_type(node->data.binary.left);
            Type *right = analyze_expression_with_type(node->data.binary.right);
            
            // Check type compatibility for comparison and equality
            if (node->data.binary.op >= OP_LT && node->data.binary.op <= OP_NE) {
                if (left && right && !types_compatible(left, right)) {
                    warning("[%d:%d] comparison between incompatible types\n",
                           node->line, node->column);
                }
            }
            return NULL;
        }
        
        case AST_UNARY_EXPR:
            if (node->data.unary.operand) {
                return analyze_expression_with_type(node->data.unary.operand);
            }
            return NULL;
            
        case AST_ASSIGNMENT_EXPR: {
            Type *left = analyze_expression_with_type(node->data.assignment.left);
            Type *right = analyze_expression_with_type(node->data.assignment.right);
            
            if (left && right && !types_compatible(left, right)) {
                const char *left_name = type_kind_name(left->kind);
                const char *right_name = type_kind_name(right->kind);
                error("[%d:%d] assignment of incompatible types (%s vs %s)\n",
                      node->line, node->column, left_name, right_name);
            }
            return left;
        }
        
        case AST_CALL_EXPR: {
            // Look up the function
            Symbol *sym = NULL;
            if (node->data.call.callee && 
                node->data.call.callee->type == AST_IDENTIFIER_EXPR) {
                const char *func_name = node->data.call.callee->data.identifier.name;
                sym = symtab_lookup(current_symtab, func_name);
                if (!sym) {
                    // Auto-declare the function as a convenience
                    sym = symtab_add(current_symtab, func_name, SYMBOL_FUNCTION, NULL);
                }
                if (sym && sym->kind != SYMBOL_FUNCTION) {
                    error("[%d:%d] '%s' is not a function\n",
                          node->line, node->column, func_name);
                }
            }
            
            // Analyze arguments
            for (size_t i = 0; i < list_size(node->data.call.args); i++) {
                analyze_expression(list_get(node->data.call.args, i));
            }
            
            return sym ? sym->type : NULL;
        }
        
        case AST_ARRAY_SUBSCRIPT_EXPR: {
            analyze_expression_with_type(node->data.subscript.array);
            analyze_expression_with_type(node->data.subscript.index);
            // Result is the element type
            if (node->data.subscript.array) {
                Type *arr_type = node->data.subscript.array->type_info;
                if (arr_type && arr_type->base) {
                    return arr_type->base;
                }
            }
            return NULL;
        }
        
        case AST_MEMBER_ACCESS_EXPR:
        case AST_POINTER_MEMBER_ACCESS_EXPR: {
            // Struct member access
            ASTNode *base = node->data.member.expr;
            const char *member_name = node->data.member.member;
            
            if (!base || !member_name) return NULL;
            
            // Analyze base expression to get its type
            Type *base_type = analyze_expression_with_type(base);
            
            // For arrow (->), we need to dereference pointer type
            if (node->type == AST_POINTER_MEMBER_ACCESS_EXPR) {
                if (base_type && base_type->kind == TYPE_POINTER) {
                    base_type = base_type->base;
                }
            }
            
            if (!base_type) return NULL;
            if (base_type->kind != TYPE_STRUCT && base_type->kind != TYPE_UNION) return NULL;
            
            // Find the member
            StructMember *member = type_find_member(base_type, member_name);
            if (!member) {
                error("[%d:%d] struct has no member named '%s'\n",
                      node->line, node->column, member_name);
                return NULL;
            }
            
            // Store the type info on the node for IR lowering
            node->type_info = member->type;
            
            return member->type;
        }
            
        case AST_CAST_EXPR:
            if (node->data.cast.cast_type) {
                // Just check the expression, ignore the cast type
            }
            if (node->data.cast.operand) {
                return analyze_expression_with_type(node->data.cast.operand);
            }
            return node->data.cast.cast_type;
            
        case AST_CONDITIONAL_EXPR: {
            if (node->data.conditional.condition) {
                analyze_expression(node->data.conditional.condition);
            }
            Type *then_type = NULL, *else_type = NULL;
            if (node->data.conditional.then_expr) {
                then_type = analyze_expression_with_type(node->data.conditional.then_expr);
            }
            if (node->data.conditional.else_expr) {
                else_type = analyze_expression_with_type(node->data.conditional.else_expr);
            }
            // Return the more "promoted" type
            return then_type ? then_type : else_type;
        }
        
        case AST_COMMA_EXPR: {
            Type *result = NULL;
            if (node->data.comma.left) {
                result = analyze_expression_with_type(node->data.comma.left);
            }
            if (node->data.comma.right) {
                result = analyze_expression_with_type(node->data.comma.right);
            }
            return result;
        }
        
        case AST_SIZEOF_EXPR: {
            // sizeof returns size_t (unsigned long)
            if (node->data.sizeof_expr.sizeof_expr) {
                analyze_expression(node->data.sizeof_expr.sizeof_expr);
            }
            // Return unsigned long type
            Type *result = type_create(TYPE_LONG);
            result->is_unsigned = true;
            return result;
        }
        
        default:
            return NULL;
    }
}

// Analyze a node
static void analyze_node(ASTNode *node) {
    if (!node) return;
    if (++recursion_depth > MAX_RECURSION) {
        recursion_depth--;
        return;
    }
    
    switch (node->type) {
        case AST_FUNCTION_DECL:
        case AST_VARIABLE_DECL:
        case AST_PARAMETER_DECL:
        case AST_STRUCT_DECL:
        case AST_UNION_DECL:
        case AST_ENUM_DECL:
        case AST_TYPEDEF_DECL:
            analyze_declaration(node);
            break;
            
        case AST_COMPOUND_STMT:
        case AST_IF_STMT:
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
        case AST_FOR_STMT:
        case AST_SWITCH_STMT:
        case AST_RETURN_STMT:
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
        case AST_GOTO_STMT:
        case AST_EXPRESSION_STMT:
        case AST_NULL_STMT:
            analyze_statement(node);
            break;
            
        default:
            analyze_expression(node);
            break;
    }
    recursion_depth--;
}

// Analyze a translation unit (list of declarations)
static void analyze_translation_unit(ASTNode *unit) {
    if (!unit || unit->type != AST_TRANSLATION_UNIT) return;
    
    for (size_t i = 0; i < list_size(unit->data.unit.declarations); i++) {
        analyze_node(list_get(unit->data.unit.declarations, i));
    }
}

AnalyzeResult *analyzer_analyze(ASTNode *ast) {
    AnalyzeResult *result = xmalloc(sizeof(AnalyzeResult));
    result->success = true;
    result->error_count = 0;
    result->warning_count = 0;
    result->symtab = symtab_create();
    
    error_count = 0;
    warning_count = 0;
    current_symtab = result->symtab;
    current_function = NULL;
    
    // Analyze the AST
    if (ast) {
        if (ast->type == AST_TRANSLATION_UNIT) {
            analyze_translation_unit(ast);
        } else {
            analyze_node(ast);
        }
    }
    
    result->error_count = error_count;
    result->warning_count = warning_count;
    result->success = (error_count == 0);
    
    return result;
}

void analyzer_free_result(AnalyzeResult *result) {
    if (!result) return;
    if (result->symtab) {
        symtab_destroy(result->symtab);
    }
    free(result);
}
