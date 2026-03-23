#include "ast.h"
#include "../common/util.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *ast_node_name(ASTNodeType type) {
    switch (type) {
        case AST_TRANSLATION_UNIT: return "TranslationUnit";
        case AST_FUNCTION_DECL: return "FunctionDecl";
        case AST_VARIABLE_DECL: return "VariableDecl";
        case AST_PARAMETER_DECL: return "ParameterDecl";
        case AST_COMPOUND_STMT: return "CompoundStmt";
        case AST_IF_STMT: return "IfStmt";
        case AST_WHILE_STMT: return "WhileStmt";
        case AST_DO_WHILE_STMT: return "DoWhileStmt";
        case AST_FOR_STMT: return "ForStmt";
        case AST_SWITCH_STMT: return "SwitchStmt";
        case AST_RETURN_STMT: return "ReturnStmt";
        case AST_BREAK_STMT: return "BreakStmt";
        case AST_CONTINUE_STMT: return "ContinueStmt";
        case AST_GOTO_STMT: return "GotoStmt";
        case AST_EXPRESSION_STMT: return "ExpressionStmt";
        case AST_NULL_STMT: return "NullStmt";
        case AST_BINARY_EXPR: return "BinaryExpr";
        case AST_UNARY_EXPR: return "UnaryExpr";
        case AST_CAST_EXPR: return "CastExpr";
        case AST_CONDITIONAL_EXPR: return "ConditionalExpr";
        case AST_CALL_EXPR: return "CallExpr";
        case AST_ARRAY_SUBSCRIPT_EXPR: return "ArraySubscript";
        case AST_MEMBER_ACCESS_EXPR: return "MemberAccess";
        case AST_POINTER_MEMBER_ACCESS_EXPR: return "PointerMemberAccess";
        case AST_IDENTIFIER_EXPR: return "Identifier";
        case AST_INTEGER_LITERAL_EXPR: return "IntegerLiteral";
        case AST_FLOAT_LITERAL_EXPR: return "FloatLiteral";
        case AST_STRING_LITERAL_EXPR: return "StringLiteral";
        case AST_ASSIGNMENT_EXPR: return "Assignment";
        case AST_COMMA_EXPR: return "CommaExpr";
        default: return "Unknown";
    }
}

