#ifndef AST_H
#define AST_H

#include "../common/list.h"
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Type Type;
typedef struct StructMember StructMember;

// Struct/Union member
struct StructMember {
    char *name;
    Type *type;
    size_t offset;
};

// AST node types
typedef enum {
    // Special
    AST_TRANSLATION_UNIT,
    
    // Declarations
    AST_FUNCTION_DECL,
    AST_VARIABLE_DECL,
    AST_PARAMETER_DECL,
    AST_STRUCT_DECL,
    AST_UNION_DECL,
    AST_ENUM_DECL,
    AST_TYPEDEF_DECL,
    AST_INITIALIZER_LIST,  // { expr1, expr2, ... }
    
    // Statements
    AST_COMPOUND_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_RETURN_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_GOTO_STMT,
    AST_EXPRESSION_STMT,
    AST_NULL_STMT,
    
    // Expressions
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CAST_EXPR,
    AST_CONDITIONAL_EXPR,
    AST_CALL_EXPR,
    AST_ARRAY_SUBSCRIPT_EXPR,
    AST_MEMBER_ACCESS_EXPR,
    AST_POINTER_MEMBER_ACCESS_EXPR,
    AST_IDENTIFIER_EXPR,
    AST_INTEGER_LITERAL_EXPR,
    AST_FLOAT_LITERAL_EXPR,
    AST_STRING_LITERAL_EXPR,
    AST_ASSIGNMENT_EXPR,
    AST_COMMA_EXPR,
    AST_SIZEOF_EXPR
} ASTNodeType;

// Binary operators
typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_LSHIFT, OP_RSHIFT,
    OP_LT, OP_GT, OP_LE, OP_GE,
    OP_EQ, OP_NE,
    OP_BITAND, OP_BITXOR, OP_BITOR,
    OP_AND, OP_OR
} BinaryOp;

// Type kinds
typedef enum {
    TYPE_VOID, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_LONG, TYPE_LONGLONG,
    TYPE_FLOAT, TYPE_DOUBLE, TYPE_LONGDOUBLE,
    TYPE_BOOL, TYPE_POINTER, TYPE_ARRAY, TYPE_FUNCTION,
    TYPE_STRUCT, TYPE_UNION, TYPE_ENUM, TYPE_TYPEDEF
} TypeKind;

// Type structure
typedef struct Type {
    TypeKind kind;
    size_t size;
    size_t align;
    bool is_unsigned;  // For unsigned integer types
    struct Type *base;  // For pointer/array
    size_t array_size;  // For arrays
    struct Type *return_type;
    struct Type **param_types;
    size_t num_params;
    bool is_variadic;
    // Struct/Union members
    char *struct_name;       // Name of struct/union (can be NULL for anonymous)
    StructMember *members;   // Array of members
    size_t num_members;      // Number of members
    size_t member_capacity;  // Capacity of members array
} Type;

// AST Node
struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    Type *type_info;
    bool is_static;  // True if declared with 'static' keyword
    union {
        struct { List *declarations; } unit;
        struct { char *name; Type *func_type; List *params; ASTNode *body; } function;
        struct { char *name; Type *var_type; ASTNode *init; } variable;
        struct { char *name; Type *param_type; } parameter;
        struct { ASTNode *condition; ASTNode *then_stmt; ASTNode *else_stmt; } if_stmt;
        struct { ASTNode *condition; ASTNode *body; } while_stmt;
        struct { ASTNode *init; ASTNode *condition; ASTNode *increment; ASTNode *body; } for_stmt;
        struct { ASTNode *expr; ASTNode *body; } switch_stmt;
        struct { ASTNode *case_expr; ASTNode *stmt; } case_stmt;
        struct { ASTNode *stmt; } default_stmt;
        struct { ASTNode *expr; } return_stmt;
        struct { char *label; } goto_stmt;
        struct { List *stmts; } compound;
        struct { ASTNode *expr; } expr_stmt;
        struct { BinaryOp op; ASTNode *left; ASTNode *right; } binary;
        struct { int op; ASTNode *operand; } unary;
        struct { Type *cast_type; ASTNode *operand; } cast;
        struct { ASTNode *condition; ASTNode *then_expr; ASTNode *else_expr; } conditional;
        struct { BinaryOp op; ASTNode *left; ASTNode *right; } assignment;
        struct { ASTNode *left; ASTNode *right; } comma;
        struct { ASTNode *callee; List *args; } call;
        struct { ASTNode *array; ASTNode *index; } subscript;
        struct { ASTNode *expr; char *member; bool is_arrow; } member;
        struct { char *name; } identifier;
        struct { long long value; } int_literal;
        struct { double value; } float_literal;
        struct { char *value; } string_literal;
        struct { Type *sizeof_type; ASTNode *sizeof_expr; } sizeof_expr;
        struct { List *elements; } init_list;
    } data;
};

// Type functions
Type *type_create(TypeKind kind);
void type_free(Type *t);
const char *type_kind_name(TypeKind kind);
Type *type_pointer(Type *base);
Type *type_array(Type *element, size_t size);
Type *type_copy(Type *t);

// Type helper functions
Type *type_void(void);
Type *type_char(void);
Type *type_short(void);
Type *type_int(void);
Type *type_long(void);
Type *type_longlong(void);
Type *type_float(void);
Type *type_double(void);
Type *type_longdouble(void);
Type *type_bool(void);

// Struct member functions
void type_add_member(Type *t, const char *name, Type *member_type);
StructMember *type_find_member(Type *t, const char *name);
size_t type_compute_struct_size(Type *t);
size_t type_compute_struct_alignment(Type *t);

// AST functions
ASTNode *ast_create(ASTNodeType type);
void ast_free(ASTNode *node);
void ast_print(const ASTNode *node, int indent);
const char *ast_node_name(ASTNodeType type);

// Translation unit (represented as an ASTNode with type AST_TRANSLATION_UNIT)
ASTNode *translation_unit_create(void);
void translation_unit_free(ASTNode *unit);
void translation_unit_print(ASTNode *unit);

#endif // AST_H