ASTNode *ast_create(ASTNodeType type) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_TRANSLATION_UNIT:
            if (node->data.unit.declarations) {
                for (size_t i = 0; i < list_size(node->data.unit.declarations); i++)
                    ast_free(list_get(node->data.unit.declarations, i));
                list_destroy(node->data.unit.declarations);
            }
            break;
        case AST_FUNCTION_DECL:
            free(node->data.function.name);
            if (node->data.function.func_type) type_free(node->data.function.func_type);
            if (node->data.function.params) {
                for (size_t i = 0; i < list_size(node->data.function.params); i++)
                    ast_free(list_get(node->data.function.params, i));
                list_destroy(node->data.function.params);
            }
            if (node->data.function.body) ast_free(node->data.function.body);
            break;
        case AST_VARIABLE_DECL:
            free(node->data.variable.name);
            if (node->data.variable.var_type) type_free(node->data.variable.var_type);
            if (node->data.variable.init) ast_free(node->data.variable.init);
            break;
        case AST_PARAMETER_DECL:
            free(node->data.parameter.name);
            if (node->data.parameter.param_type) type_free(node->data.parameter.param_type);
            break;
        case AST_IF_STMT:
            if (node->data.if_stmt.condition) ast_free(node->data.if_stmt.condition);
            if (node->data.if_stmt.then_stmt) ast_free(node->data.if_stmt.then_stmt);
            if (node->data.if_stmt.else_stmt) ast_free(node->data.if_stmt.else_stmt);
            break;
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            if (node->data.while_stmt.condition) ast_free(node->data.while_stmt.condition);
            if (node->data.while_stmt.body) ast_free(node->data.while_stmt.body);
            break;
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) ast_free(node->data.for_stmt.init);
            if (node->data.for_stmt.condition) ast_free(node->data.for_stmt.condition);
            if (node->data.for_stmt.increment) ast_free(node->data.for_stmt.increment);
            if (node->data.for_stmt.body) ast_free(node->data.for_stmt.body);
            break;
        case AST_SWITCH_STMT:
            if (node->data.switch_stmt.expr) ast_free(node->data.switch_stmt.expr);
            if (node->data.switch_stmt.body) ast_free(node->data.switch_stmt.body);
            break;
        case AST_RETURN_STMT:
            if (node->data.return_stmt.expr) ast_free(node->data.return_stmt.expr);
            break;
        case AST_GOTO_STMT:
            free(node->data.goto_stmt.label);
            break;
        case AST_COMPOUND_STMT:
            if (node->data.compound.stmts) {
                for (size_t i = 0; i < list_size(node->data.compound.stmts); i++)
                    ast_free(list_get(node->data.compound.stmts, i));
                list_destroy(node->data.compound.stmts);
            }
            break;
        case AST_EXPRESSION_STMT:
            if (node->data.expr_stmt.expr) ast_free(node->data.expr_stmt.expr);
            break;
        case AST_BINARY_EXPR:
            if (node->data.binary.left) ast_free(node->data.binary.left);
            if (node->data.binary.right) ast_free(node->data.binary.right);
            break;
        case AST_UNARY_EXPR:
            if (node->data.unary.operand) ast_free(node->data.unary.operand);
            break;
        case AST_CAST_EXPR:
            if (node->data.cast.cast_type) type_free(node->data.cast.cast_type);
            if (node->data.cast.operand) ast_free(node->data.cast.operand);
            break;
        case AST_CONDITIONAL_EXPR:
            if (node->data.conditional.condition) ast_free(node->data.conditional.condition);
            if (node->data.conditional.then_expr) ast_free(node->data.conditional.then_expr);
            if (node->data.conditional.else_expr) ast_free(node->data.conditional.else_expr);
            break;
        case AST_ASSIGNMENT_EXPR:
            if (node->data.assignment.left) ast_free(node->data.assignment.left);
            if (node->data.assignment.right) ast_free(node->data.assignment.right);
            break;
        case AST_COMMA_EXPR:
            if (node->data.comma.left) ast_free(node->data.comma.left);
            if (node->data.comma.right) ast_free(node->data.comma.right);
            break;
        case AST_CALL_EXPR:
            if (node->data.call.callee) ast_free(node->data.call.callee);
            if (node->data.call.args) {
                for (size_t i = 0; i < list_size(node->data.call.args); i++)
                    ast_free(list_get(node->data.call.args, i));
                list_destroy(node->data.call.args);
            }
            break;
        case AST_ARRAY_SUBSCRIPT_EXPR:
            if (node->data.subscript.array) ast_free(node->data.subscript.array);
            if (node->data.subscript.index) ast_free(node->data.subscript.index);
            break;
        case AST_MEMBER_ACCESS_EXPR:
        case AST_POINTER_MEMBER_ACCESS_EXPR:
            if (node->data.member.expr) ast_free(node->data.member.expr);
            free(node->data.member.member);
            break;
        case AST_IDENTIFIER_EXPR:
            free(node->data.identifier.name);
            break;
        case AST_STRING_LITERAL_EXPR:
            free(node->data.string_literal.value);
            break;
        default:
            break;
    }
    
    if (node->type_info) type_free(node->type_info);
    free(node);
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static void print_ast_recursive(const ASTNode *node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    printf("%s", ast_node_name(node->type));
    
    switch (node->type) {
        case AST_FUNCTION_DECL:
            printf(" name='%s'", node->data.function.name ? node->data.function.name : "(anon)");
            break;
        case AST_VARIABLE_DECL:
            printf(" name='%s'", node->data.variable.name ? node->data.variable.name : "(anon)");
            break;
        case AST_PARAMETER_DECL:
            printf(" name='%s'", node->data.parameter.name ? node->data.parameter.name : "(anon)");
            break;
        case AST_IDENTIFIER_EXPR:
            printf(" name='%s'", node->data.identifier.name);
            break;
        case AST_INTEGER_LITERAL_EXPR:
            printf(" value=%lld", node->data.int_literal.value);
            break;
        case AST_FLOAT_LITERAL_EXPR:
            printf(" value=%f", node->data.float_literal.value);
            break;
        case AST_STRING_LITERAL_EXPR:
            printf(" value=\"%s\"", node->data.string_literal.value);
            break;
        case AST_BINARY_EXPR:
            printf(" op=%d", node->data.binary.op);
            break;
        case AST_UNARY_EXPR:
            printf(" op=%d", node->data.unary.op);
            break;
        case AST_ASSIGNMENT_EXPR:
            printf(" op=%d", node->data.assignment.op);
            break;
        case AST_GOTO_STMT:
            printf(" label='%s'", node->data.goto_stmt.label);
            break;
        default:
            break;
    }
    printf("\n");
    
    switch (node->type) {
        case AST_FUNCTION_DECL:
            if (node->data.function.params) {
                for (size_t i = 0; i < list_size(node->data.function.params); i++)
                    print_ast_recursive(list_get(node->data.function.params, i), indent + 1);
            }
            if (node->data.function.body)
                print_ast_recursive(node->data.function.body, indent + 1);
            break;
        case AST_IF_STMT:
            print_ast_recursive(node->data.if_stmt.condition, indent + 1);
            print_ast_recursive(node->data.if_stmt.then_stmt, indent + 1);
            if (node->data.if_stmt.else_stmt)
                print_ast_recursive(node->data.if_stmt.else_stmt, indent + 1);
            break;
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            print_ast_recursive(node->data.while_stmt.condition, indent + 1);
            print_ast_recursive(node->data.while_stmt.body, indent + 1);
            break;
        case AST_FOR_STMT:
            print_ast_recursive(node->data.for_stmt.init, indent + 1);
            print_ast_recursive(node->data.for_stmt.condition, indent + 1);
            print_ast_recursive(node->data.for_stmt.increment, indent + 1);
            print_ast_recursive(node->data.for_stmt.body, indent + 1);
            break;
        case AST_COMPOUND_STMT:
            if (node->data.compound.stmts) {
                for (size_t i = 0; i < list_size(node->data.compound.stmts); i++)
                    print_ast_recursive(list_get(node->data.compound.stmts, i), indent + 1);
            }
            break;
        case AST_RETURN_STMT:
        case AST_EXPRESSION_STMT:
            print_ast_recursive(node->data.return_stmt.expr, indent + 1);
            break;
        case AST_BINARY_EXPR:
        case AST_ASSIGNMENT_EXPR:
        case AST_COMMA_EXPR:
            print_ast_recursive(node->data.binary.left, indent + 1);
            print_ast_recursive(node->data.binary.right, indent + 1);
            break;
        case AST_UNARY_EXPR:
            print_ast_recursive(node->data.unary.operand, indent + 1);
            break;
        case AST_CALL_EXPR:
            print_ast_recursive(node->data.call.callee, indent + 1);
            if (node->data.call.args) {
                for (size_t i = 0; i < list_size(node->data.call.args); i++)
                    print_ast_recursive(list_get(node->data.call.args, i), indent + 1);
            }
            break;
        case AST_ARRAY_SUBSCRIPT_EXPR:
            print_ast_recursive(node->data.subscript.array, indent + 1);
            print_ast_recursive(node->data.subscript.index, indent + 1);
            break;
        case AST_MEMBER_ACCESS_EXPR:
        case AST_POINTER_MEMBER_ACCESS_EXPR:
            print_ast_recursive(node->data.member.expr, indent + 1);
            break;
        default:
            break;
    }
}

void ast_print(const ASTNode *node, int indent) {
    print_ast_recursive(node, indent);
}

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
        case TYPE_VOID: return "void";
        case TYPE_CHAR: return "char";
        case TYPE_SHORT: return "short";
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_LONGLONG: return "long long";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_LONGDOUBLE: return "long double";
        case TYPE_BOOL: return "_Bool";
        case TYPE_POINTER: return "pointer";
        case TYPE_ARRAY: return "array";
        case TYPE_FUNCTION: return "function";
        case TYPE_STRUCT: return "struct";
        case TYPE_UNION: return "union";
        case TYPE_ENUM: return "enum";
        case TYPE_TYPEDEF: return "typedef";
        default: return "unknown";
    }
}

Type *type_create(TypeKind kind) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    switch (kind) {
        case TYPE_VOID: t->size = 0; break;
        case TYPE_CHAR: t->size = 1; t->align = 1; break;
        case TYPE_SHORT: t->size = 2; t->align = 2; break;
        case TYPE_INT: t->size = 4; t->align = 4; break;
        case TYPE_LONG: t->size = 8; t->align = 8; break;
        case TYPE_LONGLONG: t->size = 16; t->align = 8; break;
        case TYPE_FLOAT: t->size = 4; t->align = 4; break;
        case TYPE_DOUBLE: t->size = 8; t->align = 8; break;
        case TYPE_LONGDOUBLE: t->size = 16; t->align = 16; break;
        case TYPE_BOOL: t->size = 1; t->align = 1; break;
        case TYPE_POINTER: t->size = 8; t->align = 8; break;
        default: t->size = 0; t->align = 1; break;
    }
    return t;
}

void type_free(Type *t) {
    if (!t) return;
    if (t->base) type_free(t->base);
    if (t->param_types) {
        for (size_t i = 0; i < t->num_params; i++)
            if (t->param_types[i]) type_free(t->param_types[i]);
        free(t->param_types);
    }
    if (t->return_type) type_free(t->return_type);
    free(t);
}

Type *type_pointer(Type *base) {
    Type *t = type_create(TYPE_POINTER);
    t->base = base;
    return t;
}

Type *type_array(Type *element, size_t size) {
    Type *t = type_create(TYPE_ARRAY);
    t->base = element;
    t->array_size = size;
    if (size > 0 && element)
        t->size = size * element->size;
    return t;
}

Type *type_copy(Type *t) {
    if (!t) return NULL;
    Type *copy = type_create(t->kind);
    copy->size = t->size;
    copy->align = t->align;
    if (t->base) copy->base = type_copy(t->base);
    copy->array_size = t->array_size;
    if (t->return_type) copy->return_type = type_copy(t->return_type);
    if (t->param_types && t->num_params > 0) {
        copy->param_types = malloc(sizeof(Type *) * t->num_params);
        for (size_t i = 0; i < t->num_params; i++) {
            copy->param_types[i] = type_copy(t->param_types[i]);
        }
        copy->num_params = t->num_params;
    }
    copy->is_variadic = t->is_variadic;
    return copy;
}

// Type helper functions
Type *type_void(void) { return type_create(TYPE_VOID); }
Type *type_char(void) { return type_create(TYPE_CHAR); }
Type *type_short(void) { return type_create(TYPE_SHORT); }
Type *type_int(void) { return type_create(TYPE_INT); }
Type *type_long(void) { return type_create(TYPE_LONG); }
Type *type_longlong(void) { return type_create(TYPE_LONGLONG); }
Type *type_float(void) { return type_create(TYPE_FLOAT); }
Type *type_double(void) { return type_create(TYPE_DOUBLE); }
Type *type_longdouble(void) { return type_create(TYPE_LONGDOUBLE); }
Type *type_bool(void) { return type_create(TYPE_BOOL); }

ASTNode *translation_unit_create(void) {
    ASTNode *unit = ast_create(AST_TRANSLATION_UNIT);
    unit->data.unit.declarations = list_create();
    return unit;
}

void translation_unit_free(ASTNode *unit) {
    if (!unit) return;
    ast_free((ASTNode*)unit);
}

void translation_unit_print(ASTNode *unit) {
    if (!unit || unit->type != AST_TRANSLATION_UNIT) return;
    for (size_t i = 0; i < list_size(unit->data.unit.declarations); i++) {
        ast_print(list_get(unit->data.unit.declarations, i), 0);
        printf("\n");
    }
}
